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
#include "BLI_array.h"
#include "BLI_delaunay_2d.h"
#include "BLI_linklist.h"
#include "BLI_math.h"
#include "BLI_memarena.h"
#include "BLI_utildefines.h"

#include "bmesh.h"
#include "intern/bmesh_private.h"

#include "bmesh_boolean.h" /* own include */

//#include "BLI_strict_flags.h"

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
  double plane[4];  /* first 3 are normal, 4th is signed distance to plane */
  double bbmin[3];  /* bounding box min, with eps padding */
  double bbmax[3];  /* bounding box max, with eps padding */
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
  double bbmin[3];
  double bbmax[3];
  LinkNode *meshparts; /* links are MeshParts* */
  int tot_part;
  const char *label; /* for debugging */
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

typedef struct IntIntMapIterator {
  const IntIntMap *map;
  LinkNode *curlink;
  IntPair *keyvalue;
} IntIntMapIterator;

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
  double eps;
  int (*test_fn)(void *elem, void *user_data);
  void *test_fn_user_data;
} BoolState;

/* test_fn results used to distinguish parts of mesh */
enum { TEST_NONE = -1, TEST_B = 0, TEST_A = 1, TEST_ALL = 2 };

#define BOOLDEBUG
#ifdef BOOLDEBUG
/* For Debugging. */
#  define CO3(v) (v)->co[0], (v)->co[1], (v)->co[2]
#  define F2(v) (v)[0], (v)[1]
#  define F3(v) (v)[0], (v)[1], (v)[2]
#  define F4(v) (v)[0], (v)[1], (v)[2], (v)[3]
#  define BMI(e) BM_elem_index_get(e)

static void dump_part(const MeshPart *part, const char *label);
static void dump_partset(const MeshPartSet *pset);
static void dump_isect_out(const IntersectOutput *io, const char *label);
static void dump_intintmap(const IntIntMap *map, const char *label, const char *prefix);
#endif

/* Forward declarations of some static functions. */
static CDT_result *copy_cdt_result(BoolState *bs, const CDT_result *res);
static int min_int_in_array(int *array, int len);
static LinkNode *linklist_shallow_copy_arena(LinkNode *list, struct MemArena *arena);
static void calc_part_bb_eps(BoolState *bs, MeshPart *part, double eps);
static float *coordset_coord(const IndexedCoordSet *coordset, int index);
static bool find_in_intintmap(const IntIntMap *map, int key, int *r_val);

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

