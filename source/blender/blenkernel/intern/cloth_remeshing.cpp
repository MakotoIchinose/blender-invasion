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
 *
 * The Original Code is Copyright (C) Blender Foundation
 * All rights reserved.
 */

/** \file
 * \ingroup bke
 */

extern "C" {

#include "MEM_guardedalloc.h"

#include "DNA_cloth_types.h"
#include "DNA_scene_types.h"
#include "DNA_object_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_customdata_types.h"

#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_edgehash.h"
#include "BLI_linklist.h"
#include "BLI_array.h"
#include "BLI_kdopbvh.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "BKE_bvhutils.h"
#include "BKE_cloth.h"
#include "BKE_collision.h"
#include "BKE_effect.h"
#include "BKE_global.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_mesh_mapping.h"
#include "BKE_modifier.h"
#include "BKE_library.h"
#if USE_CLOTH_CACHE
#  include "BKE_pointcache.h"
#endif

#include "bmesh.h"
#include "bmesh_tools.h"

#include "ED_mesh.h"
}

#include "BKE_cloth_remeshing.h"

#include "BPH_mass_spring.h"

#include <vector>
#include <map>
#include <utility>
#include <algorithm>
using namespace std;

/******************************************************************************
 * cloth_remeshing_step - remeshing function for adaptive cloth modifier
 * reference http://graphics.berkeley.edu/papers/Narain-AAR-2012-11/index.html
 ******************************************************************************/

typedef map<BMVert *, ClothVertex> ClothVertMap;
class ClothPlane {
 public:
  float co[3];
  float no[3];
};

#define REMESHING_DATA_DEBUG 0 /* split and collapse edge count */
#define COLLAPSE_EDGES_DEBUG 0
#define FACE_SIZING_DEBUG 0
#define FACE_SIZING_DEBUG_COMP 0
#define FACE_SIZING_DEBUG_OBS 0
#define FACE_SIZING_DEBUG_SIZE 0
#define SKIP_COMP_METRIC 0
#define EXPORT_MESH 0

#define INVERT_EPSILON 0.00001f
#define EIGEN_EPSILON 1e-3f
#define NEXT(x) ((x) < 2 ? (x) + 1 : (x)-2)
#define PREV(x) ((x) > 0 ? (x)-1 : (x) + 2)

static void print_m32(float m[3][2])
{
  printf("{{%f, %f}, {%f, %f}, {%f, %f}}\n", m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1]);
}

static void print_m2(float m[2][2])
{
  printf("{{%f, %f}, {%f, %f}}\n", m[0][0], m[0][1], m[1][0], m[1][1]);
}

static void print_v2(float v[2])
{
  printf("{%f, %f}\n", v[0], v[1]);
}

static float sqr_fl(float a)
{
  return a * a;
}

static float clamp(float a, float min_val, float max_val)
{
  return min(max(a, min_val), max_val);
}

ClothSizing &ClothSizing::operator+=(const ClothSizing &size)
{
  add_m2_m2m2(m, m, size.m);
  return *this;
}

ClothSizing &ClothSizing::operator/=(float value)
{
  mul_m2_fl(m, 1.0f / value);
  return *this;
}

ClothSizing ClothSizing::operator*(float value)
{
  ClothSizing temp = *this;
  mul_m2_fl(temp.m, value);
  return temp;
}

static bool cloth_remeshing_boundary_test(BMVert *v);
static bool cloth_remeshing_boundary_test(BMEdge *e);
static bool cloth_remeshing_edge_on_seam_test(BMesh *bm, BMEdge *e, const int cd_loop_uv_offset);
static bool cloth_remeshing_vert_on_seam_test(BMesh *bm, BMVert *v, const int cd_loop_uv_offset);
static bool cloth_remeshing_vert_on_seam_or_boundary_test(BMesh *bm,
                                                          BMVert *v,
                                                          const int cd_loop_uv_offset);
static void cloth_remeshing_uv_of_vert_in_face(
    BMesh *bm, BMFace *f, BMVert *v, const int cd_loop_uv_offset, float r_uv[2]);
static float cloth_remeshing_wedge(float v_01[2], float v_02[2]);
static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                vector<BMFace *> &remove_faces,
                                                vector<BMFace *> &add_faces);
static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                BMesh *bm,
                                                BMVert *v);
static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                BMesh *bm,
                                                BMEdge *e);
static ClothSizing cloth_remeshing_find_average_sizing(ClothSizing &size_01, ClothSizing &size_02);
static void mul_m2_m2m2(float r[2][2], float a[2][2], float b[2][2]);
static BMVert *cloth_remeshing_edge_vert(BMEdge *e, int which);
static BMVert *cloth_remeshing_edge_vert(BMesh *bm, BMEdge *e, int side, int i, float r_uv[2]);
static BMVert *cloth_remeshing_edge_opposite_vert(
    BMesh *bm, BMEdge *e, int side, const int cd_loop_uv_offset, float r_uv[2]);
static void cloth_remeshing_edge_face_pair(BMEdge *e, BMFace **r_f1, BMFace **r_f2);
static void cloth_remeshing_uv_of_vert_in_edge(BMesh *bm, BMEdge *e, BMVert *v, float r_uv[2]);
static bool cloth_remeshing_edge_label_test(BMEdge *e);

CustomData_MeshMasks cloth_remeshing_get_cd_mesh_masks(void)
{
  CustomData_MeshMasks cddata_masks;

  cddata_masks.vmask = CD_MASK_ORIGINDEX;
  cddata_masks.emask = CD_MASK_ORIGINDEX;
  cddata_masks.pmask = CD_MASK_ORIGINDEX;

  return cddata_masks;
}

static bool equals_v3v3(const float v1[3], const float v2[3], float epsilon)
{
  return (fabsf(v1[0] - v2[0]) < epsilon) && (fabsf(v1[1] - v2[1]) < epsilon) &&
         (fabsf(v1[2] - v2[2]) < epsilon);
}

static void cloth_remeshing_init_bmesh(Object *ob,
                                       ClothModifierData *clmd,
                                       Mesh *mesh,
                                       ClothVertMap &cvm)
{
  if (clmd->sim_parms->remeshing_reset || !clmd->clothObject->bm_prev) {
    cloth_to_mesh(ob, clmd, mesh);

    CustomData_MeshMasks cddata_masks = cloth_remeshing_get_cd_mesh_masks();
    if (clmd->clothObject->bm_prev) {
      BM_mesh_free(clmd->clothObject->bm_prev);
      clmd->clothObject->bm_prev = NULL;
    }
    struct BMeshCreateParams bmesh_create_params;
    bmesh_create_params.use_toolflags = 0;
    struct BMeshFromMeshParams bmesh_from_mesh_params;
    bmesh_from_mesh_params.calc_face_normal = true;
    bmesh_from_mesh_params.cd_mask_extra = cddata_masks;
    clmd->clothObject->bm_prev = BKE_mesh_to_bmesh_ex(
        mesh, &bmesh_create_params, &bmesh_from_mesh_params);
    BMVert *v;
    BMIter viter;
    int i = 0;
    /* const int cd_loop_uv_offset = CustomData_get_offset(&clmd->clothObject->bm_prev->ldata, */
    /*                                                     CD_MLOOPUV); */
    BM_ITER_MESH_INDEX (v, &viter, clmd->clothObject->bm_prev, BM_VERTS_OF_MESH, i) {
      mul_m4_v3(ob->obmat, v->co);
      if (!equals_v3v3(v->co, clmd->clothObject->verts[i].x, ALMOST_ZERO)) {
        printf("copying %f %f %f into %f %f %f\n",
               clmd->clothObject->verts[i].x[0],
               clmd->clothObject->verts[i].x[1],
               clmd->clothObject->verts[i].x[2],
               v->co[0],
               v->co[1],
               v->co[2]);
        copy_v3_v3(v->co, clmd->clothObject->verts[i].x);
      }
      cvm[v] = clmd->clothObject->verts[i];
      cvm[v].flags |= CLOTH_VERT_FLAG_PRESERVE;
      /* if (cloth_remeshing_vert_on_seam_or_boundary_test( */
      /*         clmd->clothObject->bm_prev, v, cd_loop_uv_offset)) { */
      /*   cvm[v].flags |= CLOTH_VERT_FLAG_PRESERVE; */
      /* } */
    }
    BMEdge *e;
    BMIter eiter;
    BM_ITER_MESH (e, &eiter, clmd->clothObject->bm_prev, BM_EDGES_OF_MESH) {
      BM_elem_flag_enable(e, BM_ELEM_TAG);
      /* BM_elem_flag_disable(e, BM_ELEM_TAG); */
    }

    BM_mesh_normals_update(clmd->clothObject->bm_prev);

    BM_mesh_triangulate(clmd->clothObject->bm_prev,
                        MOD_TRIANGULATE_QUAD_SHORTEDGE,
                        MOD_TRIANGULATE_NGON_BEAUTY,
                        4,
                        false,
                        NULL,
                        NULL,
                        NULL);
    printf("remeshing_reset has been set to true or bm_prev does not exist\n");
  }
  else {
    BMVert *v;
    BMIter viter;
    int i = 0;
    BM_ITER_MESH_INDEX (v, &viter, clmd->clothObject->bm_prev, BM_VERTS_OF_MESH, i) {
      copy_v3_v3(v->co, clmd->clothObject->verts[i].x);
      cvm[v] = clmd->clothObject->verts[i];
    }
  }
  clmd->clothObject->mvert_num_prev = clmd->clothObject->mvert_num;
  clmd->clothObject->bm = clmd->clothObject->bm_prev;

  /* free clmd->clothObject->verts, it is already duplicated into cvm */
  if (clmd->clothObject->verts != NULL) {
    MEM_freeN(clmd->clothObject->verts);
    clmd->clothObject->verts = NULL;
  }
  clmd->clothObject->mvert_num = 0;
}

static ClothVertex cloth_remeshing_mean_cloth_vert(ClothVertex *v1, ClothVertex *v2)
{
  ClothVertex new_vert;

  new_vert.flags = 0;
  if ((v1->flags & CLOTH_VERT_FLAG_PINNED) && (v2->flags & CLOTH_VERT_FLAG_PINNED)) {
    new_vert.flags |= CLOTH_VERT_FLAG_PINNED;
  }
  if ((v1->flags & CLOTH_VERT_FLAG_NOSELFCOLL) && (v2->flags & CLOTH_VERT_FLAG_NOSELFCOLL)) {
    new_vert.flags |= CLOTH_VERT_FLAG_NOSELFCOLL;
  }
  /* We don't want the mean cloth vert to be preserved (CLOTH_VERT_FLAG_PRESERVE), only the
   * starting vertices */

  add_v3_v3v3(new_vert.v, v1->v, v2->v);
  mul_v3_fl(new_vert.v, 0.5f);

  add_v3_v3v3(new_vert.xconst, v1->xconst, v2->xconst);
  mul_v3_fl(new_vert.xconst, 0.5f);

  add_v3_v3v3(new_vert.x, v1->x, v2->x);
  mul_v3_fl(new_vert.x, 0.5f);

  add_v3_v3v3(new_vert.xold, v1->xold, v2->xold);
  mul_v3_fl(new_vert.xold, 0.5f);

  add_v3_v3v3(new_vert.tx, v1->tx, v2->tx);
  mul_v3_fl(new_vert.tx, 0.5f);

  add_v3_v3v3(new_vert.txold, v1->txold, v2->txold);
  mul_v3_fl(new_vert.txold, 0.5f);

  add_v3_v3v3(new_vert.tv, v1->tv, v2->tv);
  mul_v3_fl(new_vert.tv, 0.5f);

  add_v3_v3v3(new_vert.impulse, v1->impulse, v2->impulse);
  mul_v3_fl(new_vert.impulse, 0.5f);

  add_v3_v3v3(new_vert.xrest, v1->xrest, v2->xrest);
  mul_v3_fl(new_vert.xrest, 0.5f);

  add_v3_v3v3(new_vert.dcvel, v1->dcvel, v2->dcvel);
  mul_v3_fl(new_vert.dcvel, 0.5f);

  new_vert.mass = (v1->mass + v2->mass) * 0.5;
  new_vert.goal = (v1->goal + v2->goal) * 0.5;
  new_vert.impulse_count = (v1->impulse_count + v2->impulse_count) * 0.5;
  /* new_vert.avg_spring_len = (v1->avg_spring_len + v2->avg_spring_len) * 0.5; */
  new_vert.struct_stiff = (v1->struct_stiff + v2->struct_stiff) * 0.5;
  new_vert.bend_stiff = (v1->bend_stiff + v2->bend_stiff) * 0.5;
  new_vert.shear_stiff = (v1->shear_stiff + v2->shear_stiff) * 0.5;
  /* new_vert.spring_count = (v1->spring_count + v2->spring_count) * 0.5; */
  new_vert.shrink_factor = (v1->shrink_factor + v2->shrink_factor) * 0.5;

  return new_vert;
}

