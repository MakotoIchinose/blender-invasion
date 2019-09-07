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

/* MeshAdd holds an incremental addition to an IMesh.
 * New verts, edges, and faces are given indices starting
 * beyond those of the underlying IMesh, and that new
 * geometry is stored here. For edges and faces, the
 * indices used can be either from the IMesh or from the
 * new geometry stored here.
 * Sometimes the new geometric elements are based on
 * an example element in the underlying IMesh (the example
 * will be used to copy attributes).
 * So we store examples here too.
 */
typedef struct MeshAdd {
  LinkNodePair verts; /* Links are NewVerts. */
  LinkNodePair edges; /* Links are NewEdges. */
  LinkNodePair faces; /* Links are NewFaces. */
  int vindex_start;   /* Add this to position in verts to get index of new vert. */
  int eindex_start;   /* Add this to position in edges to get index of new edge. */
  int findex_start;   /* Add this to position in faces to get index of new face. */
  IMesh *im;          /* Underlying IMesh. */
} MeshAdd;

typedef struct NewVert {
  float co[3];
  int example; /* If not -1, example vert in IMesh. */
} NewVert;

typedef struct NewEdge {
  int v1;
  int v2;
  int example; /* If not -1, example edge in IMesh. */
} NewEdge;

typedef struct NewFace {
  IntPair *vert_edge_pairs; /* Array of len (vert, edge) pairs. */
  int len;
  int example; /* If not -1, example face in IMesh. */
} NewFace;

/* A MeshPart is a subset of the geometry of an IndexMesh,
 * with some possible additional geometry.
 * The indices refer to vertex, edges, and faces in the IndexMesh
 * that this part is based on,
 * or, if the indices are larger than the total in the IndexMesh,
 * then it is in extra geometry incrementally added.
 * Unlike for IndexMesh, the edges implied by faces need not be explicitly
 * represented here.
 * Commonly a MeshPart will contain geometry that shares a plane,
 * and when that is so, the plane member says which plane,
 * TODO: faster structure for looking up verts, edges, faces.
 */
