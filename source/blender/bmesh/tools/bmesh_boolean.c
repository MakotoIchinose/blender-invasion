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
 * Cut meshes along intersections.
 *
 * Boolean-like modeling operation (without calculating inside/outside).
 *
 * Supported:
 * - Concave faces.
 * - Non-planar faces.
 * - Custom-data (UV's etc).
 *
 * Unsupported:
 * - Intersecting between different meshes.
 * - No support for holes (cutting a hole into a single face).
 */

#include "MEM_guardedalloc.h"

#include "BLI_math.h"
#include "BLI_delaunay_2d.h"
#include "BLI_utildefines.h"
#include "BLI_memarena.h"
#include "BLI_alloca.h"

#include "BLI_linklist.h"

#include "bmesh.h"
#include "intern/bmesh_private.h"

#include "bmesh_boolean.h" /* own include */

#include "BLI_strict_flags.h"

/* A cluster is a set of coplanar faces (within eps) */
typedef struct Cluster {
  float plane[4];  /* first 3 are normal, 4th is signed distance to plane */
  LinkNode *faces; /* list where links are BMFace* */
} Cluster;

/* A cluster set is a set of Clusters.
 * For any two distinct elements of the set, either they are not
 * coplanar or if they are, they are known not to intersect.
 * TODO: faster structure for looking up by plane.
 */
typedef struct ClusterSet {
  LinkNode *clusters; /* list where links are Cluster* */
} ClusterSet;

typedef struct BoolState {
  MemArena *mem_arena;
  BLI_mempool *listpool;
  BMesh *bm;
  int boolean_mode;
  float eps;
  int (*test_fn)(BMFace *f, void *user_data);
  void *test_fn_user_data;
} BoolState;

#define BOOLDEBUG
#ifdef BOOLDEBUG
/* For Debugging. */
#  define CO3(v) (v)->co[0], (v)->co[1], (v)->co[2]
#  define F2(v) (v)[0], (v)[1]
#  define F3(v) (v)[0], (v)[1], (v)[2]
#  define F4(v) (v)[0], (v)[1], (v)[2], (v)[3]
#  define BMI(e) BM_elem_index_get(e)

static void dump_cluster(Cluster *cl, const char *label);
static void dump_clusterset(ClusterSet *clset, const char *label);
#endif

/* Make clusterset by empty. */
static void init_clusterset(ClusterSet *clusterset)
{
  clusterset->clusters = NULL;
}

/* Fill r_plane with the 4d representation of f's plane. */
static inline void fill_face_plane(float r_plane[4], BMFace *f)
{
  plane_from_point_normal_v3(r_plane, f->l_first->v->co, f->no);
}

/* Return true if a_plane and b_plane are the same plane, to within eps. */
static bool planes_are_coplanar(const float a_plane[4], const float b_plane[4], float eps)
{
  if (fabsf(a_plane[3] - b_plane[3]) > eps) {
    return false;
  }
  return fabsf(dot_v3v3(a_plane, b_plane) - 1.0f) <= eps;
}

/* Return the cluster in clusterset for plane face_plane, if it exists, else NULL. */
static Cluster *find_cluster_for_plane(BoolState *bs,
                                       ClusterSet *clusterset,
                                       const float face_plane[4])
{
  LinkNode *ln;

  for (ln = clusterset->clusters; ln; ln = ln->next) {
    Cluster *cl = (Cluster *)ln->link;
    if (planes_are_coplanar(face_plane, cl->plane, bs->eps)) {
      return cl;
    }
  }
  return NULL;
}

/* Add face f to cluster. */
static void add_face_to_cluster(BoolState *bs, Cluster *cluster, BMFace *f)
{
  BLI_linklist_prepend_arena(&cluster->faces, f, bs->mem_arena);
}

/* Make a new cluster containing face f, then add it to clusterset. */
static void add_new_cluster_to_clusterset(BoolState *bs,
                                          ClusterSet *clusterset,
                                          BMFace *f,
                                          const float plane[4])
{
  Cluster *new_cluster;

  new_cluster = BLI_memarena_alloc(bs->mem_arena, sizeof(Cluster));
  copy_v4_v4(new_cluster->plane, plane);
  new_cluster->faces = NULL;
  BLI_linklist_prepend_arena(&new_cluster->faces, f, bs->mem_arena);
  BLI_linklist_prepend_arena(&clusterset->clusters, new_cluster, bs->mem_arena);
}

/* Add face f to a cluster in clusterset with the same face, else a new cluster for f */
static void add_face_to_clusterset(BoolState *bs, ClusterSet *clusterset, BMFace *f)
{
  Cluster *matching_cluster;
  float face_plane[4];

  fill_face_plane(face_plane, f);
  matching_cluster = find_cluster_for_plane(bs, clusterset, face_plane);
  if (matching_cluster) {
    add_face_to_cluster(bs, matching_cluster, f);
  }
  else {
    add_new_cluster_to_clusterset(bs, clusterset, f, face_plane);
  }
}

/* Fill clusterset with clusters for mesh faces, putting them into
 * clusterset_a or clusterset_b as bs->test_fn returns 1 or 0.
 */
static void find_clusters(BoolState *bs, ClusterSet *clusterset_a, ClusterSet *clusterset_b)
{
  BMesh *bm = bs->bm;
  BMFace *f;
  BMIter iter;
  int testval;

  if (clusterset_a) {
    init_clusterset(clusterset_a);
  }
  if (clusterset_b) {
    init_clusterset(clusterset_b);
  }
  BM_ITER_MESH (f, &iter, bm, BM_FACES_OF_MESH) {
    testval = bs->test_fn(f, bs->test_fn_user_data);
    if (testval == 1 && clusterset_a) {
      add_face_to_clusterset(bs, clusterset_a, f);
    }
    else if (testval == 0 && clusterset_b) {
      add_face_to_clusterset(bs, clusterset_b, f);
    }
  }
}

/*
 * Intersect all of the faces, assumed to be BMFaces in bs->bm.
 * The faces are assumed to lie in the same plane, given by the plane arg.
 * Original faces may be deleted and replaced with one or more newly built ones.
 * Some BMVerts may be merged (one will be used as representative).
 */
static void intersect_planar_geometry(BoolState *bs,
                                      const float plane[4],
                                      BMFace **faces,
                                      int faces_len)
{
  CDT_input in;
  CDT_result *out;
  int nverts, nfaceverts;
  LinkNode *ln;
  LinkNode *vertlist = NULL;
  BMFace *f;
  BMVert *v;
  BMIter iter;
  float mat_2d[3][3];
  float xy[2];
  int i, faces_index, vert_coords_index, faces_table_index, v_index;

  nverts = 0;
  nfaceverts = 0;
  for (i = 0; i < faces_len; i++) {
    f = faces[i];
    BM_ITER_ELEM (v, &iter, f, BM_VERTS_OF_FACE) {
      nfaceverts++;
      /* TODO: faster lookup structure to hold verts and their indices */
      if (BLI_linklist_index(vertlist, v) == -1) {
        BLI_linklist_prepend_arena(&vertlist, v, bs->mem_arena);
        nverts++;
      }
    }
  }
  in.verts_len = nverts;
  in.edges_len = 0;
  in.faces_len = faces_len;
  in.vert_coords = BLI_array_alloca(in.vert_coords, (size_t)nverts);
  in.edges = NULL;
  in.faces = BLI_array_alloca(in.faces, (size_t)nfaceverts);
  in.faces_start_table = BLI_array_alloca(in.faces_start_table, (size_t)faces_len);
  in.faces_len_table = BLI_array_alloca(in.faces_len_table, (size_t)faces_len);
  in.epsilon = bs->eps;

  axis_dominant_v3_to_m3(mat_2d, plane);
  vert_coords_index = 0;
  for (ln = vertlist; ln; ln = ln->next) {
    v = (BMVert *)ln->link;
    mul_v2_m3v3(xy, mat_2d, v->co);
    copy_v2_v2(in.vert_coords[vert_coords_index], xy);
    vert_coords_index++;
  }

  faces_index = 0;
  faces_table_index = 0;
  for (i = 0; i < faces_len; i++) {
    f = faces[i];
    BLI_assert(faces_table_index < faces_len);
    in.faces_start_table[faces_table_index] = faces_index;
    BM_ITER_ELEM (v, &iter, f, BM_VERTS_OF_FACE) {
      v_index = BLI_linklist_index(vertlist, v);
      BLI_assert(v_index >= 0 && v_index < nverts);
      in.faces[faces_index] = v_index;
      faces_index++;
    }
    in.faces_len_table[faces_table_index] = faces_index - in.faces_start_table[faces_table_index];
    faces_table_index++;
  }

  printf("cdt input:\n");
  printf("verts_len=%d, edges_len=%d, faces_len=%d\n", in.verts_len, in.edges_len, in.faces_len);
  printf("vert_coord: ");
  for (i = 0; i < in.verts_len; i++) {
    printf("(%.3f,%.3f) ", F2(in.vert_coords[i]));
  }
  printf("\nfaces: ");
  for (i = 0; i < in.faces_start_table[in.faces_len - 1] + in.faces_len_table[in.faces_len -1]; i++) {
    printf("%d ", in.faces[i]);
  }
  printf("\nfaces_start_table: ");
  for (i = 0; i < in.faces_len; i++) {
    printf("%d ", in.faces_start_table[i]);
  }
  printf("\nfaces_len_table: ");
  for (i = 0; i < in.faces_len; i++) {
    printf("%d ", in.faces_len_table[i]);
  }
  printf("\n");

  out = BLI_delaunay_2d_cdt_calc(&in, CDT_CONSTRAINTS_VALID_BMESH);

  printf("out stats: verts_len=%d, edges_len=%d, faces_len=%d\n",
         out->verts_len,
         out->edges_len,
         out->faces_len);
}

static void intersect_clusters(BoolState *bs, Cluster *cla, Cluster *clb)
{
  BMFace **faces, **fp;
  int i, totfaces;
  LinkNode *ln;
  Cluster *cl;
  Cluster *clab[2] = {cla, clb};

  totfaces = 0;
  for (i = 0; i < 2; i++) {
    cl = clab[i];
    if (cl) {
      totfaces += BLI_linklist_count(cl->faces);
    }
  }
  fp = faces = BLI_array_alloca(faces, (size_t)totfaces);
  for (i = 0; i < 2; i++) {
    cl = clab[i];
    if (cl) {
      for (ln = cl->faces; ln; ln = ln->next) {
        *fp = (BMFace *)ln->link;
        fp++;
      }
    }
  }
  intersect_planar_geometry(bs, cla->plane, faces, totfaces);
}

/* Intersect all pairs of clusters (a,b) where a is in clusterset_a
 * and b is in clusterset_b, and where a and b are for the same plane.
 */
static void intersect_clustersets(BoolState *bs,
                                  ClusterSet *clusterset_a,
                                  ClusterSet *clusterset_b)
{
  Cluster *cla, *clb;
  LinkNode *ln;

  for (ln = clusterset_a->clusters; ln; ln = ln->next) {
    cla = (Cluster *)ln->link;
    clb = find_cluster_for_plane(bs, clusterset_b, cla->plane);
    if (clb) {
      intersect_clusters(bs, cla, clb);
    }
  }
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
                     const bool UNUSED(use_self),
                     const bool UNUSED(use_separate),
                     const int boolean_mode,
                     const float eps)
{
  BoolState bs = {NULL};
  ClusterSet a_clusters, b_clusters;

  bs.bm = bm;
  bs.boolean_mode = boolean_mode;
  bs.eps = eps;
  bs.test_fn = test_fn;
  bs.test_fn_user_data = user_data;
  bs.mem_arena = BLI_memarena_new(MEM_SIZE_OPTIMAL(1 << 16), __func__);
  bs.listpool = BLI_mempool_create(sizeof(LinkNode), 128, 128, 0);

  printf("\n\nBOOLEAN\n");
  find_clusters(&bs, &a_clusters, &b_clusters);

  dump_clusterset(&a_clusters, "A");
  dump_clusterset(&b_clusters, "B");

  intersect_clustersets(&bs, &a_clusters, &b_clusters);

  BLI_mempool_destroy(bs.listpool);
  BLI_memarena_free(bs.mem_arena);
  return true;
}

#ifdef BOOLDEBUG
static void dump_cluster(Cluster *cl, const char *label)
{
  LinkNode *ln;
  BMFace *f;

  printf("%s: (%.3f,%.3f,%.3f),%.3f: {", label, F4(cl->plane));
  for (ln = cl->faces; ln; ln = ln->next) {
    f = (BMFace *)ln->link;
    printf("f%d", BMI(f));
    if (ln->next) {
      printf(", ");
    }
  }
  printf("}\n");
}

static void dump_clusterset(ClusterSet *clset, const char *label)
{
  LinkNode *ln;
  Cluster *cl;

  printf("%s:\n", label);
  for (ln = clset->clusters; ln; ln = ln->next) {
    cl = (Cluster *)ln->link;
    dump_cluster(cl, "  ");
  }
}
#endif