static Mesh *cloth_remeshing_update_cloth_object_bmesh(Object *ob, ClothModifierData *clmd)
{
  Mesh *mesh_result = NULL;
  Cloth *cloth = clmd->clothObject;
  CustomData_MeshMasks cddata_masks = cloth_remeshing_get_cd_mesh_masks();
  mesh_result = BKE_mesh_from_bmesh_for_eval_nomain(clmd->clothObject->bm, &cddata_masks);

  /* No need to worry about cloth->verts, it has been updated during
   * reindexing */

  if (clmd->clothObject->mvert_num_prev == clmd->clothObject->mvert_num) {
    /* printf("returning because mvert_num_prev == mvert_num\n"); */
    clmd->clothObject->bm_prev = BM_mesh_copy(clmd->clothObject->bm);
    BM_mesh_free(clmd->clothObject->bm);
    clmd->clothObject->bm = NULL;
    return mesh_result;
  }

  // Free the springs.
  if (cloth->springs != NULL) {
    LinkNode *search = cloth->springs;
    while (search) {
      ClothSpring *spring = (ClothSpring *)search->link;

      MEM_SAFE_FREE(spring->pa);
      MEM_SAFE_FREE(spring->pb);

      MEM_freeN(spring);
      search = search->next;
    }
    BLI_linklist_free(cloth->springs, NULL);

    cloth->springs = NULL;
  }

  cloth->springs = NULL;
  cloth->numsprings = 0;

  // free BVH collision tree
  if (cloth->bvhtree) {
    BLI_bvhtree_free(cloth->bvhtree);
    cloth->bvhtree = NULL;
  }

  if (cloth->bvhselftree) {
    BLI_bvhtree_free(cloth->bvhselftree);
    cloth->bvhselftree = NULL;
  }

  if (cloth->implicit) {
    BPH_mass_spring_solver_free(cloth->implicit);
    cloth->implicit = NULL;
  }

  // we save our faces for collision objects
  if (cloth->tri) {
    MEM_freeN(cloth->tri);
    cloth->tri = NULL;
  }

  if (cloth->edgeset) {
    BLI_edgeset_free(cloth->edgeset);
    cloth->edgeset = NULL;
  }

  /* Now build those things */
  const MLoop *mloop = mesh_result->mloop;
  const MLoopTri *looptri = BKE_mesh_runtime_looptri_ensure(mesh_result);
  const unsigned int looptri_num = mesh_result->runtime.looptris.len;

  /* save face information */
  clmd->clothObject->tri_num = looptri_num;
  clmd->clothObject->tri = (MVertTri *)MEM_mallocN(sizeof(MVertTri) * looptri_num,
                                                   "clothLoopTris");
  if (clmd->clothObject->tri == NULL) {
    cloth_free_modifier(clmd);
    modifier_setError(&(clmd->modifier), "Out of memory on allocating clmd->clothObject->looptri");
    printf("cloth_free_modifier clmd->clothObject->looptri\n");
    return NULL;
  }
  BKE_mesh_runtime_verttri_from_looptri(clmd->clothObject->tri, mloop, looptri, looptri_num);

  /* // apply / set vertex groups */
  /* // has to be happen before springs are build! */
  /* cloth_apply_vgroup(clmd, mesh_result); */

  if (!cloth_build_springs(clmd, mesh_result)) {
    cloth_free_modifier(clmd);
    modifier_setError(&(clmd->modifier), "Cannot build springs");
    return NULL;
  }

  // init our solver
  BPH_cloth_solver_init(ob, clmd);

  BKE_cloth_solver_set_positions(clmd);

  clmd->clothObject->bvhtree = bvhtree_build_from_cloth(clmd, clmd->coll_parms->epsilon);
  clmd->clothObject->bvhselftree = bvhtree_build_from_cloth(clmd, clmd->coll_parms->selfepsilon);

  /**/

  clmd->clothObject->mvert_num_prev = clmd->clothObject->mvert_num;
  clmd->clothObject->bm_prev = BM_mesh_copy(clmd->clothObject->bm);
  BM_mesh_free(clmd->clothObject->bm);
  clmd->clothObject->bm = NULL;

  return mesh_result;
}

/* from Bossen and Heckbert 1996 */
#define CLOTH_REMESHING_EDGE_FLIP_THRESHOLD 0.001f

static bool cloth_remeshing_should_flip(BMesh *bm,
                                        BMEdge *e,
                                        ClothVertMap &cvm,
                                        const int cd_loop_uv_offset)
{
  BMVert *v1, *v2, *v3, *v4;
  float x[2], y[2], z[2], w[2];
  v1 = cloth_remeshing_edge_vert(bm, e, 0, 0, x);
  v2 = cloth_remeshing_edge_vert(bm, e, 0, 1, z);
  v3 = cloth_remeshing_edge_opposite_vert(bm, e, 0, cd_loop_uv_offset, w);
  v4 = cloth_remeshing_edge_opposite_vert(bm, e, 1, cd_loop_uv_offset, y);

  float m[2][2];
  ClothSizing size_temp_01 = cloth_remeshing_find_average_sizing(*cvm[v1].sizing, *cvm[v2].sizing);
  ClothSizing size_temp_02 = cloth_remeshing_find_average_sizing(*cvm[v3].sizing, *cvm[v4].sizing);
  ClothSizing size_temp = cloth_remeshing_find_average_sizing(size_temp_01, size_temp_02);
  copy_m2_m2(m, size_temp.m);

  float zy[2], xy[2], xw[2], mzw[2], mxy[2], zw[2];
  sub_v2_v2v2(zy, z, y);
  sub_v2_v2v2(xy, x, y);
  sub_v2_v2v2(xw, x, w);
  sub_v2_v2v2(zw, z, w);

  mul_v2_m2v2(mzw, m, zw);
  mul_v2_m2v2(mxy, m, xy);

  return cloth_remeshing_wedge(zy, xy) * dot_v2v2(xw, mzw) +
             dot_v2v2(zy, mxy) * cloth_remeshing_wedge(xw, zw) <
         -CLOTH_REMESHING_EDGE_FLIP_THRESHOLD *
             (cloth_remeshing_wedge(zy, xy) + cloth_remeshing_wedge(xw, zw));
}

static bool cloth_remeshing_edge_on_seam_or_boundary_test(BMesh *bm,
                                                          BMEdge *e,
                                                          const int cd_loop_uv_offset)
{
#if 0
  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);

  return !f1 || !f2 ||
         cloth_remeshing_edge_vert(bm, e, 0, 0, NULL) !=
             cloth_remeshing_edge_vert(bm, e, 1, 0, NULL);
#else
  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);

  if (!f1 || !f2) {
    return true;
  }
  float uv_f1_v1[2], uv_f1_v2[2], uv_f2_v1[2], uv_f2_v2[2];
  cloth_remeshing_uv_of_vert_in_face(bm, f1, e->v1, cd_loop_uv_offset, uv_f1_v1);
  cloth_remeshing_uv_of_vert_in_face(bm, f1, e->v2, cd_loop_uv_offset, uv_f1_v2);
  cloth_remeshing_uv_of_vert_in_face(bm, f2, e->v1, cd_loop_uv_offset, uv_f2_v1);
  cloth_remeshing_uv_of_vert_in_face(bm, f2, e->v2, cd_loop_uv_offset, uv_f2_v2);

  return (!equals_v2v2(uv_f1_v1, uv_f2_v1) || !equals_v2v2(uv_f1_v2, uv_f2_v2));
#endif
}

static bool cloth_remeshing_vert_on_seam_or_boundary_test(BMesh *bm,
                                                          BMVert *v,
                                                          const int cd_loop_uv_offset)
{
  BMEdge *e;
  BMIter eiter;

  BM_ITER_ELEM (e, &eiter, v, BM_EDGES_OF_VERT) {
    if (cloth_remeshing_edge_on_seam_or_boundary_test(bm, e, cd_loop_uv_offset)) {
      return true;
    }
  }
  return false;
}

static bool cloth_remeshing_edge_label_test(BMEdge *e)
{
  return BM_elem_flag_test_bool(e, BM_ELEM_TAG);
}

static vector<BMEdge *> cloth_remeshing_find_edges_to_flip(BMesh *bm,
                                                           ClothVertMap &cvm,
                                                           vector<BMFace *> &active_faces,
                                                           const int cd_loop_uv_offset)
{
  vector<BMEdge *> edges;
  for (int i = 0; i < active_faces.size(); i++) {
    BMEdge *e;
    BMIter eiter;
    BM_ITER_ELEM (e, &eiter, active_faces[i], BM_EDGES_OF_FACE) {
      edges.push_back(e);
    }
  }
  vector<BMEdge *> fedges;
  for (int i = 0; i < edges.size(); i++) {
    BMEdge *e = edges[i];
    if (cloth_remeshing_edge_on_seam_or_boundary_test(bm, e, cd_loop_uv_offset)) {
      continue;
    }
    if (cloth_remeshing_edge_label_test(e)) {
      continue;
    }
    if (!cloth_remeshing_should_flip(bm, e, cvm, cd_loop_uv_offset)) {
      continue;
    }
    fedges.push_back(e);
  }
  return fedges;
}

static bool cloth_remeshing_independent_edge_test(BMEdge *e, vector<BMEdge *> edges)
{
  for (int i = 0; i < edges.size(); i++) {
    if (e->v1 == edges[i]->v1 || e->v1 == edges[i]->v2 || e->v2 == edges[i]->v1 ||
        e->v2 == edges[i]->v2) {
      return false;
    }
  }
  return true;
}

static vector<BMEdge *> cloth_remeshing_find_independent_edges(vector<BMEdge *> edges)
{
  vector<BMEdge *> i_edges;
  for (int i = 0; i < edges.size(); i++) {
    if (cloth_remeshing_independent_edge_test(edges[i], i_edges)) {
      i_edges.push_back(edges[i]);
    }
  }
  return i_edges;
}

static bool cloth_remeshing_flip_edges(BMesh *bm,
                                       ClothVertMap &cvm,
                                       vector<BMFace *> &active_faces,
                                       const int cd_loop_uv_offset)
{
  static int prev_num_independent_edges = 0;
  vector<BMEdge *> flipable_edges = cloth_remeshing_find_edges_to_flip(
      bm, cvm, active_faces, cd_loop_uv_offset);
  vector<BMEdge *> independent_edges = cloth_remeshing_find_independent_edges(flipable_edges);
  if (independent_edges.size() == prev_num_independent_edges) {
    return false;
  }
  prev_num_independent_edges = independent_edges.size();
  for (int i = 0; i < independent_edges.size(); i++) {
    BMEdge *edge = independent_edges[i];
    vector<BMFace *> remove_faces;
    BMFace *f;
    BMIter fiter;
    BM_ITER_ELEM (f, &fiter, edge, BM_FACES_OF_EDGE) {
      remove_faces.push_back(f);
    }
    /* BM_EDGEROT_CHECK_SPLICE sets it up for BM_CREATE_NO_DOUBLE */
    BMEdge *new_edge = BM_edge_rotate(bm, edge, true, BM_EDGEROT_CHECK_SPLICE);
    /* TODO(Ish): all the edges part of independent_edges should be
     * rotatable, not sure why some of them fail to rotate, need to
     * check this later. It might be fixed after seam or boundary
     * detection is fixed */
    /* BLI_assert(new_edge != NULL); */
    /* TODO(Ish): need to check if the normals are flipped by some
     * kind of area check */

#if 0
    cloth_remeshing_update_active_faces(active_faces, bm, new_edge);
#else
    if (!new_edge) {
      continue;
    }
    vector<BMFace *> add_faces;
    BM_ITER_ELEM (f, &fiter, new_edge, BM_FACES_OF_EDGE) {
      add_faces.push_back(f);
    }
    cloth_remeshing_update_active_faces(active_faces, remove_faces, add_faces);
#endif
  }
  return true;
}