static void imesh_get_face_plane(const IMesh *im, int f, double r_plane[4])
{
  double plane_co[3];

  zero_v4_db(r_plane);
  if (im->bm) {
    BMFace *bmf = BM_face_at_index(im->bm, f);
    if (bmf) {
      /* plane_from_point_normal_v3 with mixed arithmetic */
      copy_v3db_v3fl(r_plane, bmf->no);
      copy_v3db_v3fl(plane_co, bmf->l_first->v->co);
      r_plane[3] = -dot_v3v3_db(r_plane, plane_co);
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

static void apply_isect_out_to_bmesh(BMesh *bm, const IntersectOutput *isect_out)
{
  int bm_tot_v, tot_new_v, tot_new_e, i, j, v1, v2, v1_out, v2_out;
  int tot_new_f, fstart, flen, v, v_out;
  BMVert **new_bmvs, *bmv, *bmv1, *bmv2;
  float *co;
  LinkNode *ln, *ln_map;
  CDT_result *cdt;
  IntIntMap *vmap;
  BMVert **vv = NULL;
  BLI_array_staticdeclare(vv, BM_DEFAULT_NGON_STACK_SIZE);

  printf("\n\nAPPLY_ISECT_OUT_TO_BMESH\n\n");
  /* Create the new BMVerts. */
  bm_tot_v = bm->totvert;
  tot_new_v = isect_out->new_verts.size;
  if (tot_new_v > 0) {
    new_bmvs = BLI_array_alloca(new_bmvs, (size_t)tot_new_v);
    for (i = 0; i < tot_new_v; i++) {
      co = coordset_coord(&isect_out->new_verts, bm_tot_v + i);
      /* TODO: use an example vert if there is one */
      new_bmvs[i] = BM_vert_create(bm, co, NULL, 0);
      printf("new_bmv[%d]=%p, at (%.3f,%.3f,%.3f)\n", i, new_bmvs[i], F3(co));
    }
  }

  /* Adding verts has made the vertex table dirty.
   * It is probably still ok, but just in case...
   * TODO: find a way to avoid regenerating this table, maybe.
   */
  BM_mesh_elem_table_ensure(bm, BM_VERT);

  /* Now create edges and faces from each cdt_output */
  for (ln = isect_out->cdt_outputs.list, ln_map = isect_out->vertmaps.list; ln;
       ln = ln->next, ln_map = ln_map->next) {
    printf("\napply cdt output\n");
    cdt = (CDT_result *)ln->link;
    vmap = (IntIntMap *)ln_map->link;
    tot_new_e = cdt->edges_len;
    for (i = 0; i < tot_new_e; i++) {
      v1 = cdt->edges[i][0];
      v2 = cdt->edges[i][1];
      if (!find_in_intintmap(vmap, v1, &v1_out)) {
        printf("whoops, v1 map entry missing\n");
        BLI_assert(false);
        v1_out = v1;
      }
      if (!find_in_intintmap(vmap, v2, &v2_out)) {
        printf("whoops, v2 map entry missing\n");
        BLI_assert(false);
        v2_out = v2;
      }
      printf("make edge %d %d\n", v1_out, v2_out);
      if (v1_out < bm_tot_v) {

        bmv1 = BM_vert_at_index(bm, v1_out);
      }
      else {
        BLI_assert(v1_out - bm_tot_v < tot_new_v);
        bmv1 = new_bmvs[v1_out - bm_tot_v];
      }
      if (v2_out < bm_tot_v) {
        bmv2 = BM_vert_at_index(bm, v2_out);
      }
      else {
        BLI_assert(v2_out - bm_tot_v < tot_new_v);
        bmv2 = new_bmvs[v2_out - bm_tot_v];
      }
      /* TODO: use an example, if there is one. */
      BM_edge_create(bm, bmv1, bmv2, NULL, BM_CREATE_NO_DOUBLE);
    }
    tot_new_f = cdt->faces_len;
    for (i = 0; i < tot_new_f; i++) {
      printf("make face ");
      fstart = cdt->faces_start_table[i];
      flen = cdt->faces_len_table[i];
      BLI_array_clear(vv);
      for (j = 0; j < flen; j++) {
        v = cdt->faces[fstart + j];
        if (!find_in_intintmap(vmap, v, &v_out)) {
          printf("whoops, face v map entry missing\n");
          BLI_assert(false);
          v_out = v;
        }
        printf(" %d", v_out);
        if (v_out < bm_tot_v) {
          bmv = BM_vert_at_index(bm, v_out);
        }
        else {
          BLI_assert(v_out - bm_tot_v < tot_new_v);
          bmv = new_bmvs[v_out - bm_tot_v];
        }
        BLI_array_append(vv, bmv);
      }
      printf("\n");
      /* TODO: use example */
     BM_face_create_verts(bm, vv, flen, NULL, BM_CREATE_NOP, false);
    }
  }
  BLI_array_free(vv);
}

static void apply_isect_out_to_imesh(IMesh *im, const IntersectOutput *isect_out)
{
  if (isect_out == NULL) {
    return;
  }
  if (im->bm) {
    apply_isect_out_to_bmesh(im->bm, isect_out);
  }
  else {
    /* TODO */
  }
}

static void bb_update(double bbmin[3], double bbmax[3], int v, const IMesh *im)
{
  int i;
  float vco[3];
  double vcod[3];

  imesh_get_vert_co(im, v, vco);
  copy_v3db_v3fl(vcod, vco);
  for (i = 0; i < 3; i++) {
    bbmin[i] = min_dd(vcod[i], bbmin[i]);
    bbmax[i] = max_dd(vcod[i], bbmax[i]);
  }
}

/** MeshPartSet functions. */

static void init_meshpartset(MeshPartSet *partset, const char *label)
{
  partset->meshparts = NULL;
  partset->tot_part = 0;
  zero_v3_db(partset->bbmin);
  zero_v3_db(partset->bbmax);
  partset->label = label;
}

static void add_part_to_partset(BoolState *bs, MeshPartSet *partset, MeshPart *part)
{
  BLI_linklist_prepend_arena(&partset->meshparts, part, bs->mem_arena);
  partset->tot_part++;
}

static MeshPart *partset_part(const MeshPartSet *partset, int index)
{
  LinkNode *ln = BLI_linklist_find(partset->meshparts, index);
  if (ln) {
    return (MeshPart *)ln->link;
  }
  return NULL;
}

/* Fill in partset->bbmin and partset->bbmax with axis aligned bounding box
 * for the partset.
 * Also calculates bbmin and bbmax for each part.
 * Add epsilon buffer on all sides.
 */
static void calc_partset_bb_eps(BoolState *bs, MeshPartSet *partset, double eps)
{
  LinkNode *ln;
  MeshPart *part;
  int i;

  if (partset->meshparts == NULL) {
    zero_v3_db(partset->bbmin);
    zero_v3_db(partset->bbmax);
    return;
  }
  copy_v3_db(partset->bbmin, DBL_MAX);
  copy_v3_db(partset->bbmax, -DBL_MAX);
  for (ln = partset->meshparts; ln; ln = ln->next) {
    part = (MeshPart *)ln->link;
    calc_part_bb_eps(bs, part, eps);
    for (i = 0; i < 3; i++) {
      partset->bbmin[i] = min_dd(partset->bbmin[i], part->bbmin[i]);
      partset->bbmax[i] = max_dd(partset->bbmax[i], part->bbmax[i]);
    }
  }
  /* eps padding was already added in calc_part_bb_eps. */
}

/** MeshPart functions. */

static void init_meshpart(MeshPart *part)
{
  zero_v4_db(part->plane);
  zero_v3_db(part->bbmin);
  zero_v3_db(part->bbmax);
  part->verts = NULL;
  part->edges = NULL;
  part->faces = NULL;
}

static MeshPart *copy_part(BoolState *bs, const MeshPart *part)
{
  MeshPart *copy;
  MemArena *arena = bs->mem_arena;

  copy = BLI_memarena_alloc(bs->mem_arena, sizeof(*copy));
  copy_v4_v4_db(copy->plane, part->plane);
  copy_v3_v3_db(copy->bbmin, part->bbmin);
  copy_v3_v3_db(copy->bbmax, part->bbmax);

  /* All links in lists are ints, so can use shallow copy. */
  copy->verts = linklist_shallow_copy_arena(part->verts, arena);
  copy->edges = linklist_shallow_copy_arena(part->edges, arena);
  copy->faces = linklist_shallow_copy_arena(part->faces, arena);
  return copy;
}

static int part_totface(const MeshPart *part)
{
  return BLI_linklist_count(part->faces);
}

static int part_face(const MeshPart *part, int index)
{
  LinkNode *ln = BLI_linklist_find(part->faces, index);
  if (ln) {
    return POINTER_AS_INT(ln->link);
  }
  return -1;
}

/* Fill part->bbmin and part->bbmax with the axis-aligned bounding box
 * for the part.
 * Add an epsilon buffer on all sides.
 */
static void calc_part_bb_eps(BoolState *bs, MeshPart *part, double eps)
{
  IMesh *im = &bs->im;
  LinkNode *ln;
  int v, e, f, i, flen, j;

  if (part->verts == NULL && part->edges == NULL && part->faces == NULL) {
    zero_v3_db(part->bbmin);
    zero_v3_db(part->bbmax);
    return;
  }
  copy_v3_db(part->bbmin, FLT_MAX);
  copy_v3_db(part->bbmax, -FLT_MAX);
  for (ln = part->verts; ln; ln = ln->next) {
    v = POINTER_AS_INT(ln->link);
    bb_update(part->bbmin, part->bbmax, v, im);
  }
  for (ln = part->edges; ln; ln = ln->next) {
    e = POINTER_AS_INT(ln->link);
    /* TODO: handle edge verts */
    printf("calc_part_bb_eps please implement edge (%d)\n", e);
  }
  for (ln = part->faces; ln; ln = ln->next) {
    f = POINTER_AS_INT(ln->link);
    flen = imesh_facelen(im, f);
    for (j = 0; j < flen; j++) {
      v = imesh_face_vert(im, f, j);
      bb_update(part->bbmin, part->bbmax, v, im);
    }
  }
  for (i = 0; i < 3; i++) {
    part->bbmin[i] -= eps;
    part->bbmax[i] += eps;
  }
}

static bool parts_may_intersect(const MeshPart *part1, const MeshPart *part2)
{
  return isect_aabb_aabb_v3_db(part1->bbmin, part1->bbmax, part2->bbmin, part2->bbmax);
}

static bool part_may_intersect_partset(const MeshPart *part, const MeshPartSet *partset)
{
  return isect_aabb_aabb_v3_db(part->bbmin, part->bbmax, partset->bbmin, partset->bbmax);
}

/* Return true if a_plane and b_plane are the same plane, to within eps. */
static bool planes_are_coplanar(const double a_plane[4], const double b_plane[4], double eps)
{
  if (fabs(a_plane[3] - b_plane[3]) > eps) {
    return false;
  }
  return fabs(dot_v3v3_db(a_plane, b_plane) - 1.0f) <= eps;
}

/* Return the MeshPart in partset for plane.
 * If none exists, make a new one for the plane and add
 * it to partset.
 */
static MeshPart *find_part_for_plane(BoolState *bs, MeshPartSet *partset, const double plane[4])
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
  copy_v4_v4_db(new_part->plane, plane);
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

/* Indices in IndexedCoordSets start at index_offset. */

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
  float feps = (float)bs->eps;

  if (do_dup_check) {
    i = coordset->index_offset;
    for (ln = coordset->listhead.list; ln; ln = ln->next) {
      q = (float *)ln->link;
      /* Note: compare_v3v3 checks if all three coords are within (<=) eps of each other. */
      if (compare_v3v3(p, q, feps)) {
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

static float *coordset_coord(const IndexedCoordSet *coordset, int index)
{
  int i;

  i = index - coordset->index_offset;
  if (i < 0 || i >= coordset->size) {
    return NULL;
  }
  LinkNode *ln = BLI_linklist_find(coordset->listhead.list, i);
  if (ln) {
    return (float *)(ln->link);
  }
  return NULL;
}

/** IntIntMap functions. */

static void init_intintmap(IntIntMap *intintmap)
{
  intintmap->listhead.list = NULL;
  intintmap->listhead.last_node = NULL;
}

static int intintmap_size(const IntIntMap *intintmap)
{
  return BLI_linklist_count(intintmap->listhead.list);
}

static void add_to_intintmap(BoolState *bs, IntIntMap *map, int key, int val)
{
  IntPair *keyvalpair;

  keyvalpair = BLI_memarena_alloc(bs->mem_arena, sizeof(*keyvalpair));
  keyvalpair->first = key;
  keyvalpair->second = val;
  BLI_linklist_append_arena(&map->listhead, keyvalpair, bs->mem_arena);
}

static bool find_in_intintmap(const IntIntMap *map, int key, int *r_val)
{
  LinkNode *ln;

  for (ln = map->listhead.list; ln; ln = ln->next) {
    IntPair *pair = (IntPair *)ln->link;
    if (pair->first == key) {
      *r_val = pair->second;
      return true;
    }
  }
  return false;
}

/* Note: this is a shallow copy: afterwards, dst and src will
 * share underlying list
 */
static void copy_intintmap_intintmap(IntIntMap *dst, IntIntMap *src)
{
  dst->listhead.list = src->listhead.list;
  dst->listhead.last_node = src->listhead.last_node;
}

static void set_intintmap_entry(BoolState *bs, IntIntMap *map, int key, int value)
{
  LinkNode *ln;

  for (ln = map->listhead.list; ln; ln = ln->next) {
    IntPair *pair = (IntPair *)ln->link;
    if (pair->first == key) {
      pair->second = value;
      return;
    }
  }
  add_to_intintmap(bs, map, key, value);
}

/* Abstract the IntIntMap iteration to allow for changes to
 * implementation later.
 * We allow the value of a map to be changed during
 * iteration, but not the key.
 */

static void intintmap_iter_init(IntIntMapIterator *iter, const IntIntMap *map)
{
  iter->map = map;
  iter->curlink = map->listhead.list;
  if (iter->curlink) {
    iter->keyvalue = (IntPair *)iter->curlink->link;
  }
  else {
    iter->keyvalue = NULL;
  }
}

static inline bool intintmap_iter_done(IntIntMapIterator *iter)
{
  return iter->curlink == NULL;
}

static void intintmap_iter_step(IntIntMapIterator *iter)
{
  iter->curlink = iter->curlink->next;
  if (iter->curlink) {
    iter->keyvalue = (IntPair *)iter->curlink->link;
  }
  else {
    iter->keyvalue = NULL;
  }
}

static inline int intintmap_iter_key(IntIntMapIterator *iter)
{
  return iter->keyvalue->first;
}

static inline int *intintmap_iter_valuep(IntIntMapIterator *iter)
{
  return &iter->keyvalue->second;
}

static inline int intintmap_iter_value(IntIntMapIterator *iter)
{
  return iter->keyvalue->second;
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
                                           const IntIntMap *in_to_im_vmap,
                                           const double rot_inv[3][3],
                                           const double z_for_inverse)
{
  IntersectOutput *isect_out;
  CDT_result *cout_copy;
  int out_v, in_v, im_v, start, new_v;
  IntIntMap *vmap;
  double p[3], q[3];
  float pf[3];

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
      if (!find_in_intintmap(in_to_im_vmap, in_v, &im_v)) {
        printf("whoops, didn't find %d in in_to_im_vmap\n", in_v);
        im_v = in_v;
      }
      add_to_intintmap(bs, vmap, out_v, im_v);
    }
    else {
      /* out_v needs a new vertex. Need to convert coords from 2d to 3d. */
      copy_v2db_v2fl(q, cout->vert_coords[out_v]);
      q[2] = z_for_inverse;
      mul_v3_m3v3_db(p, rot_inv, q);
      copy_v3fl_v3db(pf, p);
      new_v = add_to_coordset(bs, &isect_out->new_verts, pf, false);
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
  int min = INT_MAX;
  int i;

  for (i = 0; i < len; i++) {
    min = min_ii(min, array[i]);
  }
  return min;
}

/* Shallow copy. Result will share link pointers with original. */
static LinkNode *linklist_shallow_copy_arena(LinkNode *list, struct MemArena *arena)
{
  LinkNodePair listhead = {NULL, NULL};
  LinkNode *ln;

  for (ln = list; ln; ln = ln->next) {
    BLI_linklist_append_arena(&listhead, ln->link, arena);
  }

  return listhead.list;
}

/** Functions to move to Blenlib's math_geom when stable. */

/*
 * Return -1, 0, or 1 as (px,py) is right of, collinear (within epsilon) with, or left of line (l1,l2)
 */
static int line_point_side_v2_array_db(double l1x, double l1y, double l2x, double l2y, double px, double py, double epsilon)
{
	double det, dx, dy, lnorm;

	dx = l2x - l1x;
	dy = l2y - l1y;
	det = dx * (py - l1y) - dy * (px - l1x);
	lnorm = sqrt(dx * dx + dy * dy);
	if (fabs(det) < epsilon * lnorm)
		return 0;
	else if (det < 0)
		return -1;
	else
		return 1;
}

/**
 * Intersect two triangles, allowing for coplanar intersections too.
 *
 * The result can be any of: a single point, a segment, or an n-gon (n <= 6),
 * so can indicate all of these with a return array of coords max size 6 and a returned
 * length: if length is 1, then intersection is a point; if 2, then a segment;
 * if 3 or more, a (closed) ngon.
 * Use double arithmetic, and use consistent ordering of vertices and
 * segments in tests to make some precision problems less likely.
 * Algorithm: Tomas Möller's "A Fast Triangle-Triangle Intersection Test"
 *
 * \param r_pts, r_npts: Optional arguments to retrieve the intersection points between the 2 triangles.
 * \return true when the triangles intersect.
 *
 */
static bool UNUSED_FUNCTION(isect_tri_tri_epsilon_v3_db_ex)(
	const double t_a0[3], const double t_a1[3], const double t_a2[3],
	const double t_b0[3], const double t_b1[3], const double t_b2[3],
	double r_pts[6][3], int *r_npts,
	const double epsilon)
{
	const double *tri_pair[2][3] = {{t_a0, t_a1, t_a2}, {t_b0, t_b1, t_b2}};
	double co[2][3][3];
	double plane_a[4], plane_b[4];
	double plane_co[3], plane_no[3];
	int verti;

	BLI_assert((r_pts != NULL) == (r_npts != NULL));

        /* This is remnant from when input args were floats. TODO: remove this copying. */
	for (verti = 0; verti < 3; verti++) {
		copy_v3_v3_db(co[0][verti], tri_pair[0][verti]);
		copy_v3_v3_db(co[1][verti], tri_pair[1][verti]);
	}
	/* normalizing is needed for small triangles T46007 */
	normal_tri_v3_db(plane_a, UNPACK3(co[0]));
	normal_tri_v3_db(plane_b, UNPACK3(co[1]));

	plane_a[3] = -dot_v3v3_db(plane_a, co[0][0]);
	plane_b[3] = -dot_v3v3_db(plane_b, co[1][0]);

	if (isect_plane_plane_v3_db(plane_a, plane_b, plane_co, plane_no) &&
		(normalize_v3_d(plane_no) > epsilon))
	{
		struct {
			double min, max;
		} range[2] = {{DBL_MAX, -DBL_MAX}, {DBL_MAX, -DBL_MAX}};
		int t;
		double co_proj[3];

		closest_to_plane3_normalized_v3_db(co_proj, plane_no, plane_co);

		/* For both triangles, find the overlap with the line defined by the ray [co_proj, plane_no].
		 * When the ranges overlap we know the triangles do too. */
		for (t = 0; t < 2; t++) {
			int j, j_prev;
			double tri_proj[3][3];

			closest_to_plane3_normalized_v3_db(tri_proj[0], plane_no, co[t][0]);
			closest_to_plane3_normalized_v3_db(tri_proj[1], plane_no, co[t][1]);
			closest_to_plane3_normalized_v3_db(tri_proj[2], plane_no, co[t][2]);

			for (j = 0, j_prev = 2; j < 3; j_prev = j++) {
				/* note that its important to have a very small nonzero epsilon here
				 * otherwise this fails for very small faces.
				 * However if its too small, large adjacent faces will count as intersecting */
				const double edge_fac = line_point_factor_v3_ex_db(co_proj, tri_proj[j_prev], tri_proj[j], 1e-10, -1.0);
				if (UNLIKELY(edge_fac == -1.0)) {
					/* pass */
				}
				else if (edge_fac > -epsilon && edge_fac < 1.0 + epsilon) {
					double ix_tri[3];
					double span_fac;

					interp_v3_v3v3_db(ix_tri, co[t][j_prev], co[t][j], edge_fac);
					/* the actual distance, since 'plane_no' is normalized */
					span_fac = dot_v3v3_db(plane_no, ix_tri);

					range[t].min = min_dd(range[t].min, span_fac);
					range[t].max = max_dd(range[t].max, span_fac);
				}
			}

			if (range[t].min == DBL_MAX) {
				return false;
			}
		}

		if (((range[0].min > range[1].max + epsilon) ||
			 (range[0].max < range[1].min - epsilon)) == 0)
		{
			if (r_pts && r_npts) {
				double pt1[3], pt2[3];
				project_plane_normalized_v3_v3v3_db(plane_co, plane_co, plane_no);
				madd_v3_v3v3db_db(pt1, plane_co, plane_no, max_dd(range[0].min, range[1].min));
				madd_v3_v3v3db_db(pt2, plane_co, plane_no, min_dd(range[0].max, range[1].max));
				copy_v3_v3_db(r_pts[0], pt1);
				copy_v3_v3_db(r_pts[1], pt2);
				if (len_v3v3_db(pt1, pt2) <= epsilon)
					*r_npts = 1;
				else
					*r_npts = 2;
			}

			return true;
		}
	}
	else if (fabs(plane_a[3] - plane_b[3]) <= epsilon) {
		double pts[9][3];
		int ia, ip, ia_n, ia_nn, ip_prev, npts, same_side[6], ss, ss_prev, j;

		for (ip = 0; ip < 3; ip++)
			copy_v3_v3_db(pts[ip], co[1][ip]);
		npts = 3;

		/* a convex polygon vs convex polygon clipping algorithm */
		for (ia = 0; ia < 3; ia++) {
			ia_n = (ia + 1) % 3;
			ia_nn = (ia_n + 1) % 3;

			/* set same_side[i] = 0 if A[ia], A[ia_n], pts[ip] are collinear.
			 * else same_side[i] =1 if A[ia_nn] and pts[ip] are on same side of A[ia], A[ia_n].
			 * else same_side[i] = -1
			 */
			for (ip = 0; ip < npts; ip++) {
				double t;
				const double *l1 = co[0][ia];
				const double *l2 = co[0][ia_n];
				const double *p1 = co[0][ia_nn];
				const double *p2 = pts[ip];

				/* rather than projecting onto plane, do same-side tests projecting onto 3 ortho planes */
				t = line_point_side_v2_array_db(l1[0], l1[1], l2[0], l2[1], p1[0], p1[1], epsilon);
				if (t != 0.0) {
					same_side[ip] = t * line_point_side_v2_array_db(l1[0], l1[1], l2[0], l2[1], p2[0], p2[1], epsilon);
				}
				else {
					t = line_point_side_v2_array_db(l1[1], l1[2], l2[1], l2[2], p1[1], p1[2], epsilon);
					if (t != 0.0) {
						same_side[ip] = t * line_point_side_v2_array_db(l1[1], l1[2], l2[1], l2[2], p2[1], p2[2], epsilon);
					}
					else {
						t = line_point_side_v2_array_db(l1[0], l1[2], l2[0], l2[2], p1[0], p1[2], epsilon);
						same_side[ip] = t * line_point_side_v2_array_db(l1[0], l1[2], l2[0], l2[2], p2[0], p2[2], epsilon);
					}
				}
			}
			ip_prev = npts - 1;
			for (ip = 0; ip < (npts > 2 ? npts : npts - 1); ip++) {
				ss = same_side[ip];
				ss_prev = same_side[ip_prev];
				if ((ss_prev == 1 && ss == -1) || (ss_prev == -1 && ss == 1)) {
					/* do coplanar line-line intersect, specialized verison of isect_line_line_epsilon_v3_db */
					double *v1 = co[0][ia], *v2 = co[0][ia_n], *v3 = pts[ip_prev], *v4 = pts[ip];
					double a[3], b[3], c[3], ab[3], cb[3], isect[3], div;

					sub_v3_v3v3_db(c, v3, v1);
					sub_v3_v3v3_db(a, v2, v1);
					sub_v3_v3v3_db(b, v4, v3);

					cross_v3_v3v3_db(ab, a, b);
					div = dot_v3v3_db(ab, ab);
					if (div == 0.0) {
						/* shouldn't happen! */
						printf("div == 0, shouldn't happen\n");
						continue;
					}
					cross_v3_v3v3_db(cb, c, b);
					mul_v3db_db(a, dot_v3v3_db(cb, ab) / div);
					add_v3_v3v3_db(isect, v1, a);

					/* insert isect at current location */
					BLI_assert(npts < 9);
					for (j = npts; j > ip; j--) {
						copy_v3_v3_db(pts[j], pts[j - 1]);
						same_side[j] = same_side[j - 1];
					}
					copy_v3_v3_db(pts[ip], isect);
					same_side[ip] = 0;
					npts++;
				}
				ip_prev = ip;
			}
			/* cut out some pts that are no longer in intersection set */
			for (ip = 0; ip < npts; ) {
				if (same_side[ip] == -1) {
					/* remove point at ip */
					for (j = ip; j < npts - 1; j++) {
						copy_v3_v3_db(pts[j], pts[j + 1]);
						same_side[j] = same_side[j + 1];
					}
					npts--;
				}
				else {
					ip++;
				}
			}
		}

		*r_npts = npts;
		/* This copy is remant of old signature that returned floats. TODO: remove this copy. */
		for (ip = 0; ip < npts; ip++) {
			copy_v3_v3_db(r_pts[ip], pts[ip]);
		}
		return npts > 0;
	}
	if (r_npts)
		*r_npts = 0;
	return false;
}

/** Intersection Algorithm functions. */

/* Fill partset with parts for each plane for which there is a face
 * in bs->im.
 * Use bs->test_fn to check elements against test_val, to see whether or not it should be in the
 * result.
 */
static void find_coplanar_parts(BoolState *bs,
                                MeshPartSet *partset,
                                int test_val,
                                const char *label)
{
  IMesh *im = &bs->im;
  MeshPart *part;
  int im_nf, f, test;
  double plane[4];

  init_meshpartset(partset, label);
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
  calc_partset_bb_eps(bs, partset, bs->eps);
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
  IntIntMap in_to_im_vmap;
  double mat_2d[3][3];
  double mat_2d_inv[3][3];
  double xyz[3], save_z, p[3];
  float pf[3];
  bool ok;

  dump_part(part, "self_intersect_part");
  /* Find which vertices are needed for CDT input */
  part_nf = part_totface(part);
  if (part_nf <= 1) {
    printf("trivial 1 face case\n");
    return NULL;
  }
  init_intset(&verts_needed);
  init_intintmap(&in_to_im_vmap);
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
      add_to_intintmap(bs, &in_to_im_vmap, v_index, v);
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
  in.epsilon = (float)bs->eps;

  /* Fill in the vert_coords of CDT input */

  /* Find mat_2d: matrix to rotate so that plane normal moves to z axis */
  axis_dominant_v3_to_m3_db(mat_2d, part->plane);
  ok = invert_m3_m3_db(mat_2d_inv, mat_2d);
  BLI_assert(ok);

  for (i = 0; i < in.verts_len; i++) {
    v = intset_get_value_by_index(&verts_needed, i);
    BLI_assert(v != -1);
    imesh_get_vert_co(im, v, pf);
    copy_v3db_v3fl(p, pf);
    mul_v3_m3v3_db(xyz, mat_2d, p);
    copy_v2fl_v2db(in.vert_coords[i], xyz);
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

  isect_out = isect_out_from_cdt(bs, out, &in_to_im_vmap, mat_2d_inv, save_z);

  BLI_delaunay_2d_cdt_free(out);
  return isect_out;
}

/* Add any geometry resulting from intersectiong part1 with part2
 * into result_part.
 * If part1 and part2 have the same planes, all geometry that intersects
 * the bounding box of part1 will be added to result_part.
 */
static void add_part_intersections_to_part(BoolState *bs,
                                           MeshPart *result_part,
                                           const MeshPart *part1,
                                           const MeshPart *part2)
{
  int i, nface, f;

  if (planes_are_coplanar(part1->plane, part2->plane, bs->eps)) {
    nface = part_totface(part2);
    for (i = 0; i < nface; i++) {
      f = part_face(part2, i);
      /* TODO: see if f intersects part1's bb */
      add_face_to_part(bs, result_part, f);
    }
    /* TODO: loose verts and edges in part2 also go into result_part. */
  }
  else {
    /* TODO: non-coplanar part intersects. */
  }
}

/* Intersect part with all the parts in partset, except the part with
 * exclude_index, if that is not -1.
 */
static IntersectOutput *intersect_part_with_partset(BoolState *bs,
                                                    MeshPart *part,
                                                    MeshPartSet *partset,
                                                    int exclude_index)
{
  int i, partset_size;
  MeshPart *otherpart, *workpart;
  IntersectOutput *isect_out;

  printf("intersect_part_with_partset %s\n", partset->label);
  dump_part(part, "to intersect with");
  if (!part_may_intersect_partset(part, partset)) {
    printf("bb test rejects\n");
    return NULL;
  }
  partset_size = partset->tot_part;
  workpart = copy_part(bs, part);
  for (i = 0; i < partset_size; i++) {
    if (i != exclude_index) {
      otherpart = partset_part(partset, i);
      if (parts_may_intersect(part, otherpart)) {
        /* TODO: add any intersects between parts to copy of part */
        printf("possible part-part intersection\n");
        dump_part(otherpart, "otherpart");
        add_part_intersections_to_part(bs, workpart, part, otherpart);
      }
    }
  }
  /* TODO: self intersect the part copy and make an edit from it */
  isect_out = self_intersect_part(bs, workpart);
  return isect_out;
}

/* For building the inverse of a merge_to map. */
typedef struct MergeFrom {
  int target;
  LinkNodePair sources;
} MergeFrom;

/* Given a mergeto map, make the inverse, represented in an array of MergeFroms.
 * This means gathering together all of the sources that merge to the
 * same target.
 * Assume the merge_froms argument has enough space for the case where
 * every element of the merge_to map goes to a different target.
 * Return the number of merge_from slots actually used.
 */
static int fill_merge_from_array(BoolState *bs,
                                 MergeFrom *merge_froms,
                                 int merge_froms_size,
                                 IntIntMap *merge_to)
{
  int j, source, target, tot_merge_froms;
  IntIntMapIterator iter;

  tot_merge_froms = 0;
  for (intintmap_iter_init(&iter, merge_to); !intintmap_iter_done(&iter);
       intintmap_iter_step(&iter)) {
    target = intintmap_iter_value(&iter);
    source = intintmap_iter_key(&iter);
    /* Have we seen this target before? */
    for (j = 0; j < tot_merge_froms; j++) {
      if (merge_froms[j].target == target) {
        break;
      }
    }
    if (j >= merge_froms_size) {
      /* SHouldn't happen */
      BLI_assert(j < merge_froms_size); /* abort when debugging */
      return tot_merge_froms;
    }
    if (j == tot_merge_froms) {
      /* A new target */
      tot_merge_froms++;
      merge_froms[j].target = target;
      merge_froms[j].sources.list = NULL;
      merge_froms[j].sources.last_node = NULL;
    }
    BLI_linklist_append_arena(&merge_froms[j].sources, POINTER_FROM_INT(source), bs->mem_arena);
  }
  return tot_merge_froms;
}

/* Change the targets of both the target and all of the sources in merge_from
 * to be new_target.
 */
static void change_merge_set_target(BoolState *bs,
                                    IntIntMap *merge_to,
                                    MergeFrom *merge_from,
                                    int new_target)
{
  int source, target;
  LinkNode *ln;

  target = merge_from->target;
  set_intintmap_entry(bs, merge_to, target, new_target);
  for (ln = merge_from->sources.list; ln; ln = ln->next) {
    source = POINTER_AS_INT(ln->link);
    set_intintmap_entry(bs, merge_to, source, new_target);
  }
}

/* A merge_to map doesn't contain an entry for the usual case where
 * there is no merge, so that the source maps to itself.
 * Use this rule to apply a merge_to to a given source.
 */
static int apply_merge_to(IntIntMap *merge_to, int source)
{
  int target;

  if (find_in_intintmap(merge_to, source, &target)) {
    return target;
  }
  return source;
}

/* Make map_a have the effect of the union of map_a and map_b.
 * Each map represents how indices in the range [0, totv) merge
 * into other vertices in that range. We assume no transitive
 * merges happen in either map_a or map_b.
 */
static void union_merge_to_pair(BoolState *bs, IntIntMap *map_a, IntIntMap *map_b)
{
  int map_a_size, map_b_size, merge_from_a_size, merge_from_b_size;
  int i, j, k, merge_set_size, source, target, min_target, max_target;
  MergeFrom *merge_from_a, *merge_from_b;
  int *merge_set_rep;
  LinkNode *ln;

  map_a_size = intintmap_size(map_a);
  map_b_size = intintmap_size(map_b);

  if (map_b_size == 0) {
    return;
  }
  else if (map_a_size == 0) {
    /* Do a shallow copy (don't need map_b's container anymore). */
    copy_intintmap_intintmap(map_a, map_b);
    return;
  }
  merge_from_a = BLI_array_alloca(merge_from_a, (size_t)map_a_size);
  merge_from_a_size = fill_merge_from_array(bs, merge_from_a, map_a_size, map_a);
  merge_from_b = BLI_array_alloca(merge_from_b, (size_t)map_b_size);
  merge_from_b_size = fill_merge_from_array(bs, merge_from_b, map_b_size, map_b);
  merge_set_rep = BLI_array_alloca(merge_set_rep, (size_t)(map_b_size + 1));

  for (i = 0; i < merge_from_b_size; i++) {
    target = apply_merge_to(map_a, merge_from_b[i].target);
    merge_set_rep[0] = target;
    min_target = max_target = target;
    merge_set_size = 1;
    for (ln = merge_from_b[i].sources.list; ln; ln = ln->next) {
      source = POINTER_AS_INT(ln->link);
      target = apply_merge_to(map_a, source);
      min_target = min_ii(target, min_target);
      max_target = max_ii(target, max_target);
      BLI_assert(merge_set_size < map_b_size + 1);
      merge_set_rep[merge_set_size++] = target;
    }
    if (min_target < max_target) {
      /* Some elements of the ith merge set from map_b map to
       * different targets in map_a; so there are groups that
       * have to be merged in map_a.
       * merge_set_rep[0..j) has all the map targets in map_a
       * that must now be unified to point at the same place,
       * which for definiteness we choose to be min_target.
       */
      for (j = 0; j < merge_set_size; j++) {
        target = merge_set_rep[j];
        if (target != min_target) {
          for (k = 0; k < merge_from_a_size; k++) {
            if (merge_from_a[k].target == target) {
              change_merge_set_target(bs, map_a, &merge_from_a[k], min_target);
            }
          }
        }
      }
    }
  }
}

static void union_isect_out_pair(BoolState *bs, IntersectOutput *ioa, IntersectOutput *iob)
{
  int i, im_totv, v, v_moved, merge_target;
  int *moveto;
  float *vco;
  LinkNode *ln;
  IntIntMap *vmap;
  IntIntMapIterator iim_iter;

  /* Append iob's cdt_outputs and vertmaps to ioa's, sharing iob's list elements. */
  ioa->cdt_outputs.last_node->next = iob->cdt_outputs.list;
  ioa->cdt_outputs.last_node = iob->cdt_outputs.last_node;
  ioa->vertmaps.last_node->next = iob->vertmaps.list;
  ioa->vertmaps.last_node = iob->vertmaps.last_node;

  /* Find which of iob's new verts are needed, and put add them
   * to ioa's new verts. Indices of new verts start after last
   * index number of ioa's new_verts.
   */
  im_totv = imesh_totvert(&bs->im);
  /* moveto[i] records output index in new iob
   * for iob's output vert im_totv + i */
  moveto = BLI_array_alloca(moveto, (size_t)iob->new_verts.size);
  for (i = 0; i < iob->new_verts.size; i++) {
    vco = coordset_coord(&iob->new_verts, i + im_totv);
    BLI_assert(vco != NULL);
    v = add_to_coordset(bs, &ioa->new_verts, vco, true);
    moveto[i] = v;
  }

  union_merge_to_pair(bs, &ioa->merge_to, &iob->merge_to);

  /* Update the vertmaps for each of the cdt_outputs in iob. */
  for (ln = iob->vertmaps.list; ln; ln = ln->next) {
    vmap = (IntIntMap *)ln->link;
    for (intintmap_iter_init(&iim_iter, vmap); !intintmap_iter_done(&iim_iter);
         intintmap_iter_step(&iim_iter)) {
      int *p_v_moved = intintmap_iter_valuep(&iim_iter);
      v = intintmap_iter_key(&iim_iter);
      v_moved = *p_v_moved;
      if (v_moved < im_totv) {
        /* current map output is in index space of im */
        if (find_in_intintmap(vmap, v, &merge_target)) {
          *p_v_moved = merge_target;
        }
      }
      else {
        /* current map output is in index space of iob's new verts */
        BLI_assert(v_moved >= im_totv && v_moved < im_totv + iob->new_verts.size);
        *p_v_moved = moveto[v_moved - im_totv];
      }
    }
  }
}

/* Get the union effect of all of isect_outs[0, 1, ..., tot_isect_outs - 1].
 * The result should end up in isect_outs[0], and the rest may be altered.
 * TODO: perhpas parellize this by doing in pairs, in an accumulation tree.
 */
static void union_isect_out_array(BoolState *bs, IntersectOutput **isect_outs, int tot_isect_outs)
{
  int i;

  for (i = 1; i < tot_isect_outs; i++) {
    union_isect_out_pair(bs, isect_outs[0], isect_outs[i]);
  }
}

/* Intersect all parts.
 */
static void intersect_all_parts(BoolState *bs, MeshPartSet *partset)
{
  int i, tot_part, o_index;
  MeshPart *part;
  IntersectOutput **isect_out, *out;

  tot_part = partset->tot_part;
  isect_out = BLI_memarena_alloc(bs->mem_arena, (size_t)tot_part * sizeof(*isect_out));
  o_index = 0;
  for (i = 0; i < tot_part; i++) {
    part = partset_part(partset, i);
    out = intersect_part_with_partset(bs, part, partset, i);
    if (out) {
      isect_out[o_index++] = out;
    }
  }
  if (o_index == 0) {
    printf("got no non-null outputs\n");
    return;
  }
  printf("about to union %d outputs\n", o_index);
  union_isect_out_array(bs, isect_out, o_index);
  dump_isect_out(isect_out[0], "union");
  apply_isect_out_to_imesh(&bs->im, isect_out[0]);
}

/* Intersect all parts of a_partset with all parts of b_partset.
 */
static void intersect_parts_pair(BoolState *bs, MeshPartSet *a_partset, MeshPartSet *b_partset)
{
  int a_index, o_index, tot_part_a, tot_part_b, tot;
  MeshPart *part;
  IntersectOutput **isect_out, *out;

  tot_part_a = a_partset->tot_part;
  tot_part_b = b_partset->tot_part;
  tot = tot_part_a + tot_part_b;
  isect_out = BLI_memarena_alloc(bs->mem_arena, (size_t)tot * sizeof(IntersectOutput *));
  o_index = 0;
  for (a_index = 0; a_index < tot_part_a; a_index++) {
    part = partset_part(a_partset, a_index);
    out = intersect_part_with_partset(bs, part, b_partset, -1);
    if (out) {
      isect_out[o_index++] = out;
    }
  }
  if (o_index == 0) {
    printf("got no non-null outputs\n");
    return;
  }
  printf("about to union %d outputs\n", o_index);
  union_isect_out_array(bs, isect_out, o_index);
  dump_isect_out(isect_out[0], "union");
  apply_isect_out_to_imesh(&bs->im, isect_out[0]);
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
    find_coplanar_parts(&bs, &all_parts, TEST_ALL, "all");
    dump_partset(&all_parts);
    intersect_all_parts(&bs, &all_parts);
  }
  else {
    find_coplanar_parts(&bs, &a_parts, TEST_A, "A");
    dump_partset(&a_parts);
    find_coplanar_parts(&bs, &b_parts, TEST_B, "B");
    dump_partset(&b_parts);
    intersect_parts_pair(&bs, &a_parts, &b_parts);
  }

  BLI_memarena_free(bs.mem_arena);
  return true;
}

#ifdef BOOLDEBUG
static void dump_part(const MeshPart *part, const char *label)
{
  LinkNode *ln;
  int i;
  struct namelist {
    const char *name;
    LinkNode *list;
  } nl[3] = {{"verts", part->verts}, {"edges", part->edges}, {"faces", part->faces}};

  printf("PART %s\n", label);
  for (i = 0; i < 3; i++) {
    if (nl[i].list) {
      printf("  %s:{ ", nl[i].name);
      for (ln = nl[i].list; ln; ln = ln->next) {
        printf("%d ", POINTER_AS_INT(ln->link));
      }
      printf("}\n");
    }
  }
  printf("  plane=(%.3f,%.3f,%.3f),%.3f:\n", F4(part->plane));
  printf("  bb=(%.3f,%.3f,%.3f)(%.3f,%.3f,%.3f)\n", F3(part->bbmin), F3(part->bbmax));
}

static void dump_partset(const MeshPartSet *partset)
{
  int i;
  MeshPart *part;
  char partlab[20];

  printf("PARTSET %s\n", partset->label);
  for (i = 0; i < partset->tot_part; i++) {
    part = partset_part(partset, i);
    sprintf(partlab, "%d", i);
    if (!part) {
      printf("<NULL PART>\n");
    }
    else {
      dump_part(part, partlab);
    }
  }
  printf(
      "partset bb=(%.3f,%.3f,%.3f)(%.3f,%.3f,%.3f)\n\n", F3(partset->bbmin), F3(partset->bbmax));
}

static void dump_intintmap(const IntIntMap *map, const char *label, const char *prefix)
{
  IntIntMapIterator iter;

  printf("%sintintmap %s\n", prefix, label);
  for (intintmap_iter_init(&iter, map); !intintmap_iter_done(&iter); intintmap_iter_step(&iter)) {
    printf("%s  %d -> %d\n", prefix, intintmap_iter_key(&iter), intintmap_iter_value(&iter));
  }
}

static void dump_intlist_from_tables(const int *table,
                                     const int *start_table,
                                     const int *len_table,
                                     int index)
{
  int start, len, i;

  start = start_table[index];
  len = len_table[index];
  for (i = 0; i < len; i++) {
    printf("%d", table[start + i]);
    if (i < len - 1) {
      printf(" ");
    }
  }
}

static void dump_cdt_result(const CDT_result *cdt, const char *label, const char *prefix)
{
  int i;

  printf("%scdt result %s\n", prefix, label);
  printf("%s  verts\n", prefix);
  for (i = 0; i < cdt->verts_len; i++) {
    printf("%s  %d: (%.3f,%.3f) orig=[", prefix, i, F2(cdt->vert_coords[i]));
    dump_intlist_from_tables(
        cdt->verts_orig, cdt->verts_orig_start_table, cdt->verts_orig_len_table, i);
    printf("]\n");
  }
  printf("%s  edges\n", prefix);
  for (i = 0; i < cdt->edges_len; i++) {
    printf("%s (%d,%d) orig=[", prefix, cdt->edges[i][0], cdt->edges[i][1]);
    dump_intlist_from_tables(
        cdt->edges_orig, cdt->edges_orig_start_table, cdt->edges_orig_len_table, i);
    printf("]\n");
  }
  printf("%s  faces\n", prefix);
  for (i = 0; i < cdt->faces_len; i++) {
    printf("%s ", prefix);
    dump_intlist_from_tables(cdt->faces, cdt->faces_start_table, cdt->faces_len_table, i);
    printf(" orig=[");
    dump_intlist_from_tables(
        cdt->faces_orig, cdt->faces_orig_start_table, cdt->faces_orig_len_table, i);
    printf("]\n");
  }
}

static void dump_isect_out(const IntersectOutput *io, const char *label)
{
  int i, index;
  float *co;
  LinkNode *ln, *lnmap;
  CDT_result *cdt;
  IntIntMap *vmap;
  char lab[30];

  printf("INTERSECTOUTPUT %s\n", label);
  printf("  orig_mesh_verts_tot=%d\n", io->orig_mesh_verts_tot);
  printf("  new verts:\n");
  for (i = 0; i < io->new_verts.size; i++) {
    index = i + io->orig_mesh_verts_tot;
    co = coordset_coord(&io->new_verts, index);
    printf("    %d: (%.3f,%.3f,%.3f)\n", index, F3(co));
  }
  if (io->merge_to.listhead.list != NULL) {
    dump_intintmap(&io->merge_to, "merge_to", "    ");
  }
  i = 0;
  for (ln = io->cdt_outputs.list, lnmap = io->vertmaps.list; ln;
       ln = ln->next, lnmap = lnmap->next) {
    cdt = (CDT_result *)ln->link;
    vmap = (IntIntMap *)lnmap->link;
    sprintf(lab, "%d", i);
    dump_cdt_result(cdt, lab, "  ");
    sprintf(lab, "vertmap %d", i);
    dump_intintmap(vmap, lab, "  ");
  }
}
#endif
