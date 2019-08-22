/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup bmesh
 *
 * Cut meshes along intersections and boolean operations on the intersections.
 *
 * Supported:
 * - Concave faces.
 * - Non-planar faces.
 * - Coplanar intersections
 * - Custom-data (UV's etc).
 *
 */

#include "MEM_guardedalloc.h"

#include "BLI_alloca.h"
#include "BLI_delaunay_2d.h"
#include "BLI_linklist.h"
#include "BLI_math.h"
#include "BLI_memarena.h"
#include "BLI_utildefines.h"

#include "bmesh.h"
#include "intern/bmesh_private.h"

#include "bmesh_boolean.h" /* own include */

#include "BLI_strict_flags.h"

/* NOTE: Work in progress. Initial implementation using slow data structures and algorithms
 * just to get the correct calculations down. After that, will replace data structures with
 * faster-lookup versions (hash tables, kd-trees, bvh structures) and will parallelize.
 */

/* A Mesh Interface.
 * This would be an abstract interface in C++,
 * but similate that effect in C.
 * Idea is to write the rest of the code so that
 * it will work with either Mesh or BMesh as the
 * concrete representation.
 * Thus, editmesh and modifier can use the same
 * code but without need to convert to BMesh (or Mesh).
 *
 * Exactly one of bm and me should be non-null.
 */
typedef struct IMesh {
  BMesh *bm;
  struct Mesh *me;
} IMesh;

/* A MeshPart is a subset of the geometry of an IndexMesh.
 * The indices refer to vertex, edges, and faces in the IndexMesh
 * that this part is based on (which will be known by context).
 * Unlike for IndexMesh, the edges implied by faces need not be explicitly
 * represented here.
 * Commonly a MeshPart will contain geometry that shares a plane,
 * and when that is so, the plane member says which plane,
 * TODO: faster structure for looking up verts, edges, faces.
 */
typedef struct MeshPart {
  float plane[4];  /* first 3 are normal, 4th is signed distance to plane */
  LinkNode *verts; /* links are ints (vert indices) */
  LinkNode *edges; /* links are ints (edge indices) */
  LinkNode *faces; /* links are ints (face indices) */
} MeshPart;

/* A MeshPartSet set is a set of MeshParts.
 * For any two distinct elements of the set, either they are not
 * coplanar or if they are, they are known not to intersect.
 * TODO: faster structure for looking up by plane.
 */
typedef struct MeshPartSet {
  LinkNode *meshparts; /* links are MeshParts* */
  int tot_part;
} MeshPartSet;

/* A set of integers, where each member gets an index
 * that can be used to access the member.
 * TODO: faster structure for lookup.
 */
typedef struct IndexedIntSet {
  LinkNodePair listhead; /* links are ints */
  int size;
} IndexedIntSet;

/* A set of 3d coords, where each member gets an index
 * that can be used to access the member.
 * Comparisons for equality will be fuzzy (within epsilon).
 * TODO: faster structure for lookup.
 */
typedef struct IndexedCoordSet {
  LinkNodePair listhead; /* Links are pointers to 3 floats, allocated from arena. */
  int index_offset;      /* "index" of an element will be position in above list plus offset. */
  int size;
} IndexedCoordSet;

/* A map from int -> int.
 * TODO: faster structure for lookup.
 */
typedef struct IntIntMap {
  LinkNodePair listhead; /* Links are pointers to IntPair, allocated from arena. */
} IntIntMap;

typedef struct IntPair {
  int first;
  int second;
} IntPair;

/* A result of intersectings parts of an implict IMesh.
 * Designed so that can combine these in pairs, eventually ending up
 * with a single IntersectOutput that can be turned into an edit on the
 * underlying original IMesh.
 */
typedef struct IntersectOutput {
  /* All the individual CDT_results. Links are CDT_result pointers. */
  LinkNodePair cdt_outputs;

  /* Parallel to cdt_outputs list. The corresponding map link is
   * a pointer to an IntInt map, where the key is a vert index in the
   * output vert space of the CDT_result, and the value is a vert index
   * in the original IMesh, or, if greater than orig_mesh_verts_tot,
   * then an index in new_verts.
   * Links are pointers to IntIntMap, allocated from arena.
   */
  LinkNodePair vertmaps;

  /* Set of new vertices (not in original mesh) in resulting intersection.
   * They get an index in the range from orig_mesh_verts_tot to that
   * number plus the size of new_verts - 1.
   * This should be a de-duplicated set of coordinates.
   */
  IndexedCoordSet new_verts;

  /* A map recording which verts in the original mesh get merged to
   * other verts in that mesh.
   * If there is an entry for an index v (< orig_mesh_verts_tot), then
   * v is merged to the value of the map at v.
   * Otherwise v is not merged to anything.
   */
  IntIntMap merge_to;

  /* Number of verts in the original IMesh that this is based on. */
  int orig_mesh_verts_tot;
} IntersectOutput;

typedef struct BoolState {
  MemArena *mem_arena;
  IMesh im;
  int boolean_mode;
  float eps;
  int (*test_fn)(void *elem, void *user_data);
  void *test_fn_user_data;
} BoolState;

/* test_fn results used to distinguish parts of mesh */
enum { TEST_B = -1, TEST_NONE = 0, TEST_A = 1, TEST_ALL = 2 };

#define BOOLDEBUG
#ifdef BOOLDEBUG
/* For Debugging. */
#  define CO3(v) (v)->co[0], (v)->co[1], (v)->co[2]
#  define F2(v) (v)[0], (v)[1]
#  define F3(v) (v)[0], (v)[1], (v)[2]
#  define F4(v) (v)[0], (v)[1], (v)[2], (v)[3]
#  define BMI(e) BM_elem_index_get(e)

static void dump_part(MeshPart *part, const char *label);
static void dump_partset(MeshPartSet *pset, const char *label);
#endif

static CDT_result *copy_cdt_result(BoolState *bs, const CDT_result *res);
static int min_int_in_array(int *array, int len);

/** IMesh functions. */

static void init_imesh_from_bmesh(IMesh *im, BMesh *bm)
{
  im->bm = bm;
  im->me = NULL;
  BM_mesh_elem_table_ensure(bm, BM_VERT | BM_EDGE | BM_FACE);
}

static int imesh_totvert(const IMesh *im)
{
  if (im->bm) {
    return im->bm->totvert;
  }
  else {
    return 0; /* TODO */
  }
}

static int imesh_totface(const IMesh *im)
{
  if (im->bm) {
    return im->bm->totface;
  }
  else {
    return 0; /* TODO */
  }
}

static int imesh_facelen(const IMesh *im, int f)
{
  int ans = 0;

  if (im->bm) {
    BMFace *bmf = BM_face_at_index(im->bm, f);
    if (bmf) {
      ans = bmf->len;
    }
  }
  else {
    ; /* TODO */
  }
  return ans;
}

static int imesh_face_vert(IMesh *im, int f, int index)
{
  int i;
  int ans = -1;

  if (im->bm) {
    BMFace *bmf = BM_face_at_index(im->bm, f);
    if (bmf) {
      BMLoop *l = bmf->l_first;
      for (i = 0; i < index; i++) {
        l = l->next;
      }
      BMVert *bmv = l->v;
      ans = BM_elem_index_get(bmv);
    }
  }
  else {
    ; /* TODO */
  }
  return ans;
}

static void imesh_get_vert_co(const IMesh *im, int v, float *r_coords)
{
  if (im->bm) {
    BMVert *bmv = BM_vert_at_index(im->bm, v);
    if (bmv) {
      copy_v3_v3(r_coords, bmv->co);
      return;
    }
    else {
      zero_v3(r_coords);
    }
  }
  else {
    ; /* TODO */
  }
}

static void imesh_get_face_plane(const IMesh *im, int f, float r_plane[4])
{
  zero_v4(r_plane);
  if (im->bm) {
    BMFace *bmf = BM_face_at_index(im->bm, f);
    if (bmf) {
      plane_from_point_normal_v3(r_plane, bmf->l_first->v->co, bmf->no);
    }
  }
}

static int imesh_test_face(const IMesh *im, int (*test_fn)(void *, void *), void *user_data, int f)
{
  if (im->bm) {
    BMFace *bmf = BM_face_at_index(im->bm, f);
    if (bmf) {
      return test_fn(bmf, user_data);
    }
    return 0;
  }
  else {
    /* TODO */
    return 0;
  }
}

/** MeshPartSet functions. */

static void init_meshpartset(MeshPartSet *partset)
{
  partset->meshparts = NULL;
  partset->tot_part = 0;
}

static void add_part_to_partset(BoolState *bs, MeshPartSet *partset, MeshPart *part)
{
  BLI_linklist_prepend_arena(&partset->meshparts, part, bs->mem_arena);
  partset->tot_part++;
}

static MeshPart *partset_part(MeshPartSet *partset, int index)
{
  LinkNode *ln = BLI_linklist_find(partset->meshparts, index);
  if (ln) {
    return (MeshPart *)ln->link;
  }
  return NULL;
}

/** MeshPart functions. */

static void init_meshpart(MeshPart *part)
{
  zero_v4(part->plane);
  part->verts = NULL;
  part->edges = NULL;
  part->faces = NULL;
}

static int part_totface(MeshPart *part)
{
  return BLI_linklist_count(part->faces);
}

static int part_face(MeshPart *part, int index)
{
  LinkNode *ln = BLI_linklist_find(part->faces, index);
  if (ln) {
    return POINTER_AS_INT(ln->link);
  }
  return -1;
}

static bool parts_may_intersect(MeshPart *part1, MeshPart *part2)
{
  return (part1 && part2); /* Placeholder test, uses args */
}

/* Return true if a_plane and b_plane are the same plane, to within eps. */
static bool planes_are_coplanar(const float a_plane[4], const float b_plane[4], float eps)
{
  if (fabsf(a_plane[3] - b_plane[3]) > eps) {
    return false;
  }
  return fabsf(dot_v3v3(a_plane, b_plane) - 1.0f) <= eps;
}

/* Return the MeshPart in partset for plane.
 * If none exists, make a new one for the plane and add
 * it to partset.
 */
static MeshPart *find_part_for_plane(BoolState *bs, MeshPartSet *partset, const float plane[4])
{
  LinkNode *ln;
  MeshPart *new_part;

  for (ln = partset->meshparts; ln; ln = ln->next) {
    MeshPart *p = (MeshPart *)ln->link;
    if (planes_are_coplanar(plane, p->plane, bs->eps)) {
      return p;
    }
  }
  new_part = BLI_memarena_alloc(bs->mem_arena, sizeof(MeshPart));
  init_meshpart(new_part);
  copy_v4_v4(new_part->plane, plane);
  add_part_to_partset(bs, partset, new_part);
  return new_part;
}

static void add_face_to_part(BoolState *bs, MeshPart *meshpart, int f)
{
  BLI_linklist_prepend_arena(&meshpart->faces, POINTER_FROM_INT(f), bs->mem_arena);
}

/** IndexedIntSet functions. */

static void init_intset(IndexedIntSet *intset)
{
  intset->listhead.list = NULL;
  intset->listhead.last_node = NULL;
  intset->size = 0;
}

static int add_int_to_intset(BoolState *bs, IndexedIntSet *intset, int value)
{
  int index;

  index = BLI_linklist_index(intset->listhead.list, POINTER_FROM_INT(value));
  if (index == -1) {
    BLI_linklist_append_arena(&intset->listhead, POINTER_FROM_INT(value), bs->mem_arena);
    index = intset->size;
    intset->size++;
  }
  return index;
}

static int intset_get_value_by_index(const IndexedIntSet *intset, int index)
{
  LinkNode *ln;
  int i;

  if (index < 0 || index >= intset->size) {
    return -1;
  }
  ln = intset->listhead.list;
  for (i = 0; i < index; i++) {
    BLI_assert(ln != NULL);
    ln = ln->next;
  }
  BLI_assert(ln != NULL);
  return POINTER_AS_INT(ln->link);
}

static int intset_get_index_for_value(const IndexedIntSet *intset, int value)
{
  return BLI_linklist_index(intset->listhead.list, POINTER_FROM_INT(value));
}

/** IndexedCoordSet functions. */

static void init_coordset(IndexedCoordSet *coordset, int index_offset)
{
  coordset->listhead.list = NULL;
  coordset->listhead.last_node = NULL;
  coordset->index_offset = index_offset;
  coordset->size = 0;
}

static int add_to_coordset(BoolState *bs,
                           IndexedCoordSet *coordset,
                           const float p[3],
                           bool do_dup_check)
{
  LinkNode *ln;
  float *q;
  int i;
  int index = -1;

  if (do_dup_check) {
    i = coordset->index_offset;
    for (ln = coordset->listhead.list; ln; ln = ln->next) {
      q = (float *)ln->link;
      /* Note: compare_v3v3 checks if all three coords are within (<=) eps of each other. */
      if (compare_v3v3(p, q, bs->eps)) {
        index = i;
        break;
      }
      i++;
    }
    if (index != -1) {
      return index;
    }
  }
  q = BLI_memarena_alloc(bs->mem_arena, 3 * sizeof(float));
  copy_v3_v3(q, p);
  BLI_linklist_append_arena(&coordset->listhead, q, bs->mem_arena);
  index = coordset->index_offset + coordset->size;
  coordset->size++;
  return index;
}

/** IntIntMap functions. */

static void init_intintmap(IntIntMap *intintmap)
{
  intintmap->listhead.list = NULL;
  intintmap->listhead.last_node = NULL;
}

static void add_to_intintmap(BoolState *bs, IntIntMap *map, int key, int val)
{
  IntPair *keyvalpair;

  keyvalpair = BLI_memarena_alloc(bs->mem_arena, sizeof(*keyvalpair));
  keyvalpair->first = key;
  keyvalpair->second = val;
  BLI_linklist_append_arena(&map->listhead, keyvalpair, bs->mem_arena);
}

/** IntersectOutput functions. */

static void init_isect_out(BoolState *bs, IntersectOutput *isect_out)
{
  isect_out->orig_mesh_verts_tot = imesh_totvert(&bs->im);
  isect_out->cdt_outputs.list = NULL;
  isect_out->cdt_outputs.last_node = NULL;

  isect_out->vertmaps.list = NULL;
  isect_out->vertmaps.last_node = NULL;

  init_coordset(&isect_out->new_verts, isect_out->orig_mesh_verts_tot);

  init_intintmap(&isect_out->merge_to);
}

static void isect_add_cdt_result(BoolState *bs, IntersectOutput *isect_out, CDT_result *res)
{
  BLI_linklist_append_arena(&isect_out->cdt_outputs, res, bs->mem_arena);
}

static IntIntMap *isect_add_vertmap(BoolState *bs, IntersectOutput *isect_out)
{
  IntIntMap *ans;

  ans = BLI_memarena_alloc(bs->mem_arena, sizeof(*ans));
  init_intintmap(ans);
  BLI_linklist_append_arena(&isect_out->vertmaps, ans, bs->mem_arena);
  return ans;
}

static IntersectOutput *isect_out_from_cdt(BoolState *bs,
                                                const CDT_result *cout,
                                                const float rot_inv[3][3],
                                                const float z_for_inverse)
{
  IntersectOutput *isect_out;
  CDT_result *cout_copy;
  int out_v, in_v, start, new_v;
  IntIntMap *vmap;
  float p[3], q[3];

  isect_out = BLI_memarena_alloc(bs->mem_arena, sizeof(*isect_out));
  init_isect_out(bs, isect_out);

  /* Need to copy cdt result to arena because caller will soon call BLI_delaunay_2d_cdt_free(cout).
   */
  cout_copy = copy_cdt_result(bs, cout);
  isect_add_cdt_result(bs, isect_out, cout_copy);

  vmap = isect_add_vertmap(bs, isect_out);

  for (out_v = 0; out_v < cout->verts_len; out_v++) {
    if (cout->verts_orig_len_table[out_v] > 0) {
      /* out_v maps to an original vertex. */
      start = cout->verts_orig_start_table[out_v];
      /* Choose min index in orig list, to make for a stable algorithm. */
      in_v = min_int_in_array(&cout->verts_orig[start], cout->verts_orig_len_table[out_v]);
      add_to_intintmap(bs, vmap, out_v, in_v);
    }
    else {
      /* out_v needs a new vertex. Need to convert coords from 2d to 3d. */
      copy_v2_v2(q, cout->vert_coords[out_v]);
      q[3] = z_for_inverse;
      mul_v3_m3v3(p, rot_inv, q);
      new_v = add_to_coordset(bs, &isect_out->new_verts, p, false);
      BLI_assert(new_v >= imesh_totvert(&bs->im));
      add_to_intintmap(bs, vmap, out_v, new_v);
    }
  }

  return isect_out;
}

/** Miscellaneous utility functions. */

#define COPY_ARRAY_TO_ARENA(dst, src, len, arena) \
  { \
    dst = BLI_memarena_alloc(arena, len); \
    memmove(dst, src, len); \
  }

/* Return a copy of res, with memory allocated from bs->mem_arena. */
static CDT_result *copy_cdt_result(BoolState *bs, const CDT_result *res)
{
  CDT_result *ans;
  size_t nv, ne, nf, n;
  MemArena *arena = bs->mem_arena;

  nv = (size_t)res->verts_len;
  ne = (size_t)res->edges_len;
  nf = (size_t)res->faces_len;
  ans = BLI_memarena_calloc(arena, sizeof(*ans));
  ans->verts_len = res->verts_len;
  ans->edges_len = res->edges_len;
  ans->faces_len = res->faces_len;
  ans->face_edge_offset = res->face_edge_offset;
  if (nv > 0) {
    COPY_ARRAY_TO_ARENA(
        ans->vert_coords, res->vert_coords, nv * sizeof(ans->vert_coords[0]), arena);
    COPY_ARRAY_TO_ARENA(
        ans->verts_orig_len_table, res->verts_orig_len_table, nv * sizeof(int), arena);
    COPY_ARRAY_TO_ARENA(
        ans->verts_orig_start_table, res->verts_orig_start_table, nv * sizeof(int), arena);
    n = (size_t)(res->verts_orig_start_table[nv - 1] + res->verts_orig_len_table[nv - 1]);
    COPY_ARRAY_TO_ARENA(ans->verts_orig, res->verts_orig, n * sizeof(int), arena);
  }
  if (ne > 0) {
    COPY_ARRAY_TO_ARENA(ans->edges, res->edges, ne * sizeof(ans->edges[0]), arena);
    COPY_ARRAY_TO_ARENA(
        ans->edges_orig_len_table, res->edges_orig_len_table, ne * sizeof(int), arena);
    COPY_ARRAY_TO_ARENA(
        ans->edges_orig_start_table, res->edges_orig_start_table, ne * sizeof(int), arena);
    n = (size_t)(res->edges_orig_start_table[ne - 1] + res->edges_orig_len_table[ne - 1]);
    COPY_ARRAY_TO_ARENA(ans->edges_orig, res->edges_orig, n * sizeof(int), arena);
  }
  if (nf > 0) {
    COPY_ARRAY_TO_ARENA(ans->faces_len_table, res->faces_len_table, nf * sizeof(int), arena);
    COPY_ARRAY_TO_ARENA(ans->faces_start_table, res->faces_start_table, nf * sizeof(int), arena);
    n = (size_t)(res->faces_start_table[nf - 1] + res->faces_len_table[nf - 1]);
    COPY_ARRAY_TO_ARENA(ans->faces, res->faces, n * sizeof(int), arena);
    COPY_ARRAY_TO_ARENA(
        ans->faces_orig_len_table, res->faces_orig_len_table, nf * sizeof(int), arena);
    COPY_ARRAY_TO_ARENA(
        ans->faces_orig_start_table, res->faces_orig_start_table, nf * sizeof(int), arena);
    n = (size_t)(res->faces_orig_start_table[nf - 1] + res->faces_orig_len_table[nf - 1]);
    COPY_ARRAY_TO_ARENA(ans->faces_orig, res->faces_orig, n * sizeof(int), arena);
  }
  return ans;
}
#undef COPY_ARRAY_TO_ARENA

static int min_int_in_array(int *array, int len)
{
  int min = INT_MIN;
  int i;

  for (i = 0; i < len; i++) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

/** Intersection Algorithm functions. */

/* Fill partset with parts for each plane for which there is a face
 * in bs->im.
 * Use bs->test_fn to check elements against test_val, to see whether or not it should be in the
 * result.
 */
static void find_coplanar_parts(BoolState *bs, MeshPartSet *partset, int test_val)
{
  IMesh *im = &bs->im;
  MeshPart *part;
  int im_nf, f, test;
  float plane[4];

  init_meshpartset(partset);
  im_nf = imesh_totface(im);
  for (f = 0; f < im_nf; f++) {
    if (test_val != TEST_ALL) {
      test = imesh_test_face(im, bs->test_fn, bs->test_fn_user_data, f);
      if (test != test_val) {
        continue;
      }
    }
    imesh_get_face_plane(im, f, plane);
    part = find_part_for_plane(bs, partset, plane);
    add_face_to_part(bs, part, f);
  }
  /* TODO: look for loose verts and wire edges to add to each partset */
}

/*
 * Intersect all the geometry in part, assumed to be in one plane.
 * Return an IntersectOuput that gives the new geometry that should
 * replace the geometry in part.
 * If no output is needed, return NULL.
 */
static IntersectOutput *self_intersect_part(BoolState *bs, MeshPart *part)
{
  CDT_input in;
  CDT_result *out;
  IntersectOutput *isect_out;
  int i, j, part_nf, f, face_len, v;
  int nfaceverts, v_index, faces_index;
  IMesh *im = &bs->im;
  IndexedIntSet verts_needed;
  float mat_2d[3][3];
  float mat_2d_inv[3][3];
  float xyz[3], save_z, p[3];
  bool ok;

  dump_part(part, "self_intersect_part");
  /* Find which vertices are needed for CDT input */
  part_nf = part_totface(part);
  if (part_nf <= 1) {
    printf("trivial 1 face case\n");
    return NULL;
  }
  init_intset(&verts_needed);
  nfaceverts = 0;
  for (i = 0; i < part_nf; i++) {
    f = part_face(part, i);
    BLI_assert(f != -1);
    face_len = imesh_facelen(im, f);
    nfaceverts += face_len;
    for (j = 0; j < face_len; j++) {
      v = imesh_face_vert(im, f, j);
      BLI_assert(v != -1);
      v_index = add_int_to_intset(bs, &verts_needed, v);
    }
  }
  /* TODO: find vertices needed for loose verts and edges */

  in.verts_len = verts_needed.size;
  in.edges_len = 0;
  in.faces_len = part_nf;
  in.vert_coords = BLI_array_alloca(in.vert_coords, (size_t)in.verts_len);
  in.edges = NULL;
  in.faces = BLI_array_alloca(in.faces, (size_t)nfaceverts);
  in.faces_start_table = BLI_array_alloca(in.faces_start_table, (size_t)part_nf);
  in.faces_len_table = BLI_array_alloca(in.faces_len_table, (size_t)part_nf);
  in.epsilon = bs->eps;

  /* Fill in the vert_coords of CDT input */

  /* Find mat_2d: matrix to rotate so that plane normal moves to z axis */
  axis_dominant_v3_to_m3(mat_2d, part->plane);
  ok = invert_m3_m3(mat_2d_inv, mat_2d);
  BLI_assert(ok);

  for (i = 0; i < in.verts_len; i++) {
    v = intset_get_value_by_index(&verts_needed, i);
    BLI_assert(v != -1);
    imesh_get_vert_co(im, v, p);
    mul_v3_m3v3(xyz, mat_2d, p);
    copy_v2_v2(in.vert_coords[i], xyz);
    if (i == 0) {
      /* If part is truly coplanar, all z components of rotated v should be the same.
       * Save it so that can rotate back to correct place when done.
       */
      save_z = xyz[2];
    }
  }

  /* Fill in the face data of CDT input */
  faces_index = 0;
  for (i = 0; i < part_nf; i++) {
    f = part_face(part, i);
    BLI_assert(f != -1);
    face_len = imesh_facelen(im, f);
    in.faces_start_table[i] = faces_index;
    for (j = 0; j < face_len; j++) {
      v = imesh_face_vert(im, f, j);
      BLI_assert(v != -1);
      v_index = intset_get_index_for_value(&verts_needed, v);
      in.faces[faces_index++] = v_index;
    }
    in.faces_len_table[i] = faces_index - in.faces_start_table[i];
  }

  out = BLI_delaunay_2d_cdt_calc(&in, CDT_CONSTRAINTS_VALID_BMESH);

  printf("cdt output, %d faces\n", out->faces_len);

  isect_out = isect_out_from_cdt(bs, out, mat_2d_inv, save_z);

  BLI_delaunay_2d_cdt_free(out);
  return isect_out;
}

/* Intersect part with all the parts in partset, except the part with
 * exclude_index, if that is not -1.
 */
static IntersectOutput *intersect_part_with_partset(BoolState *bs,
                                                    MeshPart *part,
                                                    MeshPartSet *partset,
                                                    int exclude_index)
{
  int i, tot_part;
  MeshPart *otherpart;
  IntersectOutput *isect_out;

  tot_part = partset->tot_part;
  /* TODO: Make a copy of part */
  for (i = 0; i < tot_part; i++) {
    if (i != exclude_index) {
      otherpart = partset_part(partset, i);
      if (parts_may_intersect(part, otherpart)) {
        /* TODO: add any intersects between parts to copy of part */
      }
    }
  }
  /* TODO: self intersect the part copy and make an edit from it */
  isect_out = self_intersect_part(bs, part);
  return isect_out;
}

/* Intersect all parts.
 */
static void intersect_all_parts(BoolState *bs, MeshPartSet *partset)
{
  int i, tot_part;
  MeshPart *part;
  IntersectOutput **isect_out;

  tot_part = partset->tot_part;
  isect_out = BLI_memarena_alloc(bs->mem_arena, (size_t)tot_part * sizeof(*isect_out));
  for (i = 0; i < tot_part; i++) {
    part = partset_part(partset, i);
    isect_out[i] = intersect_part_with_partset(bs, part, partset, i);
  }
  /* TODO: apply the union of outputs */
}

/* Intersect all parts of a_partset with all parts of b_partset.
 */
static void intersect_parts_pair(BoolState *bs, MeshPartSet *a_partset, MeshPartSet *b_partset)
{
  int a_index, b_index, tot_part_a, tot_part_b, tot;
  MeshPart *part;
  IntersectOutput **isect_out;

  tot_part_a = a_partset->tot_part;
  tot_part_b = b_partset->tot_part;
  tot = tot_part_a + tot_part_b;
  isect_out = BLI_memarena_alloc(bs->mem_arena, (size_t)tot * sizeof(IntersectOutput *));
  for (a_index = 0; a_index < tot_part_a; a_index++) {
    part = partset_part(a_partset, a_index);
    isect_out[a_index] = intersect_part_with_partset(bs, part, b_partset, -1);
  }
  for (b_index = 0; b_index < tot_part_b; b_index++) {
    part = partset_part(b_partset, b_index);
    isect_out[tot_part_a + b_index] = intersect_part_with_partset(bs, part, a_partset, -1);
  }
  /* TODO: apply the union of outputs */
}

/**
 * Intersect faces
 * leaving the resulting edges tagged.
 *
 * \param test_fn: Return value: -1: skip, 0: tree_a, 1: tree_b (use_self == false)
 * \param boolean_mode: -1: no-boolean, 0: intersection... see #BMESH_ISECT_BOOLEAN_ISECT.
 * \return true if the mesh is changed (intersections cut or faces removed from boolean).
 */
bool BM_mesh_boolean(BMesh *bm,
                     int (*test_fn)(BMFace *f, void *user_data),
                     void *user_data,
                     const bool use_self,
                     const bool UNUSED(use_separate),
                     const int boolean_mode,
                     const float eps)
{
  BoolState bs = {NULL};
  MeshPartSet all_parts, a_parts, b_parts;

  init_imesh_from_bmesh(&bs.im, bm);
  bs.boolean_mode = boolean_mode;
  bs.eps = eps;
  bs.test_fn = (int (*)(void *, void *))test_fn;
  bs.test_fn_user_data = user_data;
  bs.mem_arena = BLI_memarena_new(MEM_SIZE_OPTIMAL(1 << 16), __func__);

  printf("\n\nBOOLEAN\n");

  if (use_self) {
    find_coplanar_parts(&bs, &all_parts, TEST_ALL);
    dump_partset(&all_parts, "all coplanar parts");
    intersect_all_parts(&bs, &all_parts);
  }
  else {
    find_coplanar_parts(&bs, &a_parts, TEST_A);
    find_coplanar_parts(&bs, &b_parts, TEST_B);
    intersect_parts_pair(&bs, &a_parts, &b_parts);
  }

  BLI_memarena_free(bs.mem_arena);
  return true;
}

#ifdef BOOLDEBUG
static void dump_part(MeshPart *part, const char *label)
{
  LinkNode *ln;
  int i;
  struct namelist {
    const char *name;
    LinkNode *list;
  } nl[3] = {{"verts", part->verts}, {"edges", part->edges}, {"faces", part->faces}};

  printf("%s: (%.3f,%.3f,%.3f),%.3f:\n", label, F4(part->plane));
  for (i = 0; i < 3; i++) {
    if (nl[i].list) {
      printf("  %s: ", nl[i].name);
      for (ln = nl[i].list; ln; ln = ln->next) {
        printf("%d ", POINTER_AS_INT(ln->link));
      }
      printf("\n");
    }
  }
}

static void dump_partset(MeshPartSet *partset, const char *label)
{
  int i;
  MeshPart *part;

  printf("%s:\n", label);
  for (i = 0; i < partset->tot_part; i++) {
    part = partset_part(partset, i);
    if (!part) {
      printf("<NULL PART>\n");
    }
    else {
      dump_part(part, "");
    }
  }
}
#endif