static bool cloth_remeshing_fix_mesh(BMesh *bm,
                                     ClothVertMap &cvm,
                                     vector<BMFace *> active_faces,
                                     const int cd_loop_uv_offset)
{
  for (int i = 0; i < bm->totvert * 3; i++) {
    if (cloth_remeshing_flip_edges(bm, cvm, active_faces, cd_loop_uv_offset) == false) {
      break;
    }
  }
  return true;
}

static ClothSizing cloth_remeshing_find_average_sizing(ClothSizing &size_01, ClothSizing &size_02)
{
  ClothSizing new_size;
  add_m2_m2m2(new_size.m, size_01.m, size_02.m);
  mul_m2_fl(new_size.m, 0.5f);
  return new_size;
}

static float cloth_remeshing_norm2(float u[2], ClothSizing &s)
{
  float temp[2];
  mul_v2_m2v2(temp, s.m, u);
  return dot_v2v2(u, temp);
}

/* Ensure the edge also has at least one face associated with it so
 * that e->l exists */
static void cloth_remeshing_uv_of_vert_in_edge(BMesh *bm, BMEdge *e, BMVert *v, float r_uv[2])
{
  BLI_assert(e->l != NULL);

  BMLoop *l;
  e->l->v == v ? l = e->l : l = e->l->next;
  const int cd_loop_uv_offset = CustomData_get_offset(&bm->ldata, CD_MLOOPUV);
  MLoopUV *luv = (MLoopUV *)BM_ELEM_CD_GET_VOID_P(l, cd_loop_uv_offset);
  copy_v2_v2(r_uv, luv->uv);
}

static float cloth_remeshing_edge_size(
    BMVert *v1, BMVert *v2, float uv1[2], float uv2[2], ClothVertMap &cvm)
{
  if (!v1 || !v2 || uv1 == NULL || uv2 == NULL) {
    return 0.0f;
  }

  float uv12[2];
  sub_v2_v2v2(uv12, uv1, uv2);

  return sqrtf((cloth_remeshing_norm2(uv12, *cvm[v1].sizing) +
                cloth_remeshing_norm2(uv12, *cvm[v2].sizing)) *
               0.5f);
}

static float cloth_remeshing_edge_size(BMesh *bm, BMEdge *edge, ClothVertMap &cvm)
{
#if 0
  float u1[2], u2[2];
  if (!edge->l) {
    return 0.0f;
  }
  cloth_remeshing_uv_of_vert_in_edge(bm, edge, edge->v1, u1);
  cloth_remeshing_uv_of_vert_in_edge(bm, edge, edge->v2, u2);
  return cloth_remeshing_edge_size(edge->v1, edge->v2, u1, u2, cvm);
#else
  float uvs[4][2];
  BMVert *v1 = cloth_remeshing_edge_vert(bm, edge, 0, 0, uvs[0]);
  BMVert *v2 = cloth_remeshing_edge_vert(bm, edge, 0, 1, uvs[1]);
  BMVert *v3 = cloth_remeshing_edge_vert(bm, edge, 1, 0, uvs[2]);
  BMVert *v4 = cloth_remeshing_edge_vert(bm, edge, 1, 1, uvs[3]);
  float m = cloth_remeshing_edge_size(v1, v2, uvs[0], uvs[1], cvm) +
            cloth_remeshing_edge_size(v3, v4, uvs[2], uvs[3], cvm);
  if (BM_edge_face_count(edge) == 2) {
    return m * 0.5f;
  }
  else {
    return m;
  }
#endif
}

typedef pair<float, BMEdge *> SizeEdgePair;

static bool cloth_remeshing_size_edge_pair_compare(const SizeEdgePair &l, const SizeEdgePair &r)
{
  return l.first > r.first;
}

static void cloth_remeshing_find_bad_edges(BMesh *bm, ClothVertMap &cvm, vector<BMEdge *> &r_edges)
{
  vector<SizeEdgePair> edge_pairs;

  int tagged = 0;
  BMEdge *e;
  BMIter eiter;
  BM_ITER_MESH (e, &eiter, bm, BM_EDGES_OF_MESH) {
    float size = cloth_remeshing_edge_size(bm, e, cvm);
#if FACE_SIZING_DEBUG
#  if FACE_SIZING_DEBUG_SIZE
    printf("size: %f in %s\n", size, __func__);
#  endif
#endif
    if (size > 1.0f) {
      edge_pairs.push_back(make_pair(size, e));
      tagged++;
    }
  }

  /* sort the list based on the size */
  sort(edge_pairs.begin(), edge_pairs.end(), cloth_remeshing_size_edge_pair_compare);

  for (int i = 0; i < edge_pairs.size(); i++) {
    r_edges.push_back(edge_pairs[i].second);
  }

  edge_pairs.clear();
}

static BMVert *cloth_remeshing_split_edge_keep_triangles(BMesh *bm,
                                                         BMEdge *e,
                                                         BMVert *v,
                                                         float fac)
{
  BLI_assert(BM_vert_in_edge(e, v) == true);

  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);
  /* There should be at least one face for that edge */
  if (!f1) {
    if (!f2) {
      return NULL;
    }
    f1 = f2;
    f2 = NULL;
  }

  /* split the edge */
  BMEdge *new_edge;
  BMVert *new_v = BM_edge_split(bm, e, v, &new_edge, fac);
  /* if (cloth_remeshing_edge_label_test(e)) { */
  /*   BM_elem_flag_enable(new_edge, BM_ELEM_TAG); */
  /* } */
  /* else { */
  /*   BM_elem_flag_disable(new_edge, BM_ELEM_TAG); */
  /* } */

  BMVert *vert;
  BMIter viter;
  /* search for vert within the face that is not part of input edge
   * create new edge between this
   * vert and newly created vert */
  BM_ITER_ELEM (vert, &viter, f1, BM_VERTS_OF_FACE) {
    if (vert == e->v1 || vert == e->v2 || vert == new_edge->v1 || vert == new_edge->v2 ||
        vert == new_v) {
      continue;
    }
    BMLoop *l_a = NULL, *l_b = NULL;
    l_a = BM_face_vert_share_loop(f1, vert);
    l_b = BM_face_vert_share_loop(f1, new_v);
    if (!BM_face_split(bm, f1, l_a, l_b, NULL, NULL, true)) {
      printf("face not split: f1\n");
    }
    break;
  }
  if (f2) {
    BM_ITER_ELEM (vert, &viter, f2, BM_VERTS_OF_FACE) {
      if (vert == e->v1 || vert == e->v2 || vert == new_edge->v1 || vert == new_edge->v2 ||
          vert == new_v) {
        continue;
      }
      BMLoop *l_a = NULL, *l_b = NULL;
      l_a = BM_face_vert_share_loop(f2, vert);
      l_b = BM_face_vert_share_loop(f2, new_v);
      if (!BM_face_split(bm, f2, l_a, l_b, NULL, NULL, true)) {
        printf("face not split: f2\n");
      }
      break;
    }
  }

  return new_v;
}

static void cloth_remeshing_export_obj(BMesh *bm, char *file_name)
{
  FILE *fout = fopen(file_name, "w");
  if (!fout) {
    printf("File not written, couldn't find path to %s\n", file_name);
    return;
  }
  printf("Writing file %s\n", file_name);
  typedef struct Vert {
    float co[3];
    float no[3];
  } Vert;
  typedef struct UV {
    float uv[2];
  } UV;
  typedef struct Face {
    int *vi;
    int *uv;
    int len;
    BMFace *bm_face;
  } Face;
  Vert *verts;
  verts = (Vert *)MEM_mallocN(sizeof(Vert) * bm->totvert, "Verts");
  UV *uvs;
  uvs = (UV *)MEM_mallocN(sizeof(UV) * bm->totvert, "UVs");
  Face *faces;
  faces = (Face *)MEM_mallocN(sizeof(Face) * bm->totface, "Faces");
  for (int i = 0; i < bm->totface; i++) {
    faces[i].vi = NULL;
    faces[i].uv = NULL;
    faces[i].len = 0;
    faces[i].bm_face = NULL;
  }
  BMVert *v;
  BMIter viter;
  int vert_i = 0;
  int face_i = 0;
  BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
    copy_v3_v3(verts[vert_i].co, v->co);
    copy_v3_v3(verts[vert_i].no, v->no);
    BMFace *f;
    BMIter fiter;
    BM_ITER_ELEM (f, &fiter, v, BM_FACES_OF_VERT) {
      int curr_i = face_i;
      for (int i = 0; i < face_i; i++) {
        if (faces[i].bm_face == f) {
          curr_i = i;
          break;
        }
      }
      if (faces[curr_i].len == 0) {
        faces[curr_i].len = 1;
        faces[curr_i].vi = (int *)MEM_mallocN(sizeof(int) * 6, "face vi");
        faces[curr_i].uv = (int *)MEM_mallocN(sizeof(int) * 6, "face uv");
        faces[curr_i].bm_face = f;
        face_i++;
      }
      faces[curr_i].vi[faces[curr_i].len - 1] = vert_i + 1;
      faces[curr_i].uv[faces[curr_i].len - 1] = vert_i + 1;
      faces[curr_i].len++;
    }
    BMLoop *l;
    BMIter liter;
    MLoopUV *luv;
    const int cd_loop_uv_offset = CustomData_get_offset(&bm->ldata, CD_MLOOPUV);

    BM_ITER_ELEM (l, &liter, v, BM_LOOPS_OF_VERT) {
      luv = (MLoopUV *)BM_ELEM_CD_GET_VOID_P(l, cd_loop_uv_offset);
      copy_v2_v2(uvs[vert_i].uv, luv->uv);
    }
    vert_i++;
  }

  for (int i = 0; i < bm->totvert; i++) {
    fprintf(fout, "v %f %f %f\n", verts[i].co[0], verts[i].co[1], verts[i].co[2]);
  }

  for (int i = 0; i < bm->totvert; i++) {
    fprintf(fout, "vt %f %f\n", uvs[i].uv[0], uvs[i].uv[1]);
  }

  for (int i = 0; i < bm->totvert; i++) {
    fprintf(fout, "vn %f %f %f\n", verts[i].no[0], verts[i].no[1], verts[i].no[2]);
  }

  for (int i = 0; i < bm->totface; i++) {
    fprintf(fout, "f ");
    for (int j = 0; j < faces[i].len - 1; j++) {
      fprintf(fout, "%d/%d/%d ", faces[i].vi[j], faces[i].uv[j], faces[i].vi[j]);
    }
    fprintf(fout, "\n");
  }

  for (int i = 0; i < bm->totface; i++) {
    if (faces[i].vi) {
      MEM_freeN(faces[i].vi);
    }
    if (faces[i].uv) {
      MEM_freeN(faces[i].uv);
    }
  }

  MEM_freeN(faces);
  MEM_freeN(uvs);
  MEM_freeN(verts);

  fclose(fout);
  printf("File %s written\n", file_name);
}

static void cloth_remeshing_print_all_verts(ClothVertex *verts, int vert_num)
{
  for (int i = 0; i < vert_num; i++) {
    printf("%f %f %f\n", verts[i].xold[0], verts[i].xold[1], verts[i].xold[2]);
  }
}