typedef struct MeshPart {
  double plane[4]; /* First 3 are normal, 4th is signed distance to plane. */
  double bbmin[3]; /* Bounding box min, with eps padding. */
  double bbmax[3]; /* Bounding box max, with eps padding. */
  LinkNode *verts; /* Links are ints (vert indices). */
  LinkNode *edges; /* Links are ints (edge indices). */
  LinkNode *faces; /* Links are ints (face indices). */
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

/* An IMeshPlus is an IMesh plus a MeshAdd.
 * If the element indices are in range for the IMesh, then functions
 * access those, else they access the MeshAdd.
 */
typedef struct IMeshPlus {
  IMesh *im;
  MeshAdd *meshadd;
} IMeshPlus;

/* Result of intersecting two MeshParts.
 * This only need identify the thngs that probably intersect,
 * as the actual intersections will be done later, when
 * parts are self-intersected. Dedup will handle any problems.
 * It is not necessary to include verts that are part of included
 * edges, nor edges that are part of included faces.
 */
typedef struct PartPartIntersect {
  LinkNodePair verts; /* Links are vert indices. */
  LinkNodePair edges; /* Links are edge indices. */
  LinkNodePair faces; /* Links are face indices. */
  int a_index;
  int b_index;
} PartPartIntersect;

typedef struct BoolState {
  MemArena *mem_arena;
  IMesh im;
  int boolean_mode;
  double eps;
  int (*test_fn)(void *elem, void *user_data);
  void *test_fn_user_data;
} BoolState;

/* test_fn results used to distinguish parts of mesh. */
enum { TEST_NONE = -1, TEST_B = 0, TEST_A = 1, TEST_ALL = 2 };

/* Decoration to shut up gnu 'unused function' warning. */
#ifdef __GNUC__
#  define ATTU __attribute__((unused))
#else
#  define ATTU
#endif

#define BOOLDEBUG
#ifdef BOOLDEBUG
/* For Debugging. */
#  define CO3(v) (v)->co[0], (v)->co[1], (v)->co[2]
#  define F2(v) (v)[0], (v)[1]
#  define F3(v) (v)[0], (v)[1], (v)[2]
#  define F4(v) (v)[0], (v)[1], (v)[2], (v)[3]
#  define BMI(e) BM_elem_index_get(e)

ATTU static void dump_part(const MeshPart *part, const char *label);
ATTU static void dump_partset(const MeshPartSet *pset);
ATTU static void dump_partpartintersect(const PartPartIntersect *ppi, const char *label);
ATTU static void dump_meshadd(const MeshAdd *ma, const char *label);
ATTU static void dump_intintmap(const IntIntMap *map, const char *label, const char *prefix);
ATTU static void dump_cdt_input(const CDT_input *cdt, const char *label);
ATTU static void dump_cdt_result(const CDT_result *cdt, const char *label, const char *prefix);
#endif

/* Forward declarations of some static functions. */
static int min_int_in_array(int *array, int len);
static LinkNode *linklist_shallow_copy_arena(LinkNode *list, struct MemArena *arena);
static void calc_part_bb_eps(BoolState *bs, MeshPart *part, double eps);
static float *coordset_coord(const IndexedCoordSet *coordset, int index);
static bool find_in_intintmap(const IntIntMap *map, int key, int *r_val);

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

ATTU static void init_coordset(IndexedCoordSet *coordset, int index_offset)
{
  coordset->listhead.list = NULL;
  coordset->listhead.last_node = NULL;
  coordset->index_offset = index_offset;
  coordset->size = 0;
}

ATTU static int add_to_coordset(BoolState *bs,
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

ATTU static float *coordset_coord(const IndexedCoordSet *coordset, int index)
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

ATTU static inline int *intintmap_iter_valuep(IntIntMapIterator *iter)
{
  return &iter->keyvalue->second;
}

static inline int intintmap_iter_value(IntIntMapIterator *iter)
{
  return iter->keyvalue->second;
}

/** Miscellaneous utility functions. */
#pragma mark Miscellaneous utility functions

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

/* Get to last node of a liskned list. Also return list size in r_count. */
ATTU static LinkNode *linklist_last(LinkNode *ln, int *r_count)
{
  int i;

  if (ln) {
    i = 1;
    for (; ln->next; ln = ln->next) {
      i++;
    }
    *r_count = i;
    return ln;
  }
  *r_count = 0;
  return NULL;
}

/** Functions to move to Blenlib's math_geom when stable. */

#if 0
/* This general tri-tri intersect routine may be useful as an optimization later, but is unused for now. */

/*
 * Return -1, 0, or 1 as (px,py) is right of, collinear (within epsilon) with, or left of line
 * (l1,l2)
 */
static int line_point_side_v2_array_db(
    double l1x, double l1y, double l2x, double l2y, double px, double py, double epsilon)
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
 * \param r_pts, r_npts: Optional arguments to retrieve the intersection points between the 2
 * triangles. \return true when the triangles intersect.
 *
 */
static bool isect_tri_tri_epsilon_v3_db_ex(const double t_a0[3],
                                           const double t_a1[3],
                                           const double t_a2[3],
                                           const double t_b0[3],
                                           const double t_b1[3],
                                           const double t_b2[3],
                                           double r_pts[6][3],
                                           int *r_npts,
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
      (normalize_v3_d(plane_no) > epsilon)) {
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
        const double edge_fac = line_point_factor_v3_ex_db(
            co_proj, tri_proj[j_prev], tri_proj[j], 1e-10, -1.0);
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

    if (((range[0].min > range[1].max + epsilon) || (range[0].max < range[1].min - epsilon)) ==
        0) {
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
          same_side[ip] = t * line_point_side_v2_array_db(
                                  l1[0], l1[1], l2[0], l2[1], p2[0], p2[1], epsilon);
        }
        else {
          t = line_point_side_v2_array_db(l1[1], l1[2], l2[1], l2[2], p1[1], p1[2], epsilon);
          if (t != 0.0) {
            same_side[ip] = t * line_point_side_v2_array_db(
                                    l1[1], l1[2], l2[1], l2[2], p2[1], p2[2], epsilon);
          }
          else {
            t = line_point_side_v2_array_db(l1[0], l1[2], l2[0], l2[2], p1[0], p1[2], epsilon);
            same_side[ip] = t * line_point_side_v2_array_db(
                                    l1[0], l1[2], l2[0], l2[2], p2[0], p2[2], epsilon);
          }
        }
      }
      ip_prev = npts - 1;
      for (ip = 0; ip < (npts > 2 ? npts : npts - 1); ip++) {
        ss = same_side[ip];
        ss_prev = same_side[ip_prev];
        if ((ss_prev == 1 && ss == -1) || (ss_prev == -1 && ss == 1)) {
          /* do coplanar line-line intersect, specialized verison of isect_line_line_epsilon_v3_db
           */
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
      for (ip = 0; ip < npts;) {
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
#endif

/* Does the line intersect the segment (within epsilon)?
 * If line and seg are parallel and coincident, return 2.
 * If line intersects the segment within epsilon, return 1.
 * Otherwise return 0.
 * If the return value is 1, put the intersection point in isect,
 * and the fraction of the way from line_v1 to line_v2 to isect in *r_lambda
 * (may be outside of the range [0,1]) and the fraction of the way from seg_v1
 * to seg_v2 in *r_mu.
 */
static int isect_line_seg_epsilon_v3_db(const double line_v1[3],
                                        const double line_v2[3],
                                        const double seg_v1[3],
                                        const double seg_v2[3],
                                        double epsilon,
                                        double isect[3],
                                        double *r_lambda,
                                        double *r_mu)
{
  double fac_a, fac_b;
  double a_dir[3], b_dir[3], a0b0[3], crs_ab[3];
  double epsilon_squared = epsilon * epsilon;
  sub_v3_v3v3_db(a_dir, line_v2, line_v1);
  sub_v3_v3v3_db(b_dir, seg_v2, seg_v1);
  cross_v3_v3v3_db(crs_ab, b_dir, a_dir);
  const double nlen = len_squared_v3_db(crs_ab);

  if (nlen < epsilon_squared) {
    /* Parallel Lines or near parallel. Are the coincident? */
    double d1, d2;
    d1 = dist_squared_to_line_v3_db(seg_v1, line_v1, line_v2);
    d2 = dist_squared_to_line_v3_db(seg_v2, line_v1, line_v2);
    if (d1 <= epsilon_squared || d2 <= epsilon_squared) {
      return 2;
    }
    else {
      return 0;
    }
  }
  else {
    double c[3], cray[3];
    double blen_squared = len_squared_v3_db(b_dir);
    sub_v3_v3v3_db(a0b0, seg_v1, line_v1);
    sub_v3_v3v3_db(c, crs_ab, a0b0);

    cross_v3_v3v3_db(cray, c, b_dir);
    fac_a = dot_v3v3_db(cray, crs_ab) / nlen;

    cross_v3_v3v3_db(cray, c, a_dir);
    fac_b = dot_v3v3_db(cray, crs_ab) / nlen;

    if (fac_b * blen_squared < -epsilon_squared ||
        (fac_b - 1.0) * blen_squared > epsilon_squared) {
      return 0;
    }
    else {
      madd_v3_v3v3db_db(isect, seg_v1, b_dir, fac_b);
      *r_lambda = fac_a;
      *r_mu = fac_b;
      return 1;
    }
  }
}

/** IMesh functions. */
#pragma mark IMesh functions

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

static int imesh_totedge(const IMesh *im)
{
  if (im->bm) {
    return im->bm->totedge;
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

static int imesh_face_vert(const IMesh *im, int f, int index)
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

static void imesh_get_vert_co_db(const IMesh *im, int v, double *r_coords)
{
  if (im->bm) {
    BMVert *bmv = BM_vert_at_index(im->bm, v);
    if (bmv) {
      copy_v3db_v3fl(r_coords, bmv->co);
      return;
    }
    else {
      zero_v3_db(r_coords);
    }
  }
  else {
    ; /* TODO */
  }
}

/* Find a vertex in im eps-close to co, if it exists.
 * Else return -1.
 * TODO: speed this up.
 */
static int imesh_find_co_db(const IMesh *im, const double co[3], double eps)
{
  int v;
  BMesh *bm = im->bm;
  float feps = (float)eps;
  float fco[3];

  copy_v3fl_v3db(fco, co);
  if (bm) {
    for (v = 0; v < bm->totvert; v++) {
      BMVert *bmv = BM_vert_at_index(bm, v);
      if (compare_v3v3(fco, bmv->co, feps)) {
        return v;
      }
    }
    return -1;
  }
  else {
    return -1; /* TODO */
  }
}

/* Find an edge in im between given two verts (either order ok), if it exists.
 * Else return -1.
 * TODO: speed this up.
 */
static int imesh_find_edge(const IMesh *im, int v1, int v2)
{
  BMesh *bm = im->bm;
  int e;

  if (bm) {
    if (v1 >= bm->totvert || v2 >= bm->totvert) {
      return -1;
    }
    for (e = 0; e < bm->totedge; e++) {
      BMEdge *bme = BM_edge_at_index(bm, e);
      int ev1 = BM_elem_index_get(bme->v1);
      int ev2 = BM_elem_index_get(bme->v2);
      if ((ev1 == v1 && ev2 == v2) || (ev1 == v2 && ev2 == v1)) {
        return e;
      }
    }
    return -1;
  }
  else {
    return -1; /* TODO */
  }
}

static void imesh_get_edge_cos_db(const IMesh *im, int e, double *r_coords1, double *r_coords2)
{
  if (im->bm) {
    BMEdge *bme = BM_edge_at_index(im->bm, e);
    if (bme) {
      copy_v3db_v3fl(r_coords1, bme->v1->co);
      copy_v3db_v3fl(r_coords2, bme->v2->co);
    }
    else {
      zero_v3_db(r_coords1);
      zero_v3_db(r_coords2);
    }
  }
  else {
    ; /* TODO */
  }
}

static void imesh_get_edge_verts(const IMesh *im, int e, int *r_v1, int *r_v2)
{
  if (im->bm) {
    BMEdge *bme = BM_edge_at_index(im->bm, e);
    if (bme) {
      *r_v1 = BM_elem_index_get(bme->v1);
      *r_v2 = BM_elem_index_get(bme->v2);
    }
    else {
      *r_v1 = -1;
      *r_v2 = -1;
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

#if 0
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
#endif

static void apply_meshadd_to_bmesh(BMesh *bm,
                                   const MeshAdd *meshadd,
                                   const IntIntMap *vert_merge_map)
{
  /* TODO: implement apply_meshadd_to_bmesh */
  UNUSED_VARS(bm, meshadd, vert_merge_map);
}

static void apply_meshadd_to_imesh(IMesh *im,
                                   const MeshAdd *meshadd,
                                   const IntIntMap *vert_merge_map)
{
  if (meshadd == NULL) {
    return;
  }
  if (im->bm) {
    apply_meshadd_to_bmesh(im->bm, meshadd, vert_merge_map);
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

/** MeshAdd functions. */
#pragma mark MeshAdd functions

static void init_meshadd(BoolState *bs, MeshAdd *meshadd)
{
  IMesh *im = &bs->im;

  memset(meshadd, 0, sizeof(MeshAdd));
  meshadd->im = im;
  meshadd->vindex_start = imesh_totvert(im);
  meshadd->eindex_start = imesh_totedge(im);
  meshadd->findex_start = imesh_totface(im);
}

static int meshadd_add_vert(
    BoolState *bs, MeshAdd *meshadd, const float co[3], int example, bool checkdup)
{
  NewVert *newv;
  MemArena *arena = bs->mem_arena;
  LinkNode *ln;
  int i;

  if (checkdup) {
    for (ln = meshadd->verts.list, i = meshadd->vindex_start; ln; ln = ln->next, i++) {
      if (compare_v3v3((float *)ln->link, co, bs->eps)) {
        return i;
      }
    }
  }
  newv = BLI_memarena_alloc(arena, sizeof(*newv));
  copy_v3_v3(newv->co, co);
  newv->example = example;
  BLI_linklist_append_arena(&meshadd->verts, newv, arena);
  return meshadd->vindex_start + BLI_linklist_count(meshadd->verts.list) - 1;
}

static int meshadd_add_vert_db(
    BoolState *bs, MeshAdd *meshadd, const double co[3], int example, bool checkdup)
{
  float fco[3];
  copy_v3fl_v3db(fco, co);
  return meshadd_add_vert(bs, meshadd, fco, example, checkdup);
}

static int meshadd_add_edge(
    BoolState *bs, MeshAdd *meshadd, int v1, int v2, int example, bool checkdup)
{
  NewEdge *newe;
  MemArena *arena = bs->mem_arena;
  LinkNode *ln;
  int i;

  if (checkdup) {
    for (ln = meshadd->edges.list, i = meshadd->eindex_start; ln; ln = ln->next, i++) {
      int *pair = (int *)ln->link;
      if ((pair[0] == v1 && pair[1] == v2) || (pair[0] == v2 && pair[1] == v1)) {
        return i;
      }
    }
  }
  newe = BLI_memarena_alloc(arena, sizeof(*newe));
  newe->v1 = v1;
  newe->v2 = v2;
  newe->example = example;
  BLI_linklist_append_arena(&meshadd->edges, newe, arena);
  return meshadd->eindex_start + BLI_linklist_count(meshadd->edges.list) - 1;
}

/* This assumes that vert_edge is an arena-allocated array that will persist. */
static int meshadd_add_face(
    BoolState *bs, MeshAdd *meshadd, IntPair *vert_edge, int len, int example)
{
  NewFace *newf;
  MemArena *arena = bs->mem_arena;

  newf = BLI_memarena_alloc(arena, sizeof(*newf));
  newf->vert_edge_pairs = vert_edge;
  newf->len = len;
  newf->example = example;
  BLI_linklist_append_arena(&meshadd->faces, newf, arena);
  return meshadd->findex_start + BLI_linklist_count(meshadd->faces.list) - 1;
}

static int meshadd_facelen(const MeshAdd *meshadd, int f)
{
  LinkNode *ln;
  NewFace *nf;

  ln = BLI_linklist_find(meshadd->faces.list, f - meshadd->findex_start);
  if (ln) {
    nf = (NewFace *)ln->link;
    return nf->len;
  }
  return 0;
}

static int meshadd_face_vert(const MeshAdd *meshadd, int f, int index)
{
  LinkNode *ln;
  NewFace *nf;

  ln = BLI_linklist_find(meshadd->faces.list, f - meshadd->findex_start);
  if (ln) {
    nf = (NewFace *)ln->link;
    if (index >= 0 && index < nf->len) {
      return nf->vert_edge_pairs[index].first;
    }
  }
  return -1;
}

static void meshadd_get_vert_co(const MeshAdd *meshadd, int v, float *r_coords)
{
  LinkNode *ln;
  NewVert *nv;

  ln = BLI_linklist_find(meshadd->verts.list, v - meshadd->vindex_start);
  if (ln) {
    nv = (NewVert *)ln->link;
    copy_v3_v3(r_coords, nv->co);
  }
  else {
    zero_v3(r_coords);
  }
}

static void meshadd_get_vert_co_db(const MeshAdd *meshadd, int v, double *r_coords)
{
  float fco[3];

  meshadd_get_vert_co(meshadd, v, fco);
  copy_v3db_v3fl(r_coords, fco);
}

static void meshadd_get_edge_verts(const MeshAdd *meshadd, int e, int *r_v1, int *r_v2)
{
  LinkNode *ln;
  NewEdge *ne;

  ln = BLI_linklist_find(meshadd->edges.list, e - meshadd->eindex_start);
  if (ln) {
    ne = (NewEdge *)ln->link;
    *r_v1 = ne->v1;
    *r_v2 = ne->v2;
  }
  else {
    *r_v1 = -1;
    *r_v2 = -1;
  }
}

static int find_edge_by_verts_in_meshadd(const MeshAdd *meshadd, int v1, int v2)
{
  LinkNode *ln;
  NewEdge *ne;
  int i;

  for (ln = meshadd->edges.list, i = 0; ln; ln = ln->next, i++) {
    ne = (NewEdge *)ln->link;
    if ((ne->v1 == v1 && ne->v2 == v2) || (ne->v1 == v2 && ne->v2 == v1)) {
      return meshadd->eindex_start + i;
    }
  }
  return -1;
}

/** MeshPartSet functions. */
#pragma mark MeshPartSet functions

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
#pragma mark MeshPart functions

static void init_meshpart(BoolState *UNUSED(bs), MeshPart *part)
{
  memset(part, 0, sizeof(*part));
}

ATTU static MeshPart *copy_part(BoolState *bs, const MeshPart *part)
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

static int part_totvert(const MeshPart *part)
{
  return BLI_linklist_count(part->verts);
}

static int part_totedge(const MeshPart *part)
{
  return BLI_linklist_count(part->edges);
}

static int part_totface(const MeshPart *part)
{
  return BLI_linklist_count(part->faces);
}

/*
 * Return the index in MeshPart space of the index'th face in part.
 * "MeshPart space" means that if the f returned is in the range of
 * face indices in the underlying IMesh, then it represents the face
 * in the IMesh. If f is greater than or equal to that, then it represents
 * the face that is (f - im's totf)th in the new_faces list.
 */
static int part_face(const MeshPart *part, int index)
{
  LinkNode *ln;

  ln = BLI_linklist_find(part->faces, index);
  if (ln) {
    return POINTER_AS_INT(ln->link);
  }
  return -1;
}

/* Like part_face, but for vertices. */
static int part_vert(const MeshPart *part, int index)
{
  LinkNode *ln;

  ln = BLI_linklist_find(part->verts, index);
  if (ln) {
    return POINTER_AS_INT(ln->link);
  }
  return -1;
}

/* Like part_face, but for edges. */
static int part_edge(const MeshPart *part, int index)
{
  LinkNode *ln;

  ln = BLI_linklist_find(part->edges, index);
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

  copy_v3_db(part->bbmin, DBL_MAX);
  copy_v3_db(part->bbmax, -DBL_MAX);
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
  if (part->bbmin[0] == DBL_MAX) {
    zero_v3_db(part->bbmin);
    zero_v3_db(part->bbmax);
    return;
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
  init_meshpart(bs, new_part);
  copy_v4_v4_db(new_part->plane, plane);
  add_part_to_partset(bs, partset, new_part);
  return new_part;
}

ATTU static void add_vert_to_part(BoolState *bs, MeshPart *part, int v)
{
  BLI_linklist_prepend_arena(&part->verts, POINTER_FROM_INT(v), bs->mem_arena);
}

ATTU static void add_edge_to_part(BoolState *bs, MeshPart *part, int e)
{
  BLI_linklist_prepend_arena(&part->verts, POINTER_FROM_INT(e), bs->mem_arena);
}

static void add_face_to_part(BoolState *bs, MeshPart *part, int f)
{
  BLI_linklist_prepend_arena(&part->faces, POINTER_FROM_INT(f), bs->mem_arena);
}

/* If part consists of only one face from IMesh, return the number of vertices
 * in the face. Else return 0.
 */
static int part_is_one_im_face(BoolState *bs, const MeshPart *part)
{
  int f;

  if (part->verts == NULL && part->edges == NULL && part->faces != NULL &&
      BLI_linklist_count(part->faces) == 1) {
    f = POINTER_AS_INT(part->faces->link);
    return imesh_facelen(&bs->im, f);
  }
  return 0;
}

/** IMeshPlus functions. */
#pragma mark IMeshPlus functions

static void init_imeshplus(IMeshPlus *imp, IMesh *im, MeshAdd *meshadd)
{
  imp->im = im;
  imp->meshadd = meshadd;
}

static int imeshplus_facelen(const IMeshPlus *imp, int f)
{
  IMesh *im = imp->im;
  return (f < imesh_totface(im)) ? imesh_facelen(im, f) : meshadd_facelen(imp->meshadd, f);
}

static int imeshplus_face_vert(const IMeshPlus *imp, int f, int index)
{
  IMesh *im = imp->im;
  return (f < imesh_totface(im)) ? imesh_face_vert(im, f, index) :
                                   meshadd_face_vert(imp->meshadd, f, index);
}

ATTU static void imeshplus_get_vert_co(const IMeshPlus *imp, int v, float *r_coords)
{
  IMesh *im = imp->im;
  if (v < imesh_totvert(im)) {
    imesh_get_vert_co(im, v, r_coords);
  }
  else {
    meshadd_get_vert_co(imp->meshadd, v, r_coords);
  }
}

static void imeshplus_get_vert_co_db(const IMeshPlus *imp, int v, double *r_coords)
{
  IMesh *im = imp->im;
  if (v < imesh_totvert(im)) {
    imesh_get_vert_co_db(im, v, r_coords);
  }
  else {
    meshadd_get_vert_co_db(imp->meshadd, v, r_coords);
  }
}

static void imeshplus_get_edge_verts(const IMeshPlus *imp, int e, int *r_v1, int *r_v2)
{
  IMesh *im = imp->im;
  if (e < imesh_totedge(im)) {
    imesh_get_edge_verts(im, e, r_v1, r_v2);
  }
  else {
    meshadd_get_edge_verts(imp->meshadd, e, r_v1, r_v2);
  }
}

/** PartPartIntersect functions. */
#pragma mark PartPartIntersect functions

static void init_partpartintersect(PartPartIntersect *ppi)
{
  memset(ppi, 0, sizeof(*ppi));
}

static void add_vert_to_partpartintersect(BoolState *bs, PartPartIntersect *ppi, int v)
{
  BLI_linklist_append_arena(&ppi->verts, POINTER_FROM_INT(v), bs->mem_arena);
}

static void add_edge_to_partpartintersect(BoolState *bs, PartPartIntersect *ppi, int e)
{
  BLI_linklist_append_arena(&ppi->edges, POINTER_FROM_INT(e), bs->mem_arena);
}

static void add_face_to_partpartintersect(BoolState *bs, PartPartIntersect *ppi, int f)
{
  BLI_linklist_append_arena(&ppi->faces, POINTER_FROM_INT(f), bs->mem_arena);
}

/** Intersection Algorithm functions. */
#pragma mark Intersection Algorithm functions

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
 * Intersect all the geometry in part, assumed to be in one plane, together with other
 * geometry as given in the ppis list (each link of which is a PartPartIntersect *).
 * Return a PartPartIntersect that gives the new geometry that should
 * replace the geometry in part. May also add new elements in meshadd.
 * If no output is needed, return NULL.
 */
static PartPartIntersect *self_intersect_part_and_ppis(BoolState *bs,
                                                       MeshPart *part,
                                                       LinkNodePair *ppis,
                                                       MeshAdd *meshadd)
{
  CDT_input in;
  CDT_result *out;
  LinkNode *ln, *lnv, *lne, *lnf;
  PartPartIntersect *ppi, *ppi_out;
  MemArena *arena = bs->mem_arena;
  IMeshPlus imp;
  int i, j, part_nf, part_ne, part_nv, tot_ne, face_len, v, e, f, v1, v2;
  int nfaceverts, v_index, e_index, f_index, faces_index;
  int in_v, out_v, out_v2, start, in_e, out_e, in_f, out_f, e_eg, f_eg;
  int *imp_v, *imp_e;
  IntPair *new_face_data;
  IMesh *im = &bs->im;
  IndexedIntSet verts_needed;
  IndexedIntSet edges_needed;
  IndexedIntSet faces_needed;
  IntIntMap in_to_vmap;
  IntIntMap in_to_emap;
  IntIntMap in_to_fmap;
  double mat_2d[3][3];
  double mat_2d_inv[3][3];
  double xyz[3], save_z, p[3], q[3];
  bool ok;

  dump_part(part, "self_intersect_part");
  printf("ppis\n");
  for (ln = ppis->list; ln; ln = ln->next) {
    ppi = (PartPartIntersect *)ln->link;
    dump_partpartintersect(ppi, "");
  }
  /* Find which vertices are needed for CDT input */
  part_nf = part_totface(part);
  part_ne = part_totedge(part);
  part_nv = part_totvert(part);
  if (part_nf <= 1 && part_ne == 0 && part_nv == 0 && ppis->list == NULL) {
    printf("trivial 1 face case\n");
    return NULL;
  }
  init_intset(&verts_needed);
  init_intset(&edges_needed);
  init_intset(&faces_needed);
  init_intintmap(&in_to_vmap);
  init_intintmap(&in_to_emap);
  init_intintmap(&in_to_fmap);
  init_imeshplus(&imp, &bs->im, meshadd);

  /* nfaceverts will accumulate the total lengths of all faces added. */
  printf("gathering needed edges and verts\n");
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
      add_to_intintmap(bs, &in_to_vmap, v_index, v);
      printf("A. in_to_vmap[%d] = %d\n", v_index, v);
    }
    f_index = add_int_to_intset(bs, &faces_needed, f);
    add_to_intintmap(bs, &in_to_fmap, f_index, f);
    printf("B. in_to_fmap[%d] = %d\n", f_index, f);
  }
  for (i = 0; i < part_ne; i++) {
    e = part_edge(part, i);
    BLI_assert(e != -1);
    imeshplus_get_edge_verts(&imp, e, &v1, &v2);
    BLI_assert(v1 != -1 && v2 != -1);
    v_index = add_int_to_intset(bs, &verts_needed, v1);
    add_to_intintmap(bs, &in_to_vmap, v_index, v1);
    printf("C. in_to_vmap[%d] = %d\n", v_index, v1);
    v_index = add_int_to_intset(bs, &verts_needed, v2);
    add_to_intintmap(bs, &in_to_vmap, v_index, v2);
    printf("D. in_to_vmap[%d] = %d\n", v_index, v2);
    e_index = add_int_to_intset(bs, &edges_needed, e);
    add_to_intintmap(bs, &in_to_emap, e_index, e);
    printf("E. in_to_emap[%d] = %d\n", e_index, e);
  }
  for (i = 0; i < part_nv; i++) {
    v = part_vert(part, i);
    BLI_assert(v != -1);
    v_index = add_int_to_intset(bs, &verts_needed, v);
    add_to_intintmap(bs, &in_to_vmap, v_index, v);
    printf("F. in_to_vmap[%d] = %d\n", v_index, v);
  }
  for (ln = ppis->list; ln; ln = ln->next) {
    ppi = (PartPartIntersect *)ln->link;
    for (lnv = ppi->verts.list; lnv; lnv = lnv->next) {
      v = POINTER_AS_INT(lnv->link);
      v_index = add_int_to_intset(bs, &verts_needed, v);
      add_to_intintmap(bs, &in_to_vmap, v_index, v);
      printf("G. in_to_vmap[%d] = %d\n", v_index, v);
    }
    for (lne = ppi->edges.list; lne; lne = lne->next) {
      e = POINTER_AS_INT(lne->link);
      imeshplus_get_edge_verts(&imp, e, &v1, &v2);
      BLI_assert(v1 != -1 && v2 != -1);
      v_index = add_int_to_intset(bs, &verts_needed, v1);
      add_to_intintmap(bs, &in_to_vmap, v_index, v1);
      printf("H. in_to_vmap[%d] = %d\n", v_index, v1);
      v_index = add_int_to_intset(bs, &verts_needed, v2);
      add_to_intintmap(bs, &in_to_vmap, v_index, v2);
      printf("I. in_to_vmap[%d] = %d\n", v_index, v2);
      e_index = add_int_to_intset(bs, &edges_needed, e);
      add_to_intintmap(bs, &in_to_emap, e_index, e);
      printf("J. in_to_emap[%d] = %d\n", e_index, e);
    }
    for (lnf = ppi->faces.list; lnf; lnf = lnf->next) {
      f = POINTER_AS_INT(lne->link);
      face_len = imeshplus_facelen(&imp, f);
      nfaceverts += face_len;
      f_index = add_int_to_intset(bs, &faces_needed, f);
      add_to_intintmap(bs, &in_to_fmap, f_index, f);
      printf("K. in_to_fmap[%d] = %d\n", f_index, f);
    }
  }
  /* Edges implicit in faces will come back as orig edges, so handle those. */
  tot_ne = edges_needed.size;
  for (i = 0; i < part_nf; i++) {
    f = part_face(part, i);
    face_len = imesh_facelen(im, f);
    for (j = 0; j < face_len; j++) {
      v1 = imesh_face_vert(im, f, j);
      v2 = imesh_face_vert(im, f, (j + 1) % face_len);
      e = imesh_find_edge(im, v1, v2);
      BLI_assert(e != -1);
      add_to_intintmap(bs, &in_to_emap, j + tot_ne, e);
      printf("L. in_to_emap[%d] = %d\n", j + tot_ne, e);
      }
  }

  in.verts_len = verts_needed.size;
  in.edges_len = edges_needed.size;
  in.faces_len = faces_needed.size;
  in.vert_coords = BLI_array_alloca(in.vert_coords, (size_t)in.verts_len);
  if (in.edges_len != 0) {
    in.edges = BLI_array_alloca(in.edges, (size_t)in.edges_len);
  }
  else {
    in.edges = NULL;
  }
  in.faces = BLI_array_alloca(in.faces, (size_t)nfaceverts);
  in.faces_start_table = BLI_array_alloca(in.faces_start_table, (size_t)in.faces_len);
  in.faces_len_table = BLI_array_alloca(in.faces_len_table, (size_t)in.faces_len);
  in.epsilon = (float)bs->eps;

  /* Fill in the vert_coords of CDT input */

  /* Find mat_2d: matrix to rotate so that plane normal moves to z axis */
  axis_dominant_v3_to_m3_db(mat_2d, part->plane);
  ok = invert_m3_m3_db(mat_2d_inv, mat_2d);
  BLI_assert(ok);

  for (i = 0; i < in.verts_len; i++) {
    v = intset_get_value_by_index(&verts_needed, i);
    BLI_assert(v != -1);
    imeshplus_get_vert_co_db(&imp, v, p);
    mul_v3_m3v3_db(xyz, mat_2d, p);
    copy_v2fl_v2db(in.vert_coords[i], xyz);
    printf("in vert %d (needed vert %d) was (%g,%g,%g), rotated (%g,%g,%g)\n", i, v, F3(p), F3(xyz));
    if (i == 0) {
      /* If part is truly coplanar, all z components of rotated v should be the same.
       * Save it so that can rotate back to correct place when done.
       */
      save_z = xyz[2];
    }
  }

  /* Fill in the face data of CDT input. */
  /* faces_index is next place in flattened faces table to put a vert index. */
  faces_index = 0;
  for (i = 0; i < in.faces_len; i++) {
    f = intset_get_value_by_index(&faces_needed, i);
    face_len = imeshplus_facelen(&imp, f);
    in.faces_start_table[i] = faces_index;
    for (j = 0; j < face_len; j++) {
      v = imeshplus_face_vert(&imp, f, j);
      BLI_assert(v != -1);
      v_index = intset_get_index_for_value(&verts_needed, v);
      in.faces[faces_index++] = v_index;
    }
    in.faces_len_table[i] = faces_index - in.faces_start_table[i];
  }

  /* Fill in edge data of CDT input. */
  for (i = 0; i < in.edges_len; i++) {
    e = intset_get_value_by_index(&edges_needed, i);
    imeshplus_get_edge_verts(&imp, e, &v1, &v2);
    BLI_assert(v1 != -1 && v2 != -1);
    in.edges[i][0] = intset_get_index_for_value(&verts_needed, v1);
    in.edges[i][1] = intset_get_index_for_value(&verts_needed, v2);
  }

  /* TODO: fill in loose vert data of CDT input. */

  dump_cdt_input(&in, "input");
  out = BLI_delaunay_2d_cdt_calc(&in, CDT_CONSTRAINTS_VALID_BMESH);
  dump_cdt_result(out, "output", "");

  /* Make the PartPartIntersect that represents the output of the CDT. */
  ppi_out = BLI_memarena_alloc(bs->mem_arena, sizeof(*ppi_out));
  init_partpartintersect(ppi_out);

  /* imp_v will map an output vert index to an IMesh + MeshAdd space vertex. */
  imp_v = BLI_array_alloca(imp_v, (size_t)out->verts_len);
  for (out_v = 0; out_v < out->verts_len; out_v++) {
    printf("process out_v=%d\n", out_v);
    if (out->verts_orig_len_table[out_v] > 0) {
      /* out_v maps to a vertex we fed in from verts_needed. */
      start = out->verts_orig_start_table[out_v];
      /* Choose min index in orig list, to make for a stable algorithm. */
      in_v = min_int_in_array(&out->verts_orig[start], out->verts_orig_len_table[out_v]);
      printf("in_v=%d\n", in_v);
      if (!find_in_intintmap(&in_to_vmap, in_v, &v)) {
        printf("shouldn't happen, %d not in in_to_vmap\n", in_v);
        BLI_assert(false);
      }
    }
    else {
      /* Need a new imp vertex for out_v. */
      copy_v2db_v2fl(q, out->vert_coords[out_v]);
      q[2] = save_z;
      mul_v3_m3v3_db(p, mat_2d_inv, q);
      /* p should not already be in the IMesh because such verts should have been added to the
       * input. However, it is possible that the vert might already be in meshadd.  */
      v = meshadd_add_vert_db(bs, meshadd, p, -1, true);
      printf("added new v=%d to meshadd\n", v);
    }
    imp_v[out_v] = v;
    add_vert_to_partpartintersect(bs, ppi_out, v);
    printf("imp_v[%d] = %d\n", out_v, v);
  }

  /* Similar to above code, but for edges. */
  imp_e = BLI_array_alloca(imp_e, (size_t)out->edges_len);
  for (out_e = 0; out_e < out->edges_len; out_e++) {
    printf("process out_e=%d\n", out_e);
    e_eg = -1;
    if (out->edges_orig_len_table[out_e] > 0) {
      start = out->edges_orig_start_table[out_e];
      in_e = min_int_in_array(&out->edges_orig[start], out->edges_orig_len_table[out_e]);
      printf("in_e=%d\n", in_e);
      if (!find_in_intintmap(&in_to_emap, in_e, &e_eg)) {
        printf("shouldn't happen, %d not in in_to_emap\n", in_e);
        BLI_assert(false);
      }
    }
    /* If e_eg != -1 now, out_e may be only a part of e_eg; if so, make a new e but use e_eg as example.
     */
    v1 = imp_v[out->edges[out_e][0]];
    v2 = imp_v[out->edges[out_e][1]];
    if (e != -1) {
      int ev1, ev2;
      imeshplus_get_edge_verts(&imp, e, &ev1, &ev2);
      if (!((v1 == ev1 && v2 == ev2) || (v1 == ev2 && v2 == ev1))) {
        e = meshadd_add_edge(bs, meshadd, v1, v2, e_eg, true);
        printf("added new edge %d=(%d,%d) to meshadd\n", e, v1, v2);
      }
    }
    else {
      e = meshadd_add_edge(bs, meshadd, v1, v2, e_eg, true);
      printf("added new edge' %d=(%d,%d) to meshadd\n", e, v1, v2);
    }
    imp_e[out_e] = e;
    printf("imp_e[%d]=%d\n", out_e, e);
    add_edge_to_partpartintersect(bs, ppi_out, e);
  }

  /* Now for the faces. */
  for (out_f = 0; out_f < out->faces_len; out_f++) {
    printf("process out_f = %d\n", out_f);
    in_f = -1;
    f_eg = -1;
    if (out->faces_orig_len_table[out_f] > 0) {
      start = out->faces_orig_start_table[out_f];
      in_f = min_int_in_array(&out->faces_orig[start], out->faces_orig_len_table[out_f]);
      printf("in_f = %d\n", in_f);
      if (!find_in_intintmap(&in_to_fmap, in_f, &f_eg)) {
        printf("shouldn't happen, %d not in in_to_fmap\n", in_f);
        BLI_assert(false);
      }
    }
    /* Even if f is same as an existing face, we make a new one, to simplify "what to delete" bookkeeping later. */
    face_len = out->faces_len_table[out_f];
    start = out->faces_start_table[out_f];
    new_face_data = (IntPair *)BLI_memarena_alloc(arena, (size_t)face_len * sizeof(new_face_data[0]));
    for (i = 0; i < face_len; i++) {
      out_v = out->faces[start + i];
      v = imp_v[out_v];
      new_face_data[i].first = v;
      out_v2 = out->faces[start + ((i + 1) % face_len)];
      v2 = imp_v[out_v2];
      /* Edge (v, v2) should be an edge already added to ppi_out. Also e is either in im or meshadd. */
      e = find_edge_by_verts_in_meshadd(meshadd, v, v2);
      if (e == -1) {
        e = imesh_find_edge(&bs->im, v, v2);
      }
      if (e == -1) {
        printf("shouldn't happen: couldn't find e=(%d,%d)\n", v, v2);
        BLI_assert(false);
      }
      new_face_data[i].second = e;
    }
    f = meshadd_add_face(bs, meshadd, new_face_data, face_len, f_eg);
    printf("added new face %d\n", f);
    add_face_to_partpartintersect(bs, ppi_out, f);
  }
  
  BLI_delaunay_2d_cdt_free(out);
  return ppi_out;
}

/* Add any geometry resulting from intersectiong part1 with part2
 * into result_part.
 * If part1 and part2 have the same planes, all geometry that intersects
 * the bounding box of part1 will be added to result_part.
 */
ATTU static void add_part_intersections_to_part(BoolState *bs,
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
    /* Start with some special cases. */
    if (part_is_one_im_face(bs, part1) == 3 && part_is_one_im_face(bs, part2) == 3) {
#if 0
      add_tri_tri_intersection_to_part(bs, result_part, part1, part2);
#endif
    }
  }
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
ATTU static void union_merge_to_pair(BoolState *bs, IntIntMap *map_a, IntIntMap *map_b)
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

/* Find geometry that in the coplanar parts which may intersect.
 * For now, just assume all can intersect.
 */
static PartPartIntersect *coplanar_part_part_intersect(
    BoolState *bs, MeshPart *part_a, int a_index, MeshPart *part_b, int b_index)
{
  MemArena *arena = bs->mem_arena;
  PartPartIntersect *ppi;
  int totv_a, tote_a, totf_a, totv_b, tote_b, totf_b;
  int i, v, e, f;

  ppi = BLI_memarena_alloc(arena, sizeof(*ppi));
  ppi->a_index = a_index;
  ppi->b_index = b_index;
  init_partpartintersect(ppi);
  totv_a = part_totvert(part_a);
  tote_a = part_totedge(part_a);
  totf_a = part_totface(part_a);
  totv_b = part_totvert(part_b);
  tote_b = part_totedge(part_b);
  totf_b = part_totedge(part_b);

  for (i = 0; i < totv_a; i++) {
    v = part_vert(part_a, i);
    add_vert_to_partpartintersect(bs, ppi, v);
  }
  for (i = 0; i < totv_b; i++) {
    v = part_vert(part_b, i);
    add_vert_to_partpartintersect(bs, ppi, v);
  }
  for (i = 0; i < tote_a; i++) {
    e = part_edge(part_a, i);
    add_edge_to_partpartintersect(bs, ppi, e);
  }
  for (i = 0; i < tote_b; i++) {
    e = part_edge(part_b, i);
    add_edge_to_partpartintersect(bs, ppi, e);
  }
  for (i = 0; i < totf_a; i++) {
    f = part_face(part_a, i);
    add_edge_to_partpartintersect(bs, ppi, f);
  }
  for (i = 0; i < totf_b; i++) {
    f = part_face(part_b, i);
    add_face_to_partpartintersect(bs, ppi, f);
  }
  return ppi;
}

typedef struct FaceEdgeInfo {
  double co[3];
  double isect[3];
  double dist_along_line;
  int v;
  bool v_on;
  bool e_on;
  bool isect_ok;
} FaceEdgeInfo;

/* Find intersection of a face with a line and return the intervals on line.
 * It is known that the line is in the same plane as the face.
 * If there is any intersection, it should be a series of points
 * and line segments on the line.
 * A point on the line can be represented as fractions of the distance from
 * line_co1 to line_co2. (The fractions can be negative or greater than 1.)
 * The intervals will be returned as a linked list of (start, end) fractions,
 * where single points will be represented by a pair (start, start).
 * The returned list will be in increasing order of the start fraction, and
 * the intervals will not overlap, though they may touch.
 * [TODO: handle case where face has several
 * edges on the line, or where faces fold back on themselves.]
 */
static void find_face_line_intersects(BoolState *bs,
                                      LinkNodePair *intervals,
                                      int f,
                                      const double line_co1[3],
                                      const double line_co2[3])
{
  IMesh *im = &bs->im;
  int flen, i, is;
  int i1, i2, i3, ion1, ion2;
  double eps = bs->eps;
  FaceEdgeInfo *finfo, *fi, *finext;
  double co_close[3];
  double *interval;
  double l_no[3], mu;

  intervals[0].list = NULL;
  intervals[0].last_node = NULL;
  printf("intersecting face %d with line\n", f);
  printf("line: (%.5f,%.5f,%.5f) (%.5f,%.5f,%.5f)\n", F3(line_co1), F3(line_co2));
  flen = imesh_facelen(im, f);
  finfo = BLI_array_alloca(finfo, (size_t)flen);
  for (i = 0; i < flen; i++) {
    fi = &finfo[i];
    fi->v = imesh_face_vert(im, f, i);
    imesh_get_vert_co_db(im, fi->v, fi->co);
    closest_to_line_v3_db(co_close, fi->co, line_co1, line_co2);
    fi->v_on = compare_v3v3_db(fi->co, co_close, eps);
    fi->e_on = false;
    fi->isect_ok = false;
    zero_v3_db(fi->isect);
  }
  sub_v3_v3v3_db(l_no, line_co2, line_co1);
  normalize_v3_d(l_no);
  for (i = 0; i < flen; i++) {
    fi = &finfo[i];
    finext = &finfo[(i + 1) % flen];
    is = isect_line_seg_epsilon_v3_db(
        line_co1, line_co2, fi->co, finext->co, eps, fi->isect, &fi->dist_along_line, &mu);
    if (is > 0) {
      fi->isect_ok = true;
      fi->e_on = (is == 2);
    }
    printf(" (%d,%d) co=(%.5f,%.5f,%.5f) v=%d v_on=%d e_on=%d isect_ok=%d\n",
           i,
           (i + 1) % flen,
           F3(fi->co),
           fi->v,
           fi->v_on,
           fi->e_on,
           fi->isect_ok);
    if (fi->isect_ok) {
      printf("  isect=(%.5f,%.5f,%.5f) lambda=%f\n", F3(fi->isect), fi->dist_along_line);
    }
  }
  /* For now just handle case of convex faces, which should be one of the following
   * cases: (1) no intersects; (2) 1 intersect (a vertex); (2) 2 intersects on two
   * edges; (3) line coincindes with one edge.
   * TODO: handle general case. Needs ray shooting to test inside/outside
   * or division into convex pieces or something.
   */
  i1 = -1;
  i2 = -1;
  i3 = -1;
  ion1 = -1;
  ion2 = -1;
  for (i = 0; i < flen; i++) {
    fi = &finfo[i];
    if (fi->isect_ok) {
      if (i1 == -1) {
        i1 = i;
      }
      else if (i2 == -1) {
        i2 = i;
      }
      else if (i3 == -1) {
        i3 = i;
      }
    }
    else if (fi->e_on) {
      if (ion1 == -1) {
        ion1 = i;
      }
      else if (ion2 == -1) {
        ion2 = i;
      }
    }
  }
  if (i3 != -1 || ion2 != -1 || (i1 != -1 && ion1 != -1)) {
    printf("can't happen for convex faces: i1=%d i2=%d i3=%d ion1=%d ion2=%d\n",
           i1,
           i2,
           i3,
           ion1,
           ion2);
    return;
  }
  if (i1 == -1 && ion1 == -1) {
    /* No intersections. */
    return;
  }
  /* Just 1 intersection */
  interval = BLI_memarena_alloc(bs->mem_arena, 2 * sizeof(double));
  if (i1 != -1 && i2 != -1) {
    if (finfo[i1].dist_along_line <= finfo[2].dist_along_line) {
      interval[0] = finfo[i1].dist_along_line;
      interval[1] = finfo[i2].dist_along_line;
    }
    else {
      interval[0] = finfo[i2].dist_along_line;
      interval[1] = finfo[i1].dist_along_line;
    }
  }
  else if (i1 != -1) {
    interval[0] = finfo[i1].dist_along_line;
    interval[1] = finfo[i1].dist_along_line;
  }
  else {
    BLI_assert(ion1 != -1);
    interval[0] = finfo[ion1].dist_along_line;
    interval[1] = finfo[(ion1 + 1) % flen].dist_along_line;
  }
  BLI_linklist_append_arena(intervals, interval, bs->mem_arena);
}

/* Find geometry that in the non-coplanar parts which may intersect.
 * Needs to be the part of the geometry that is on the common line
 * of intersection, so that it is in the plane of both parts.
 */
static PartPartIntersect *non_coplanar_part_part_intersect(
    BoolState *bs, MeshPart *part_a, int a_index, MeshPart *part_b, int b_index, MeshAdd *meshadd)
{
  MemArena *arena = bs->mem_arena;
  IMesh *im = &bs->im;
  PartPartIntersect *ppi;
  MeshPart *part;
  int totv[2], tote[2], totf[2];
  int i, v, e, f, fa, fb, pi, v1, v2;
  int index_a, index_b;
  LinkNodePair *intervals_a;
  LinkNodePair *intervals_b;
  LinkNodePair *intervals;
  LinkNode *ln, *lna, *lnb;
  int is;
  double co[3], co1[3], co2[3], co_close[3], line_co1[3], line_co2[3], line_dir[3];
  double co_close1[3], co_close2[3];
  double elen_squared;
  double faca1, faca2, facb1, facb2, facstart, facend, *inta, *intb;
  double eps = bs->eps;
  double eps_squared = eps * eps;
  bool on1, on2;

  printf("non_coplanar_part_part_interesect\n");
  dump_part(part_a, "a");
  dump_part(part_b, "b");
  if (!isect_plane_plane_v3_db(part_a->plane, part_b->plane, line_co1, line_dir)) {
    /* Presumably the planes are parallel if they are not coplanar and don't intersect. */
    printf("planes don't intersect\n");
    return NULL;
  }
  add_v3_v3v3_db(line_co2, line_co1, line_dir);

  ppi = BLI_memarena_alloc(arena, sizeof(*ppi));
  ppi->a_index = a_index;
  ppi->b_index = b_index;
  init_partpartintersect(ppi);

  for (pi = 0; pi < 2; pi++) {
    part = pi == 0 ? part_a : part_b;
    totv[pi] = part_totvert(part);
    for (i = 0; i < totv[pi]; i++) {
      v = part_vert(part, i);
      imesh_get_vert_co_db(im, v, co);
      closest_to_line_v3_db(co_close, co, line_co1, line_co2);
      if (compare_v3v3_db(co, co_close, eps)) {
        add_vert_to_partpartintersect(bs, ppi, v);
      }
    }
  }
  for (pi = 0; pi < 2; pi++) {
    part = pi == 0 ? part_a : part_b;
    tote[pi] = part_totedge(part);
    for (i = 0; i < tote[pi]; i++) {
      e = part_edge(part, i);
      imesh_get_edge_cos_db(im, e, co1, co2);
      /* First check if co1 and/or co2 are on line, within eps. */
      closest_to_line_v3_db(co_close1, co1, line_co1, line_co2);
      closest_to_line_v3_db(co_close2, co2, line_co1, line_co2);
      on1 = compare_v3v3_db(co1, co_close1, eps);
      on2 = compare_v3v3_db(co2, co_close2, eps);
      if (on1 || on2) {
        if (on1 && on2) {
          add_edge_to_partpartintersect(bs, ppi, e);
        }
        else {
          imesh_get_edge_verts(im, e, &v1, &v2);
          add_vert_to_partpartintersect(bs, ppi, on1 ? v1 : v2);
        }
      }
      else {
        is = isect_line_line_epsilon_v3_db(
            line_co1, line_co2, co1, co2, co_close1, co_close2, eps);
        if (is > 0) {
          /* co_close1 is closest on line to segment (co1,co2). */
          if (is == 1 || compare_v3v3_db(co_close1, co_close2, eps)) {
            /* Intersection is on line or within eps. Is it on e's segment? */
            elen_squared = len_squared_v3v3_db(co1, co2) + eps_squared;
            if (len_squared_v3v3_db(co_close1, co1) <= elen_squared &&
                len_squared_v3v3_db(co_close1, co2) <= elen_squared) {
              /* Maybe intersection point is some other point in mesh. */
              v = imesh_find_co_db(im, co_close1, eps);
              if (v == -1) {
                /* A new point. Need to add to meshadd. */
                v = meshadd_add_vert_db(bs, meshadd, co, -1, true);
              }
              add_vert_to_partpartintersect(bs, ppi, v);
            }
          }
        }
      }
    }
  }
  totf[0] = part_totface(part_a);
  totf[1] = part_totface(part_b);
  intervals_a = BLI_array_alloca(intervals_a, (size_t)totf[0]);
  intervals_b = BLI_array_alloca(intervals_b, (size_t)totf[1]);
  for (pi = 0; pi < 2; pi++) {
    printf("doing faces from part %s\n", pi == 0 ? "a" : "b");
    part = pi == 0 ? part_a : part_b;
    totf[pi] = part_totface(part);
    for (i = 0; i < totf[pi]; i++) {
      printf("doing %dth face of part\n", i);
      f = part_face(part, i);
      intervals = pi == 0 ? intervals_a : intervals_b;
      find_face_line_intersects(bs, &intervals[i], f, line_co1, line_co2);
      printf("result:\n");
      if (intervals[i].list == NULL) {
        printf("no intersections\n");
      }
      else {
        for (ln = intervals[i].list; ln; ln = ln->next) {
          inta = (double *)ln->link;
          faca1 = inta[0];
          faca2 = inta[1];
          madd_v3_v3v3db_db(co, line_co1, line_dir, faca1);
          madd_v3_v3v3db_db(co2, line_co1, line_dir, faca2);
          printf("  (%f,%f) = (%.3f,%.3f,%.3f)(%.3f,%.3f,%.3f)\n", faca1, faca2, F3(co), F3(co2));
        }
      }
    }
  }
  /* Need to intersect the intervals of each face pair's intervals. */
  for (index_a = 0; index_a < totf[0]; index_a++) {
    lna = intervals_a[index_a].list;
    if (lna == NULL) {
      continue;
    }
    fa = part_face(part_a, index_a);
    for (index_b = 0; index_b < totf[1]; index_b++) {
      lnb = intervals_b[index_b].list;
      if (lnb == NULL) {
        continue;
      }
      fb = part_face(part_b, index_b);
      printf("intersect intervals for faces %d and %d\n", fa, fb);
      if (BLI_linklist_count(lna) == 1 && BLI_linklist_count(lnb) == 1) {
        /* Common special case of two single intervals to intersect. */
        inta = (double *)lna->link;
        intb = (double *)lnb->link;
        faca1 = inta[0];
        faca2 = inta[1];
        facb1 = intb[0];
        facb2 = intb[1];
        facstart = max_dd(faca1, facb1);
        facend = min_dd(faca2, facb2);
        if (facend < facstart - eps) {
          printf("  no intersection\n");
        }
        else {
          madd_v3_v3v3db_db(co, line_co1, line_dir, facstart);
          madd_v3_v3v3db_db(co2, line_co1, line_dir, facend);
          printf("  interval result: (%f,%f) = (%.5f,%.5f,%.5f)(%.5f,%.5f,%.5f)\n",
                 facstart,
                 facend,
                 F3(co),
                 F3(co2));
          if (compare_v3v3_db(co, co2, eps)) {
            /* Add a single vertex. */
            v = imesh_find_co_db(im, co, eps);
            if (v == -1) {
              /* A new point. Need to add to meshadd. */
              v = meshadd_add_vert_db(bs, meshadd, co, -1, true);
            }
            add_vert_to_partpartintersect(bs, ppi, v);
          }
          else {
            /* Add an edge. */
            v1 = imesh_find_co_db(im, co, eps);
            if (v1 == -1) {
              /* A new point. Need to add to meshadd. */
              v1 = meshadd_add_vert_db(bs, meshadd, co, -1, true);
            }
            v2 = imesh_find_co_db(im, co2, eps);
            if (v2 == -1) {
              v2 = meshadd_add_vert_db(bs, meshadd, co2, -1, true);
            }
            e = imesh_find_edge(im, v1, v2);
            if (e == -1) {
              /* TODO: if overlaps an existing edge, use as example. */
              e = meshadd_add_edge(bs, meshadd, v1, v2, -1, true);
            }
            add_edge_to_partpartintersect(bs, ppi, e);
          }
        }
      }
      else {
        printf("implement the multi-interval intersect case\n");
      }
    }
  }
  return ppi;
}

static PartPartIntersect *part_part_intersect(
    BoolState *bs, MeshPart *part_a, int a_index, MeshPart *part_b, int b_index, MeshAdd *meshadd)
{
  PartPartIntersect *ans;
  if (!parts_may_intersect(part_a, part_b)) {
    ans = NULL;
  }
  printf("Doing part-part intersect\n");
  dump_part(part_a, "a");
  dump_part(part_b, "b");
  if (planes_are_coplanar(part_a->plane, part_b->plane, bs->eps)) {
    ans = coplanar_part_part_intersect(bs, part_a, a_index, part_b, b_index);
  }
  else {
    ans = non_coplanar_part_part_intersect(bs, part_a, a_index, part_b, b_index, meshadd);
  }
  if (ans) {
    dump_partpartintersect(ans, "part-part result");
    dump_meshadd(meshadd, "after part-part");
  }
  return ans;
}

/* Intersect all parts of a_partset with all parts of b_partset.
 */
static void intersect_partset_pair(BoolState *bs, MeshPartSet *a_partset, MeshPartSet *b_partset)
{
  int a_index, b_index, tot_part_a, tot_part_b, bstart;
  MeshPart *part_a, *part_b;
  MemArena *arena = bs->mem_arena;
  IntIntMap vert_merge_map;
  MeshAdd meshadd;
  bool same_partsets = (a_partset == b_partset);
  LinkNodePair *a_isects; /* Array of List of PairPartIntersect. */
  LinkNodePair *b_isects; /* Array of List of PairPartIntersect. */
  PartPartIntersect *isect;

  tot_part_a = a_partset->tot_part;
  tot_part_b = b_partset->tot_part;
  init_intintmap(&vert_merge_map);
  init_meshadd(bs, &meshadd);
  a_isects = BLI_memarena_calloc(arena, (size_t)tot_part_a * sizeof(a_isects[0]));
  if (same_partsets) {
    b_isects = NULL;
  }
  else {
    b_isects = BLI_memarena_calloc(arena, (size_t)tot_part_b * sizeof(b_isects[0]));
  }

  for (a_index = 0; a_index < tot_part_a; a_index++) {
    part_a = partset_part(a_partset, a_index);
    if (!part_may_intersect_partset(part_a, b_partset)) {
      continue;
    }
    bstart = same_partsets ? a_index + 1 : 0;
    for (b_index = bstart; b_index < tot_part_b; b_index++) {
      part_b = partset_part(b_partset, b_index);
      isect = part_part_intersect(bs, part_a, a_index, part_b, b_index, &meshadd);
      if (isect != NULL) {
        BLI_linklist_append_arena(&a_isects[a_index], isect, arena);
        if (!same_partsets) {
          BLI_linklist_append_arena(&b_isects[b_index], isect, arena);
        }
      }
    }
    /* Now self-intersect the parts with their lists of isects. */
    for (a_index = 0; a_index < tot_part_a; a_index++) {
      printf("self intersect part a%d with its ppis\n", a_index);
      isect = self_intersect_part_and_ppis(bs, part_a, a_isects, &meshadd);
      if (isect) {
        dump_partpartintersect(isect, "after self intersect");
        dump_meshadd(&meshadd, "after self intersect");
      }
    }
    if (!same_partsets) {
      for (b_index = 0; b_index < tot_part_b; b_index++) {
        printf("self intersect part b%d with its ppis\n", b_index);
        isect = self_intersect_part_and_ppis(bs, part_b, b_isects, &meshadd);
        if (isect) {
          dump_partpartintersect(isect, "after self intersect");
          dump_meshadd(&meshadd, "after self intersect");
        }
      }
    }
  }
  apply_meshadd_to_imesh(&bs->im, &meshadd, &vert_merge_map);
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
    intersect_partset_pair(&bs, &all_parts, &all_parts);
  }
  else {
    find_coplanar_parts(&bs, &a_parts, TEST_A, "A");
    dump_partset(&a_parts);
    find_coplanar_parts(&bs, &b_parts, TEST_B, "B");
    dump_partset(&b_parts);
    intersect_partset_pair(&bs, &a_parts, &b_parts);
  }

  BLI_memarena_free(bs.mem_arena);
  return true;
}

#ifdef BOOLDEBUG
#  pragma mark Debug functions

ATTU static void dump_part(const MeshPart *part, const char *label)
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
      printf("  %s:{", nl[i].name);
      for (ln = nl[i].list; ln; ln = ln->next) {
        printf("%d", POINTER_AS_INT(ln->link));
        if (ln->next) {
          printf(", ");
        }
      }
      printf("}\n");
    }
  }
  printf("  plane=(%.3f,%.3f,%.3f),%.3f:\n", F4(part->plane));
  printf("  bb=(%.3f,%.3f,%.3f)(%.3f,%.3f,%.3f)\n", F3(part->bbmin), F3(part->bbmax));
}

ATTU static void dump_partset(const MeshPartSet *partset)
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

ATTU static void dump_partpartintersect(const PartPartIntersect *ppi, const char *label)
{
  struct namelist {
    const char *name;
    LinkNode *list;
  } nl[3] = {{"verts", ppi->verts.list}, {"edges", ppi->edges.list}, {"faces", ppi->faces.list}};
  LinkNode *ln;
  int i;

  printf("PARTPARTINTERSECT %s parts a[%d] and b[%d]\n", label, ppi->a_index, ppi->b_index);

  for (i = 0; i < 3; i++) {
    if (nl[i].list) {
      printf("  %s:{", nl[i].name);
      for (ln = nl[i].list; ln; ln = ln->next) {
        printf("%d", POINTER_AS_INT(ln->link));
        if (ln->next) {
          printf(", ");
        }
      }
      printf("}\n");
    }
  }
}

ATTU static void dump_meshadd(const MeshAdd *ma, const char *label)
{
  LinkNode *ln;
  NewVert *nv;
  NewEdge *ne;
  NewFace *nf;
  int i, j;

  printf("MESHADD %s\n", label);
  if (ma->verts.list) {
    printf("verts:\n");
    i = ma->vindex_start;
    for (ln = ma->verts.list; ln; ln = ln->next) {
      nv = (NewVert *)ln->link;
      printf("  %d: (%f,%f,%f) %d\n", i, F3(nv->co), nv->example);
      i++;
    }
  }
  if (ma->edges.list) {
    printf("edges:\n");
    i = ma->eindex_start;
    for (ln = ma->edges.list; ln; ln = ln->next) {
      ne = (NewEdge *)ln->link;
      printf("  %d: (%d,%d) %d\n", i, ne->v1, ne->v2, ne->example);
      i++;
    }
  }
  if (ma->faces.list) {
    printf("faces:\n");
    i = ma->findex_start;
    for (ln = ma->faces.list; ln; ln = ln->next) {
      nf = (NewFace *)ln->link;
      printf("  %d: face of length %d, example %d\n     ", i, nf->len, nf->example);
      for (j = 0; j < nf->len; j++) {
        printf("(v=%d,e=%d)", nf->vert_edge_pairs[j].first, nf->vert_edge_pairs[j].second);
      }
      printf("\n");
      i++;
    }
  }
}

ATTU static void dump_intintmap(const IntIntMap *map, const char *label, const char *prefix)
{
  IntIntMapIterator iter;

  printf("%sintintmap %s\n", prefix, label);
  for (intintmap_iter_init(&iter, map); !intintmap_iter_done(&iter); intintmap_iter_step(&iter)) {
    printf("%s  %d -> %d\n", prefix, intintmap_iter_key(&iter), intintmap_iter_value(&iter));
  }
}

ATTU static void dump_intlist_from_tables(const int *table,
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

ATTU static void dump_cdt_input(const CDT_input *cdt, const char *label)
{
  int i;

  printf("cdt input %s\n", label);
  printf("  verts\n");
  for (i = 0; i < cdt->verts_len; i++) {
    printf("  %d: (%.3f,%.3f)\n", i, F2(cdt->vert_coords[i]));
  }
  printf("  edges\n");
  for (i = 0; i < cdt->edges_len; i++) {
    printf("  %d: (%d,%d)\n", i, cdt->edges[i][0], cdt->edges[i][1]);
  }
  printf("  faces\n");
  for (i = 0; i < cdt->faces_len; i++) {
    printf("  %d: ", i);
    dump_intlist_from_tables(cdt->faces, cdt->faces_start_table, cdt->faces_len_table, i);
    printf("\n");
  }
}

ATTU static void dump_cdt_result(const CDT_result *cdt, const char *label, const char *prefix)
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
    printf("%s  %d: (%d,%d) orig=[", prefix, i, cdt->edges[i][0], cdt->edges[i][1]);
    dump_intlist_from_tables(
        cdt->edges_orig, cdt->edges_orig_start_table, cdt->edges_orig_len_table, i);
    printf("]\n");
  }
  printf("%s  faces\n", prefix);
  for (i = 0; i < cdt->faces_len; i++) {
    printf("%s  %d: ", prefix, i);
    dump_intlist_from_tables(cdt->faces, cdt->faces_start_table, cdt->faces_len_table, i);
    printf(" orig=[");
    dump_intlist_from_tables(
        cdt->faces_orig, cdt->faces_orig_start_table, cdt->faces_orig_len_table, i);
    printf("]\n");
  }
}
#endif