static void cloth_remeshing_add_vertex_to_cloth(BMVert *v1,
                                                BMVert *v2,
                                                BMVert *new_vert,
                                                ClothVertMap &cvm)
{
  ClothVertex *cv1, *cv2;
  cv1 = &cvm[v1];
  cv2 = &cvm[v2];
#if 0
    printf("v1: %f %f %f\n", v1->co[0], v1->co[1], v1->co[2]);
    printf("v2: %f %f %f\n", v2->co[0], v2->co[1], v2->co[2]);
#endif
  BLI_assert(cv1 != NULL);
  BLI_assert(cv2 != NULL);
  cvm[new_vert] = cloth_remeshing_mean_cloth_vert(cv1, cv2);

  ClothSizing *temp_sizing = (ClothSizing *)MEM_mallocN(sizeof(ClothSizing),
                                                        "New Vertex ClothSizing");
  *temp_sizing = cloth_remeshing_find_average_sizing(*cvm[v1].sizing, *cvm[v2].sizing);
  cvm[new_vert].sizing = temp_sizing;
}

static BMEdge *cloth_remeshing_find_next_loose_edge(BMVert *v)
{
  BMEdge *e;
  BMIter eiter;

  BM_ITER_ELEM (e, &eiter, v, BM_EDGES_OF_VERT) {
    if (BM_edge_face_count(e) == 0) {
      return e;
    }
  }

  return NULL;
}

static void cloth_remeshing_print_all_verts(BMesh *bm)
{
  BMVert *v;
  BMIter iter;
  int i = 0;
  BM_ITER_MESH_INDEX (v, &iter, bm, BM_VERTS_OF_MESH, i) {
    printf("v%3d: %f %f %f\n", i, v->co[0], v->co[1], v->co[2]);
  }
}

/*
 * v5    v4
 * *-----*
 * |     |
 * *--*--*
 * v1 v2 v3
 *
 * v2 same as new_vert
 **/
#if 0
static BMEdge *cloth_remeshing_fix_sewing_verts(
    Cloth *cloth, BMesh *bm, BMVert *v1, BMVert *new_vert, BMVert *v3, ClothVertMap &cvm)
{
  /* cloth_remeshing_print_all_verts(bm); */
  BMEdge *v3v4 = cloth_remeshing_find_next_loose_edge(v3);
  BMVert *v4, *v5;
  v3v4->v1 == v3 ? v4 = v3v4->v2 : v4 = v3v4->v1;

  BMEdge *v4v5;
  BMEdge *v5v1;
  BMIter v4v5_iter;
  bool found_v4v5 = false;
  BM_ITER_ELEM (v4v5, &v4v5_iter, v4, BM_EDGES_OF_VERT) {
    v4v5->v1 == v4 ? v5 = v4v5->v2 : v5 = v4v5->v1;

    v5v1 = cloth_remeshing_find_next_loose_edge(v5);
    if (v5v1 != NULL && v5v1 != v3v4) {
      /* printf("found v4v5\n"); */
      /* printf("v1: %f %f %f\n", v1->co[0], v1->co[1], v1->co[2]); */
      /* printf("v2: %f %f %f\n", new_vert->co[0], new_vert->co[1], new_vert->co[2]); */
      /* printf("v3: %f %f %f\n", v3->co[0], v3->co[1], v3->co[2]); */
      /* printf("v4: %f %f %f\n", v4->co[0], v4->co[1], v4->co[2]); */
      /* printf("v5: %f %f %f\n", v5->co[0], v5->co[1], v5->co[2]); */
      if (v5v1->v1 == v1 || v5v1->v2 == v1) {
        found_v4v5 = true;
        break;
      }
    }
  }

  if (!found_v4v5) {
    return NULL;
  }

  float v4v5_size = cloth_remeshing_edge_size(bm, v4v5, cvm);
  BMVert *v6 = NULL;
  if (v4v5_size > 1.0f) {
    BMEdge v4v5_old = *v4v5;
    v6 = cloth_remeshing_split_edge_keep_triangles(bm, v4v5, v4, 0.5);
    cloth->verts = (ClothVertex *)MEM_reallocN(cloth->verts,
                                               (cloth->mvert_num + 1) * sizeof(ClothVertex));
    cloth_remeshing_add_vertex_to_cloth(v4v5_old.v1, v4v5_old.v2, v6, cvm);
    printf("joining new_vert and v6\n");
    return BM_edge_create(bm, new_vert, v6, v3v4, BM_CREATE_NO_DOUBLE);
  }
  else {
    float v2v5[3];
    copy_v3_v3(v2v5, new_vert->co);
    sub_v3_v3(v2v5, v5->co);
    float v2v4[3];
    copy_v3_v3(v2v4, new_vert->co);
    sub_v3_v3(v2v4, v4->co);
    if (len_v3(v2v5) < len_v3(v2v4)) {
      printf("joining new_vert and v5\n");
      return BM_edge_create(bm, new_vert, v5, v3v4, BM_CREATE_NO_DOUBLE);
    }
    else {
      printf("joining new_vert and v4\n");
      return BM_edge_create(bm, new_vert, v4, v3v4, BM_CREATE_NO_DOUBLE);
    }
  }
}
#else
static BMEdge *cloth_remeshing_fix_sewing_verts(
    Cloth *cloth, BMesh *bm, BMVert *v1, BMVert *new_vert, BMVert *v3, ClothVertMap &cvm)
{
  BMVert *v4, *v5;
  BMIter iter;
  BMEdge *v3v4, *v4v5, *v5v1;
  v3v4 = cloth_remeshing_find_next_loose_edge(v3);
  v4 = v3v4->v1 == v3 ? v3v4->v2 : v3v4->v1;

  BM_ITER_ELEM (v4v5, &iter, v4, BM_EDGES_OF_VERT) {
    v5 = v4v5->v1 == v4 ? v4v5->v2 : v4v5->v1;

    v5v1 = cloth_remeshing_find_next_loose_edge(v5);
    if (v5v1) {
      if (v5v1->v1 == v1 || v5v1->v2 == v1) {
        break;
      }
    }
  }

  if (!v4 || !v5 || !v5v1 || !v4v5) {
    return NULL;
  }

  float size_v4v5 = cloth_remeshing_edge_size(bm, v4v5, cvm);
  if (size_v4v5 > 1.0f) {
    BMEdge v4v5_old = *v4v5;
    BMVert *v6 = cloth_remeshing_split_edge_keep_triangles(bm, v4v5, v4, 0.5f);
    if (!v6) {
      return NULL;
    }
    cloth_remeshing_add_vertex_to_cloth(v4v5_old.v1, v4v5_old.v2, v6, cvm);
    return BM_edge_create(bm, new_vert, v6, v3v4, BM_CREATE_NO_DOUBLE);
  }
  return BM_edge_create(bm, new_vert, v5, v3v4, BM_CREATE_NO_DOUBLE);
}
#endif

static bool cloth_remeshing_split_edges(ClothModifierData *clmd,
                                        ClothVertMap &cvm,
                                        const int cd_loop_uv_offset)
{
  BMesh *bm = clmd->clothObject->bm;
  static int prev_num_bad_edges = 0;
  vector<BMEdge *> bad_edges;
  cloth_remeshing_find_bad_edges(bm, cvm, bad_edges);
#if REMESHING_DATA_DEBUG
  printf("split edges tagged: %d\n", (int)bad_edges.size());
#endif
  if (prev_num_bad_edges == bad_edges.size()) {
    bad_edges.clear();
    return false;
  }
  prev_num_bad_edges = bad_edges.size();
  BMEdge *e;
  for (int i = 0; i < bad_edges.size(); i++) {
    e = bad_edges[i];
    if (!(e->head.htype == BM_EDGE)) {
      /* TODO(Ish): this test should not be required in theory, it
       * might be fixed after seam or boundary detection is fixed */
      continue;
    }
    BMEdge old_edge = *e;
    BMVert *new_vert = cloth_remeshing_split_edge_keep_triangles(bm, e, e->v1, 0.5);
    if (!new_vert) {
      printf("new_vert == NULL\n");
      continue;
    }

    cloth_remeshing_add_vertex_to_cloth(old_edge.v1, old_edge.v2, new_vert, cvm);

    vector<BMFace *> active_faces;
    BMFace *af;
    BMIter afiter;
    BM_ITER_ELEM (af, &afiter, new_vert, BM_FACES_OF_VERT) {
      active_faces.push_back(af);
    }
    cloth_remeshing_fix_mesh(bm, cvm, active_faces, cd_loop_uv_offset);

    if (clmd->sim_parms->flags & CLOTH_SIMSETTINGS_FLAG_SEW) {
      if (cloth_remeshing_find_next_loose_edge(old_edge.v1) != NULL &&
          cloth_remeshing_find_next_loose_edge(old_edge.v2) != NULL) {
        cloth_remeshing_fix_sewing_verts(
            clmd->clothObject, bm, old_edge.v1, new_vert, old_edge.v2, cvm);
      }
    }
  }
  bool r_value = !bad_edges.empty();
  bad_edges.clear();
  return r_value;
}

static void cloth_remeshing_uv_of_vert_in_face(
    BMesh *bm, BMFace *f, BMVert *v, const int cd_loop_uv_offset, float r_uv[2])
{
  BLI_assert(BM_vert_in_face(v, f));

  BMLoop *l = BM_face_vert_share_loop(f, v);
  MLoopUV *luv = (MLoopUV *)BM_ELEM_CD_GET_VOID_P(l, cd_loop_uv_offset);
  copy_v2_v2(r_uv, luv->uv);
}

static inline float cloth_remeshing_wedge(float v_01[2], float v_02[2])
{
  return v_01[0] * v_02[1] - v_01[1] * v_02[0];
}

#define SQRT3 1.732050808f

static float cloth_remeshing_area(float u1[2], float u2[2], float u3[2])
{
  float u21[2], u31[2];
  sub_v2_v2v2(u21, u2, u1);
  sub_v2_v2v2(u31, u3, u1);
  return 0.5 * cloth_remeshing_wedge(u21, u31);
}

static float cloth_remeshing_perimeter(float u1[2], float u2[2], float u3[2])
{
  float u21[2], u31[2], u23[2];
  sub_v2_v2v2(u21, u2, u1);
  sub_v2_v2v2(u31, u3, u1);
  sub_v2_v2v2(u23, u2, u3);
  return len_v2(u21) + len_v2(u31) + len_v2(u23);
}

static float cloth_remeshing_aspect_ratio(float u1[2], float u2[2], float u3[2])
{
  return 12.0f * SQRT3 * cloth_remeshing_area(u1, u2, u3) /
         sqr_fl(cloth_remeshing_perimeter(u1, u2, u3));
}

static BMVert *cloth_remeshing_edge_vert(BMEdge *e, int which)
{
  if (which == 0) {
    return e->v1;
  }
  else {
    return e->v2;
  }
}

static void cloth_remeshing_edge_face_pair(BMEdge *e, BMFace **r_f1, BMFace **r_f2)
{
  BMFace *f;
  BMIter fiter;
  int i = 0;
  *r_f1 = NULL;
  *r_f2 = NULL;
  BM_ITER_ELEM_INDEX (f, &fiter, e, BM_FACES_OF_EDGE, i) {
    if (i == 0) {
      *r_f1 = f;
    }
    else if (i == 1) {
      *r_f2 = f;
    }
    else {
      break;
    }
  }
}

/* r_uv is the uv of the BMVert that is returned */
static BMVert *cloth_remeshing_edge_vert(BMesh *bm, BMEdge *e, int side, int i, float r_uv[2])
{
  /* BMFace *f, *f1 = NULL, *f2 = NULL; */
  /* BMIter fiter; */
  /* int fi = 0; */
  /* BM_ITER_ELEM_INDEX (f, &fiter, e, BM_FACES_OF_EDGE, fi) { */
  /*   if (fi == 0) { */
  /*     f1 = f; */
  /*   } */
  /*   else if (fi == 1) { */
  /*     f2 = f; */
  /*   } */
  /*   if (fi == side) { */
  /*     break; */
  /*   } */
  /* } */
  BMFace *f, *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);
  side == 0 ? f = f1 : f = f2;
  if (!f) {
#if 0
    printf("didn't find f in %s\n", __func__);
#endif
    return NULL;
  }
  BMVert *vs[3];
  BM_face_as_array_vert_tri(f, vs);
  for (int j = 0; j < 3; j++) {
    if (vs[j] == cloth_remeshing_edge_vert(e, i)) {
      if (r_uv != NULL) {
        cloth_remeshing_uv_of_vert_in_edge(bm, e, vs[j], r_uv);
      }
      return vs[j];
    }
  }
  return NULL;
}

/* r_uv is the uv of the BMVert that is returned */
static BMVert *cloth_remeshing_edge_opposite_vert(
    BMesh *bm, BMEdge *e, int side, const int cd_loop_uv_offset, float r_uv[2])
{
  /* BMFace *f; */
  /* BMIter fiter; */
  /* int fi = 0; */
  /* BM_ITER_ELEM_INDEX (f, &fiter, e, BM_FACES_OF_EDGE, fi) { */
  /*   if (fi == side) { */
  /*     break; */
  /*   } */
  /* } */
  BMFace *f, *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);
  side == 0 ? f = f1 : f = f2;
  if (!f) {
#if 0
    printf("didn't find f in %s\n", __func__);
#endif
    return NULL;
  }
  BMVert *vs[3];
  BM_face_as_array_vert_tri(f, vs);
  for (int j = 0; j < 3; j++) {
    if (vs[j] == cloth_remeshing_edge_vert(e, side)) {
      if (r_uv != NULL) {
        cloth_remeshing_uv_of_vert_in_face(bm, f, vs[PREV(j)], cd_loop_uv_offset, r_uv);
      }
      return vs[PREV(j)];
    }
  }
  return NULL;
}

#define REMESHING_HYSTERESIS_PARAMETER 0.2
static bool cloth_remeshing_can_collapse_edge(ClothModifierData *clmd,
                                              BMesh *bm,
                                              BMEdge *e,
                                              int which,
                                              ClothVertMap &cvm,
                                              const int cd_loop_uv_offset)
{
  for (int s = 0; s < 2; s++) {
    BMVert *v1 = cloth_remeshing_edge_vert(bm, e, s, which, NULL);
    float uv_v2[2];
    BMVert *v2 = cloth_remeshing_edge_vert(bm, e, s, 1 - which, uv_v2);

    if (!v1 || (s == 1 && v1 == cloth_remeshing_edge_vert(bm, e, 0, which, NULL))) {
#if COLLAPSE_EDGES_DEBUG
      if (!v1) {
        printf("continued at !v1\n");
      }
      else {
        printf("(s == 1 && v1 == ......)\n");
      }
#endif
      continue;
    }

    BMFace *f;
    BMIter fiter;
    BM_ITER_ELEM (f, &fiter, v1, BM_FACES_OF_VERT) {
      if (BM_vert_in_face(v2, f)) {
#if COLLAPSE_EDGES_DEBUG
        printf("continued at BM_vert_in_face(v2, f)\n");
#endif
        continue;
      }
      BMVert *vs[3];
      BM_face_as_array_vert_tri(f, vs);
      float uvs[3][2];
      cloth_remeshing_uv_of_vert_in_face(bm, f, vs[0], cd_loop_uv_offset, uvs[0]);
      cloth_remeshing_uv_of_vert_in_face(bm, f, vs[1], cd_loop_uv_offset, uvs[1]);
      cloth_remeshing_uv_of_vert_in_face(bm, f, vs[2], cd_loop_uv_offset, uvs[2]);
      /* Replace the v1 with v2 in vs */
      for (int i = 0; i < 3; i++) {
        if (vs[i] == v1) {
          vs[i] = v2;
          copy_v2_v2(uvs[i], uv_v2);
          break;
        }
      }

      /* Aspect ratio part */
      float uv_21[2], uv_31[2];
      sub_v2_v2v2(uv_21, uvs[1], uvs[0]);
      sub_v2_v2v2(uv_31, uvs[2], uvs[0]);
      float a = cloth_remeshing_wedge(uv_21, uv_31) * 0.5f;
      float asp = cloth_remeshing_aspect_ratio(uvs[0], uvs[1], uvs[2]);
      if (a < 1e-6 || asp < clmd->sim_parms->aspect_min) {
#if COLLAPSE_EDGES_DEBUG
        printf("a: %f < 1e-6 || aspect %f < aspect_min, returning\n\n", a, asp);
#endif
        return false;
      }

      float size;
      for (int ei = 0; ei < 3; ei++) {
        if (vs[ei] != v2) {
          size = cloth_remeshing_edge_size(
              vs[NEXT(ei)], vs[PREV(ei)], uvs[NEXT(ei)], uvs[PREV(ei)], cvm);
          if (size > 1.0f - REMESHING_HYSTERESIS_PARAMETER) {
#if COLLAPSE_EDGES_DEBUG
            printf("size %f > 1.0f - REMESHING_HYSTERESIS_PARAMETER, returning\n\n", size);
#endif
            return false;
          }
#if COLLAPSE_EDGES_DEBUG
          printf("size: %f ", size);
#endif
        }
      }
    }
  }
#if COLLAPSE_EDGES_DEBUG
  printf("sucessfully collapsed\n");
#endif
  return true;
}

static void cloth_remeshing_remove_vertex_from_cloth(BMVert *v, ClothVertMap &cvm)
{
#if 0
  printf("removed: %f %f %f\n",
         cvm[v].x[0],
         cvm[v].x[1],
         cvm[v].x[2]);
#endif
  MEM_freeN(cvm[v].sizing);
  cvm.erase(v);
}

static bool cloth_remeshing_boundary_test(BMEdge *e)
{
  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);
  return !f1 || !f2;
}

static bool cloth_remeshing_boundary_test(BMVert *v)
{
  BMFace *f;
  BMEdge *e;
  BMIter iter;
  int face_count = 0, vert_count = 0;

  BM_ITER_ELEM (f, &iter, v, BM_FACES_OF_VERT) {
    face_count++;
  }

  BM_ITER_ELEM (e, &iter, v, BM_EDGES_OF_VERT) {
    if (e->v1 != v || e->v2 != v) {
      vert_count++;
    }
  }

  return face_count != vert_count;
}

static bool cloth_remeshing_edge_on_seam_test(BMesh *bm, BMEdge *e, const int cd_loop_uv_offset)
{
  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(e, &f1, &f2);
  if (!f1 || !f2) {
    return true;
  }
  float uv_f1_v1[2], uv_f1_v2[2], uv_f2_v1[2], uv_f2_v2[2];
  cloth_remeshing_uv_of_vert_in_face(bm, f1, e->v1, cd_loop_uv_offset, uv_f1_v1);
  cloth_remeshing_uv_of_vert_in_face(bm, f1, e->v2, cd_loop_uv_offset, uv_f1_v2);
  cloth_remeshing_uv_of_vert_in_face(bm, f2, e->v1, cd_loop_uv_offset, uv_f2_v1);
  cloth_remeshing_uv_of_vert_in_face(bm, f2, e->v2, cd_loop_uv_offset, uv_f2_v2);

  return (!equals_v2v2(uv_f1_v1, uv_f2_v1) || !equals_v2v2(uv_f1_v2, uv_f2_v2));
}

static bool cloth_remeshing_vert_on_seam_test(BMesh *bm, BMVert *v, const int cd_loop_uv_offset)
{
  BMEdge *e;
  BMIter eiter;
  BM_ITER_ELEM (e, &eiter, v, BM_EDGES_OF_VERT) {
    if (cloth_remeshing_edge_on_seam_test(bm, e, cd_loop_uv_offset)) {
      return true;
    }
  }
  return false;
}

static BMVert *cloth_remeshing_collapse_edge(BMesh *bm, BMEdge *e, int which, ClothVertMap &cvm)
{
  BMVert *v1 = cloth_remeshing_edge_vert(e, which);
  /* Need to store the value of vertex v1 which will be killed */
  BMVert *v = v1;
  BMVert *v2 = BM_edge_collapse(bm, e, v1, true, true);

  if (v2) {
    cloth_remeshing_remove_vertex_from_cloth(v, cvm);
  }
#if COLLAPSE_EDGES_DEBUG
  printf("killed %f %f %f into %f %f %f\n",
         v.co[0],
         v.co[1],
         v.co[2],
         v2->co[0],
         v2->co[1],
         v2->co[2]);
#endif
  return v2;
}

static bool cloth_remeshing_has_labeled_edges(BMVert *v)
{
  BMEdge *e;
  BMIter eiter;

  BM_ITER_ELEM (e, &eiter, v, BM_EDGES_OF_VERT) {
    if (BM_elem_flag_test_bool(e, BM_ELEM_TAG)) {
      return true;
    }
  }
  return false;
}

static BMVert *cloth_remeshing_try_edge_collapse(
    ClothModifierData *clmd, BMEdge *e, int which, ClothVertMap &cvm, const int cd_loop_uv_offset)
{
  Cloth *cloth = clmd->clothObject;
  BMesh *bm = cloth->bm;
  BMVert *v1 = cloth_remeshing_edge_vert(e, which);

  /* if (cloth_remeshing_boundary_test(v1) && !cloth_remeshing_boundary_test(e)) { */
  /*   return NULL; */
  /* } */

  /* if (cloth_remeshing_vert_on_seam_test(bm, v1) && !cloth_remeshing_edge_on_seam_test(bm, e)) {
   */
  /*   return NULL; */
  /* } */

  if (cvm[v1].flags & CLOTH_VERT_FLAG_PRESERVE) {
    return NULL;
  }

  if (cloth_remeshing_vert_on_seam_or_boundary_test(bm, v1, cd_loop_uv_offset) &&
      !cloth_remeshing_edge_on_seam_or_boundary_test(bm, e, cd_loop_uv_offset)) {
#if COLLAPSE_EDGES_DEBUG
    printf("vertex %f %f %f on seam or boundary but not edge\n", v1->co[0], v1->co[1], v1->co[2]);
#endif
    return NULL;
  }

  if (cloth_remeshing_has_labeled_edges(v1) && !cloth_remeshing_edge_label_test(e)) {
    return NULL;
  }

  if (!cloth_remeshing_can_collapse_edge(clmd, bm, e, which, cvm, cd_loop_uv_offset)) {
    return NULL;
  }

  return cloth_remeshing_collapse_edge(bm, e, which, cvm);
}

static void cloth_remeshing_remove_face(vector<BMFace *> &faces, int index)
{
  faces[index] = faces.back();
  faces.pop_back();
}

static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                vector<BMFace *> &remove_faces,
                                                vector<BMFace *> &add_faces)
{
  vector<BMFace *> new_active_faces;

  for (int i = 0; i < add_faces.size(); i++) {
    new_active_faces.push_back(add_faces[i]);
  }
  for (int i = 0; i < active_faces.size(); i++) {
    if (find(remove_faces.begin(), remove_faces.end(), active_faces[i]) != remove_faces.end()) {
      continue;
    }
    new_active_faces.push_back(active_faces[i]);
  }

  active_faces.swap(new_active_faces);
  new_active_faces.clear();
}

static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                BMesh *bm,
                                                BMEdge *e)
{
  BMFace *f, *f2;
  BMIter fiter;
  vector<BMFace *> new_active_faces;
  /* add the newly created faces, all those that have that edge e */
  BM_ITER_ELEM (f, &fiter, e, BM_FACES_OF_EDGE) {
    new_active_faces.push_back(f);
  }

  /* remove the faces from active_faces that have been removed from
   * bmesh */
  for (int i = 0; i < active_faces.size(); i++) {
    bool already_exists = false;
    f = active_faces[i];
    for (int j = 0; j < new_active_faces.size(); j++) {
      if (f == new_active_faces[j]) {
        already_exists = true;
        break;
      }
    }
    if (already_exists) {
      break;
    }
    BM_ITER_MESH (f2, &fiter, bm, BM_FACES_OF_MESH) {
      if (f == f2) {
        new_active_faces.push_back(f);
        break;
      }
    }
  }

  active_faces.swap(new_active_faces);
  new_active_faces.clear();
}

static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                BMesh *bm,
                                                BMVert *v)
{
  BMFace *f, *f2;
  BMIter fiter;
  vector<BMFace *> new_active_faces;
  /* add the newly created faces, all those that have that vertex v */
  BM_ITER_ELEM (f, &fiter, v, BM_FACES_OF_VERT) {
    new_active_faces.push_back(f);
  }

  /* remove the faces from active_faces that have been removed from
   * bmesh */
  for (int i = 0; i < active_faces.size(); i++) {
    bool already_exists = false;
    f = active_faces[i];
    for (int j = 0; j < new_active_faces.size(); j++) {
      if (f == new_active_faces[j]) {
        already_exists = true;
        break;
      }
    }
    if (already_exists) {
      break;
    }
    BM_ITER_MESH (f2, &fiter, bm, BM_FACES_OF_MESH) {
      if (f == f2) {
        new_active_faces.push_back(f);
        break;
      }
    }
  }

  active_faces.swap(new_active_faces);
  new_active_faces.clear();
}

/* Assumed that active_faces and fix_active have been updated before
 * using either of the other 2 update_active_faces function so that
 * there is no face that is not part of bm */
static void cloth_remeshing_update_active_faces(vector<BMFace *> &active_faces,
                                                vector<BMFace *> &fix_active)
{
  for (int i = 0; i < fix_active.size(); i++) {
    bool already_exists = false;
    for (int j = 0; j < active_faces.size(); j++) {
      if (active_faces[j] == fix_active[i]) {
        already_exists = true;
        break;
      }
    }
    if (already_exists) {
      continue;
    }
    active_faces.push_back(fix_active[i]);
  }
}

static bool cloth_remeshing_collapse_edges(ClothModifierData *clmd,
                                           ClothVertMap &cvm,
                                           vector<BMFace *> &active_faces,
                                           const int cd_loop_uv_offset,
                                           int &count)
{
  for (int i = 0; i < active_faces.size(); i++) {
    BMFace *f = active_faces[i];
    /* TODO(Ish): its a hack, while updating faces, it should have
     * taken care of this stuff */
    if (f->head.htype != BM_FACE) {
      cloth_remeshing_remove_face(active_faces, i--);
      continue;
    }
    BMEdge *e;
    BMIter eiter;
    BM_ITER_ELEM (e, &eiter, f, BM_EDGES_OF_FACE) {
      BMVert *temp_vert;
      temp_vert = cloth_remeshing_try_edge_collapse(clmd, e, 0, cvm, cd_loop_uv_offset);
      if (!temp_vert) {
        temp_vert = cloth_remeshing_try_edge_collapse(clmd, e, 1, cvm, cd_loop_uv_offset);
        if (!temp_vert) {
          continue;
        }
      }

      /* update active_faces */
      cloth_remeshing_update_active_faces(active_faces, clmd->clothObject->bm, temp_vert);

      /* run cloth_remeshing_fix_mesh on newly created faces by
       * cloth_remeshing_try_edge_collapse */
      vector<BMFace *> fix_active;
      BMFace *new_f;
      BMIter new_f_iter;
      BM_ITER_ELEM (new_f, &new_f_iter, temp_vert, BM_FACES_OF_VERT) {
        fix_active.push_back(new_f);
      }
      cloth_remeshing_fix_mesh(clmd->clothObject->bm, cvm, fix_active, cd_loop_uv_offset);

      /* update active_faces */
      cloth_remeshing_update_active_faces(active_faces, fix_active);

      count++;
      return true;
    }
    /* printf("Skipped previous part! active_faces.size: %d i: %d ", (int)active_faces.size(), i);
     */
    cloth_remeshing_remove_face(active_faces, i--);
    /* TODO(Ish): a double i-- is a hacky way to ensure there is no
     * crash when removing a face */
    /* i--; */
    /* printf("new active_faces.size: %d new i: %d\n", (int)active_faces.size(), i); */
  }
  return false;
}

/* Assign memory to cloth->verts since it was deallocated earlier and ensure indexing matches that
 * of the bmesh */
static void cloth_remeshing_reindex_cloth_vertices(Cloth *cloth, BMesh *bm, ClothVertMap &cvm)
{
  ClothVertex *new_verts = (ClothVertex *)MEM_mallocN(sizeof(ClothVertex) * cvm.size(),
                                                      "New ClothVertex Array");
  BMVert *v;
  BMIter viter;
  int i = 0;
  BM_ITER_MESH_INDEX (v, &viter, bm, BM_VERTS_OF_MESH, i) {
    new_verts[i] = cvm[v];
  }

  if (cloth->verts != NULL) {
    MEM_freeN(cloth->verts);
  }
  cloth->verts = new_verts;
  cloth->mvert_num = cvm.size();
}

static void cloth_remeshing_delete_sizing(BMesh *bm, ClothVertMap &cvm)
{
  BMVert *v;
  BMIter viter;

  BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
    if (cvm[v].sizing != NULL) {
      MEM_freeN(cvm[v].sizing);
      cvm[v].sizing = NULL;
    }
  }
}

static void cloth_remeshing_static(ClothModifierData *clmd,
                                   ClothVertMap &cvm,
                                   const int cd_loop_uv_offset)
{
  /**
   * Define sizing staticly
   */
  BMVert *v;
  BMIter viter;
  BM_ITER_MESH (v, &viter, clmd->clothObject->bm, BM_VERTS_OF_MESH) {
    ClothSizing *size = (ClothSizing *)MEM_mallocN(sizeof(ClothSizing), "ClothSizing static");
    unit_m2(size->m);
    mul_m2_fl(size->m, 1.0f / clmd->sim_parms->size_min);
    cvm[v].sizing = size;
  }

  /**
   * Split edges
   */
  while (cloth_remeshing_split_edges(clmd, cvm, cd_loop_uv_offset)) {
    /* empty while */
  }

  /**
   * Collapse edges
   */

#if 1
  vector<BMFace *> active_faces;
  BMFace *f;
  BMIter fiter;
  BM_ITER_MESH (f, &fiter, clmd->clothObject->bm, BM_FACES_OF_MESH) {
    active_faces.push_back(f);
  }
  int count = 0;
  while (cloth_remeshing_collapse_edges(clmd, cvm, active_faces, cd_loop_uv_offset, count)) {
    /* empty while */
  }
#  if REMESHING_DATA_DEBUG
  printf("collapse edges count: %d\n", count);
#  endif

#endif

  /**
   * Delete sizing
   */
  cloth_remeshing_delete_sizing(clmd->clothObject->bm, cvm);

  /**
   * Reindex clmd->clothObject->verts to match clmd->clothObject->bm
   */

#if 1
  cloth_remeshing_reindex_cloth_vertices(clmd->clothObject, clmd->clothObject->bm, cvm);
#endif

#if 1
  active_faces.clear();
#endif

#if EXPORT_MESH
  static int file_no = 0;
  file_no++;
  char file_name[100];
  sprintf(file_name, "/tmp/objs/%03d.obj", file_no);
  cloth_remeshing_export_obj(clmd->clothObject->bm, file_name);
#endif

#if 0
  int v_count = 0;
  BM_ITER_MESH (v, &viter, clmd->clothObject->bm, BM_VERTS_OF_MESH) {
    if (cloth_remeshing_vert_on_seam_or_boundary_test(clmd->clothObject->bm, v, cd_loop_uv_offset)) {
      printf("%f %f %f on seam or boundary %d\n ", v->co[0], v->co[1], v->co[2], ++v_count);
    }
  }
  printf("\n\n\n\n\n");
#endif
}

static void cloth_remeshing_compute_vertex_sizing(ClothModifierData *clmd,
                                                  map<BMVert *, ClothPlane> &planes,
                                                  ClothVertMap &cvm,
                                                  const int cd_loop_uv_offset);

static void cloth_remeshing_find_planes(Depsgraph *depsgraph,
                                        Object *ob,
                                        ClothModifierData *clmd,
                                        map<BMVert *, ClothPlane> &r_planes);

static void cloth_remeshing_dynamic(Depsgraph *depsgraph,
                                    Object *ob,
                                    ClothModifierData *clmd,
                                    ClothVertMap &cvm,
                                    const int cd_loop_uv_offset)
{
  /**
   * Define sizing dynamicly
   */
  map<BMVert *, ClothPlane> planes;
  cloth_remeshing_find_planes(depsgraph, ob, clmd, planes);
  cloth_remeshing_compute_vertex_sizing(clmd, planes, cvm, cd_loop_uv_offset);

  /**
   * Split edges
   */
  while (cloth_remeshing_split_edges(clmd, cvm, cd_loop_uv_offset)) {
    /* empty while */
  }

  /**
   * Collapse edges
   */

#if 1
  vector<BMFace *> active_faces;
  BMFace *f;
  BMIter fiter;
  BM_ITER_MESH (f, &fiter, clmd->clothObject->bm, BM_FACES_OF_MESH) {
    active_faces.push_back(f);
  }
  int count = 0;
  while (cloth_remeshing_collapse_edges(clmd, cvm, active_faces, cd_loop_uv_offset, count)) {
    /* empty while */
  }
#  if REMESHING_DATA_DEBUG
  printf("collapse edges count: %d\n", count);
#  endif

#endif

  /**
   * Delete sizing
   */
  cloth_remeshing_delete_sizing(clmd->clothObject->bm, cvm);

  /**
   * Reindex clmd->clothObject->verts to match clmd->clothObject->bm
   */

#if 1
  cloth_remeshing_reindex_cloth_vertices(clmd->clothObject, clmd->clothObject->bm, cvm);
#endif

#if 1
  active_faces.clear();
#endif

#if EXPORT_MESH
  static int file_no = 0;
  file_no++;
  char file_name[100];
  sprintf(file_name, "/tmp/objs/%03d.obj", file_no);
  cloth_remeshing_export_obj(clmd->clothObject->bm, file_name);
#endif

#if 0
  BMVert *v;
  BMIter viter;
  int preserve_count = 0;
  BM_ITER_MESH (v, &viter, clmd->clothObject->bm, BM_VERTS_OF_MESH) {
    if (cvm[v].flags & CLOTH_VERT_FLAG_PRESERVE) {
      preserve_count++;
    }
  }
  printf("Preserve Count: %d\n", preserve_count);

  BMEdge *e;
  BMIter eiter;
  int label_count = 0;
  BM_ITER_MESH (e, &eiter, clmd->clothObject->bm, BM_EDGES_OF_MESH) {
    if (cloth_remeshing_edge_label_test(e)) {
      label_count++;
    }
  }
  printf("Label Count: %d\n", label_count);
#endif
}

static void cloth_remeshing_face_data(BMesh *bm,
                                      BMFace *f,
                                      const int cd_loop_uv_offset,
                                      float r_mat[2][2])
{
  BMVert *v[3];
  float uv[3][2];
  BM_face_as_array_vert_tri(f, v);
  for (int i = 0; i < 3; i++) {
    cloth_remeshing_uv_of_vert_in_face(bm, f, v[i], cd_loop_uv_offset, uv[i]);
  }
  sub_v2_v2v2(r_mat[0], uv[1], uv[0]);
  sub_v2_v2v2(r_mat[1], uv[2], uv[0]);
}

static float cloth_remeshing_calc_area(BMesh *bm, BMFace *f)
{
  const int cd_loop_uv_offset = CustomData_get_offset(&bm->ldata, CD_MLOOPUV);
  return BM_face_calc_area_uv(f, cd_loop_uv_offset);
}

static float cloth_remeshing_calc_area(BMesh *bm, BMVert *v)
{
  BMFace *f;
  BMIter fiter;
  float area = 0.0f;
  int i = 0;
  BM_ITER_ELEM_INDEX (f, &fiter, v, BM_FACES_OF_VERT, i) {
    area += cloth_remeshing_calc_area(bm, f);
  }
  return area / (float)i;
}

static void cloth_remeshing_eigen_decomposition(float mat[2][2], float r_mat[2][2], float r_vec[2])
{
  float a = mat[0][0], b = mat[0][1], d = mat[1][1];
  float amd = a - d;
  float apd = a + d;
  float b2 = b * b;
  float det = sqrtf(4 * b2 + amd * amd);
  float l1 = 0.5 * (apd + det);
  float l2 = 0.5 * (apd - det);
  float v0, v1, vn;

  r_vec[0] = l1;
  r_vec[1] = l2;

  if (b) {
    v0 = l1 - d;
    v1 = b;
    vn = sqrtf(v0 * v0 + b2);
    r_mat[0][0] = v0 / vn;
    r_mat[0][1] = v1 / vn;

    v0 = l2 - d;
    vn = sqrtf(v0 * v0 + b2);
    r_mat[1][0] = v0 / vn;
    r_mat[1][1] = v1 / vn;
  }
  else {
    r_mat[0][0] = 0;
    r_mat[0][1] = 1;
    r_mat[1][0] = 1;
    r_mat[1][1] = 0;
  }
}

static void diag_m2_v2(float r[2][2], float a[2])
{
  zero_m2(r);
  for (int i = 0; i < 2; i++) {
    r[i][i] = a[i];
  }
}

static void cloth_remeshing_compression_metric(float mat[2][2], float r_mat[2][2])
{
  float l[2];
  float q[2][2];

  cloth_remeshing_eigen_decomposition(mat, q, l);
  float q_t[2][2];
  copy_m2_m2(q_t, q);
  transpose_m2(q_t);

  for (int i = 0; i < 2; i++) {
    l[i] = max(1 - sqrtf(l[i]), 0.0f);
  }

  float temp_mat[2][2];
  float diag_l[2][2];
  diag_m2_v2(diag_l, l);
  mul_m2_m2m2(temp_mat, q_t, diag_l);
  mul_m2_m2m2(r_mat, temp_mat, q);
#if FACE_SIZING_DEBUG
#  if FACE_SIZING_DEBUG_COMP
  printf("comp- l: ");
  print_v2(l);
  printf("comp- q: ");
  print_m2(q);
  printf("comp- diagl: ");
  print_m2(diag_l);
#  endif
#endif
}

static float cloth_remeshing_dihedral_angle(BMVert *v1, BMVert *v2)
{
  BMEdge *edge = BM_edge_exists(v1, v2);
  if (edge == NULL) {
    return 0.0f;
  }

  BMFace *f1, *f2;
  cloth_remeshing_edge_face_pair(edge, &f1, &f2);
  if (f1 == NULL || f2 == NULL) {
    return 0.0f;
  }

  float e[3];
  sub_v3_v3v3(e, v2->co, v1->co);
  normalize_v3(e);

  if (dot_v3v3(e, e) == 0.0f) {
    return 0.0f;
  }

  if (dot_v3v3(f1->no, f1->no) == 0.0f || dot_v3v3(f2->no, f2->no) == 0.0f) {
    return 0.0f;
  }

  float cosine = dot_v3v3(f1->no, f2->no);
  float n_cross[3];
  cross_v3_v3v3(n_cross, f1->no, f2->no);
  float sine = -dot_v3v3(e, n_cross);

  return atan2f(sine, cosine);
}

static void cloth_remeshing_outer_m2_v2v2(float r_mat[2][2], float vec_01[2], float vec_02[2])
{
  for (int i = 0; i < 2; i++) {
    r_mat[i][0] = vec_01[0] * vec_02[i];
    r_mat[i][1] = vec_01[1] * vec_02[i];
  }
}

static void cloth_remeshing_curvature(BMesh *bm,
                                      BMFace *f,
                                      const int cd_loop_uv_offset,
                                      float r_mat[2][2])
{
  zero_m2(r_mat);
  BMVert *v[3];
  BM_face_as_array_vert_tri(f, v);
  for (int i = 0; i < 3; i++) {
    BMVert *v_01 = v[NEXT(i)], *v_02 = v[PREV(i)];
    float uv_01[2], uv_02[2];
    cloth_remeshing_uv_of_vert_in_face(bm, f, v_01, cd_loop_uv_offset, uv_01);
    cloth_remeshing_uv_of_vert_in_face(bm, f, v_02, cd_loop_uv_offset, uv_02);

    float e_mat[2];
    sub_v2_v2v2(e_mat, uv_02, uv_01);
    float t_mat[2];
    float e_mat_norm[2];
    normalize_v2_v2(e_mat_norm, e_mat);
    t_mat[0] = -e_mat_norm[1];
    t_mat[1] = e_mat_norm[0];

    float thetha = cloth_remeshing_dihedral_angle(v_01, v_02);

    float outer[2][2];
    cloth_remeshing_outer_m2_v2v2(outer, t_mat, t_mat);

    mul_m2_fl(outer, 1 / 2.0f * thetha * len_v2(e_mat));
    sub_m2_m2m2(r_mat, r_mat, outer);
  }
  mul_m2_fl(r_mat, 1.0f / cloth_remeshing_calc_area(bm, f));
}

static void transpose_m32_m23(float r_mat[3][2], float mat[2][3])
{
  r_mat[0][0] = mat[0][0];
  r_mat[0][1] = mat[1][0];

  r_mat[1][0] = mat[0][1];
  r_mat[1][1] = mat[1][1];

  r_mat[2][0] = mat[0][2];
  r_mat[2][1] = mat[1][2];
}

static void zero_m23(float r[2][3])
{
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      r[i][j] = 0.0f;
    }
  }
}

static void zero_m32(float r[3][2])
{
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      r[i][j] = 0.0f;
    }
  }
}

static void mul_m23_m22m23(float r[2][3], float a[2][2], float b[2][3])
{
  zero_m23(r);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 2; k++) {
        r[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

static void mul_m32_m32m22(float r[3][2], float a[3][2], float b[2][2])
{
  zero_m32(r);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        r[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

static void cloth_remeshing_derivative(BMesh *bm,
                                       float m_01,
                                       float m_02,
                                       float m_03,
                                       BMFace *f,
                                       const int cd_loop_uv_offset,
                                       float r_vec[2])
{
  float temp_vec[2];
  temp_vec[0] = m_02 - m_01;
  temp_vec[1] = m_03 - m_01;
  float face_dm[2][2];
  float face_dm_inv[2][2];
  cloth_remeshing_face_data(bm, f, cd_loop_uv_offset, face_dm);
  if (invert_m2_m2(face_dm_inv, face_dm, INVERT_EPSILON) == false) {
    zero_m2(face_dm_inv);
  }
  transpose_m2(face_dm_inv);

  mul_v2_m2v2(r_vec, face_dm_inv, temp_vec);
#if FACE_SIZING_DEBUG_OBS
  printf("face_dm: ");
  print_m2(face_dm);
  printf("face_dm_inv: ");
  print_m2(face_dm_inv);
  printf("temp_vec: ");
  print_v2(temp_vec);
  printf("r_vec: ");
  print_v2(r_vec);
#endif
}

static void cloth_remeshing_derivative(BMesh *bm,
                                       float m_01[3],
                                       float m_02[3],
                                       float m_03[3],
                                       BMFace *f,
                                       const int cd_loop_uv_offset,
                                       float r_mat[2][3])
{
  float mat[2][3];
  sub_v3_v3v3(mat[0], m_02, m_01);
  sub_v3_v3v3(mat[1], m_03, m_01);

  float face_dm[2][2];
  float face_dm_inv[2][2];
  cloth_remeshing_face_data(bm, f, cd_loop_uv_offset, face_dm);
  if (invert_m2_m2(face_dm_inv, face_dm, INVERT_EPSILON) == false) {
    zero_m2(face_dm_inv);
  }

  /* mul_m32_m32m22(r_mat, mat_t, face_dm_inv); */
  mul_m23_m22m23(r_mat, face_dm_inv, mat);
}

static void transpose_m23_m32(float r_mat[2][3], float mat[3][2])
{
  r_mat[0][0] = mat[0][0];
  r_mat[0][1] = mat[1][0];
  r_mat[0][2] = mat[2][0];

  r_mat[1][0] = mat[0][1];
  r_mat[1][1] = mat[1][1];
  r_mat[1][2] = mat[2][1];
}

static void mul_m2_m2m2(float r[2][2], float a[2][2], float b[2][2])
{
  zero_m2(r);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        r[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

static void mul_m2_m23m32(float r[2][2], float a[2][3], float b[3][2])
{
  zero_m2(r);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 3; k++) {
        r[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

/* Adapted from editmesh_bvh.c */
struct FaceSearchUserData {
  MVert *mvert;
  MVertTri *mvert_tri;

  float dist_max_sq;
};

static void cloth_remeshing_find_closest_cb(void *userdata,
                                            int index,
                                            const float co[3],
                                            BVHTreeNearest *hit)
{
  struct FaceSearchUserData *data = (FaceSearchUserData *)userdata;
  const float dist_max_sq = data->dist_max_sq;

  float tri_co_01[3];
  float tri_co_02[3];
  float tri_co_03[3];

  copy_v3_v3(tri_co_01, data->mvert[data->mvert_tri[index].tri[0]].co);
  copy_v3_v3(tri_co_02, data->mvert[data->mvert_tri[index].tri[1]].co);
  copy_v3_v3(tri_co_03, data->mvert[data->mvert_tri[index].tri[2]].co);

  float co_close[3];
  closest_on_tri_to_point_v3(co_close, co, tri_co_01, tri_co_02, tri_co_03);
  const float dist_sq = len_squared_v3v3(co, co_close);
  if (dist_sq < hit->dist_sq && dist_sq < dist_max_sq) {
    copy_v3_v3(hit->co, co_close);
    float no[3];
    sub_v3_v3v3(no, co, co_close);
    normalize_v3(no);
    copy_v3_v3(hit->no, no);
    hit->dist_sq = dist_sq;
    hit->index = index;
  }
}

static void cloth_remeshing_find_nearest_planes(BMesh *bm,
                                                CollisionModifierData *collmd,
                                                float dist_max,
                                                map<BMVert *, ClothPlane> &r_planes)
{
  map<BMVert *, ClothPlane> &planes = r_planes;
  BMVert *v;
  BMIter viter;
  BVHTree *bvhtree = collmd->bvhtree;

  float dist_max_sq = dist_max * dist_max;
  BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
    BVHTreeNearest hit;
    FaceSearchUserData data;
    hit.dist_sq = dist_max_sq;
    hit.index = -1;

    data.mvert = collmd->x;
    data.mvert_tri = collmd->tri;
    data.dist_max_sq = dist_max_sq;
    BLI_bvhtree_find_nearest(bvhtree, v->co, &hit, cloth_remeshing_find_closest_cb, &data);
    if (hit.index != -1) {
      /* printf("There was a hit! "); */
      if (planes.count(v)) {
        /* printf("There was a hit already! "); */
        if (len_v3v3(v->co, hit.co) > len_v3v3(v->co, planes.find(v)->second.co)) {
          /* printf("There was a closer hit already!\n"); */
          continue;
        }
      }
      /* printf("This is a closer hit at %4f %4f %4f!\n", hit.co[0], hit.co[1], hit.co[2]); */
      ClothPlane temp_plane;
      copy_v3_v3(temp_plane.co, hit.co);
      copy_v3_v3(temp_plane.no, hit.no);
      planes[v] = temp_plane;
    }
    else {
      if (planes.count(v)) {
        continue;
      }
      ClothPlane temp_plane;
      zero_v3(temp_plane.co);
      zero_v3(temp_plane.no);
      planes[v] = temp_plane;
    }
  }
}

static void cloth_remeshing_find_planes(Depsgraph *depsgraph,
                                        Object *ob,
                                        ClothModifierData *clmd,
                                        map<BMVert *, ClothPlane> &r_planes)
{
  /* Same as BPH_cloth_solve() to get the collision objects' bvh trees */
  Cloth *cloth = clmd->clothObject;
  BMesh *bm = cloth->bm;
  BVHTree *cloth_bvh = cloth->bvhtree;
  Object **collobjs = NULL;
  unsigned int numcollobj = 0;

  float step = 0.0f;
  float dt = clmd->sim_parms->dt * clmd->sim_parms->timescale;

  if ((clmd->sim_parms->flags & CLOTH_SIMSETTINGS_FLAG_COLLOBJ) || cloth_bvh == NULL) {
    return;
  }

  if (clmd->coll_parms->flags & CLOTH_COLLSETTINGS_FLAG_ENABLED) {
    bvhtree_update_from_cloth(clmd, false, false);

    collobjs = BKE_collision_objects_create(
        depsgraph, ob, clmd->coll_parms->group, &numcollobj, eModifierType_Collision);

    if (collobjs) {
      for (int i = 0; i < numcollobj; i++) {
        Object *collob = collobjs[i];
        CollisionModifierData *collmd = (CollisionModifierData *)modifiers_findByType(
            collob, eModifierType_Collision);

        if (!collmd->bvhtree) {
          continue;
        }

        /* Move object to position (step) in time. */
        collision_move_object(collmd, step + dt, step);

        /*Now, actual obstacle metric calculation */
        cloth_remeshing_find_nearest_planes(
            bm, collmd, 10.0f * clmd->coll_parms->epsilon, r_planes);
      }
      BKE_collision_objects_free(collobjs);
    }
  }
}

static void cloth_remeshing_obstacle_metric_calculation(BMesh *bm,
                                                        BMFace *f,
                                                        map<BMVert *, ClothPlane> &planes,
                                                        const int cd_loop_uv_offset,
                                                        float r_mat[2][2])
{
  zero_m2(r_mat);
  BMVert *v[3];
  BM_face_as_array_vert_tri(f, v);

  for (int i = 0; i < 3; i++) {
    ClothPlane plane = planes[v[i]];

    if (len_squared_v3(plane.no) == 0.0f) {
#if FACE_SIZING_DEBUG_OBS
      printf("continuing since len_squared_v3(plane.no) == 0.0f\n");
      printf("plane.co: {%f %f %f}\n", plane.co[0], plane.co[1], plane.co[2]);
      printf("v[i]->co: {%f %f %f}\n", v[i]->co[0], v[i]->co[1], v[i]->co[2]);
#endif
      continue;
    }

    float h[3];
    for (int j = 0; j < 3; j++) {
      float diff[3];
      sub_v3_v3v3(diff, v[j]->co, plane.co);
      h[j] = dot_v3v3(diff, plane.no);
    }

    float dh[2];
    cloth_remeshing_derivative(bm, h[0], h[1], h[2], f, cd_loop_uv_offset, dh);

    float outer[2][2];
    cloth_remeshing_outer_m2_v2v2(outer, dh, dh);
#if FACE_SIZING_DEBUG_OBS
    printf("plane.co: {%f %f %f}\n", plane.co[0], plane.co[1], plane.co[2]);
    printf("plane.no: {%f %f %f}\n", plane.no[0], plane.no[1], plane.no[2]);
    printf("h: {%f %f %f}\n", h[0], h[1], h[2]);
    printf("dh: ");
    print_v2(dh);
    printf("outer-dhdh: ");
    print_m2(outer);
#endif
    mul_m2_fl(outer, h[i] * h[i]);
    add_m2_m2m2(r_mat, r_mat, outer);
  }
  mul_m2_fl(r_mat, 1.0f / 3.0f);
}

static void cloth_remeshing_obstacle_metric(BMesh *bm,
                                            BMFace *f,
                                            map<BMVert *, ClothPlane> &planes,
                                            const int cd_loop_uv_offset,
                                            float r_mat[2][2])
{
  if (planes.empty()) {
    zero_m2(r_mat);
#if FACE_SIZING_DEBUG_OBS
    printf("planes is empty, returning\n");
#endif
    return;
  }
  cloth_remeshing_obstacle_metric_calculation(bm, f, planes, cd_loop_uv_offset, r_mat);
}

static ClothSizing cloth_remeshing_compute_face_sizing(ClothModifierData *clmd,
                                                       BMFace *f,
                                                       map<BMVert *, ClothPlane> &planes,
                                                       ClothVertMap &cvm,
                                                       const int cd_loop_uv_offset)
{
  /* get the cloth verts for the respective verts of the face f */
  Cloth *cloth = clmd->clothObject;
  BMesh *bm = cloth->bm;
  BMVert *v[3];
  BM_face_as_array_vert_tri(f, v);
  ClothVertex *cv[3];
  for (int i = 0; i < 3; i++) {
    cv[i] = &cvm[v[i]];
  }

  float sizing_s[2][2];
  cloth_remeshing_curvature(bm, f, cd_loop_uv_offset, sizing_s);
  float sizing_f[2][3];
  cloth_remeshing_derivative(bm, cv[0]->x, cv[1]->x, cv[2]->x, f, cd_loop_uv_offset, sizing_f);
  float sizing_v[2][3];
  cloth_remeshing_derivative(bm, cv[0]->v, cv[1]->v, cv[2]->v, f, cd_loop_uv_offset, sizing_v);

  float sizing_s_t[2][2];
  copy_m2_m2(sizing_s_t, sizing_s);
  transpose_m2(sizing_s_t);
  float sizing_f_t[3][2];
  transpose_m32_m23(sizing_f_t, sizing_f);
  float sizing_v_t[3][2];
  transpose_m32_m23(sizing_v_t, sizing_v);

  float curv[2][2];
  /* mul_m2_m2m2(curv, sizing_s_t, sizing_s); */
  mul_m2_m2m2(curv, sizing_s, sizing_s_t);

  float comp[2][2];
  float f_x_ft[2][2];
  mul_m2_m23m32(f_x_ft, sizing_f, sizing_f_t);
#if FACE_SIZING_DEBUG
#  if FACE_SIZING_DEBUG_COMP
  printf("sizing_f: ");
  print_m32(sizing_f);
  printf("comp- input mat (f_x_ft): ");
  print_m2(f_x_ft);
#  endif
#endif
  cloth_remeshing_compression_metric(f_x_ft, comp);

  float dvel[2][2];
  mul_m2_m23m32(dvel, sizing_v, sizing_v_t);

  float obs[2][2];
  cloth_remeshing_obstacle_metric(bm, f, planes, cd_loop_uv_offset, obs);

  float m[2][2];
  float curv_temp[2][2];
  float comp_temp[2][2];
  float dvel_temp[2][2];
  float obs_temp[2][2];
  copy_m2_m2(curv_temp, curv);
  mul_m2_fl(curv_temp, 1.0f / (clmd->sim_parms->refine_angle * clmd->sim_parms->refine_angle));
  copy_m2_m2(comp_temp, comp);
  mul_m2_fl(comp_temp,
            1.0f / (clmd->sim_parms->refine_compression * clmd->sim_parms->refine_compression));
  copy_m2_m2(dvel_temp, dvel);
  mul_m2_fl(dvel_temp,
            1.0f / (clmd->sim_parms->refine_velocity * clmd->sim_parms->refine_velocity));
  copy_m2_m2(obs_temp, obs);
  mul_m2_fl(obs_temp, 1.0f / sqr_fl(clmd->sim_parms->refine_obstacle));

  /* Adding curv_temp, comp_temp, dvel_temp, obs */

#if SKIP_COMP_METRIC
  zero_m2(comp_temp);
#endif
  add_m2_m2m2(m, curv_temp, comp_temp);
  add_m2_m2m2(m, m, dvel_temp);
  add_m2_m2m2(m, m, obs_temp);

#if FACE_SIZING_DEBUG
  printf("curv_temp: ");
  print_m2(curv_temp);
  printf("comp_temp: ");
  print_m2(comp_temp);
  printf("dvel_temp: ");
  print_m2(dvel_temp);
  printf("obs_temp: ");
  print_m2(obs_temp);
  printf("m: ");
  print_m2(m);
#endif

#if FACE_SIZING_DEBUG_OBS
#  if FACE_SIZING_DEBUG
#  else
  printf("obs: ");
  print_m2(obs);
  printf("obs_temp: ");
  print_m2(obs_temp);
  printf("\n");
#  endif
#endif

  /* eigen decomposition on m */
  float l[2];    /* Eigen values */
  float q[2][2]; /* Eigen Matrix */
  cloth_remeshing_eigen_decomposition(m, q, l);
  for (int i = 0; i < 2; i++) {
    l[i] = clamp(
        l[i], 1.0f / sqr_fl(clmd->sim_parms->size_max), 1.0f / sqr_fl(clmd->sim_parms->size_min));
  }
  float lmax = max(l[0], l[1]);
  float lmin = lmax * clmd->sim_parms->aspect_min * clmd->sim_parms->aspect_min;
  for (int i = 0; i < 2; i++) {
    if (l[i] < lmin) {
      l[i] = lmin;
    }
  }
  float result[2][2];
  float temp_result[2][2];
  float diag_l[2][2];
  diag_m2_v2(diag_l, l);
  float q_t[2][2];
  copy_m2_m2(q_t, q);
  transpose_m2(q_t);

  mul_m2_m2m2(temp_result, q_t, diag_l);
  mul_m2_m2m2(result, temp_result, q);

#if FACE_SIZING_DEBUG
  printf("l: ");
  print_v2(l);
  printf("q: ");
  print_m2(q);
  printf("result: ");
  print_m2(result);
  printf("\n");
#endif

  return ClothSizing(result);
}

static ClothSizing cloth_remeshing_compute_vertex_sizing(BMesh *bm,
                                                         BMVert *v,
                                                         map<BMFace *, ClothSizing> &face_sizing)
{
  ClothSizing sizing;
  BMFace *f;
  BMIter fiter;
  BM_ITER_ELEM (f, &fiter, v, BM_FACES_OF_VERT) {
    sizing += face_sizing.find(f)->second * (cloth_remeshing_calc_area(bm, f) / 3.0f);
  }
  sizing /= cloth_remeshing_calc_area(bm, v);
  return sizing;
}

static void cloth_remeshing_compute_vertex_sizing(ClothModifierData *clmd,
                                                  map<BMVert *, ClothPlane> &planes,
                                                  ClothVertMap &cvm,
                                                  const int cd_loop_uv_offset)
{
  Cloth *cloth = clmd->clothObject;
  BMesh *bm = cloth->bm;
  map<BMFace *, ClothSizing> face_sizing;

  BMFace *f;
  BMIter fiter;
  BM_ITER_MESH (f, &fiter, bm, BM_FACES_OF_MESH) {
    face_sizing[f] = cloth_remeshing_compute_face_sizing(clmd, f, planes, cvm, cd_loop_uv_offset);
  }

  BMVert *v;
  BMIter viter;
  BM_ITER_MESH (v, &viter, bm, BM_VERTS_OF_MESH) {
    ClothSizing *temp_sizing = (ClothSizing *)MEM_mallocN(sizeof(ClothSizing),
                                                          "Compute Vertex Sizing");
    *temp_sizing = cloth_remeshing_compute_vertex_sizing(bm, v, face_sizing);
    cvm[v].sizing = temp_sizing;
  }
}

Mesh *cloth_remeshing_step(Depsgraph *depsgraph, Object *ob, ClothModifierData *clmd, Mesh *mesh)
{
  ClothVertMap cvm;
  cloth_remeshing_init_bmesh(ob, clmd, mesh, cvm);

  const int cd_loop_uv_offset = CustomData_get_offset(&clmd->clothObject->bm->ldata, CD_MLOOPUV);

  if (true) {
    cloth_remeshing_static(clmd, cvm, cd_loop_uv_offset);
  }
  else {
    cloth_remeshing_dynamic(depsgraph, ob, clmd, cvm, cd_loop_uv_offset);
  }

  /* printf("totvert: %d totedge: %d totface: %d\n", */
  /*        clmd->clothObject->bm->totvert, */
  /*        clmd->clothObject->bm->totedge, */
  /*        clmd->clothObject->bm->totface); */

  Mesh *mesh_result = cloth_remeshing_update_cloth_object_bmesh(ob, clmd);

  /* printf("mesh: totvert: %d totedge: %d totface: %d\n", */
  /*        mesh_result->totvert, */
  /*        mesh_result->totedge, */
  /*        mesh_result->totpoly); */

  cvm.clear();
  return mesh_result;
}

/* TODO(Ish): update the BM_ELEM_TAG on the edges */
/* TODO(Ish): the mesh randomly flips again, might be due to bringing
 * back bm_prev, need to look into this heavily later, noticed again
 * at commit 5035e4cb34cc2d60ec49c5009599af6eab864313 */
