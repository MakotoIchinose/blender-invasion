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
 * The Original Code is Copyright (C) 2017 by Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup draw
 *
 * \brief Mesh API for render engines
 */

#include "MEM_guardedalloc.h"

#include "BLI_bitmap.h"
#include "BLI_buffer.h"
#include "BLI_utildefines.h"
#include "BLI_math_vector.h"
#include "BLI_math_bits.h"
#include "BLI_string.h"
#include "BLI_alloca.h"
#include "BLI_edgehash.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BKE_customdata.h"
#include "BKE_deform.h"
#include "BKE_editmesh.h"
#include "BKE_editmesh_cache.h"
#include "BKE_editmesh_tangent.h"
#include "BKE_mesh.h"
#include "BKE_mesh_tangent.h"
#include "BKE_mesh_runtime.h"
#include "BKE_modifier.h"
#include "BKE_object_deform.h"

#include "atomic_ops.h"

#include "bmesh.h"

#include "GPU_batch.h"
#include "GPU_extensions.h"
#include "GPU_material.h"

#include "DRW_render.h"

#include "ED_mesh.h"
#include "ED_uvedit.h"

#include "draw_cache_inline.h"

#include "draw_cache_impl.h" /* own include */

static void mesh_batch_cache_clear(Mesh *me);

/* Vertex Group Selection and display options */
typedef struct DRW_MeshWeightState {
  int defgroup_active;
  int defgroup_len;

  short flags;
  char alert_mode;

  /* Set of all selected bones for Multipaint. */
  bool *defgroup_sel; /* [defgroup_len] */
  int defgroup_sel_count;
} DRW_MeshWeightState;

typedef struct DRW_MeshCDMask {
  uint32_t uv : 8;
  uint32_t tan : 8;
  uint32_t vcol : 8;
  uint32_t orco : 1;
  uint32_t tan_orco : 1;
} DRW_MeshCDMask;

typedef enum eMRIterType {
  MR_ITER_LOOPTRI = 1 << 0,
  MR_ITER_LOOP = 1 << 1,
  MR_ITER_LEDGE = 1 << 2,
  MR_ITER_LVERT = 1 << 3,
} eMRIterType;

typedef enum eMRDataType {
  MR_DATA_POLY_NOR = 1 << 1,
  MR_DATA_LOOP_NOR = 1 << 2,
  MR_DATA_LOOPTRI = 1 << 3,
} eMRDataType;

typedef enum eMRExtractType {
  MR_EXTRACT_BMESH,
  MR_EXTRACT_MAPPED,
  MR_EXTRACT_MESH,
} eMRExtractType;

/* DRW_MeshWeightState.flags */
enum {
  DRW_MESH_WEIGHT_STATE_MULTIPAINT = (1 << 0),
  DRW_MESH_WEIGHT_STATE_AUTO_NORMALIZE = (1 << 1),
};

/* ---------------------------------------------------------------------- */
/** \name Mesh/BMesh Interface (indirect, partially cached access to complex data).
 * \{ */

typedef struct MeshRenderData {
  eMRExtractType extract_type;

  int poly_len, edge_len, vert_len, loop_len;
  int edge_loose_len;
  int vert_loose_len;
  int loop_loose_len;
  int tri_len;
  int mat_len;

  bool use_mapped;
  bool use_hide;
  bool use_subsurf_fdots;

  const ToolSettings *toolsettings;
  /* HACK not supposed to be there but it's needed. */
  struct MeshBatchCache *cache;
  /** Edit Mesh */
  BMEditMesh *edit_bmesh;
  BMesh *bm;
  EditMeshData *edit_data;
  int *v_origindex, *e_origindex, *p_origindex;
  int crease_ofs;
  int bweight_ofs;
  int freestyle_edge_ofs;
  int freestyle_face_ofs;
  /** Mesh */
  Mesh *me;
  const MVert *mvert;
  const MEdge *medge;
  const MLoop *mloop;
  const MPoly *mpoly;
  BMVert *eve_act;
  BMEdge *eed_act;
  BMFace *efa_act;
  BMFace *efa_act_uv;
  /* Data created on-demand (usually not for bmesh-based data). */
  MLoopTri *mlooptri;
  float (*loop_normals)[3];
  float (*poly_normals)[3];
  int *lverts, *ledges;
} MeshRenderData;

/**
 * These functions look like they would be slow but they will typically return true on the first
 * iteration. Only false when all attached elements are hidden.
 */
static bool bm_vert_has_visible_edge(const BMVert *v)
{
  const BMEdge *e_iter, *e_first;

  e_iter = e_first = v->e;
  do {
    if (!BM_elem_flag_test(e_iter, BM_ELEM_HIDDEN)) {
      return true;
    }
  } while ((e_iter = BM_DISK_EDGE_NEXT(e_iter, v)) != e_first);
  return false;
}

static bool bm_edge_has_visible_face(const BMEdge *e)
{
  const BMLoop *l_iter, *l_first;
  l_iter = l_first = e->l;
  do {
    if (!BM_elem_flag_test(l_iter->f, BM_ELEM_HIDDEN)) {
      return true;
    }
  } while ((l_iter = l_iter->radial_next) != l_first);
  return false;
}

BLI_INLINE bool bm_vert_is_loose_and_visible(const BMVert *v)
{
  return (!BM_elem_flag_test(v, BM_ELEM_HIDDEN) && (v->e == NULL || !bm_vert_has_visible_edge(v)));
}

BLI_INLINE bool bm_edge_is_loose_and_visible(const BMEdge *e)
{
  return (!BM_elem_flag_test(e, BM_ELEM_HIDDEN) && (e->l == NULL || !bm_edge_has_visible_face(e)));
}

/* Return true is all layers in _b_ are inside _a_. */
BLI_INLINE bool mesh_cd_layers_type_overlap(DRW_MeshCDMask a, DRW_MeshCDMask b)
{
  return (*((uint32_t *)&a) & *((uint32_t *)&b)) == *((uint32_t *)&b);
}

BLI_INLINE bool mesh_cd_layers_type_equal(DRW_MeshCDMask a, DRW_MeshCDMask b)
{
  return *((uint32_t *)&a) == *((uint32_t *)&b);
}

BLI_INLINE int mesh_render_mat_len_get(Mesh *me)
{
  return MAX2(1, me->totcol);
}

BLI_INLINE void mesh_cd_layers_type_merge(DRW_MeshCDMask *a, DRW_MeshCDMask b)
{
  atomic_fetch_and_or_uint32((uint32_t *)a, *(uint32_t *)&b);
}

BLI_INLINE void mesh_cd_layers_type_clear(DRW_MeshCDMask *a)
{
  *((uint32_t *)a) = 0;
}

static void mesh_cd_calc_active_uv_layer(const Mesh *me, DRW_MeshCDMask *cd_used)
{
  const CustomData *cd_ldata = (me->edit_mesh) ? &me->edit_mesh->bm->ldata : &me->ldata;

  int layer = CustomData_get_active_layer(cd_ldata, CD_MLOOPUV);
  if (layer != -1) {
    cd_used->uv |= (1 << layer);
  }
}

static void mesh_cd_calc_active_mask_uv_layer(const Mesh *me, DRW_MeshCDMask *cd_used)
{
  const CustomData *cd_ldata = (me->edit_mesh) ? &me->edit_mesh->bm->ldata : &me->ldata;

  int layer = CustomData_get_stencil_layer(cd_ldata, CD_MLOOPUV);
  if (layer != -1) {
    cd_used->uv |= (1 << layer);
  }
}

static void mesh_cd_calc_active_vcol_layer(const Mesh *me, DRW_MeshCDMask *cd_used)
{
  const CustomData *cd_ldata = (me->edit_mesh) ? &me->edit_mesh->bm->ldata : &me->ldata;

  int layer = CustomData_get_active_layer(cd_ldata, CD_MLOOPCOL);
  if (layer != -1) {
    cd_used->vcol |= (1 << layer);
  }
}

static DRW_MeshCDMask mesh_cd_calc_used_gpu_layers(const Mesh *me,
                                                   struct GPUMaterial **gpumat_array,
                                                   int gpumat_array_len)
{
  const CustomData *cd_ldata = (me->edit_mesh) ? &me->edit_mesh->bm->ldata : &me->ldata;

  /* See: DM_vertex_attributes_from_gpu for similar logic */
  DRW_MeshCDMask cd_used;
  mesh_cd_layers_type_clear(&cd_used);

  for (int i = 0; i < gpumat_array_len; i++) {
    GPUMaterial *gpumat = gpumat_array[i];
    if (gpumat) {
      GPUVertAttrLayers gpu_attrs;
      GPU_material_vertex_attrs(gpumat, &gpu_attrs);
      for (int j = 0; j < gpu_attrs.totlayer; j++) {
        const char *name = gpu_attrs.layer[j].name;
        int type = gpu_attrs.layer[j].type;
        int layer = -1;

        if (type == CD_AUTO_FROM_NAME) {
          /* We need to deduct what exact layer is used.
           *
           * We do it based on the specified name.
           */
          if (name[0] != '\0') {
            layer = CustomData_get_named_layer(cd_ldata, CD_MLOOPUV, name);
            type = CD_MTFACE;

            if (layer == -1) {
              layer = CustomData_get_named_layer(cd_ldata, CD_MLOOPCOL, name);
              type = CD_MCOL;
            }
#if 0 /* Tangents are always from UV's - this will never happen. */
            if (layer == -1) {
              layer = CustomData_get_named_layer(cd_ldata, CD_TANGENT, name);
              type = CD_TANGENT;
            }
#endif
            if (layer == -1) {
              continue;
            }
          }
          else {
            /* Fall back to the UV layer, which matches old behavior. */
            type = CD_MTFACE;
          }
        }

        switch (type) {
          case CD_MTFACE: {
            if (layer == -1) {
              layer = (name[0] != '\0') ? CustomData_get_named_layer(cd_ldata, CD_MLOOPUV, name) :
                                          CustomData_get_render_layer(cd_ldata, CD_MLOOPUV);
            }
            if (layer != -1) {
              cd_used.uv |= (1 << layer);
            }
            break;
          }
          case CD_TANGENT: {
            if (layer == -1) {
              layer = (name[0] != '\0') ? CustomData_get_named_layer(cd_ldata, CD_MLOOPUV, name) :
                                          CustomData_get_render_layer(cd_ldata, CD_MLOOPUV);

              /* Only fallback to orco (below) when we have no UV layers, see: T56545 */
              if (layer == -1 && name[0] != '\0') {
                layer = CustomData_get_render_layer(cd_ldata, CD_MLOOPUV);
              }
            }
            if (layer != -1) {
              cd_used.tan |= (1 << layer);
            }
            else {
              /* no UV layers at all => requesting orco */
              cd_used.tan_orco = 1;
              cd_used.orco = 1;
            }
            break;
          }
          case CD_MCOL: {
            if (layer == -1) {
              layer = (name[0] != '\0') ? CustomData_get_named_layer(cd_ldata, CD_MLOOPCOL, name) :
                                          CustomData_get_render_layer(cd_ldata, CD_MLOOPCOL);
            }
            if (layer != -1) {
              cd_used.vcol |= (1 << layer);
            }
            break;
          }
          case CD_ORCO: {
            cd_used.orco = 1;
            break;
          }
        }
      }
    }
  }
  return cd_used;
}

static void mesh_cd_extract_auto_layers_names_and_srgb(Mesh *me,
                                                       DRW_MeshCDMask cd_used,
                                                       char **r_auto_layers_names,
                                                       int **r_auto_layers_srgb,
                                                       int *r_auto_layers_len)
{
  const CustomData *cd_ldata = (me->edit_mesh) ? &me->edit_mesh->bm->ldata : &me->ldata;

  int uv_len_used = count_bits_i(cd_used.uv);
  int vcol_len_used = count_bits_i(cd_used.vcol);
  int uv_len = CustomData_number_of_layers(cd_ldata, CD_MLOOPUV);
  int vcol_len = CustomData_number_of_layers(cd_ldata, CD_MLOOPCOL);

  uint auto_names_len = 32 * (uv_len_used + vcol_len_used);
  uint auto_ofs = 0;
  /* Allocate max, resize later. */
  char *auto_names = MEM_callocN(sizeof(char) * auto_names_len, __func__);
  int *auto_is_srgb = MEM_callocN(sizeof(int) * (uv_len_used + vcol_len_used), __func__);

  for (int i = 0; i < uv_len; i++) {
    if ((cd_used.uv & (1 << i)) != 0) {
      const char *name = CustomData_get_layer_name(cd_ldata, CD_MLOOPUV, i);
      uint hash = BLI_ghashutil_strhash_p(name);
      /* +1 to include '\0' terminator. */
      auto_ofs += 1 + BLI_snprintf_rlen(
                          auto_names + auto_ofs, auto_names_len - auto_ofs, "ba%u", hash);
    }
  }

  uint auto_is_srgb_ofs = uv_len_used;
  for (int i = 0; i < vcol_len; i++) {
    if ((cd_used.vcol & (1 << i)) != 0) {
      const char *name = CustomData_get_layer_name(cd_ldata, CD_MLOOPCOL, i);
      /* We only do vcols that are not overridden by a uv layer with same name. */
      if (CustomData_get_named_layer_index(cd_ldata, CD_MLOOPUV, name) == -1) {
        uint hash = BLI_ghashutil_strhash_p(name);
        /* +1 to include '\0' terminator. */
        auto_ofs += 1 + BLI_snprintf_rlen(
                            auto_names + auto_ofs, auto_names_len - auto_ofs, "ba%u", hash);
        auto_is_srgb[auto_is_srgb_ofs] = true;
        auto_is_srgb_ofs++;
      }
    }
  }

  auto_names = MEM_reallocN(auto_names, sizeof(char) * auto_ofs);
  auto_is_srgb = MEM_reallocN(auto_is_srgb, sizeof(int) * auto_is_srgb_ofs);

  /* WATCH: May have been referenced somewhere before freeing. */
  MEM_SAFE_FREE(*r_auto_layers_names);
  MEM_SAFE_FREE(*r_auto_layers_srgb);

  *r_auto_layers_names = auto_names;
  *r_auto_layers_srgb = auto_is_srgb;
  *r_auto_layers_len = auto_is_srgb_ofs;
}

static MeshRenderData *mesh_render_data_create(Mesh *me,
                                               const bool do_final,
                                               const eMRIterType iter_type,
                                               const eMRDataType data_flag,
                                               const DRW_MeshCDMask *UNUSED(cd_used),
                                               const ToolSettings *ts)
{
  MeshRenderData *mr = MEM_callocN(sizeof(*mr), __func__);
  mr->toolsettings = ts;
  mr->mat_len = mesh_render_mat_len_get(me);

  const bool is_auto_smooth = (me->flag & ME_AUTOSMOOTH) != 0;
  const float split_angle = is_auto_smooth ? me->smoothresh : (float)M_PI;

  if (me->edit_mesh) {
    BLI_assert(me->edit_mesh->mesh_eval_cage && me->edit_mesh->mesh_eval_final);
    mr->bm = me->edit_mesh->bm;
    mr->edit_bmesh = me->edit_mesh;
    mr->edit_data = me->runtime.edit_data;
    mr->me = (do_final) ? me->edit_mesh->mesh_eval_final : me->edit_mesh->mesh_eval_cage;
    mr->use_mapped = mr->me && !mr->me->runtime.is_original;

    int bm_ensure_types = BM_VERT | BM_EDGE | BM_LOOP | BM_FACE;

    BM_mesh_elem_index_ensure(mr->bm, bm_ensure_types);
    BM_mesh_elem_table_ensure(mr->bm, bm_ensure_types & ~BM_LOOP);

    mr->efa_act_uv = EDBM_uv_active_face_get(mr->edit_bmesh, false, false);
    mr->efa_act = BM_mesh_active_face_get(mr->bm, false, true);
    mr->eed_act = BM_mesh_active_edge_get(mr->bm);
    mr->eve_act = BM_mesh_active_vert_get(mr->bm);

    mr->crease_ofs = CustomData_get_offset(&mr->bm->edata, CD_CREASE);
    mr->bweight_ofs = CustomData_get_offset(&mr->bm->edata, CD_BWEIGHT);
#ifdef WITH_FREESTYLE
    mr->freestyle_edge_ofs = CustomData_get_offset(&mr->bm->edata, CD_FREESTYLE_EDGE);
    mr->freestyle_face_ofs = CustomData_get_offset(&mr->bm->pdata, CD_FREESTYLE_FACE);
#endif

    if (mr->use_mapped) {
      mr->v_origindex = CustomData_get_layer(&mr->me->vdata, CD_ORIGINDEX);
      mr->e_origindex = CustomData_get_layer(&mr->me->edata, CD_ORIGINDEX);
      mr->p_origindex = CustomData_get_layer(&mr->me->pdata, CD_ORIGINDEX);

      mr->use_mapped = (mr->v_origindex || mr->e_origindex || mr->p_origindex);
    }

    mr->extract_type = mr->use_mapped ? MR_EXTRACT_MAPPED : MR_EXTRACT_BMESH;

    /* Seems like the mesh_eval_final do not have the right origin indices.
     * Force not mapped in this case. */
    if (do_final && me->edit_mesh->mesh_eval_final != me->edit_mesh->mesh_eval_cage) {
      mr->use_mapped = false;
      // mr->edit_bmesh = NULL;
      mr->extract_type = MR_EXTRACT_MESH;
    }
  }
  else {
    mr->me = me;
    mr->edit_bmesh = NULL;
    mr->extract_type = MR_EXTRACT_MESH;
  }

  if (mr->extract_type != MR_EXTRACT_BMESH) {
    /* Mesh */
    mr->vert_len = mr->me->totvert;
    mr->edge_len = mr->me->totedge;
    mr->loop_len = mr->me->totloop;
    mr->poly_len = mr->me->totpoly;
    mr->tri_len = poly_to_tri_count(mr->poly_len, mr->loop_len);

    mr->mvert = CustomData_get_layer(&mr->me->vdata, CD_MVERT);
    mr->medge = CustomData_get_layer(&mr->me->edata, CD_MEDGE);
    mr->mloop = CustomData_get_layer(&mr->me->ldata, CD_MLOOP);
    mr->mpoly = CustomData_get_layer(&mr->me->pdata, CD_MPOLY);

    mr->v_origindex = CustomData_get_layer(&mr->me->vdata, CD_ORIGINDEX);
    mr->e_origindex = CustomData_get_layer(&mr->me->edata, CD_ORIGINDEX);
    mr->p_origindex = CustomData_get_layer(&mr->me->pdata, CD_ORIGINDEX);

    if (data_flag & (MR_DATA_POLY_NOR | MR_DATA_LOOP_NOR)) {
      mr->poly_normals = MEM_mallocN(sizeof(*mr->poly_normals) * mr->poly_len, __func__);
      BKE_mesh_calc_normals_poly((MVert *)mr->mvert,
                                 NULL,
                                 mr->vert_len,
                                 mr->mloop,
                                 mr->mpoly,
                                 mr->loop_len,
                                 mr->poly_len,
                                 mr->poly_normals,
                                 true);
    }
    if (data_flag & MR_DATA_LOOP_NOR) {
      mr->loop_normals = MEM_mallocN(sizeof(*mr->loop_normals) * mr->loop_len, __func__);
      short(*clnors)[2] = CustomData_get_layer(&mr->me->ldata, CD_CUSTOMLOOPNORMAL);
      BKE_mesh_normals_loop_split(mr->me->mvert,
                                  mr->vert_len,
                                  mr->me->medge,
                                  mr->edge_len,
                                  mr->me->mloop,
                                  mr->loop_normals,
                                  mr->loop_len,
                                  mr->me->mpoly,
                                  mr->poly_normals,
                                  mr->poly_len,
                                  is_auto_smooth,
                                  split_angle,
                                  NULL,
                                  clnors,
                                  NULL);
    }
    if ((iter_type & MR_ITER_LOOPTRI) || (data_flag & MR_DATA_LOOPTRI)) {
      mr->mlooptri = MEM_mallocN(sizeof(*mr->mlooptri) * mr->tri_len, "MR_DATATYPE_LOOPTRI");
      BKE_mesh_recalc_looptri(mr->me->mloop,
                              mr->me->mpoly,
                              mr->me->mvert,
                              mr->me->totloop,
                              mr->me->totpoly,
                              mr->mlooptri);
    }
    if (iter_type & (MR_ITER_LEDGE | MR_ITER_LVERT)) {
      mr->vert_loose_len = 0;
      mr->edge_loose_len = 0;

      BLI_bitmap *lvert_map = BLI_BITMAP_NEW(mr->vert_len, "lvert map");

      mr->ledges = MEM_mallocN(mr->edge_len * sizeof(int), __func__);
      const MEdge *medge = mr->medge;
      for (int e = 0; e < mr->edge_len; e++, medge++) {
        if (medge->flag & ME_LOOSEEDGE) {
          mr->ledges[mr->edge_loose_len++] = e;
        }
        /* Tag verts as not loose. */
        BLI_BITMAP_ENABLE(lvert_map, medge->v1);
        BLI_BITMAP_ENABLE(lvert_map, medge->v2);
      }
      if (mr->edge_loose_len < mr->edge_len) {
        mr->ledges = MEM_reallocN(mr->ledges, mr->edge_loose_len * sizeof(*mr->ledges));
      }

      mr->lverts = MEM_mallocN(mr->vert_len * sizeof(*mr->lverts), __func__);
      for (int v = 0; v < mr->vert_len; v++) {
        if (BLI_BITMAP_TEST(lvert_map, v)) {
          mr->lverts[mr->vert_loose_len++] = v;
        }
      }
      if (mr->vert_loose_len < mr->vert_len) {
        mr->lverts = MEM_reallocN(mr->lverts, mr->vert_loose_len * sizeof(*mr->lverts));
      }

      MEM_freeN(lvert_map);

      mr->loop_loose_len = mr->vert_loose_len + mr->edge_loose_len * 2;
    }
  }
  else {
    /* BMesh */
    BMesh *bm = mr->bm;

    mr->vert_len = bm->totvert;
    mr->edge_len = bm->totedge;
    mr->loop_len = bm->totloop;
    mr->poly_len = bm->totface;
    mr->tri_len = poly_to_tri_count(mr->poly_len, mr->loop_len);

    if (data_flag & MR_DATA_POLY_NOR) {
      /* Use bmface->no instead. */
    }
    if (data_flag & MR_DATA_LOOP_NOR) {
      mr->loop_normals = MEM_mallocN(sizeof(*mr->loop_normals) * mr->loop_len, __func__);
      int clnors_offset = CustomData_get_offset(&mr->bm->ldata, CD_CUSTOMLOOPNORMAL);
      BM_loops_calc_normal_vcos(mr->bm,
                                NULL,
                                NULL,
                                NULL,
                                is_auto_smooth,
                                split_angle,
                                mr->loop_normals,
                                NULL,
                                NULL,
                                clnors_offset,
                                false);
    }
    if ((iter_type & MR_ITER_LOOPTRI) || (data_flag & MR_DATA_LOOPTRI)) {
      /* Edit mode ensures this is valid, no need to calculate. */
      BLI_assert((bm->totloop == 0) || (mr->edit_bmesh->looptris != NULL));
    }
    if (iter_type & (MR_ITER_LEDGE | MR_ITER_LVERT)) {
      int elem_id;
      BMIter iter;
      BMVert *eve;
      BMEdge *ede;
      mr->vert_loose_len = 0;
      mr->edge_loose_len = 0;

      mr->lverts = MEM_mallocN(mr->vert_len * sizeof(*mr->lverts), __func__);
      BM_ITER_MESH_INDEX (eve, &iter, bm, BM_VERTS_OF_MESH, elem_id) {
        if (eve->e == NULL) {
          mr->lverts[mr->vert_loose_len++] = elem_id;
        }
      }
      if (mr->vert_loose_len < mr->vert_len) {
        mr->lverts = MEM_reallocN(mr->lverts, mr->vert_loose_len * sizeof(*mr->lverts));
      }

      mr->ledges = MEM_mallocN(mr->edge_len * sizeof(*mr->ledges), __func__);
      BM_ITER_MESH_INDEX (ede, &iter, bm, BM_EDGES_OF_MESH, elem_id) {
        if (ede->l == NULL) {
          mr->ledges[mr->edge_loose_len++] = elem_id;
        }
      }
      if (mr->edge_loose_len < mr->edge_len) {
        mr->ledges = MEM_reallocN(mr->ledges, mr->edge_loose_len * sizeof(*mr->ledges));
      }

      mr->loop_loose_len = mr->vert_loose_len + mr->edge_loose_len * 2;
    }
  }
  return mr;
}

/* Warning replace mesh pointer. */
#define MBC_GET_FINAL_MESH(me) \
  /* Hack to show the final result. */ \
  const bool _use_em_final = ((me)->edit_mesh && (me)->edit_mesh->mesh_eval_final && \
                              ((me)->edit_mesh->mesh_eval_final->runtime.is_original == false)); \
  Mesh _me_fake; \
  if (_use_em_final) { \
    _me_fake = *(me)->edit_mesh->mesh_eval_final; \
    _me_fake.mat = (me)->mat; \
    _me_fake.totcol = (me)->totcol; \
    (me) = &_me_fake; \
  } \
  ((void)0)

static void mesh_render_data_free(MeshRenderData *mr)
{
  MEM_SAFE_FREE(mr->mlooptri);
  MEM_SAFE_FREE(mr->poly_normals);
  MEM_SAFE_FREE(mr->loop_normals);

  MEM_SAFE_FREE(mr->lverts);
  MEM_SAFE_FREE(mr->ledges);

  MEM_freeN(mr);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Vertex Group Selection
 * \{ */

/** Reset the selection structure, deallocating heap memory as appropriate. */
static void drw_mesh_weight_state_clear(struct DRW_MeshWeightState *wstate)
{
  MEM_SAFE_FREE(wstate->defgroup_sel);

  memset(wstate, 0, sizeof(*wstate));

  wstate->defgroup_active = -1;
}

/** Copy selection data from one structure to another, including heap memory. */
static void drw_mesh_weight_state_copy(struct DRW_MeshWeightState *wstate_dst,
                                       const struct DRW_MeshWeightState *wstate_src)
{
  MEM_SAFE_FREE(wstate_dst->defgroup_sel);

  memcpy(wstate_dst, wstate_src, sizeof(*wstate_dst));

  if (wstate_src->defgroup_sel) {
    wstate_dst->defgroup_sel = MEM_dupallocN(wstate_src->defgroup_sel);
  }
}

/** Compare two selection structures. */
static bool drw_mesh_weight_state_compare(const struct DRW_MeshWeightState *a,
                                          const struct DRW_MeshWeightState *b)
{
  return a->defgroup_active == b->defgroup_active && a->defgroup_len == b->defgroup_len &&
         a->flags == b->flags && a->alert_mode == b->alert_mode &&
         a->defgroup_sel_count == b->defgroup_sel_count &&
         ((!a->defgroup_sel && !b->defgroup_sel) ||
          (a->defgroup_sel && b->defgroup_sel &&
           memcmp(a->defgroup_sel, b->defgroup_sel, a->defgroup_len * sizeof(bool)) == 0));
}

static void drw_mesh_weight_state_extract(Object *ob,
                                          Mesh *me,
                                          const ToolSettings *ts,
                                          bool paint_mode,
                                          struct DRW_MeshWeightState *wstate)
{
  /* Extract complete vertex weight group selection state and mode flags. */
  memset(wstate, 0, sizeof(*wstate));

  wstate->defgroup_active = ob->actdef - 1;
  wstate->defgroup_len = BLI_listbase_count(&ob->defbase);

  wstate->alert_mode = ts->weightuser;

  if (paint_mode && ts->multipaint) {
    /* Multipaint needs to know all selected bones, not just the active group.
     * This is actually a relatively expensive operation, but caching would be difficult. */
    wstate->defgroup_sel = BKE_object_defgroup_selected_get(
        ob, wstate->defgroup_len, &wstate->defgroup_sel_count);

    if (wstate->defgroup_sel_count > 1) {
      wstate->flags |= DRW_MESH_WEIGHT_STATE_MULTIPAINT |
                       (ts->auto_normalize ? DRW_MESH_WEIGHT_STATE_AUTO_NORMALIZE : 0);

      if (me->editflag & ME_EDIT_MIRROR_X) {
        BKE_object_defgroup_mirror_selection(ob,
                                             wstate->defgroup_len,
                                             wstate->defgroup_sel,
                                             wstate->defgroup_sel,
                                             &wstate->defgroup_sel_count);
      }
    }
    /* With only one selected bone Multipaint reverts to regular mode. */
    else {
      wstate->defgroup_sel_count = 0;
      MEM_SAFE_FREE(wstate->defgroup_sel);
    }
  }
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Mesh GPUBatch Cache
 * \{ */

typedef enum DRWBatchFlag {
  MBC_SURFACE = (1 << 0),
  MBC_SURFACE_WEIGHTS = (1 << 1),
  MBC_EDIT_TRIANGLES = (1 << 2),
  MBC_EDIT_VERTICES = (1 << 3),
  MBC_EDIT_EDGES = (1 << 4),
  MBC_EDIT_VNOR = (1 << 5),
  MBC_EDIT_LNOR = (1 << 6),
  MBC_EDIT_FACEDOTS = (1 << 7),
  MBC_EDIT_MESH_ANALYSIS = (1 << 8),
  MBC_EDITUV_FACES_STRECH_AREA = (1 << 9),
  MBC_EDITUV_FACES_STRECH_ANGLE = (1 << 10),
  MBC_EDITUV_FACES = (1 << 11),
  MBC_EDITUV_EDGES = (1 << 12),
  MBC_EDITUV_VERTS = (1 << 13),
  MBC_EDITUV_FACEDOTS = (1 << 14),
  MBC_EDIT_SELECTION_VERTS = (1 << 15),
  MBC_EDIT_SELECTION_EDGES = (1 << 16),
  MBC_EDIT_SELECTION_FACES = (1 << 17),
  MBC_EDIT_SELECTION_FACEDOTS = (1 << 18),
  MBC_ALL_VERTS = (1 << 19),
  MBC_ALL_EDGES = (1 << 20),
  MBC_LOOSE_EDGES = (1 << 21),
  MBC_EDGE_DETECTION = (1 << 22),
  MBC_WIRE_EDGES = (1 << 23),
  MBC_WIRE_LOOPS = (1 << 24),
  MBC_WIRE_LOOPS_UVS = (1 << 25),
  MBC_SURF_PER_MAT = (1 << 26),
} DRWBatchFlag;

#define MBC_EDITUV \
  (MBC_EDITUV_FACES_STRECH_AREA | MBC_EDITUV_FACES_STRECH_ANGLE | MBC_EDITUV_FACES | \
   MBC_EDITUV_EDGES | MBC_EDITUV_VERTS | MBC_EDITUV_FACEDOTS | MBC_WIRE_LOOPS_UVS)

#define FOREACH_MESH_BUFFER_CACHE(batch_cache, mbc) \
  for (MeshBufferCache *mbc = &batch_cache->final; \
       mbc == &batch_cache->final || mbc == &batch_cache->cage; \
       mbc = (mbc == &batch_cache->final) ? &batch_cache->cage : NULL)

typedef struct MeshBufferCache {
  /* Every VBO below contains at least enough
   * data for every loops in the mesh (except facedots).
   * For some VBOs, it extends to (in this exact order) :
   * loops + loose_edges*2 + loose_verts */
  struct {
    GPUVertBuf *pos_nor;  /* extend */
    GPUVertBuf *lnor;     /* extend */
    GPUVertBuf *edge_fac; /* extend */
    GPUVertBuf *weights;  /* extend */
    GPUVertBuf *uv_tan;
    GPUVertBuf *vcol;
    GPUVertBuf *orco;
    /* Only for edit mode. */
    GPUVertBuf *data; /* extend */
    GPUVertBuf *data_edituv;
    GPUVertBuf *stretch_area;
    GPUVertBuf *stretch_angle;
    GPUVertBuf *mesh_analysis;
    GPUVertBuf *facedots_pos;
    GPUVertBuf *facedots_nor;
    GPUVertBuf *facedots_uv;
    GPUVertBuf *facedots_data; /* inside facedots_nor for now. */
    GPUVertBuf *facedots_data_edituv;
    /* Selection */
    GPUVertBuf *vert_idx; /* extend */
    GPUVertBuf *edge_idx; /* extend */
    GPUVertBuf *face_idx;
    GPUVertBuf *facedots_idx;
  } vbo;
  /* Index Buffers:
   * Only need to be updated when topology changes. */
  struct {
    /* Indices to vloops. */
    GPUIndexBuf *tris;  /* Ordered per material. */
    GPUIndexBuf *lines; /* Loose edges last. */
    GPUIndexBuf *points;
    GPUIndexBuf *facedots;
    /* 3D overlays. */
    GPUIndexBuf *lines_paint_mask; /* no loose edges. */
    GPUIndexBuf *lines_adjacency;
    /* Uv overlays. (visibility can differ from 3D view) */
    GPUIndexBuf *edituv_tris;
    GPUIndexBuf *edituv_lines;
    GPUIndexBuf *edituv_points;
    GPUIndexBuf *edituv_facedots;
  } ibo;
} MeshBufferCache;

typedef struct MeshBatchCache {
  MeshBufferCache final, cage;

  struct {
    /* Surfaces / Render */
    GPUBatch *surface;
    GPUBatch *surface_weights;
    /* Edit mode */
    GPUBatch *edit_triangles;
    GPUBatch *edit_vertices;
    GPUBatch *edit_edges;
    GPUBatch *edit_vnor;
    GPUBatch *edit_lnor;
    GPUBatch *edit_facedots;
    GPUBatch *edit_mesh_analysis;
    /* Edit UVs */
    GPUBatch *edituv_faces_strech_area;
    GPUBatch *edituv_faces_strech_angle;
    GPUBatch *edituv_faces;
    GPUBatch *edituv_edges;
    GPUBatch *edituv_verts;
    GPUBatch *edituv_facedots;
    /* Edit selection */
    GPUBatch *edit_selection_verts;
    GPUBatch *edit_selection_edges;
    GPUBatch *edit_selection_faces;
    GPUBatch *edit_selection_facedots;
    /* Common display / Other */
    GPUBatch *all_verts;
    GPUBatch *all_edges;
    GPUBatch *loose_edges;
    GPUBatch *edge_detection;
    GPUBatch *wire_edges;     /* Individual edges with face normals. */
    GPUBatch *wire_loops;     /* Loops around faces. no edges between selected faces */
    GPUBatch *wire_loops_uvs; /* Same as wire_loops but only has uvs. */
  } batch;

  GPUBatch **surface_per_mat;

  /* HACK: Temp copy to init the subrange IBOs. */
  int *tri_mat_start, *tri_mat_end;
  int edge_loose_start, edge_loose_end;

  /* arrays of bool uniform names (and value) that will be use to
   * set srgb conversion for auto attributes.*/
  char *auto_layer_names;
  int *auto_layer_is_srgb;
  int auto_layer_len;

  DRWBatchFlag batch_requested;
  DRWBatchFlag batch_ready;

  /* settings to determine if cache is invalid */
  int edge_len;
  int tri_len;
  int poly_len;
  int vert_len;
  int mat_len;
  bool is_dirty; /* Instantly invalidates cache, skipping mesh check */
  bool is_editmode;
  bool is_uvsyncsel;

  struct DRW_MeshWeightState weight_state;

  DRW_MeshCDMask cd_used, cd_needed, cd_used_over_time;

  int lastmatch;

  /* Valid only if edge_detection is up to date. */
  bool is_manifold;

  bool no_loose_wire;
} MeshBatchCache;

BLI_INLINE void mesh_batch_cache_add_request(MeshBatchCache *cache, DRWBatchFlag new_flag)
{
  atomic_fetch_and_or_uint32((uint32_t *)(&cache->batch_requested), *(uint32_t *)&new_flag);
}

/* GPUBatch cache management. */

static bool mesh_batch_cache_valid(Mesh *me)
{
  MeshBatchCache *cache = me->runtime.batch_cache;

  if (cache == NULL) {
    return false;
  }

  if (cache->is_editmode != (me->edit_mesh != NULL)) {
    return false;
  }

  if (cache->is_dirty) {
    return false;
  }

  if (cache->mat_len != mesh_render_mat_len_get(me)) {
    return false;
  }

  return true;
}

static void mesh_batch_cache_init(Mesh *me)
{
  MeshBatchCache *cache = me->runtime.batch_cache;

  if (!cache) {
    cache = me->runtime.batch_cache = MEM_callocN(sizeof(*cache), __func__);
  }
  else {
    memset(cache, 0, sizeof(*cache));
  }

  cache->is_editmode = me->edit_mesh != NULL;

  if (cache->is_editmode == false) {
    // cache->edge_len = mesh_render_edges_len_get(me);
    // cache->tri_len = mesh_render_looptri_len_get(me);
    // cache->poly_len = mesh_render_polys_len_get(me);
    // cache->vert_len = mesh_render_verts_len_get(me);
  }

  cache->mat_len = mesh_render_mat_len_get(me);
  cache->surface_per_mat = MEM_callocN(sizeof(*cache->surface_per_mat) * cache->mat_len, __func__);

  cache->is_dirty = false;
  cache->batch_ready = 0;
  cache->batch_requested = 0;

  drw_mesh_weight_state_clear(&cache->weight_state);
}

void DRW_mesh_batch_cache_validate(Mesh *me)
{
  if (!mesh_batch_cache_valid(me)) {
    mesh_batch_cache_clear(me);
    mesh_batch_cache_init(me);
  }
}

static MeshBatchCache *mesh_batch_cache_get(Mesh *me)
{
  return me->runtime.batch_cache;
}

static void mesh_batch_cache_check_vertex_group(MeshBatchCache *cache,
                                                const struct DRW_MeshWeightState *wstate)
{
  if (!drw_mesh_weight_state_compare(&cache->weight_state, wstate)) {
    FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
    {
      GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.weights);
    }
    GPU_BATCH_CLEAR_SAFE(cache->batch.surface_weights);

    cache->batch_ready &= ~MBC_SURFACE_WEIGHTS;

    drw_mesh_weight_state_clear(&cache->weight_state);
  }
}

static void mesh_batch_cache_discard_shaded_tri(MeshBatchCache *cache)
{
  FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
  {
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.pos_nor);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.uv_tan);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.vcol);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.orco);
  }

  if (cache->surface_per_mat) {
    for (int i = 0; i < cache->mat_len; i++) {
      GPU_BATCH_DISCARD_SAFE(cache->surface_per_mat[i]);
    }
  }
  MEM_SAFE_FREE(cache->surface_per_mat);

  cache->batch_ready &= ~MBC_SURF_PER_MAT;

  MEM_SAFE_FREE(cache->auto_layer_names);
  MEM_SAFE_FREE(cache->auto_layer_is_srgb);

  mesh_cd_layers_type_clear(&cache->cd_used);

  cache->mat_len = 0;
}

static void mesh_batch_cache_discard_uvedit(MeshBatchCache *cache)
{
  FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
  {
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.stretch_angle);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.stretch_area);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.uv_tan);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.data_edituv);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.facedots_uv);
    GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.facedots_data_edituv);
    GPU_INDEXBUF_DISCARD_SAFE(mbufcache->ibo.edituv_tris);
    GPU_INDEXBUF_DISCARD_SAFE(mbufcache->ibo.edituv_lines);
    GPU_INDEXBUF_DISCARD_SAFE(mbufcache->ibo.edituv_points);
    GPU_INDEXBUF_DISCARD_SAFE(mbufcache->ibo.edituv_facedots);
  }
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces_strech_area);
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces_strech_angle);
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces);
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_edges);
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_verts);
  GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_facedots);
  GPU_BATCH_DISCARD_SAFE(cache->batch.wire_loops_uvs);

  cache->batch_ready &= ~MBC_EDITUV;
}

void DRW_mesh_batch_cache_dirty_tag(Mesh *me, int mode)
{
  MeshBatchCache *cache = me->runtime.batch_cache;
  if (cache == NULL) {
    return;
  }
  switch (mode) {
    case BKE_MESH_BATCH_DIRTY_SELECT:
      FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
      {
        GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.data);
        GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.facedots_nor);
      }
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_triangles);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_vertices);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_edges);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_facedots);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_selection_verts);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_selection_edges);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_selection_faces);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_selection_facedots);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edit_mesh_analysis);
      cache->batch_ready &= ~(MBC_EDIT_TRIANGLES | MBC_EDIT_VERTICES | MBC_EDIT_EDGES |
                              MBC_EDIT_FACEDOTS | MBC_EDIT_SELECTION_FACEDOTS |
                              MBC_EDIT_SELECTION_FACES | MBC_EDIT_SELECTION_EDGES |
                              MBC_EDIT_SELECTION_VERTS | MBC_EDIT_MESH_ANALYSIS);
      /* Because visible UVs depends on edit mode selection, discard everything. */
      mesh_batch_cache_discard_uvedit(cache);
      break;
    case BKE_MESH_BATCH_DIRTY_SELECT_PAINT:
      /* Paint mode selection flag is packed inside the nor attrib.
       * Note that it can be slow if auto smooth is enabled. (see T63946) */
      FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
      {
        GPU_INDEXBUF_DISCARD_SAFE(mbufcache->ibo.lines_paint_mask);
        GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.pos_nor);
      }
      GPU_BATCH_DISCARD_SAFE(cache->batch.surface);
      GPU_BATCH_DISCARD_SAFE(cache->batch.wire_loops);
      GPU_BATCH_DISCARD_SAFE(cache->batch.wire_edges);
      if (cache->surface_per_mat) {
        for (int i = 0; i < cache->mat_len; i++) {
          GPU_BATCH_DISCARD_SAFE(cache->surface_per_mat[i]);
        }
      }
      cache->batch_ready &= ~(MBC_SURFACE | MBC_WIRE_EDGES | MBC_WIRE_LOOPS | MBC_SURF_PER_MAT);
      break;
    case BKE_MESH_BATCH_DIRTY_ALL:
      cache->is_dirty = true;
      break;
    case BKE_MESH_BATCH_DIRTY_SHADING:
      mesh_batch_cache_discard_shaded_tri(cache);
      mesh_batch_cache_discard_uvedit(cache);
      break;
    case BKE_MESH_BATCH_DIRTY_UVEDIT_ALL:
      mesh_batch_cache_discard_uvedit(cache);
      break;
    case BKE_MESH_BATCH_DIRTY_UVEDIT_SELECT:
      FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
      {
        GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.data_edituv);
        GPU_VERTBUF_DISCARD_SAFE(mbufcache->vbo.facedots_data_edituv);
      }
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces_strech_area);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces_strech_angle);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_faces);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_edges);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_verts);
      GPU_BATCH_DISCARD_SAFE(cache->batch.edituv_facedots);
      cache->batch_ready &= ~MBC_EDITUV;
      break;
    default:
      BLI_assert(0);
  }
}

static void mesh_batch_cache_clear(Mesh *me)
{
  MeshBatchCache *cache = me->runtime.batch_cache;
  if (!cache) {
    return;
  }
  FOREACH_MESH_BUFFER_CACHE(cache, mbufcache)
  {
    GPUVertBuf **vbos = (GPUVertBuf **)&mbufcache->vbo;
    GPUIndexBuf **ibos = (GPUIndexBuf **)&mbufcache->ibo;
    for (int i = 0; i < sizeof(mbufcache->vbo) / sizeof(void *); ++i) {
      GPU_VERTBUF_DISCARD_SAFE(vbos[i]);
    }
    for (int i = 0; i < sizeof(mbufcache->ibo) / sizeof(void *); ++i) {
      GPU_INDEXBUF_DISCARD_SAFE(ibos[i]);
    }
  }
  for (int i = 0; i < sizeof(cache->batch) / sizeof(void *); ++i) {
    GPUBatch **batch = (GPUBatch **)&cache->batch;
    GPU_BATCH_DISCARD_SAFE(batch[i]);
  }

  mesh_batch_cache_discard_shaded_tri(cache);

  mesh_batch_cache_discard_uvedit(cache);

  MEM_SAFE_FREE(cache->tri_mat_end);
  MEM_SAFE_FREE(cache->tri_mat_start);

  cache->batch_ready = 0;

  drw_mesh_weight_state_clear(&cache->weight_state);
}

void DRW_mesh_batch_cache_free(Mesh *me)
{
  mesh_batch_cache_clear(me);
  MEM_SAFE_FREE(me->runtime.batch_cache);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Mesh Elements Extract Iter
 * \{ */

typedef struct MeshExtractIterData {
  /* NULL if in edit mode and not mapped. */
  const MVert *mvert;
  const MEdge *medge;
  const MPoly *mpoly;
  const MLoop *mloop;
  const MLoopTri *mlooptri;
  /* NULL if not in edit mode. */
  BMVert *eve;
  BMEdge *eed, *eed_prev;
  BMFace *efa;
  BMLoop *eloop;
  BMLoop **elooptri;
  /* Index of current element. Is not BMesh index if iteration is on mapped. */
  int vert_idx, edge_idx, face_idx, loop_idx, tri_idx;
  /* Copy of the loop index if the iter element.
   * NEVER use for vertex index, only for edge and looptri.
   * Use loop_idx for vertices */
  int v1, v2, v3;

  bool use_hide;
  MeshRenderData *mr;
} MeshExtractIterData;

typedef void *(MeshExtractInitFn)(const MeshRenderData *mr, void *buffer);
typedef void(MeshExtractIterFn)(const MeshExtractIterData *iter, void *buffer, void *user_data);
typedef void(MeshExtractFinishFn)(const MeshRenderData *mr, void *buffer, void *user_data);

typedef struct MeshExtract {
  MeshExtractInitFn *init;
  MeshExtractIterFn *iter, *iter_edit;
  MeshExtractFinishFn *finish;
  eMRIterType iter_flag;
  eMRDataType data_flag;
} MeshExtract;

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Triangles Indices
 * \{ */

typedef struct MeshExtract_Tri_Data {
  GPUIndexBufBuilder elb;
  uint32_t mat_tri_idx[0];
} MeshExtract_Tri_Data;

static void *mesh_tri_init(const MeshRenderData *mr, void *UNUSED(ibo))
{
  size_t mat_tri_idx_size = sizeof(uint32_t) * mr->mat_len;
  MeshExtract_Tri_Data *data = MEM_callocN(sizeof(*data) + mat_tri_idx_size, __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_TRIS, mr->tri_len, mr->loop_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(data->elb.data, 0xFF, sizeof(uint) * mr->tri_len * 3);
  /* Count how many triangle for each material. */
  if (mr->mat_len > 1) {
    if (mr->extract_type == MR_EXTRACT_BMESH) {
      BMIter iter;
      BMFace *efa;
      BM_ITER_MESH (efa, &iter, mr->bm, BM_FACES_OF_MESH) {
        BLI_assert(efa->len > 2);
        data->mat_tri_idx[efa->mat_nr] += efa->len - 2;
      }
    }
    else {
      const MPoly *mpoly = mr->mpoly;
      for (int p = 0; p < mr->poly_len; p++, mpoly++) {
        BLI_assert(mpoly->totloop > 2);
        data->mat_tri_idx[mpoly->mat_nr] += mpoly->totloop - 2;
      }
    }
  }
  /* Accumulate tri len per mat to have correct offsets. */
  int ofs = data->mat_tri_idx[0];
  data->mat_tri_idx[0] = 0;
  for (int i = 1; i < mr->mat_len; i++) {
    int tmp = data->mat_tri_idx[i];
    data->mat_tri_idx[i] = ofs;
    ofs += tmp;
  }
  /* HACK pass per mat triangle start and end outside the extract loop. */
  MEM_SAFE_FREE(mr->cache->tri_mat_end);
  MEM_SAFE_FREE(mr->cache->tri_mat_start);
  mr->cache->tri_mat_end = MEM_mallocN(mat_tri_idx_size, __func__);
  mr->cache->tri_mat_start = MEM_mallocN(mat_tri_idx_size, __func__);
  memcpy(mr->cache->tri_mat_start, data->mat_tri_idx, mat_tri_idx_size);

  return data;
}
static void mesh_tri_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *user_data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    MeshExtract_Tri_Data *data = user_data;
    uint32_t tri_idx = atomic_fetch_and_add_uint32(&data->mat_tri_idx[iter->mpoly->mat_nr], 1u);
    GPU_indexbuf_set_tri_verts(&data->elb, tri_idx, iter->v1, iter->v2, iter->v3);
  }
}
static void mesh_tri_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *user_data)
{
  if (!iter->efa) {
    mesh_tri_iter(iter, NULL, user_data);
  }
  else if (!BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN)) {
    MeshExtract_Tri_Data *data = user_data;
    uint32_t tri_idx = atomic_fetch_and_add_uint32(&data->mat_tri_idx[iter->efa->mat_nr], 1u);
    GPU_indexbuf_set_tri_verts(&data->elb, tri_idx, iter->v1, iter->v2, iter->v3);
  }
}
static void mesh_tri_finish(const MeshRenderData *mr, void *ibo, void *user_data)
{
  MeshExtract_Tri_Data *data = user_data;
  GPU_indexbuf_build_in_place(&data->elb, ibo);
  /* HACK pass per mat triangle start and end outside the extract loop. */
  memcpy(mr->cache->tri_mat_end, data->mat_tri_idx, sizeof(uint32_t) * mr->mat_len);

  MEM_freeN(user_data);
}

MeshExtract extract_tris = {
    mesh_tri_init, mesh_tri_iter, mesh_tri_iter_edit, mesh_tri_finish, MR_ITER_LOOPTRI, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edges Indices
 * \{ */

static void *mesh_edge_init(const MeshRenderData *mr, void *UNUSED(buf))
{
  GPUIndexBufBuilder *elb = MEM_mallocN(sizeof(*elb), __func__);
  /* Allocate a buffer that is twice as large to sort loose edges in the second half. */
  GPU_indexbuf_init(elb, GPU_PRIM_LINES, mr->edge_len * 2, mr->loop_len + mr->loop_loose_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(elb->data, 0xFF, sizeof(uint) * mr->edge_len * 2 * 2);
  return elb;
}
static void mesh_edge_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (!(iter->use_hide && (iter->medge->flag & ME_HIDE))) {
    /* XXX TODO Make it threadsafe */
    GPU_indexbuf_set_line_verts(elb, iter->edge_idx, iter->v1, iter->v2);
  }
}
static void mesh_edge_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (iter->eed && !BM_elem_flag_test(iter->eed, BM_ELEM_HIDDEN)) {
    /* XXX TODO Make it threadsafe */
    GPU_indexbuf_set_line_verts(elb, iter->edge_idx, iter->v1, iter->v2);
  }
}
static void mesh_edge_finish(const MeshRenderData *mr, void *ibo, void *elb)
{
  /* Sort loose edges at the end of the index buffer. */
  /* TODO We must take into consideration loose edges from hidden faces in edit mode. */
  if (mr->edge_loose_len > 0) {
    uint(*indices)[2] = (uint(*)[2])((GPUIndexBufBuilder *)elb)->data;
    for (int e = 0; e < mr->edge_loose_len; e++) {
      uint src = mr->ledges[e];
      uint dst = mr->edge_len + e;
      indices[dst][0] = indices[src][0];
      indices[dst][1] = indices[src][1];
      /* Set as restart index, we don't want to draw twice. */
      indices[src][0] = indices[src][1] = 0xFFFFFFFF;
    }
    /* TODO Search last visible loose edge for that. */
    ((GPUIndexBufBuilder *)elb)->index_len = (mr->edge_len + mr->edge_loose_len) * 2;
    /* TODO Remove holes. */

    mr->cache->edge_loose_start = mr->edge_len;
    mr->cache->edge_loose_end = mr->edge_len + mr->edge_loose_len;
  }
  else {
    mr->cache->edge_loose_start = 0;
    mr->cache->edge_loose_end = 0;
  }

  GPU_indexbuf_build_in_place(elb, ibo);
  MEM_freeN(elb);
}

MeshExtract extract_lines = {mesh_edge_init,
                             mesh_edge_iter,
                             mesh_edge_iter_edit,
                             mesh_edge_finish,
                             MR_ITER_LOOP | MR_ITER_LEDGE,
                             0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Point Indices
 * \{ */

static void *mesh_vert_init(const MeshRenderData *mr, void *UNUSED(buf))
{
  GPUIndexBufBuilder *elb = MEM_mallocN(sizeof(*elb), __func__);
  GPU_indexbuf_init(elb, GPU_PRIM_POINTS, mr->vert_len, mr->loop_len + mr->loop_loose_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(elb->data, 0xFF, sizeof(uint) * mr->vert_len);
  return elb;
}
static void mesh_vert_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (!(iter->use_hide && (iter->mvert->flag & ME_HIDE))) {
    GPU_indexbuf_set_point_vert(elb, iter->vert_idx, iter->loop_idx);
  }
}
static void mesh_vert_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (iter->eve && !BM_elem_flag_test(iter->eve, BM_ELEM_HIDDEN)) {
    GPU_indexbuf_set_point_vert(elb, iter->vert_idx, iter->loop_idx);
  }
}
static void mesh_vert_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *elb)
{
  GPU_indexbuf_build_in_place(elb, ibo);
  MEM_freeN(elb);
}

MeshExtract extract_points = {mesh_vert_init,
                              mesh_vert_iter,
                              mesh_vert_iter_edit,
                              mesh_vert_finish,
                              MR_ITER_LOOP | MR_ITER_LEDGE | MR_ITER_LVERT,
                              0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Facedots Indices
 * \{ */

static void *mesh_facedot_init(const MeshRenderData *mr, void *UNUSED(buf))
{
  GPUIndexBufBuilder *elb = MEM_mallocN(sizeof(*elb), __func__);
  GPU_indexbuf_init(elb, GPU_PRIM_POINTS, mr->poly_len, mr->poly_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(elb->data, 0xFF, sizeof(uint) * mr->poly_len);
  return elb;
}
static void mesh_facedot_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE)) &&
      (!iter->mr->use_subsurf_fdots || (iter->mvert->flag & ME_VERT_FACEDOT))) {
    GPU_indexbuf_set_point_vert(elb, iter->face_idx, iter->face_idx);
  }
}
static void mesh_facedot_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *elb)
{
  if (iter->efa && !BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN) &&
      (!iter->mr->use_subsurf_fdots || (iter->mvert->flag & ME_VERT_FACEDOT))) {
    GPU_indexbuf_set_point_vert(elb, iter->face_idx, iter->face_idx);
  }
}
static void mesh_facedot_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *elb)
{
  GPU_indexbuf_build_in_place(elb, ibo);
  MEM_freeN(elb);
}

MeshExtract extract_facedots = {mesh_facedot_init,
                                mesh_facedot_iter,
                                mesh_facedot_iter_edit,
                                mesh_facedot_finish,
                                MR_ITER_LOOP,
                                0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Paint Mask Line Indices
 * \{ */

typedef struct MeshExtract_LinePaintMask_Data {
  GPUIndexBufBuilder elb;
  /** One bit per edge set if face is selected. */
  BLI_bitmap select_map[0];
} MeshExtract_LinePaintMask_Data;

static void *mesh_lines_paint_mask_init(const MeshRenderData *mr, void *UNUSED(buf))
{
  size_t bitmap_size = BLI_BITMAP_SIZE(mr->edge_len);
  MeshExtract_LinePaintMask_Data *data = MEM_callocN(sizeof(*data) + bitmap_size, __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_LINES, mr->edge_len, mr->loop_len);
  return data;
}
static void mesh_lines_paint_mask_iter(const MeshExtractIterData *iter,
                                       void *UNUSED(buf),
                                       void *data)
{
  MeshExtract_LinePaintMask_Data *extract_data = (MeshExtract_LinePaintMask_Data *)data;
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    if (iter->mpoly->flag & ME_FACE_SEL) {
      if (BLI_BITMAP_TEST_AND_SET_ATOMIC(extract_data->select_map, iter->edge_idx)) {
        /* Hide edge as it has more than 2 selected loop. */
        uint *elems = &extract_data->elb.data[iter->edge_idx * 2];
        elems[0] = elems[1] = 0xFFFFFFFFu;
      }
      else {
        /* First selected loop. Set edge visible, overwritting any unsel loop. */
        GPU_indexbuf_set_line_verts(&extract_data->elb, iter->edge_idx, iter->v1, iter->v2);
      }
    }
    else {
      /* Set theses unselected loop only if this edge has no other selected loop. */
      if (!BLI_BITMAP_TEST(extract_data->select_map, iter->edge_idx)) {
        GPU_indexbuf_set_line_verts(&extract_data->elb, iter->edge_idx, iter->v1, iter->v2);
      }
    }
  }
}
static void mesh_lines_paint_mask_iter_edit(const MeshExtractIterData *UNUSED(iter),
                                            void *UNUSED(buf),
                                            void *UNUSED(elb))
{
  /* painting does not use the edit_bmesh */
  BLI_assert(0);
}
static void mesh_lines_paint_mask_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *data)
{
  MeshExtract_LinePaintMask_Data *extract_data = (MeshExtract_LinePaintMask_Data *)data;
  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

MeshExtract extract_lines_paint_mask = {mesh_lines_paint_mask_init,
                                        mesh_lines_paint_mask_iter,
                                        mesh_lines_paint_mask_iter_edit,
                                        mesh_lines_paint_mask_finish,
                                        MR_ITER_LOOP,
                                        0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Line Adjacency Indices
 * \{ */

#define NO_EDGE INT_MAX

typedef struct MeshExtract_LineAdjacency_Data {
  GPUIndexBufBuilder elb;
  EdgeHash *eh;
  bool is_manifold;
  /* Array to convert vert index to any loop index of this vert. */
  uint vert_to_loop[0];
} MeshExtract_LineAdjacency_Data;

static void *mesh_lines_adjacency_init(const MeshRenderData *mr, void *UNUSED(buf))
{
  /* Similar to poly_to_tri_count().
   * There is always loop + tri - 1 edges inside a polygon.
   * Cummulate for all polys and you get : */
  uint tess_edge_len = mr->loop_len + mr->tri_len - mr->poly_len;

  size_t vert_to_loop_size = sizeof(uint) * mr->vert_len;

  MeshExtract_LineAdjacency_Data *data = MEM_callocN(sizeof(*data) + vert_to_loop_size, __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_LINES_ADJ, tess_edge_len, mr->loop_len);
  data->eh = BLI_edgehash_new_ex(__func__, tess_edge_len);
  data->is_manifold = true;
  return data;
}
static void mesh_lines_adjacency_triangle(
    uint v1, uint v2, uint v3, uint l1, uint l2, uint l3, MeshExtract_LineAdjacency_Data *data)
{
  GPUIndexBufBuilder *elb = &data->elb;
  /* Iter around the triangle's edges. */
  for (int e = 0; e < 3; e++) {
    uint tmp = v1;
    v1 = v2, v2 = v3, v3 = tmp;
    tmp = l1;
    l1 = l2, l2 = l3, l3 = tmp;

    bool inv_indices = (v2 > v3);
    void **pval;
    bool value_is_init = BLI_edgehash_ensure_p(data->eh, v2, v3, &pval);
    int v_data = POINTER_AS_INT(*pval);
    if (!value_is_init || v_data == NO_EDGE) {
      /* Save the winding order inside the sign bit. Because the
       * edgehash sort the keys and we need to compare winding later. */
      int value = (int)l1 + 1; /* 0 cannot be signed so add one. */
      *pval = POINTER_FROM_INT((inv_indices) ? -value : value);
      /* Store loop indices for remaining non-manifold edges. */
      data->vert_to_loop[v2] = l2;
      data->vert_to_loop[v3] = l3;
    }
    else {
      /* HACK Tag as not used. Prevent overhead of BLI_edgehash_remove. */
      *pval = POINTER_FROM_INT(NO_EDGE);
      bool inv_opposite = (v_data < 0);
      uint l_opposite = (uint)abs(v_data) - 1;
      /* TODO Make this part threadsafe. */
      if (inv_opposite == inv_indices) {
        /* Don't share edge if triangles have non matching winding. */
        GPU_indexbuf_add_line_adj_verts(elb, l1, l2, l3, l1);
        GPU_indexbuf_add_line_adj_verts(elb, l_opposite, l2, l3, l_opposite);
        data->is_manifold = false;
      }
      else {
        GPU_indexbuf_add_line_adj_verts(elb, l1, l2, l3, l_opposite);
      }
    }
  }
}
static void mesh_lines_adjacency_iter(const MeshExtractIterData *iter,
                                      void *UNUSED(buf),
                                      void *data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    mesh_lines_adjacency_triangle(iter->mr->mloop[iter->mlooptri->tri[0]].v,
                                  iter->mr->mloop[iter->mlooptri->tri[1]].v,
                                  iter->mr->mloop[iter->mlooptri->tri[2]].v,
                                  iter->v1,
                                  iter->v2,
                                  iter->v3,
                                  data);
  }
}
static void mesh_lines_adjacency_iter_edit(const MeshExtractIterData *iter,
                                           void *UNUSED(buf),
                                           void *data)
{
  if (BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN)) {
    mesh_lines_adjacency_triangle(BM_elem_index_get(iter->elooptri[0]->v),
                                  BM_elem_index_get(iter->elooptri[1]->v),
                                  BM_elem_index_get(iter->elooptri[2]->v),
                                  iter->v1,
                                  iter->v2,
                                  iter->v3,
                                  data);
  }
}
static void mesh_lines_adjacency_finish(const MeshRenderData *mr, void *ibo, void *data)
{
  MeshExtract_LineAdjacency_Data *extract_data = (MeshExtract_LineAdjacency_Data *)data;
  /* Create edges for remaining non manifold edges. */
  EdgeHashIterator *ehi = BLI_edgehashIterator_new(extract_data->eh);
  for (; !BLI_edgehashIterator_isDone(ehi); BLI_edgehashIterator_step(ehi)) {
    uint v2, v3, l1, l2, l3;
    int v_data = POINTER_AS_INT(BLI_edgehashIterator_getValue(ehi));
    if (v_data != NO_EDGE) {
      BLI_edgehashIterator_getKey(ehi, &v2, &v3);
      l1 = (uint)abs(v_data) - 1;
      if (v_data < 0) { /* inv_opposite  */
        SWAP(uint, v2, v3);
      }
      l2 = extract_data->vert_to_loop[v2];
      l3 = extract_data->vert_to_loop[v3];
      GPU_indexbuf_add_line_adj_verts(&extract_data->elb, l1, l2, l3, l1);
      extract_data->is_manifold = false;
    }
  }
  BLI_edgehashIterator_free(ehi);
  BLI_edgehash_free(extract_data->eh, NULL);

  mr->cache->is_manifold = extract_data->is_manifold;

  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

#undef NO_EDGE

MeshExtract extract_lines_adjacency = {mesh_lines_adjacency_init,
                                       mesh_lines_adjacency_iter,
                                       mesh_lines_adjacency_iter_edit,
                                       mesh_lines_adjacency_finish,
                                       MR_ITER_LOOPTRI,
                                       0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV Triangles Indices
 * \{ */

typedef struct MeshExtract_EditUvTri_Data {
  GPUIndexBufBuilder elb;
  bool sync_selection;
} MeshExtract_EditUvTri_Data;

static void *mesh_edituv_tri_init(const MeshRenderData *mr, void *UNUSED(ibo))
{
  MeshExtract_EditUvTri_Data *data = MEM_callocN(sizeof(*data), __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_TRIS, mr->tri_len, mr->loop_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(data->elb.data, 0xFF, sizeof(uint) * mr->tri_len * 3);

  data->sync_selection = (mr->toolsettings->uv_flag & UV_SYNC_SELECTION) != 0;
  return data;
}
static void mesh_edituv_tri_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    MeshExtract_EditUvTri_Data *extract_data = (MeshExtract_EditUvTri_Data *)data;
    GPU_indexbuf_set_tri_verts(&extract_data->elb, iter->tri_idx, iter->v1, iter->v2, iter->v3);
  }
}
static void mesh_edituv_tri_iter_edit(const MeshExtractIterData *iter,
                                      void *UNUSED(buf),
                                      void *data)
{
  MeshExtract_EditUvTri_Data *extract_data = (MeshExtract_EditUvTri_Data *)data;
  if (!iter->efa) {
    mesh_edituv_tri_iter(iter, NULL, data);
  }
  else if (!BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN) &&
           (extract_data->sync_selection || BM_elem_flag_test(iter->efa, BM_ELEM_SELECT))) {
    GPU_indexbuf_set_tri_verts(&extract_data->elb, iter->tri_idx, iter->v1, iter->v2, iter->v3);
  }
}
static void mesh_edituv_tri_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *data)
{
  MeshExtract_EditUvTri_Data *extract_data = (MeshExtract_EditUvTri_Data *)data;
  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

MeshExtract extract_edituv_tris = {mesh_edituv_tri_init,
                                   mesh_edituv_tri_iter,
                                   mesh_edituv_tri_iter_edit,
                                   mesh_edituv_tri_finish,
                                   MR_ITER_LOOPTRI,
                                   0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV Line Indices around faces
 * \{ */

typedef struct MeshExtract_EditUvLine_Data {
  GPUIndexBufBuilder elb;
  bool sync_selection;
} MeshExtract_EditUvLine_Data;

static void *mesh_edituv_line_init(const MeshRenderData *mr, void *UNUSED(ibo))
{
  MeshExtract_EditUvLine_Data *data = MEM_callocN(sizeof(*data), __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_LINES, mr->loop_len, mr->loop_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(data->elb.data, 0xFF, sizeof(uint) * mr->loop_len * 2);

  data->sync_selection = (mr->toolsettings->uv_flag & UV_SYNC_SELECTION) != 0;
  return data;
}
static void mesh_edituv_line_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    MeshExtract_EditUvLine_Data *extract_data = (MeshExtract_EditUvLine_Data *)data;
    GPU_indexbuf_set_line_verts(&extract_data->elb, iter->loop_idx, iter->v1, iter->v2);
  }
}
static void mesh_edituv_line_iter_edit(const MeshExtractIterData *iter,
                                       void *UNUSED(buf),
                                       void *data)
{
  MeshExtract_EditUvLine_Data *extract_data = (MeshExtract_EditUvLine_Data *)data;
  if (iter->eed && !BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN) &&
      (extract_data->sync_selection || BM_elem_flag_test(iter->efa, BM_ELEM_SELECT))) {
    GPU_indexbuf_set_line_verts(&extract_data->elb, iter->loop_idx, iter->v1, iter->v2);
  }
}
static void mesh_edituv_line_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *data)
{
  MeshExtract_EditUvLine_Data *extract_data = (MeshExtract_EditUvLine_Data *)data;
  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

MeshExtract extract_edituv_lines = {mesh_edituv_line_init,
                                    mesh_edituv_line_iter,
                                    mesh_edituv_line_iter_edit,
                                    mesh_edituv_line_finish,
                                    MR_ITER_LOOP,
                                    0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV Points Indices
 * \{ */

typedef struct MeshExtract_EditUvPoint_Data {
  GPUIndexBufBuilder elb;
  bool sync_selection;
} MeshExtract_EditUvPoint_Data;

static void *mesh_edituv_point_init(const MeshRenderData *mr, void *UNUSED(ibo))
{
  MeshExtract_EditUvPoint_Data *data = MEM_callocN(sizeof(*data), __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_POINTS, mr->loop_len, mr->loop_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(data->elb.data, 0xFF, sizeof(uint) * mr->loop_len);

  data->sync_selection = (mr->toolsettings->uv_flag & UV_SYNC_SELECTION) != 0;
  return data;
}
static void mesh_edituv_point_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE))) {
    MeshExtract_EditUvPoint_Data *extract_data = (MeshExtract_EditUvPoint_Data *)data;
    GPU_indexbuf_set_point_vert(&extract_data->elb, iter->loop_idx, iter->loop_idx);
  }
}
static void mesh_edituv_point_iter_edit(const MeshExtractIterData *iter,
                                        void *UNUSED(buf),
                                        void *data)
{
  MeshExtract_EditUvPoint_Data *extract_data = (MeshExtract_EditUvPoint_Data *)data;
  if (iter->eve && !BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN) &&
      (extract_data->sync_selection || BM_elem_flag_test(iter->efa, BM_ELEM_SELECT))) {
    GPU_indexbuf_set_point_vert(&extract_data->elb, iter->loop_idx, iter->loop_idx);
  }
}
static void mesh_edituv_point_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *data)
{
  MeshExtract_EditUvPoint_Data *extract_data = (MeshExtract_EditUvPoint_Data *)data;
  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

MeshExtract extract_edituv_points = {mesh_edituv_point_init,
                                     mesh_edituv_point_iter,
                                     mesh_edituv_point_iter_edit,
                                     mesh_edituv_point_finish,
                                     MR_ITER_LOOP,
                                     0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV Facedots Indices
 * \{ */

typedef struct MeshExtract_EditUvFdot_Data {
  GPUIndexBufBuilder elb;
  bool sync_selection;
} MeshExtract_EditUvFdot_Data;

static void *mesh_edituv_fdot_init(const MeshRenderData *mr, void *UNUSED(ibo))
{
  MeshExtract_EditUvFdot_Data *data = MEM_callocN(sizeof(*data), __func__);
  GPU_indexbuf_init(&data->elb, GPU_PRIM_POINTS, mr->poly_len, mr->poly_len);
  /* Set all indices to primitive restart. We do this to bypass hidden elements. */
  /* NOTE: This is not the best method as it leave holes in the index buffer.
   * TODO(fclem): Better approach would be to remove the holes by moving all the indices block to
   * have a continuous buffer. The issue is that we need to randomly set indices in the buffer
   * for multithreading. */
  memset(data->elb.data, 0xFF, sizeof(uint) * mr->poly_len);

  data->sync_selection = (mr->toolsettings->uv_flag & UV_SYNC_SELECTION) != 0;
  return data;
}
static void mesh_edituv_fdot_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (!(iter->use_hide && (iter->mpoly->flag & ME_HIDE)) &&
      (!iter->mr->use_subsurf_fdots || (iter->mvert->flag & ME_VERT_FACEDOT))) {
    MeshExtract_EditUvFdot_Data *extract_data = (MeshExtract_EditUvFdot_Data *)data;
    GPU_indexbuf_set_point_vert(&extract_data->elb, iter->face_idx, iter->face_idx);
  }
}
static void mesh_edituv_fdot_iter_edit(const MeshExtractIterData *iter,
                                       void *UNUSED(buf),
                                       void *data)
{
  MeshExtract_EditUvFdot_Data *extract_data = (MeshExtract_EditUvFdot_Data *)data;
  if (iter->efa && !BM_elem_flag_test(iter->efa, BM_ELEM_HIDDEN) &&
      (extract_data->sync_selection || BM_elem_flag_test(iter->efa, BM_ELEM_SELECT)) &&
      (!iter->mr->use_subsurf_fdots || (iter->mvert->flag & ME_VERT_FACEDOT))) {
    GPU_indexbuf_set_point_vert(&extract_data->elb, iter->face_idx, iter->face_idx);
  }
}
static void mesh_edituv_fdot_finish(const MeshRenderData *UNUSED(mr), void *ibo, void *data)
{
  MeshExtract_EditUvFdot_Data *extract_data = (MeshExtract_EditUvFdot_Data *)data;
  GPU_indexbuf_build_in_place(&extract_data->elb, ibo);
  MEM_freeN(extract_data);
}

MeshExtract extract_edituv_facedots = {mesh_edituv_fdot_init,
                                       mesh_edituv_fdot_iter,
                                       mesh_edituv_fdot_iter_edit,
                                       mesh_edituv_fdot_finish,
                                       MR_ITER_LOOP,
                                       0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Position and Vertex Normal
 * \{ */

typedef struct PosNorLoop {
  float pos[3];
  GPUPackedNormal nor;
} PosNorLoop;

static void *mesh_pos_nor_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* WARNING Adjust PosNorLoop struct accordingly. */
    GPU_vertformat_attr_add(&format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
    GPU_vertformat_attr_add(&format, "nor", GPU_COMP_I10, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
    GPU_vertformat_alias_add(&format, "vnor");
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len + mr->loop_loose_len);
  return vbo->data;
}
static void mesh_pos_nor_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  PosNorLoop *vert = (PosNorLoop *)data + iter->loop_idx;
  copy_v3_v3(vert->pos, iter->mvert->co);
  vert->nor = GPU_normal_convert_i10_s3(iter->mvert->no);
  /* Flag for paint mode overlay. */
  if (iter->mpoly) {
    if (iter->mpoly->flag & ME_HIDE)
      vert->nor.w = -1;
    else if (iter->mpoly->flag & ME_FACE_SEL)
      vert->nor.w = 1;
    else
      vert->nor.w = 0;
  }
}
static void mesh_pos_nor_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (iter->mvert) {
    mesh_pos_nor_iter(iter, NULL, data);
  }
  else {
    PosNorLoop *vert = (PosNorLoop *)data + iter->loop_idx;
    copy_v3_v3(vert->pos, iter->eve->co);
    vert->nor = GPU_normal_convert_i10_v3(iter->eve->no);
  }
}

MeshExtract extract_pos_nor = {mesh_pos_nor_init,
                               mesh_pos_nor_iter,
                               mesh_pos_nor_iter_edit,
                               NULL,
                               MR_ITER_LOOP | MR_ITER_LEDGE | MR_ITER_LVERT,
                               0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Loop Normal
 * \{ */

static void *mesh_lnor_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "nor", GPU_COMP_I10, 3, GPU_FETCH_INT_TO_FLOAT_UNIT);
    GPU_vertformat_alias_add(&format, "lnor");
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  return vbo->data;
}
static void mesh_lnor_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  GPUPackedNormal *lnors_packed = (GPUPackedNormal *)data + iter->loop_idx;
  *lnors_packed = GPU_normal_convert_i10_v3(iter->mr->loop_normals[iter->loop_idx]);
}

MeshExtract extract_lnor = {
    mesh_lnor_init, mesh_lnor_iter, mesh_lnor_iter, NULL, MR_ITER_LOOP, MR_DATA_LOOP_NOR};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract UV / Tangent layers
 * \{ */

static void *mesh_uv_tan_init(const MeshRenderData *mr, void *buf)
{
  GPUVertFormat format = {0};
  GPU_vertformat_deinterleave(&format);

  CustomData *cd_ldata = &mr->me->ldata;
  CustomData *cd_vdata = &mr->me->vdata;
  uint32_t uv_layers = mr->cache->cd_used.uv;
  uint32_t tan_layers = mr->cache->cd_used.tan;
  float(*orco)[3] = CustomData_get_layer(cd_vdata, CD_ORCO);
  bool orco_allocated = false;
  const bool use_orco_tan = mr->cache->cd_used.tan_orco != 0;

  /* XXX FIXME XXX */
  /* We use a hash to identify each data layer based on its name.
   * Gawain then search for this name in the current shader and bind if it exists.
   * NOTE : This is prone to hash collision.
   * One solution to hash collision would be to format the cd layer name
   * to a safe glsl var name, but without name clash.
   * NOTE 2 : Replicate changes to code_generate_vertex_new() in gpu_codegen.c */
  for (int i = 0; i < MAX_MTFACE; i++) {
    if (uv_layers & (1 << i)) {
      char attr_name[32];
      const char *layer_name = CustomData_get_layer_name(cd_ldata, CD_MLOOPUV, i);
      uint hash = BLI_ghashutil_strhash_p(layer_name);
      /* UV layer name. */
      BLI_snprintf(attr_name, sizeof(attr_name), "u%u", hash);
      GPU_vertformat_attr_add(&format, attr_name, GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
      /* Auto layer name. */
      BLI_snprintf(attr_name, sizeof(attr_name), "a%u", hash);
      GPU_vertformat_alias_add(&format, attr_name);
      /* Active render layer name. */
      if (i == CustomData_get_render_layer(cd_ldata, CD_MLOOPUV)) {
        GPU_vertformat_alias_add(&format, "u");
      }
      /* Active display layer name. */
      if (i == CustomData_get_active_layer(cd_ldata, CD_MLOOPUV)) {
        GPU_vertformat_alias_add(&format, "au");
        /* Alias to pos for edit uvs. */
        GPU_vertformat_alias_add(&format, "pos");
      }
      /* Stencil mask uv layer name. */
      if (i == CustomData_get_stencil_layer(cd_ldata, CD_MLOOPUV)) {
        GPU_vertformat_alias_add(&format, "mu");
      }
    }
  }

  int tan_len = 0;
  char tangent_names[MAX_MTFACE][MAX_NAME];

  for (int i = 0; i < MAX_MTFACE; i++) {
    if (tan_layers & (1 << i)) {
      char attr_name[32];
      const char *layer_name = CustomData_get_layer_name(cd_ldata, CD_MLOOPUV, i);
      uint hash = BLI_ghashutil_strhash_p(layer_name);
      /* Tangent layer name. */
      BLI_snprintf(attr_name, sizeof(attr_name), "t%u", hash);
      GPU_vertformat_attr_add(&format, attr_name, GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
      /* Active render layer name. */
      if (i == CustomData_get_render_layer(cd_ldata, CD_MLOOPUV)) {
        GPU_vertformat_alias_add(&format, "t");
      }
      /* Active display layer name. */
      if (i == CustomData_get_active_layer(cd_ldata, CD_MLOOPUV)) {
        GPU_vertformat_alias_add(&format, "at");
      }

      BLI_strncpy(tangent_names[tan_len++], layer_name, MAX_NAME);
    }
  }
  if (use_orco_tan && orco == NULL) {
    /* If orco is not available compute it ourselves */
    orco_allocated = true;
    orco = MEM_mallocN(sizeof(*orco) * mr->vert_len, __func__);

    if (mr->extract_type == MR_EXTRACT_BMESH) {
      BMesh *bm = mr->bm;
      for (int v = 0; v < mr->vert_len; v++) {
        copy_v3_v3(orco[v], BM_vert_at_index(bm, v)->co);
      }
    }
    else {
      const MVert *mvert = mr->mvert;
      for (int v = 0; v < mr->vert_len; v++, mvert++) {
        copy_v3_v3(orco[v], mvert->co);
      }
    }
    BKE_mesh_orco_verts_transform(mr->me, orco, mr->vert_len, 0);
  }

  /* Start Fresh */
  CustomData_free_layers(cd_ldata, CD_TANGENT, mr->loop_len);

  if (tan_len != 0 || use_orco_tan) {
    short tangent_mask = 0;
    bool calc_active_tangent = false;
    if (mr->extract_type == MR_EXTRACT_BMESH) {
      BKE_editmesh_loop_tangent_calc(mr->edit_bmesh,
                                     calc_active_tangent,
                                     tangent_names,
                                     tan_len,
                                     mr->poly_normals,
                                     mr->loop_normals,
                                     orco,
                                     cd_ldata,
                                     mr->loop_len,
                                     &tangent_mask);
    }
    else {
      BKE_mesh_calc_loop_tangent_ex(mr->mvert,
                                    mr->mpoly,
                                    mr->poly_len,
                                    mr->mloop,
                                    mr->mlooptri,
                                    mr->tri_len,
                                    cd_ldata,
                                    calc_active_tangent,
                                    tangent_names,
                                    tan_len,
                                    mr->poly_normals,
                                    mr->loop_normals,
                                    orco,
                                    cd_ldata,
                                    mr->loop_len,
                                    &tangent_mask);
    }
  }

  if (use_orco_tan) {
    char attr_name[32];
    const char *layer_name = CustomData_get_layer_name(cd_ldata, CD_TANGENT, 0);
    uint hash = BLI_ghashutil_strhash_p(layer_name);
    BLI_snprintf(attr_name, sizeof(*attr_name), "t%u", hash);
    GPU_vertformat_attr_add(&format, attr_name, GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
    GPU_vertformat_alias_add(&format, "t");
    GPU_vertformat_alias_add(&format, "at");
  }

  if (orco_allocated) {
    MEM_SAFE_FREE(orco);
  }

  int v_len = mr->loop_len;
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "dummy", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
    /* VBO will not be used, only allocate minimum of memory. */
    v_len = 1;
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, v_len);

  float(*uv_data)[2] = (float(*)[2])vbo->data;
  for (int i = 0; i < MAX_MTFACE; i++) {
    if (uv_layers & (1 << i)) {
      MLoopUV *layer_data = CustomData_get_layer_n(cd_ldata, CD_MLOOPUV, i);
      for (int l = 0; l < mr->loop_len; l++, uv_data++, layer_data++) {
        memcpy(uv_data, layer_data->uv, sizeof(*uv_data));
      }
    }
  }
  /* Start tan_data after uv_data. */
  float(*tan_data)[4] = (float(*)[4])uv_data;
  for (int i = 0; i < MAX_MTFACE; i++) {
    if (tan_layers & (1 << i)) {
      void *layer_data = CustomData_get_layer_n(cd_ldata, CD_TANGENT, i);
      memcpy(tan_data, layer_data, sizeof(*tan_data) * mr->loop_len);
      tan_data += mr->loop_len;
    }
  }
  if (use_orco_tan) {
    void *layer_data = CustomData_get_layer_n(cd_ldata, CD_TANGENT, 0);
    memcpy(tan_data, layer_data, sizeof(*tan_data) * mr->loop_len);
  }

  CustomData_free_layers(cd_ldata, CD_TANGENT, mr->loop_len);

  return NULL;
}

MeshExtract extract_uv_tan = {
    mesh_uv_tan_init, NULL, NULL, NULL, 0, MR_DATA_POLY_NOR | MR_DATA_LOOP_NOR | MR_DATA_LOOPTRI};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract VCol
 * \{ */

static void *mesh_vcol_init(const MeshRenderData *mr, void *buf)
{
  GPUVertFormat format = {0};
  GPU_vertformat_deinterleave(&format);

  CustomData *cd_ldata = &mr->me->ldata;
  uint32_t vcol_layers = mr->cache->cd_used.vcol;

  /* XXX FIXME XXX */
  /* We use a hash to identify each data layer based on its name.
   * Gawain then search for this name in the current shader and bind if it exists.
   * NOTE : This is prone to hash collision.
   * One solution to hash collision would be to format the cd layer name
   * to a safe glsl var name, but without name clash.
   * NOTE 2 : Replicate changes to code_generate_vertex_new() in gpu_codegen.c */
  for (int i = 0; i < 8; i++) {
    if (vcol_layers & (1 << i)) {
      char attr_name[32];
      const char *layer_name = CustomData_get_layer_name(cd_ldata, CD_MLOOPCOL, i);
      uint hash = BLI_ghashutil_strhash_p(layer_name);

      BLI_snprintf(attr_name, sizeof(attr_name), "c%u", hash);
      GPU_vertformat_attr_add(&format, attr_name, GPU_COMP_U8, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);

      if (i == CustomData_get_render_layer(cd_ldata, CD_MLOOPCOL)) {
        GPU_vertformat_alias_add(&format, "c");
      }
      if (i == CustomData_get_active_layer(cd_ldata, CD_MLOOPCOL)) {
        GPU_vertformat_alias_add(&format, "ac");
      }
      /* Gather number of auto layers. */
      /* We only do vcols that are not overridden by uvs */
      if (CustomData_get_named_layer_index(cd_ldata, CD_MLOOPUV, layer_name) == -1) {
        BLI_snprintf(attr_name, sizeof(attr_name), "a%u", hash);
        GPU_vertformat_alias_add(&format, attr_name);
      }
    }
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  MLoopCol *vcol_data = (MLoopCol *)vbo->data;
  for (int i = 0; i < 8; i++) {
    if (vcol_layers & (1 << i)) {
      void *layer_data = CustomData_get_layer_n(cd_ldata, CD_MLOOPCOL, i);
      memcpy(vcol_data, layer_data, sizeof(*vcol_data) * mr->loop_len);
      vcol_data += mr->loop_len;
    }
  }
  return NULL;
}

MeshExtract extract_vcol = {mesh_vcol_init, NULL, NULL, NULL, 0, 0};

/** \} */ /** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Orco
 * \{ */

typedef struct MeshExtract_Orco_Data {
  float (*vbo_data)[4];
  float (*orco)[3];
} MeshExtract_Orco_Data;

static void *mesh_orco_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* FIXME(fclem): We use the last component as a way to differentiate from generic vertex
     * attribs. This is a substential waste of Vram and should be done another way.
     * Unfortunately, at the time of writting, I did not found any other "non disruptive"
     * alternative. */
    GPU_vertformat_attr_add(&format, "orco", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  CustomData *cd_vdata = &mr->me->vdata;

  MeshExtract_Orco_Data *data = MEM_mallocN(sizeof(*data), __func__);
  data->vbo_data = (float(*)[4])vbo->data;
  data->orco = CustomData_get_layer(cd_vdata, CD_ORCO);
  /* Make sure orco layer was requested only if needed! */
  BLI_assert(data->orco);
  return data;
}
static void mesh_orco_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_Orco_Data *orco_data = (MeshExtract_Orco_Data *)data;
  float *loop_orco = orco_data->vbo_data[iter->loop_idx];
  copy_v3_v3(loop_orco, orco_data->orco[iter->vert_idx]);
  loop_orco[3] = 0.0; /* Tag as not a generic attrib */
}
static void mesh_orco_finish(const MeshRenderData *UNUSED(mr), void *UNUSED(buf), void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_orco = {
    mesh_orco_init, mesh_orco_iter, mesh_orco_iter, mesh_orco_finish, MR_ITER_LOOP, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edge Factor
 * Defines how much an edge is visible.
 * \{ */

typedef struct MeshExtract_EdgeFac_Data {
  uchar *vbo_data;
  bool use_edge_render;
  /* Number of loop per edge. */
  uint32_t edge_loop_count[0];
} MeshExtract_EdgeFac_Data;

static float mesh_loop_edge_factor_get(const float f_no[3],
                                       const float v_co[3],
                                       const float v_no[3],
                                       const float v_next_co[3])
{
  float enor[3], evec[3];
  sub_v3_v3v3(evec, v_next_co, v_co);
  cross_v3_v3v3(enor, v_no, evec);
  normalize_v3(enor);
  float d = fabsf(dot_v3v3(enor, f_no));
  /* Rescale to the slider range. */
  d *= (1.0f / 0.065f);
  CLAMP(d, 0.0f, 1.0f);
  return d;
}

static void *mesh_edge_fac_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "wd", GPU_COMP_U8, 1, GPU_FETCH_INT_TO_FLOAT_UNIT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len + mr->loop_loose_len);

  MeshExtract_EdgeFac_Data *data;

  if (mr->extract_type == MR_EXTRACT_MESH) {
    size_t edge_loop_count_size = sizeof(uint32_t) * mr->edge_len;
    data = MEM_callocN(sizeof(*data) + edge_loop_count_size, __func__);

    /* HACK(fclem) Detecting the need for edge render.
     * We could have a flag in the mesh instead or check the modifier stack. */
    const MEdge *medge = mr->medge;
    for (int e = 0; e < mr->edge_len; e++, medge++) {
      if ((medge->flag & ME_EDGERENDER) == 0) {
        data->use_edge_render = true;
        break;
      }
    }
  }
  else {
    data = MEM_callocN(sizeof(*data), __func__);
    /* HACK to bypass non-manifold check in mesh_edge_fac_finish(). */
    data->use_edge_render = true;
  }

  data->vbo_data = vbo->data;
  return data;
}
static void mesh_edge_fac_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_EdgeFac_Data *efac_data = (MeshExtract_EdgeFac_Data *)data;
  if (efac_data->use_edge_render) {
    efac_data->vbo_data[iter->loop_idx] = (iter->medge->flag & ME_EDGERENDER) ? 255 : 0;
  }
  else if (iter->mpoly) {
    const MVert *mvert = iter->mr->mvert;
    const MLoop *mloop = iter->mr->mloop;
    const float *fnor = iter->mr->poly_normals[iter->face_idx];
    const MVert *v1 = &mvert[mloop[iter->v1].v], *v2 = &mvert[mloop[iter->v2].v];
    float vnor_f[3];
    normal_short_to_float_v3(vnor_f, iter->mvert->no);
    float ratio = mesh_loop_edge_factor_get(fnor, v1->co, vnor_f, v2->co);
    efac_data->vbo_data[iter->loop_idx] = ratio * 253 + 1;
    /* Count loop per edge to detect non-manifold. */
    atomic_fetch_and_add_uint32(&efac_data->edge_loop_count[iter->edge_idx], 1u);
  }
}
static void mesh_edge_fac_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_EdgeFac_Data *efac_data = (MeshExtract_EdgeFac_Data *)data;
  if (iter->efa != NULL) {
    float ratio = mesh_loop_edge_factor_get(
        iter->efa->no, iter->eloop->v->co, iter->eloop->v->no, iter->eloop->next->v->co);
    efac_data->vbo_data[iter->loop_idx] = ratio * 253 + 1;
  }
  else {
    efac_data->vbo_data[iter->loop_idx] = 255;
  }
}
static void mesh_edge_fac_finish(const MeshRenderData *mr, void *buf, void *data)
{
  MeshExtract_EdgeFac_Data *efac_data = (MeshExtract_EdgeFac_Data *)data;

  if (!efac_data->use_edge_render) {
    /* Set non-manifold edges to ratio 1. */
    const MLoop *mloop = mr->mloop;
    for (int l = 0; l < mr->loop_len; l++, mloop++) {
      if (efac_data->edge_loop_count[mloop->e] != 2) {
        efac_data->vbo_data[l] = 255;
      }
    }
  }

  if (GPU_crappy_amd_driver()) {
    GPUVertBuf *vbo = (GPUVertBuf *)buf;
    /* Some AMD drivers strangely crash with VBOs with a one byte format.
     * To workaround we reinit the vbo with another format and convert
     * all bytes to floats. */
    static GPUVertFormat format = {0};
    if (format.attr_len == 0) {
      GPU_vertformat_attr_add(&format, "wd", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
    }
    /* We keep the data reference in efac_data->vbo_data. */
    vbo->data = NULL;
    GPU_vertbuf_clear(vbo);

    int buf_len = mr->loop_len + mr->loop_loose_len;
    GPU_vertbuf_init_with_format(vbo, &format);
    GPU_vertbuf_data_alloc(vbo, buf_len);

    float *fdata = (float *)vbo->data;
    for (int l = 0; l < buf_len; l++, fdata++) {
      *fdata = efac_data->vbo_data[l] / 255.0f;
    }
    /* Free old byte data. */
    MEM_freeN(efac_data->vbo_data);
  }
  MEM_freeN(efac_data);
}

MeshExtract extract_edge_fac = {mesh_edge_fac_init,
                                mesh_edge_fac_iter,
                                mesh_edge_fac_iter_edit,
                                mesh_edge_fac_finish,
                                MR_ITER_LOOP | MR_ITER_LEDGE,
                                MR_DATA_POLY_NOR};

/** \} */
/* ---------------------------------------------------------------------- */
/** \name Extract Vertex Weight
 * \{ */

typedef struct MeshExtract_Weight_Data {
  float *vbo_data;
  const DRW_MeshWeightState *wstate;
  const MDeformVert *dvert; /* For Mesh. */
  int cd_ofs;               /* For BMesh. */
} MeshExtract_Weight_Data;

static float evaluate_vertex_weight(const MDeformVert *dvert, const DRW_MeshWeightState *wstate)
{
  /* Error state. */
  if ((wstate->defgroup_active < 0) && (wstate->defgroup_len > 0)) {
    return -2.0f;
  }
  else if (dvert == NULL) {
    return (wstate->alert_mode != OB_DRAW_GROUPUSER_NONE) ? -1.0f : 0.0f;
  }

  float input = 0.0f;
  if (wstate->flags & DRW_MESH_WEIGHT_STATE_MULTIPAINT) {
    /* Multi-Paint feature */
    input = BKE_defvert_multipaint_collective_weight(
        dvert,
        wstate->defgroup_len,
        wstate->defgroup_sel,
        wstate->defgroup_sel_count,
        (wstate->flags & DRW_MESH_WEIGHT_STATE_AUTO_NORMALIZE) != 0);
    /* make it black if the selected groups have no weight on a vertex */
    if (input == 0.0f) {
      return -1.0f;
    }
  }
  else {
    /* default, non tricky behavior */
    input = defvert_find_weight(dvert, wstate->defgroup_active);

    if (input == 0.0f) {
      switch (wstate->alert_mode) {
        case OB_DRAW_GROUPUSER_ACTIVE:
          return -1.0f;
          break;
        case OB_DRAW_GROUPUSER_ALL:
          if (defvert_is_weight_zero(dvert, wstate->defgroup_len)) {
            return -1.0f;
          }
          break;
      }
    }
  }
  CLAMP(input, 0.0f, 1.0f);
  return input;
}

static void *mesh_weight_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "weight", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len + mr->loop_loose_len);

  MeshExtract_Weight_Data *data = MEM_callocN(sizeof(*data), __func__);
  data->vbo_data = (float *)vbo->data;
  data->wstate = &mr->cache->weight_state;

  if (data->wstate->defgroup_active == -1) {
    /* Nothing to show. */
    data->dvert = NULL;
    data->cd_ofs = -1;
  }
  else if (mr->extract_type == MR_EXTRACT_BMESH) {
    data->dvert = NULL;
    data->cd_ofs = CustomData_get_offset(&mr->bm->vdata, CD_MDEFORMVERT);
  }
  else {
    data->dvert = CustomData_get_layer(&mr->me->vdata, CD_MDEFORMVERT);
    data->cd_ofs = -1;
  }
  return data;
}
static void mesh_weight_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_Weight_Data *wdata = (MeshExtract_Weight_Data *)data;
  const MDeformVert *dvert = NULL;
  if (wdata->dvert != NULL) {
    dvert = &wdata->dvert[iter->vert_idx];
  }
  wdata->vbo_data[iter->loop_idx] = evaluate_vertex_weight(dvert, wdata->wstate);
}
static void mesh_weight_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_Weight_Data *wdata = (MeshExtract_Weight_Data *)data;
  const MDeformVert *dvert = NULL;
  if (wdata->dvert != NULL) {
    dvert = &wdata->dvert[iter->vert_idx];
  }
  else if (wdata->cd_ofs != -1) {
    dvert = BM_ELEM_CD_GET_VOID_P(iter->eve, wdata->cd_ofs);
  }
  wdata->vbo_data[iter->loop_idx] = evaluate_vertex_weight(dvert, wdata->wstate);
}
static void mesh_weight_finish(const MeshRenderData *UNUSED(mr), void *UNUSED(buf), void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_weights = {mesh_weight_init,
                               mesh_weight_iter,
                               mesh_weight_iter_edit,
                               mesh_weight_finish,
                               MR_ITER_LOOP | MR_ITER_LEDGE | MR_ITER_LVERT,
                               0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit Mode Data / Flags
 * \{ */

typedef struct EditLoopData {
  uchar v_flag;
  uchar e_flag;
  uchar crease;
  uchar bweight;
} EditLoopData;

static void mesh_render_data_face_flag(MeshRenderData *mr,
                                       const BMFace *efa,
                                       const int cd_ofs,
                                       EditLoopData *eattr)
{
  if (efa == mr->efa_act) {
    eattr->v_flag |= VFLAG_FACE_ACTIVE;
  }
  if (BM_elem_flag_test(efa, BM_ELEM_SELECT)) {
    eattr->v_flag |= VFLAG_FACE_SELECTED;
  }

  if (efa == mr->efa_act_uv) {
    eattr->v_flag |= VFLAG_FACE_UV_ACTIVE;
  }
  if ((cd_ofs != -1) && uvedit_face_select_test_ex(mr->toolsettings, (BMFace *)efa, cd_ofs)) {
    eattr->v_flag |= VFLAG_FACE_UV_SELECT;
  }

#ifdef WITH_FREESTYLE
  if (mr->freestyle_face_ofs != -1) {
    const FreestyleFace *ffa = BM_ELEM_CD_GET_VOID_P(efa, mr->freestyle_face_ofs);
    if (ffa->flag & FREESTYLE_FACE_MARK) {
      eattr->v_flag |= VFLAG_FACE_FREESTYLE;
    }
  }
#endif
}

static void mesh_render_data_edge_flag(const MeshRenderData *mr,
                                       const BMEdge *eed,
                                       EditLoopData *eattr)
{
  const ToolSettings *ts = mr->toolsettings;
  const bool is_vertex_select_mode = (ts != NULL) && (ts->selectmode & SCE_SELECT_VERTEX) != 0;
  const bool is_face_only_select_mode = (ts != NULL) && (ts->selectmode == SCE_SELECT_FACE);

  if (eed == mr->eed_act) {
    eattr->e_flag |= VFLAG_EDGE_ACTIVE;
  }
  if (!is_vertex_select_mode && BM_elem_flag_test(eed, BM_ELEM_SELECT)) {
    eattr->e_flag |= VFLAG_EDGE_SELECTED;
  }
  if (is_vertex_select_mode && BM_elem_flag_test(eed->v1, BM_ELEM_SELECT) &&
      BM_elem_flag_test(eed->v2, BM_ELEM_SELECT)) {
    eattr->e_flag |= VFLAG_EDGE_SELECTED;
    eattr->e_flag |= VFLAG_VERT_SELECTED;
  }
  if (BM_elem_flag_test(eed, BM_ELEM_SEAM)) {
    eattr->e_flag |= VFLAG_EDGE_SEAM;
  }
  if (!BM_elem_flag_test(eed, BM_ELEM_SMOOTH)) {
    eattr->e_flag |= VFLAG_EDGE_SHARP;
  }

  /* Use active edge color for active face edges because
   * specular highlights make it hard to see T55456#510873.
   *
   * This isn't ideal since it can't be used when mixing edge/face modes
   * but it's still better then not being able to see the active face. */
  if (is_face_only_select_mode) {
    if (mr->efa_act != NULL) {
      if (BM_edge_in_face(eed, mr->efa_act)) {
        eattr->e_flag |= VFLAG_EDGE_ACTIVE;
      }
    }
  }

  /* Use a byte for value range */
  if (mr->crease_ofs != -1) {
    float crease = BM_ELEM_CD_GET_FLOAT(eed, mr->crease_ofs);
    if (crease > 0) {
      eattr->crease = (uchar)(crease * 255.0f);
    }
  }
  /* Use a byte for value range */
  if (mr->bweight_ofs != -1) {
    float bweight = BM_ELEM_CD_GET_FLOAT(eed, mr->bweight_ofs);
    if (bweight > 0) {
      eattr->bweight = (uchar)(bweight * 255.0f);
    }
  }
#ifdef WITH_FREESTYLE
  if (mr->freestyle_edge_ofs != -1) {
    const FreestyleEdge *fed = BM_ELEM_CD_GET_VOID_P(eed, mr->freestyle_edge_ofs);
    if (fed->flag & FREESTYLE_EDGE_MARK) {
      eattr->e_flag |= VFLAG_EDGE_FREESTYLE;
    }
  }
#endif
}

static void mesh_render_data_loop_flag(MeshRenderData *mr,
                                       BMLoop *loop,
                                       const int cd_ofs,
                                       EditLoopData *eattr)
{
  if (cd_ofs == -1) {
    return;
  }
  MLoopUV *luv = BM_ELEM_CD_GET_VOID_P(loop, cd_ofs);
  if (luv != NULL && (luv->flag & MLOOPUV_PINNED)) {
    eattr->v_flag |= VFLAG_VERT_UV_PINNED;
  }
  if (uvedit_uv_select_test_ex(mr->toolsettings, loop, cd_ofs)) {
    eattr->v_flag |= VFLAG_VERT_UV_SELECT;
  }
}

static void mesh_render_data_loop_edge_flag(MeshRenderData *mr,
                                            BMLoop *loop,
                                            const int cd_ofs,
                                            EditLoopData *eattr)
{
  if (cd_ofs == -1) {
    return;
  }
  if (uvedit_edge_select_test_ex(mr->toolsettings, loop, cd_ofs)) {
    eattr->v_flag |= VFLAG_EDGE_UV_SELECT;
    eattr->v_flag |= VFLAG_VERT_UV_SELECT;
  }
}

static void mesh_render_data_vert_flag(MeshRenderData *mr, const BMVert *eve, EditLoopData *eattr)
{
  if (eve == mr->eve_act) {
    eattr->e_flag |= VFLAG_VERT_ACTIVE;
  }
  if (BM_elem_flag_test(eve, BM_ELEM_SELECT)) {
    eattr->e_flag |= VFLAG_VERT_SELECTED;
  }
}

static void *mesh_edit_data_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* WARNING Adjust EditLoopData struct accordingly. */
    GPU_vertformat_attr_add(&format, "data", GPU_COMP_U8, 4, GPU_FETCH_INT);
    GPU_vertformat_alias_add(&format, "flag");
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len + mr->loop_loose_len);
  return vbo->data;
}
static void mesh_edit_data_iter(const MeshExtractIterData *UNUSED(iter),
                                void *UNUSED(buf),
                                void *UNUSED(data))
{
  BLI_assert(0);
}
static void mesh_edit_data_iter_edit(const MeshExtractIterData *iter,
                                     void *UNUSED(buf),
                                     void *data)
{
  EditLoopData *eldata = (EditLoopData *)data + iter->loop_idx;
  memset(eldata, 0x0, sizeof(*eldata));
  if (iter->efa) {
    mesh_render_data_face_flag(iter->mr, iter->efa, -1, eldata);
  }
  if (iter->eed) {
    mesh_render_data_edge_flag(iter->mr, iter->eed, eldata);
  }
  if (iter->eve) {
    mesh_render_data_vert_flag(iter->mr, iter->eve, eldata);
  }
}

MeshExtract extract_edit_data = {mesh_edit_data_init,
                                 mesh_edit_data_iter,
                                 mesh_edit_data_iter_edit,
                                 NULL,
                                 MR_ITER_LOOP | MR_ITER_LEDGE | MR_ITER_LVERT,
                                 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV Data / Flags
 * \{ */

typedef struct MeshExtract_EditUVData_Data {
  EditLoopData *vbo_data;
  int cd_ofs;
} MeshExtract_EditUVData_Data;

static void *mesh_edituv_data_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* WARNING Adjust EditLoopData struct accordingly. */
    GPU_vertformat_attr_add(&format, "data", GPU_COMP_U8, 4, GPU_FETCH_INT);
    GPU_vertformat_alias_add(&format, "flag");
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  CustomData *cd_ldata = &mr->me->ldata;

  MeshExtract_EditUVData_Data *data = MEM_callocN(sizeof(*data), __func__);
  data->vbo_data = (EditLoopData *)vbo->data;
  data->cd_ofs = CustomData_get_offset(cd_ldata, CD_MLOOPUV);
  return data;
}
static void mesh_edituv_data_iter(const MeshExtractIterData *UNUSED(iter),
                                  void *UNUSED(buf),
                                  void *UNUSED(data))
{
  BLI_assert(0);
}
static void mesh_edituv_data_iter_edit(const MeshExtractIterData *iter,
                                       void *UNUSED(buf),
                                       void *data)
{
  MeshExtract_EditUVData_Data *edituv_data = (MeshExtract_EditUVData_Data *)data;
  EditLoopData *eldata = edituv_data->vbo_data + iter->loop_idx;
  memset(eldata, 0x0, sizeof(*eldata));
  if (iter->efa) {
    mesh_render_data_face_flag(iter->mr, iter->efa, edituv_data->cd_ofs, eldata);
  }
  if (iter->eloop) {
    /* Normal edit points. */
    mesh_render_data_loop_flag(iter->mr, iter->eloop, edituv_data->cd_ofs, eldata);
    mesh_render_data_loop_edge_flag(iter->mr, iter->eloop, edituv_data->cd_ofs, eldata);
  }
  else if (iter->efa && iter->eed && iter->eve) {
    /* Mapped points between 2 edit verts. */
    BMLoop *loop = BM_face_edge_share_loop(iter->efa, iter->eed);
    mesh_render_data_loop_flag(iter->mr, loop, edituv_data->cd_ofs, eldata);
    mesh_render_data_loop_edge_flag(iter->mr, loop, edituv_data->cd_ofs, eldata);
  }
  else if (iter->efa && (iter->eed || iter->eed_prev)) {
    /* Mapped points on an edge between two edit verts. */
    BMEdge *eed = iter->eed ? iter->eed : iter->eed_prev;
    BMLoop *loop = BM_face_edge_share_loop(iter->efa, eed);
    mesh_render_data_loop_edge_flag(iter->mr, loop, edituv_data->cd_ofs, eldata);
  }
}
static void mesh_edituv_data_finish(const MeshRenderData *UNUSED(mr),
                                    void *UNUSED(buf),
                                    void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_edituv_data = {mesh_edituv_data_init,
                                   mesh_edituv_data_iter,
                                   mesh_edituv_data_iter_edit,
                                   mesh_edituv_data_finish,
                                   MR_ITER_LOOP,
                                   0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV area stretch
 * \{ */

typedef struct MeshExtract_StretchArea_Data {
  uint16_t *vbo_data;
  float tot_ratio, inv_tot_ratio;
  float area_ratio[0];
} MeshExtract_StretchArea_Data;

static void *mesh_stretch_area_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "stretch", GPU_COMP_U16, 1, GPU_FETCH_INT_TO_FLOAT_UNIT);
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  size_t size_of_area_ratio = sizeof(float) * ((mr->bm) ? mr->bm->totface : 0);
  MeshExtract_StretchArea_Data *data = MEM_mallocN(sizeof(*data) + size_of_area_ratio, __func__);
  data->vbo_data = (uint16_t *)vbo->data;

  float totarea = 0, totuvarea = 0;
  float *area_ratio = data->area_ratio;

  if (mr->bm != NULL) {
    CustomData *cd_ldata = &mr->bm->ldata;
    int uv_ofs = CustomData_get_offset(cd_ldata, CD_MLOOPUV);

    BMFace *efa;
    BMIter f_iter;
    BM_ITER_MESH (efa, &f_iter, mr->bm, BM_FACES_OF_MESH) {
      float area = BM_face_calc_area(efa);
      float uvarea = BM_face_calc_area_uv(efa, uv_ofs);

      totarea += area;
      totuvarea += uvarea;

      float *ratio = area_ratio + BM_elem_index_get(efa);

      if (area < FLT_EPSILON || uvarea < FLT_EPSILON) {
        *ratio = 0.0f;
      }
      else if (area > uvarea) {
        *ratio = (uvarea / area);
      }
      else {
        /* Tag inversion by using the sign. */
        *ratio = -(area / uvarea);
      }
    }
  }
  else {
    /* Should not happen. */
    BLI_assert(0);
  }

  if (totarea < FLT_EPSILON || totuvarea < FLT_EPSILON) {
    data->tot_ratio = 0.0f;
    data->inv_tot_ratio = 0.0f;
  }
  else {
    data->tot_ratio = totarea / totuvarea;
    data->inv_tot_ratio = totuvarea / totarea;
  }

  return data;
}
static void mesh_stretch_area_iter(const MeshExtractIterData *UNUSED(iter),
                                   void *UNUSED(buf),
                                   void *UNUSED(data))
{
  BLI_assert(0);
}
static void mesh_stretch_area_iter_edit(const MeshExtractIterData *iter,
                                        void *UNUSED(buf),
                                        void *data)
{
  MeshExtract_StretchArea_Data *stretch_data = (MeshExtract_StretchArea_Data *)data;
  float stretch = 0.0f;
  if (iter->efa) {
    float ratio = stretch_data->area_ratio[BM_elem_index_get(iter->efa)];
    if (ratio > 0.0f) {
      stretch = ratio * stretch_data->tot_ratio;
    }
    else {
      stretch = -ratio * stretch_data->inv_tot_ratio;
    }
    if (stretch > 1.0f) {
      stretch = 1.0f / stretch;
    }
  }
  stretch_data->vbo_data[iter->loop_idx] = (1.0f - stretch) * 65534.0f;
}
static void mesh_stretch_area_finish(const MeshRenderData *UNUSED(mr),
                                     void *UNUSED(buf),
                                     void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_stretch_area = {mesh_stretch_area_init,
                                    mesh_stretch_area_iter,
                                    mesh_stretch_area_iter_edit,
                                    mesh_stretch_area_finish,
                                    MR_ITER_LOOP,
                                    0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV angle stretch
 * \{ */

typedef struct UVStretchAngle {
  int16_t angle;
  int16_t uv_angles[2];
} UVStretchAngle;

static void compute_normalize_edge_vectors(float auv[2][2],
                                           float av[2][3],
                                           const float uv[2],
                                           const float uv_prev[2],
                                           const float co[2],
                                           const float co_prev[2])
{
  /* Move previous edge. */
  copy_v2_v2(auv[0], auv[1]);
  copy_v3_v3(av[0], av[1]);
  /* 2d edge */
  sub_v2_v2v2(auv[1], uv_prev, uv);
  normalize_v2(auv[1]);
  /* 3d edge */
  sub_v3_v3v3(av[1], co_prev, co);
  normalize_v3(av[1]);
}

static short v2_to_short_angle(float v[2])
{
  return atan2f(v[1], v[0]) * (float)M_1_PI * SHRT_MAX;
}

static void edit_uv_get_stretch_angle(float auv[2][2], float av[2][3], UVStretchAngle *r_stretch)
{
  /* Send uvs to the shader and let it compute the aspect corrected angle. */
  r_stretch->uv_angles[0] = v2_to_short_angle(auv[0]);
  r_stretch->uv_angles[1] = v2_to_short_angle(auv[1]);
  /* Compute 3D angle here. */
  r_stretch->angle = angle_normalized_v3v3(av[0], av[1]) * (float)M_1_PI * SHRT_MAX;

#if 0 /* here for reference, this is done in shader now. */
  float uvang = angle_normalized_v2v2(auv0, auv1);
  float ang = angle_normalized_v3v3(av0, av1);
  float stretch = fabsf(uvang - ang) / (float)M_PI;
  return 1.0f - pow2f(1.0f - stretch);
#endif
}

static void *mesh_stretch_angle_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* WARNING Adjust UVStretchAngle struct accordingly. */
    GPU_vertformat_attr_add(&format, "angle", GPU_COMP_I16, 1, GPU_FETCH_INT_TO_FLOAT_UNIT);
    GPU_vertformat_attr_add(&format, "uv_angles", GPU_COMP_I16, 2, GPU_FETCH_INT_TO_FLOAT_UNIT);
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);
  UVStretchAngle *vbo_data = (UVStretchAngle *)vbo->data;

  /* Special iter nneded to save about half of the computing cost. */
  if (mr->extract_type == MR_EXTRACT_MAPPED) {
    const MLoopUV *uv_data = CustomData_get_layer(&mr->me->ldata, CD_MLOOPUV);

    const MPoly *mpoly = mr->mpoly;
    for (int p = 0; p < mr->poly_len; p++, mpoly++) {
      float auv[2][2], last_auv[2];
      float av[2][3], last_av[3];

      int l = mpoly->loopstart + mpoly->totloop - 1;
      int l_next = mpoly->loopstart;
      const MVert *v = &mr->mvert[mr->mloop[l].v];
      const MVert *v_next = &mr->mvert[mr->mloop[l_next].v];
      compute_normalize_edge_vectors(
          auv, av, uv_data[l].uv, uv_data[l_next].uv, v->co, v_next->co);
      /* Save last edge. */
      copy_v2_v2(last_auv, auv[1]);
      copy_v3_v3(last_av, av[1]);

      int loopend = mpoly->loopstart + mpoly->totloop;
      for (l_next++, l = mpoly->loopstart; l < loopend; l++, l_next++) {
        if (l_next == loopend) {
          l_next = mpoly->loopstart;
          /* Move previous edge. */
          copy_v2_v2(auv[0], auv[1]);
          copy_v3_v3(av[0], av[1]);
          /* Copy already calculated last edge. */
          copy_v2_v2(auv[1], last_auv);
          copy_v3_v3(av[1], last_av);
        }
        else {
          v = &mr->mvert[mr->mloop[l].v];
          v_next = &mr->mvert[mr->mloop[l_next].v];
          compute_normalize_edge_vectors(
              auv, av, uv_data[l].uv, uv_data[l_next].uv, v->co, v_next->co);
        }
        edit_uv_get_stretch_angle(auv, av, vbo_data + l);
      }
    }
  }
  else {
    int uv_ofs = CustomData_get_offset(&mr->bm->ldata, CD_MLOOPUV);

    BMFace *efa;
    BMIter f_iter;
    BM_ITER_MESH (efa, &f_iter, mr->bm, BM_FACES_OF_MESH) {
      float auv[2][2], last_auv[2];
      float av[2][3], last_av[3];

      BMLoop *l = efa->l_first->prev, *l_next = efa->l_first;
      MLoopUV *luv = BM_ELEM_CD_GET_VOID_P(l, uv_ofs);
      MLoopUV *luv_next = BM_ELEM_CD_GET_VOID_P(l_next, uv_ofs);
      compute_normalize_edge_vectors(auv, av, luv->uv, luv_next->uv, l->v->co, l_next->v->co);
      /* Save last edge. */
      copy_v2_v2(last_auv, auv[1]);
      copy_v3_v3(last_av, av[1]);

      BMIter l_iter;
      BM_ITER_ELEM (l, &l_iter, efa, BM_LOOPS_OF_FACE) {
        l_next = l->next;
        if (l_next == efa->l_first) {
          /* Move previous edge. */
          copy_v2_v2(auv[0], auv[1]);
          copy_v3_v3(av[0], av[1]);
          /* Copy already calculated last edge. */
          copy_v2_v2(auv[1], last_auv);
          copy_v3_v3(av[1], last_av);
        }
        else {
          luv = BM_ELEM_CD_GET_VOID_P(l, uv_ofs);
          luv_next = BM_ELEM_CD_GET_VOID_P(l_next, uv_ofs);
          compute_normalize_edge_vectors(auv, av, luv->uv, luv_next->uv, l->v->co, l_next->v->co);
        }
        edit_uv_get_stretch_angle(auv, av, vbo_data + BM_elem_index_get(l));
      }
    }
  }

  return NULL;
}

MeshExtract extract_stretch_angle = {mesh_stretch_angle_init, NULL, NULL, NULL, 0, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Edit UV angle stretch
 * \{ */

static void *mesh_mesh_analysis_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "weight_color", GPU_COMP_U8, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
  }

  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len);

  BLI_assert(mr->edit_bmesh);

  /* TODO: This is only doing proper statvis for the cage. A good enhancement would be to compute
   * the statvis here and on the final mesh. This could potentially avoid a copy and multithread
   * the computation. */
  /* Stores weight color into derivedVertColor */
  BKE_editmesh_statvis_calc(mr->edit_bmesh, mr->edit_data, &mr->toolsettings->statvis);

  return vbo->data;
}
static void mesh_mesh_analysis_iter(const MeshExtractIterData *UNUSED(iter),
                                    void *UNUSED(buf),
                                    void *UNUSED(data))
{
#if 0 /* Not used for now. */
  const MeshRenderData *mr = iter->mr;
  const bool is_vertex_data = mr->toolsettings->statvis.type == SCE_STATVIS_SHARP;
  uchar(*l_weight)[4] = (uchar(*)[4])data;
  if (is_vertex_data && (mr->v_origindex[iter->vert_idx] != ORIGINDEX_NONE)) {
    uchar(*weight)[4] = &mr->edit_bmesh->derivedVertColor[mr->v_origindex[iter->vert_idx]];
    copy_v4_v4_uchar(l_weight[iter->loop_idx], weight);
  }
  else if (!is_vertex_data && (mr->p_origindex[iter->face_idx] != ORIGINDEX_NONE)) {
    uchar(*weight)[4] = &mr->edit_bmesh->derivedFaceColor[mr->p_origindex[iter->face_idx]];
    copy_v4_v4_uchar(l_weight[iter->loop_idx], weight);
  }
  else {
    uchar col_fallback[4] = {64, 64, 64, 255}; /* gray */
    copy_v4_v4_uchar(l_weight[iter->loop_idx], col_fallback);
  }
#endif
}
static void mesh_mesh_analysis_iter_edit(const MeshExtractIterData *iter,
                                         void *UNUSED(buf),
                                         void *data)
{
  const MeshRenderData *mr = iter->mr;
  const bool is_vertex_data = mr->toolsettings->statvis.type == SCE_STATVIS_SHARP;
  uchar(*l_weight)[4] = (uchar(*)[4])data;
  if (is_vertex_data && iter->eve) {
    uchar *weight = mr->edit_bmesh->derivedVertColor[BM_elem_index_get(iter->eve)];
    copy_v4_v4_uchar(l_weight[iter->loop_idx], weight);
  }
  else if (!is_vertex_data && iter->efa) {
    uchar *weight = mr->edit_bmesh->derivedFaceColor[BM_elem_index_get(iter->efa)];
    copy_v4_v4_uchar(l_weight[iter->loop_idx], weight);
  }
  else {
    uchar col_fallback[4] = {64, 64, 64, 255}; /* gray */
    copy_v4_v4_uchar(l_weight[iter->loop_idx], col_fallback);
  }
}
static void mesh_mesh_analysis_finish(const MeshRenderData *mr,
                                      void *UNUSED(buf),
                                      void *UNUSED(data))
{
  BKE_editmesh_color_free(mr->edit_bmesh);
}

MeshExtract extract_mesh_analysis = {mesh_mesh_analysis_init,
                                     mesh_mesh_analysis_iter,
                                     mesh_mesh_analysis_iter_edit,
                                     mesh_mesh_analysis_finish,
                                     MR_ITER_LOOP,
                                     0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Facedots positions
 * \{ */

static void *mesh_fdots_pos_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->poly_len);
  /* Clear so we can accumulate on it. */
  memset(vbo->data, 0x0, mr->poly_len * vbo->format.stride);
  return vbo->data;
}
static void mesh_fdots_pos_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  float(*center)[3] = (float(*)[3])data;
  if (iter->mr->use_subsurf_fdots) {
    if (iter->mvert->flag & ME_VERT_FACEDOT) {
      copy_v3_v3(center[iter->face_idx], iter->mvert->co);
    }
  }
  else {
    float w = 1.0f / (float)iter->mpoly->totloop;
    /* This is thread safe since we are spliting workload in faces (for loop iter) so one face is
     * only touched by one thread. */
    madd_v3_v3fl(center[iter->face_idx], iter->mvert->co, w);
  }
}
static void mesh_fdots_pos_iter_edit(const MeshExtractIterData *iter,
                                     void *UNUSED(buf),
                                     void *data)
{
  if (iter->mpoly) {
    mesh_fdots_pos_iter(iter, NULL, data);
  }
  else {
    float(*center)[3] = (float(*)[3])data;
    float w = 1.0f / (float)iter->efa->len;
    /* This is thread safe since we are spliting workload in faces (for loop iter) so one face is
     * only touched by one thread. */
    madd_v3_v3fl(center[iter->face_idx], iter->eve->co, w);
  }
}

MeshExtract extract_facedots_pos = {
    mesh_fdots_pos_init, mesh_fdots_pos_iter, mesh_fdots_pos_iter_edit, NULL, MR_ITER_LOOP, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Facedots Normal and edit flag
 * \{ */

static void *mesh_fdots_nor_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "norAndFlag", GPU_COMP_I10, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->poly_len);
  return vbo->data;
}
static void mesh_fdots_nor_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  GPUPackedNormal *nor = (GPUPackedNormal *)data;
  nor[iter->face_idx] = GPU_normal_convert_i10_v3(iter->mr->poly_normals[iter->face_idx]);
}
static void mesh_fdots_nor_iter_edit(const MeshExtractIterData *iter,
                                     void *UNUSED(buf),
                                     void *data)
{
  GPUPackedNormal *nor = (GPUPackedNormal *)data;
  nor[iter->face_idx] = GPU_normal_convert_i10_v3(iter->efa->no);
  /* Select / Active Flag. */
  nor[iter->face_idx].w = BM_elem_flag_test(iter->efa, BM_ELEM_SELECT) ?
                              ((iter->efa == iter->mr->efa_act) ? -1 : 1) :
                              0;
}

MeshExtract extract_facedots_nor = {mesh_fdots_nor_init,
                                    mesh_fdots_nor_iter,
                                    mesh_fdots_nor_iter_edit,
                                    NULL,
                                    MR_ITER_LOOP,
                                    MR_DATA_POLY_NOR};
/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Facedots Normal and edit flag
 * \{ */

typedef struct MeshExtract_FdotUV_Data {
  float (*vbo_data)[2];
  MLoopUV *uv_data;
} MeshExtract_FdotUV_Data;

static void *mesh_fdots_uv_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "u", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
    GPU_vertformat_alias_add(&format, "au");
    GPU_vertformat_alias_add(&format, "pos");
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->poly_len);
  /* Clear so we can accumulate on it. */
  memset(vbo->data, 0x0, mr->poly_len * vbo->format.stride);

  CustomData *cd_ldata = &mr->me->ldata;

  MeshExtract_FdotUV_Data *data = MEM_callocN(sizeof(*data), __func__);
  data->vbo_data = (float(*)[2])vbo->data;
  data->uv_data = CustomData_get_layer(cd_ldata, CD_MLOOPUV);
  return data;
}
static void mesh_fdots_uv_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  MeshExtract_FdotUV_Data *extract_data = (MeshExtract_FdotUV_Data *)data;
  if (iter->mr->use_subsurf_fdots) {
    if (iter->mvert->flag & ME_VERT_FACEDOT) {
      copy_v2_v2(extract_data->vbo_data[iter->face_idx], extract_data->uv_data[iter->loop_idx].uv);
    }
  }
  else {
    float w = 1.0f / (float)iter->mpoly->totloop;
    /* This is thread safe since we are spliting workload in faces (for loop iter) so one face is
     * only touched by one thread. */
    madd_v2_v2fl(
        extract_data->vbo_data[iter->face_idx], extract_data->uv_data[iter->loop_idx].uv, w);
  }
}
static void mesh_fdots_uv_iter_edit(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  if (iter->mpoly) {
    mesh_fdots_uv_iter(iter, NULL, data);
  }
  else {
    MeshExtract_FdotUV_Data *extract_data = (MeshExtract_FdotUV_Data *)data;
    float w = 1.0f / (float)iter->efa->len;
    /* This is thread safe since we are spliting workload in faces (for loop iter) so one face is
     * only touched by one thread. */
    madd_v2_v2fl(
        extract_data->vbo_data[iter->face_idx], extract_data->uv_data[iter->loop_idx].uv, w);
  }
}
static void mesh_fdots_uv_finish(const MeshRenderData *UNUSED(mr), void *UNUSED(buf), void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_facedots_uv = {mesh_fdots_uv_init,
                                   mesh_fdots_uv_iter,
                                   mesh_fdots_uv_iter_edit,
                                   mesh_fdots_uv_finish,
                                   MR_ITER_LOOP,
                                   0};
/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Facedots  Edit UV flag
 * \{ */

typedef struct MeshExtract_EditUVFdotData_Data {
  EditLoopData *vbo_data;
  int cd_ofs;
} MeshExtract_EditUVFdotData_Data;

static void *mesh_fdots_edituv_data_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    GPU_vertformat_attr_add(&format, "flag", GPU_COMP_U8, 4, GPU_FETCH_INT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->poly_len);

  CustomData *cd_ldata = &mr->me->ldata;

  MeshExtract_EditUVFdotData_Data *data = MEM_callocN(sizeof(*data), __func__);
  data->vbo_data = (EditLoopData *)vbo->data;
  data->cd_ofs = CustomData_get_offset(cd_ldata, CD_MLOOPUV);
  return data;
}
static void mesh_fdots_edituv_data_iter(const MeshExtractIterData *UNUSED(iter),
                                        void *UNUSED(buf),
                                        void *UNUSED(data))
{
  BLI_assert(0);
}
static void mesh_fdots_edituv_data_iter_edit(const MeshExtractIterData *iter,
                                             void *UNUSED(buf),
                                             void *data)
{
  MeshExtract_EditUVFdotData_Data *edituv_data = (MeshExtract_EditUVFdotData_Data *)data;
  EditLoopData *eldata = edituv_data->vbo_data + iter->face_idx;
  memset(eldata, 0x0, sizeof(*eldata));
  if (iter->efa) {
    mesh_render_data_face_flag(iter->mr, iter->efa, edituv_data->cd_ofs, eldata);
  }
}
static void mesh_fdots_edituv_data_finish(const MeshRenderData *UNUSED(mr),
                                          void *UNUSED(buf),
                                          void *data)
{
  MEM_freeN(data);
}

MeshExtract extract_facedots_edituv_data = {mesh_fdots_edituv_data_init,
                                            mesh_fdots_edituv_data_iter,
                                            mesh_fdots_edituv_data_iter_edit,
                                            mesh_fdots_edituv_data_finish,
                                            MR_ITER_LOOP,
                                            0};
/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Selection Index
 * \{ */

static void *mesh_select_idx_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* TODO rename "color" to something more descriptive. */
    GPU_vertformat_attr_add(&format, "color", GPU_COMP_U32, 1, GPU_FETCH_INT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->loop_len + mr->loop_loose_len);
  return vbo->data;
}
static void *mesh_select_fdot_idx_init(const MeshRenderData *mr, void *buf)
{
  static GPUVertFormat format = {0};
  if (format.attr_len == 0) {
    /* TODO rename "color" to something more descriptive. */
    GPU_vertformat_attr_add(&format, "color", GPU_COMP_U32, 1, GPU_FETCH_INT);
  }
  GPUVertBuf *vbo = buf;
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, mr->poly_len);
  return vbo->data;
}
static void mesh_vert_idx_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  ((uint32_t *)data)[iter->loop_idx] = (iter->eve) ? BM_elem_index_get(iter->eve) :
                                                     (iter->mr->v_origindex ?
                                                          iter->mr->v_origindex[iter->vert_idx] :
                                                          iter->vert_idx);
}
static void mesh_edge_idx_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  ((uint32_t *)data)[iter->loop_idx] = (iter->eed) ? BM_elem_index_get(iter->eed) :
                                                     (iter->mr->e_origindex ?
                                                          iter->mr->e_origindex[iter->edge_idx] :
                                                          iter->edge_idx);
}
static void mesh_face_idx_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  ((uint32_t *)data)[iter->loop_idx] = (iter->efa) ? BM_elem_index_get(iter->efa) :
                                                     (iter->mr->p_origindex ?
                                                          iter->mr->p_origindex[iter->face_idx] :
                                                          iter->face_idx);
}
static void mesh_fdot_idx_iter(const MeshExtractIterData *iter, void *UNUSED(buf), void *data)
{
  ((uint32_t *)data)[iter->face_idx] = (iter->efa) ? BM_elem_index_get(iter->efa) :
                                                     (iter->mr->p_origindex ?
                                                          iter->mr->p_origindex[iter->face_idx] :
                                                          iter->face_idx);
}

MeshExtract extract_vert_idx = {mesh_select_idx_init,
                                mesh_vert_idx_iter,
                                mesh_vert_idx_iter,
                                NULL,
                                MR_ITER_LOOP | MR_ITER_LEDGE | MR_ITER_LVERT,
                                0};

MeshExtract extract_edge_idx = {mesh_select_idx_init,
                                mesh_edge_idx_iter,
                                mesh_edge_idx_iter,
                                NULL,
                                MR_ITER_LOOP | MR_ITER_LEDGE,
                                0};

MeshExtract extract_face_idx = {
    mesh_select_idx_init, mesh_face_idx_iter, mesh_face_idx_iter, NULL, MR_ITER_LOOP, 0};

MeshExtract extract_fdot_idx = {
    mesh_select_fdot_idx_init, mesh_fdot_idx_iter, mesh_fdot_idx_iter, NULL, MR_ITER_LOOP, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Dummy Extract
 * \{ */

static void *mesh_dummy_vbo_init(const MeshRenderData *UNUSED(mr), void *vbo)
{
  GPUVertFormat format = {0};
  GPU_vertformat_attr_add(&format, "dummy", GPU_COMP_U8, 1, GPU_FETCH_INT_TO_FLOAT_UNIT);
  GPU_vertbuf_init_with_format(vbo, &format);
  GPU_vertbuf_data_alloc(vbo, 3);
  return NULL;
}

MeshExtract extract_dummy_vbo = {mesh_dummy_vbo_init, NULL, NULL, NULL, 0, 0};

static void *mesh_dummy_ibo_init(const MeshRenderData *UNUSED(mr), void *ibo)
{
  GPUIndexBufBuilder elb;
  GPU_indexbuf_init(&elb, GPU_PRIM_TRIS, 1, 3);
  GPU_indexbuf_add_tri_verts(&elb, 0, 1, 2);
  GPU_indexbuf_build_in_place(&elb, ibo);
  return NULL;
}

MeshExtract extract_dummy_ibo = {mesh_dummy_ibo_init, NULL, NULL, NULL, 0, 0};

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Extract Loop
 * \{ */

typedef struct MeshExtractFnRef {
  MeshExtractInitFn *init;
  MeshExtractIterFn *iter;
  MeshExtractFinishFn *finish;
  void *buffer;
  void *user_data;
  int iter_flag;
} MeshExtractFnRef;

typedef struct MeshExtractIterFnRef {
  MeshExtractIterFn *iter;
  void *buffer;
  void *user_data;
} MeshExtractIterFnRef;

BLI_INLINE BMFace *bm_original_face_get(BMesh *bm, int idx)
{
  return ((idx != ORIGINDEX_NONE) ? BM_face_at_index(bm, idx) : NULL);
}

BLI_INLINE BMEdge *bm_original_edge_get(BMesh *bm, int idx)
{
  return ((idx != ORIGINDEX_NONE) ? BM_edge_at_index(bm, idx) : NULL);
}

BLI_INLINE BMVert *bm_original_vert_get(BMesh *bm, int idx)
{
  return ((idx != ORIGINDEX_NONE) ? BM_vert_at_index(bm, idx) : NULL);
}

static int iter_callback_gather(MeshExtractFnRef *extract_refs,
                                MeshExtractIterFnRef *r_iter_callback,
                                int cb_total_len,
                                int iter_flag)
{
  int iter_cb_idx = 0;
  for (int i = 0; i < cb_total_len; i++) {
    if (extract_refs[i].iter_flag & iter_flag) {
      r_iter_callback[iter_cb_idx].iter = extract_refs[i].iter;
      r_iter_callback[iter_cb_idx].user_data = extract_refs[i].user_data;
      r_iter_callback[iter_cb_idx].buffer = extract_refs[i].buffer;
      iter_cb_idx++;
    }
  }
  return iter_cb_idx;
}

static void mesh_extract_init(MeshRenderData *mr, MeshExtractFnRef *extract_refs, int cb_total_len)
{
  for (int i = 0; i < cb_total_len; i++) {
    if (extract_refs[i].init) {
      extract_refs[i].user_data = extract_refs[i].init(mr, extract_refs[i].buffer);
    }
  }
}

static void mesh_extract_finish(MeshRenderData *mr,
                                MeshExtractFnRef *extract_refs,
                                int cb_total_len)
{
  for (int i = 0; i < cb_total_len; i++) {
    if (extract_refs[i].finish) {
      extract_refs[i].finish(mr, extract_refs[i].buffer, extract_refs[i].user_data);
    }
  }
}

static void mesh_extract_iter_looptri(MeshRenderData *mr,
                                      MeshExtractFnRef *extract_refs,
                                      int cb_total_len)
{
  MeshExtractIterFnRef *itr_fn = BLI_array_alloca(itr_fn, cb_total_len);
  int itr_fn_len = iter_callback_gather(extract_refs, itr_fn, cb_total_len, MR_ITER_LOOPTRI);

  if (itr_fn_len == 0) {
    return;
  }

  MeshExtractIterData itr = {NULL};
  itr.mr = mr;
  itr.use_hide = mr->use_hide;

  switch (mr->extract_type) {
    case MR_EXTRACT_BMESH:
      for (itr.tri_idx = 0; itr.tri_idx < mr->tri_len; itr.tri_idx++) {
        itr.elooptri = &mr->edit_bmesh->looptris[itr.tri_idx][0];
        itr.efa = itr.elooptri[0]->f;
        itr.v1 = BM_elem_index_get(itr.elooptri[0]);
        itr.v2 = BM_elem_index_get(itr.elooptri[1]);
        itr.v3 = BM_elem_index_get(itr.elooptri[2]);
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
    case MR_EXTRACT_MAPPED:
      for (itr.tri_idx = 0; itr.tri_idx < mr->tri_len; itr.tri_idx++) {
        itr.mlooptri = &mr->mlooptri[itr.tri_idx];
        itr.face_idx = itr.mlooptri->poly;
        itr.mpoly = &mr->mpoly[itr.face_idx];
        itr.efa = bm_original_face_get(mr->bm, mr->p_origindex[itr.face_idx]);
        itr.v1 = itr.mlooptri->tri[0];
        itr.v2 = itr.mlooptri->tri[1];
        itr.v3 = itr.mlooptri->tri[2];
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
    case MR_EXTRACT_MESH:
      for (itr.tri_idx = 0; itr.tri_idx < mr->tri_len; itr.tri_idx++) {
        itr.mlooptri = &mr->mlooptri[itr.tri_idx];
        itr.face_idx = itr.mlooptri->poly;
        itr.mpoly = &mr->mpoly[itr.face_idx];
        itr.v1 = itr.mlooptri->tri[0];
        itr.v2 = itr.mlooptri->tri[1];
        itr.v3 = itr.mlooptri->tri[2];
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
  }
}

static void mesh_extract_iter_loop(MeshRenderData *mr,
                                   MeshExtractFnRef *extract_refs,
                                   int cb_total_len)
{
  MeshExtractIterFnRef *itr_fn = BLI_array_alloca(itr_fn, cb_total_len);
  int itr_fn_len = iter_callback_gather(extract_refs, itr_fn, cb_total_len, MR_ITER_LOOP);

  if (itr_fn_len == 0) {
    return;
  }

  BMIter iter_efa, iter_loop;
  MeshExtractIterData itr = {NULL};
  itr.mr = mr;
  itr.use_hide = mr->use_hide;

  switch (mr->extract_type) {
    case MR_EXTRACT_BMESH:
      itr.loop_idx = 0;
      /* TODO try multithread opti but this is likely to be faster due to
       * data access pattern & cache coherence against using BM_face_at_index(). */
      BM_ITER_MESH_INDEX (itr.efa, &iter_efa, mr->bm, BM_FACES_OF_MESH, itr.face_idx) {
        BM_ITER_ELEM (itr.eloop, &iter_loop, itr.efa, BM_LOOPS_OF_FACE) {
          itr.eed = itr.eloop->e;
          itr.eve = itr.eloop->v;
          itr.v1 = BM_elem_index_get(itr.eloop);
          itr.v2 = BM_elem_index_get(itr.eloop->next);
          itr.edge_idx = BM_elem_index_get(itr.eed);
          itr.vert_idx = BM_elem_index_get(itr.eve);
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
          itr.loop_idx++;
        }
      }
      break;
    case MR_EXTRACT_MAPPED:
      itr.loop_idx = 0;
      itr.mloop = mr->mloop;
      itr.mpoly = mr->mpoly;
      for (itr.face_idx = 0; itr.face_idx < mr->poly_len; itr.face_idx++, itr.mpoly++) {
        itr.efa = bm_original_face_get(mr->bm, mr->p_origindex[itr.face_idx]);
        int loopend = itr.mpoly->loopstart + itr.mpoly->totloop;
        itr.eed_prev = bm_original_edge_get(mr->bm, mr->e_origindex[mr->mloop[loopend - 1].e]);
        for (itr.v2 = itr.v1 + 1; itr.v1 < loopend;
             itr.v1++, itr.v2++, itr.loop_idx++, itr.mloop++) {
          if (itr.v2 == loopend) {
            itr.v2 = itr.mpoly->loopstart;
          }
          itr.edge_idx = itr.mloop->e;
          itr.vert_idx = itr.mloop->v;
          itr.eed = bm_original_edge_get(mr->bm, mr->e_origindex[itr.edge_idx]);
          itr.eve = bm_original_vert_get(mr->bm, mr->v_origindex[itr.vert_idx]);
          itr.medge = &mr->medge[itr.edge_idx];
          itr.mvert = &mr->mvert[itr.vert_idx];
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
          itr.eed_prev = itr.eed;
        }
      }
      break;
    case MR_EXTRACT_MESH:
      itr.loop_idx = itr.v1 = 0;
      itr.mloop = mr->mloop;
      itr.mpoly = mr->mpoly;
      for (itr.face_idx = 0; itr.face_idx < mr->poly_len; itr.face_idx++, itr.mpoly++) {
        int loopend = itr.mpoly->loopstart + itr.mpoly->totloop;
        for (itr.v2 = itr.v1 + 1; itr.v1 < loopend;
             itr.v1++, itr.v2++, itr.loop_idx++, itr.mloop++) {
          if (itr.v2 == loopend) {
            itr.v2 = itr.mpoly->loopstart;
          }
          itr.edge_idx = itr.mloop->e;
          itr.vert_idx = itr.mloop->v;
          itr.medge = &mr->medge[itr.edge_idx];
          itr.mvert = &mr->mvert[itr.vert_idx];
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
        }
      }
      break;
  }
}

static void mesh_extract_iter_ledge(MeshRenderData *mr,
                                    MeshExtractFnRef *extract_refs,
                                    int cb_total_len)
{
  MeshExtractIterFnRef *itr_fn = BLI_array_alloca(itr_fn, cb_total_len);
  int itr_fn_len = iter_callback_gather(extract_refs, itr_fn, cb_total_len, MR_ITER_LEDGE);

  if (itr_fn_len == 0) {
    return;
  }

  MeshExtractIterData itr = {NULL};
  itr.mr = mr;
  itr.use_hide = mr->use_hide;
  itr.loop_idx = mr->loop_len;

  switch (mr->extract_type) {
    case MR_EXTRACT_BMESH:
      for (int e = 0; e < mr->edge_loose_len; e++) {
        itr.edge_idx = mr->ledges[e];
        itr.eed = BM_edge_at_index(mr->bm, itr.edge_idx);
        itr.v1 = itr.loop_idx;
        itr.v2 = itr.loop_idx + 1;
        for (int v = 0; v < 2; v++, itr.loop_idx++) {
          itr.eve = (v == 0) ? itr.eed->v1 : itr.eed->v2;
          itr.vert_idx = BM_elem_index_get(itr.eve);
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
        }
      }
      break;
    case MR_EXTRACT_MAPPED:
      for (int e = 0; e < mr->edge_loose_len; e++) {
        itr.edge_idx = mr->ledges[e];
        itr.medge = &mr->medge[itr.edge_idx];
        itr.eed = bm_original_edge_get(mr->bm, mr->e_origindex[itr.edge_idx]);
        itr.v1 = itr.loop_idx;
        itr.v2 = itr.loop_idx + 1;
        for (int v = 0; v < 2; v++, itr.loop_idx++) {
          itr.vert_idx = (v == 0) ? itr.medge->v1 : itr.medge->v2;
          itr.mvert = &mr->mvert[itr.vert_idx];
          itr.eve = bm_original_vert_get(mr->bm, mr->v_origindex[itr.vert_idx]);
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
        }
      }
      break;
    case MR_EXTRACT_MESH:
      for (int e = 0; e < mr->edge_loose_len; e++) {
        itr.edge_idx = mr->ledges[e];
        itr.medge = &mr->medge[itr.edge_idx];
        itr.v1 = itr.loop_idx;
        itr.v2 = itr.loop_idx + 1;
        for (int v = 0; v < 2; v++, itr.loop_idx++) {
          itr.vert_idx = (v == 0) ? itr.medge->v1 : itr.medge->v2;
          itr.mvert = &mr->mvert[itr.vert_idx];
          for (int fn = 0; fn < itr_fn_len; fn++) {
            itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
          }
        }
      }
      break;
  }
}

static void mesh_extract_iter_lvert(MeshRenderData *mr,
                                    MeshExtractFnRef *extract_refs,
                                    int cb_total_len)
{
  MeshExtractIterFnRef *itr_fn = BLI_array_alloca(itr_fn, cb_total_len);
  int itr_fn_len = iter_callback_gather(extract_refs, itr_fn, cb_total_len, MR_ITER_LVERT);

  if (itr_fn_len == 0) {
    return;
  }

  MeshExtractIterData itr = {NULL};
  itr.mr = mr;
  itr.use_hide = mr->use_hide;
  itr.loop_idx = mr->loop_len + mr->edge_loose_len * 2;

  switch (mr->extract_type) {
    case MR_EXTRACT_BMESH:
      for (int v = 0; v < mr->vert_loose_len; v++, itr.loop_idx++) {
        itr.vert_idx = mr->lverts[v];
        itr.eve = BM_vert_at_index(mr->bm, itr.vert_idx);
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
    case MR_EXTRACT_MAPPED:
      for (int v = 0; v < mr->vert_loose_len; v++, itr.loop_idx++) {
        itr.vert_idx = mr->lverts[v];
        itr.mvert = &mr->mvert[itr.vert_idx];
        itr.eve = bm_original_vert_get(mr->bm, mr->v_origindex[itr.vert_idx]);
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
    case MR_EXTRACT_MESH:
      for (int v = 0; v < mr->vert_loose_len; ++v, itr.loop_idx++) {
        itr.vert_idx = mr->lverts[v];
        itr.mvert = &mr->mvert[itr.vert_idx];
        for (int fn = 0; fn < itr_fn_len; fn++) {
          itr_fn[fn].iter(&itr, itr_fn[fn].buffer, itr_fn[fn].user_data);
        }
      }
      break;
  }
}

static void mesh_buffer_cache_create_requested(MeshBatchCache *cache,
                                               MeshBufferCache mbc,
                                               Mesh *me,
                                               const bool do_final,
                                               const bool use_subsurf_fdots,
                                               const DRW_MeshCDMask *cd_layer_used,
                                               const ToolSettings *ts,
                                               const bool use_hide)
{
  /* A bit nasty but at least it is clear. */
  union {
    MeshBufferCache infos;
    MeshExtract *fn[0];
  } extract = {{{
                    .pos_nor = (void *)&extract_pos_nor,
                    .lnor = (void *)&extract_lnor,
                    .edge_fac = (void *)&extract_edge_fac,
                    .weights = (void *)&extract_weights,
                    .uv_tan = (void *)&extract_uv_tan,
                    .vcol = (void *)&extract_vcol,
                    .orco = (void *)&extract_orco,

                    .data = (void *)&extract_edit_data,
                    .data_edituv = (void *)&extract_edituv_data,
                    .stretch_area = (void *)&extract_stretch_area,
                    .stretch_angle = (void *)&extract_stretch_angle,
                    .mesh_analysis = (void *)&extract_mesh_analysis,
                    .facedots_pos = (void *)&extract_facedots_pos,
                    .facedots_nor = (void *)&extract_facedots_nor,
                    .facedots_uv = (void *)&extract_facedots_uv,
                    .facedots_data = (void *)&extract_dummy_vbo, /* Not used for now. */
                    .facedots_data_edituv = (void *)&extract_facedots_edituv_data,

                    .vert_idx = (void *)&extract_vert_idx,
                    .edge_idx = (void *)&extract_edge_idx,
                    .face_idx = (void *)&extract_face_idx,
                    .facedots_idx = (void *)&extract_fdot_idx,
                },
                {
                    .tris = (void *)&extract_tris,
                    .lines = (void *)&extract_lines,
                    .points = (void *)&extract_points,
                    .facedots = (void *)&extract_facedots,

                    .lines_paint_mask = (void *)&extract_lines_paint_mask,
                    .lines_adjacency = (void *)&extract_lines_adjacency,

                    .edituv_tris = (void *)&extract_edituv_tris,
                    .edituv_lines = (void *)&extract_edituv_lines,
                    .edituv_points = (void *)&extract_edituv_points,
                    .edituv_facedots = (void *)&extract_edituv_facedots,
                }}};

  eMRIterType iter_flag = 0;
  eMRDataType data_flag = 0;
  int cb_total_len = 0;

  int j = 0;
  for (int i = 0; i < sizeof(mbc.vbo) / sizeof(void *); i++, j++) {
    GPUVertBuf **vbo = (GPUVertBuf **)&mbc.vbo;
    if (DRW_TEST_ASSIGN_VBO(vbo[i])) {
      iter_flag |= extract.fn[j]->iter_flag;
      data_flag |= extract.fn[j]->data_flag;
      cb_total_len++;
    }
  }
  for (int i = 0; i < sizeof(mbc.ibo) / sizeof(void *); i++, j++) {
    GPUIndexBuf **ibo = (GPUIndexBuf **)&mbc.ibo;
    if (DRW_TEST_ASSIGN_IBO(ibo[i])) {
      iter_flag |= extract.fn[j]->iter_flag;
      data_flag |= extract.fn[j]->data_flag;
      cb_total_len++;
    }
  }

  MeshExtractFnRef *extract_refs = BLI_array_alloca(extract_refs, cb_total_len);

  MeshRenderData *mr = mesh_render_data_create(
      me, do_final, iter_flag, data_flag, cd_layer_used, ts);
  mr->cache = cache; /* HACK */
  mr->use_hide = use_hide;
  mr->use_subsurf_fdots = use_subsurf_fdots;

  int ref_id = 0;
  for (int i = 0; i < sizeof(mbc) / sizeof(void *); i++) {
    BLI_assert(extract.fn[i] != NULL);
    /* HACK to iter over all VBOs/IBOs. */
    if (((void **)&mbc)[i] != NULL) {
      extract_refs[ref_id].init = extract.fn[i]->init;
      if (mr->extract_type == MR_EXTRACT_MESH) {
        extract_refs[ref_id].iter = extract.fn[i]->iter;
      }
      else {
        extract_refs[ref_id].iter = extract.fn[i]->iter_edit;
      }
      extract_refs[ref_id].finish = extract.fn[i]->finish;
      extract_refs[ref_id].iter_flag = extract.fn[i]->iter_flag;
      extract_refs[ref_id].buffer = ((void **)&mbc)[i];
      ref_id++;
    }
  }

  mesh_extract_init(mr, extract_refs, cb_total_len);
  mesh_extract_iter_looptri(mr, extract_refs, cb_total_len);
  mesh_extract_iter_loop(mr, extract_refs, cb_total_len);
  mesh_extract_iter_ledge(mr, extract_refs, cb_total_len);
  mesh_extract_iter_lvert(mr, extract_refs, cb_total_len);
  mesh_extract_finish(mr, extract_refs, cb_total_len);

  mesh_render_data_free(mr);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Public API
 * \{ */

static void texpaint_request_active_uv(MeshBatchCache *cache, Mesh *me)
{
  DRW_MeshCDMask cd_needed;
  mesh_cd_layers_type_clear(&cd_needed);
  mesh_cd_calc_active_uv_layer(me, &cd_needed);

  BLI_assert(cd_needed.uv != 0 &&
             "No uv layer available in texpaint, but batches requested anyway!");

  mesh_cd_calc_active_mask_uv_layer(me, &cd_needed);
  mesh_cd_layers_type_merge(&cache->cd_needed, cd_needed);
}

static void texpaint_request_active_vcol(MeshBatchCache *cache, Mesh *me)
{
  DRW_MeshCDMask cd_needed;
  mesh_cd_layers_type_clear(&cd_needed);
  mesh_cd_calc_active_vcol_layer(me, &cd_needed);

  BLI_assert(cd_needed.vcol != 0 &&
             "No vcol layer available in vertpaint, but batches requested anyway!");

  mesh_cd_layers_type_merge(&cache->cd_needed, cd_needed);
}

GPUBatch *DRW_mesh_batch_cache_get_all_verts(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_ALL_VERTS);
  return DRW_batch_request(&cache->batch.all_verts);
}

GPUBatch *DRW_mesh_batch_cache_get_all_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_ALL_EDGES);
  return DRW_batch_request(&cache->batch.all_edges);
}

GPUBatch *DRW_mesh_batch_cache_get_surface(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_SURFACE);
  return DRW_batch_request(&cache->batch.surface);
}

GPUBatch *DRW_mesh_batch_cache_get_loose_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_LOOSE_EDGES);
  if (cache->no_loose_wire) {
    return NULL;
  }
  else {
    return DRW_batch_request(&cache->batch.loose_edges);
  }
}

GPUBatch *DRW_mesh_batch_cache_get_surface_weights(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_SURFACE_WEIGHTS);
  return DRW_batch_request(&cache->batch.surface_weights);
}

GPUBatch *DRW_mesh_batch_cache_get_edge_detection(Mesh *me, bool *r_is_manifold)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDGE_DETECTION);
  /* Even if is_manifold is not correct (not updated),
   * the default (not manifold) is just the worst case. */
  if (r_is_manifold) {
    *r_is_manifold = cache->is_manifold;
  }
  return DRW_batch_request(&cache->batch.edge_detection);
}

GPUBatch *DRW_mesh_batch_cache_get_wireframes_face(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_WIRE_EDGES);
  return DRW_batch_request(&cache->batch.wire_edges);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_mesh_analysis(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_MESH_ANALYSIS);
  return DRW_batch_request(&cache->batch.edit_mesh_analysis);
}

GPUBatch **DRW_mesh_batch_cache_get_surface_shaded(Mesh *me,
                                                   struct GPUMaterial **gpumat_array,
                                                   uint gpumat_array_len,
                                                   char **auto_layer_names,
                                                   int **auto_layer_is_srgb,
                                                   int *auto_layer_count)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  DRW_MeshCDMask cd_needed = mesh_cd_calc_used_gpu_layers(me, gpumat_array, gpumat_array_len);

  BLI_assert(gpumat_array_len == cache->mat_len);

  mesh_cd_layers_type_merge(&cache->cd_needed, cd_needed);

  if (!mesh_cd_layers_type_overlap(cache->cd_used, cd_needed)) {
    mesh_cd_extract_auto_layers_names_and_srgb(me,
                                               cache->cd_needed,
                                               &cache->auto_layer_names,
                                               &cache->auto_layer_is_srgb,
                                               &cache->auto_layer_len);
  }

  mesh_batch_cache_add_request(cache, MBC_SURF_PER_MAT);

  if (auto_layer_names) {
    *auto_layer_names = cache->auto_layer_names;
    *auto_layer_is_srgb = cache->auto_layer_is_srgb;
    *auto_layer_count = cache->auto_layer_len;
  }
  for (int i = 0; i < cache->mat_len; ++i) {
    DRW_batch_request(&cache->surface_per_mat[i]);
  }
  return cache->surface_per_mat;
}

GPUBatch **DRW_mesh_batch_cache_get_surface_texpaint(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_SURF_PER_MAT);
  texpaint_request_active_uv(cache, me);
  for (int i = 0; i < cache->mat_len; ++i) {
    DRW_batch_request(&cache->surface_per_mat[i]);
  }
  return cache->surface_per_mat;
}

GPUBatch *DRW_mesh_batch_cache_get_surface_texpaint_single(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_SURFACE);
  return DRW_batch_request(&cache->batch.surface);
}

GPUBatch *DRW_mesh_batch_cache_get_surface_vertpaint(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_vcol(cache, me);
  mesh_batch_cache_add_request(cache, MBC_SURFACE);
  return DRW_batch_request(&cache->batch.surface);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Edit Mode API
 * \{ */

GPUBatch *DRW_mesh_batch_cache_get_edit_triangles(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_TRIANGLES);
  return DRW_batch_request(&cache->batch.edit_triangles);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_EDGES);
  return DRW_batch_request(&cache->batch.edit_edges);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_vertices(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_VERTICES);
  return DRW_batch_request(&cache->batch.edit_vertices);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_vnors(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_VNOR);
  return DRW_batch_request(&cache->batch.edit_vnor);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_lnors(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_LNOR);
  return DRW_batch_request(&cache->batch.edit_lnor);
}

GPUBatch *DRW_mesh_batch_cache_get_edit_facedots(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_FACEDOTS);
  return DRW_batch_request(&cache->batch.edit_facedots);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Edit Mode selection API
 * \{ */

GPUBatch *DRW_mesh_batch_cache_get_triangles_with_select_id(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_SELECTION_FACES);
  return DRW_batch_request(&cache->batch.edit_selection_faces);
}

GPUBatch *DRW_mesh_batch_cache_get_facedots_with_select_id(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_SELECTION_FACEDOTS);
  return DRW_batch_request(&cache->batch.edit_selection_facedots);
}

GPUBatch *DRW_mesh_batch_cache_get_edges_with_select_id(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_SELECTION_EDGES);
  return DRW_batch_request(&cache->batch.edit_selection_edges);
}

GPUBatch *DRW_mesh_batch_cache_get_verts_with_select_id(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  mesh_batch_cache_add_request(cache, MBC_EDIT_SELECTION_VERTS);
  return DRW_batch_request(&cache->batch.edit_selection_verts);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name UV Image editor API
 * \{ */

GPUBatch *DRW_mesh_batch_cache_get_edituv_faces_strech_area(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_FACES_STRECH_AREA);
  return DRW_batch_request(&cache->batch.edituv_faces_strech_area);
}

GPUBatch *DRW_mesh_batch_cache_get_edituv_faces_strech_angle(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_FACES_STRECH_ANGLE);
  return DRW_batch_request(&cache->batch.edituv_faces_strech_angle);
}

GPUBatch *DRW_mesh_batch_cache_get_edituv_faces(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_FACES);
  return DRW_batch_request(&cache->batch.edituv_faces);
}

GPUBatch *DRW_mesh_batch_cache_get_edituv_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_EDGES);
  return DRW_batch_request(&cache->batch.edituv_edges);
}

GPUBatch *DRW_mesh_batch_cache_get_edituv_verts(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_VERTS);
  return DRW_batch_request(&cache->batch.edituv_verts);
}

GPUBatch *DRW_mesh_batch_cache_get_edituv_facedots(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_EDITUV_FACEDOTS);
  return DRW_batch_request(&cache->batch.edituv_facedots);
}

GPUBatch *DRW_mesh_batch_cache_get_uv_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_WIRE_LOOPS_UVS);
  return DRW_batch_request(&cache->batch.wire_loops_uvs);
}

GPUBatch *DRW_mesh_batch_cache_get_surface_edges(Mesh *me)
{
  MeshBatchCache *cache = mesh_batch_cache_get(me);
  texpaint_request_active_uv(cache, me);
  mesh_batch_cache_add_request(cache, MBC_WIRE_LOOPS);
  return DRW_batch_request(&cache->batch.wire_loops);
}

/** \} */

/* ---------------------------------------------------------------------- */
/** \name Grouped batch generation
 * \{ */

/* Thread safety need to be assured by caller. Don't call this during drawing.
 * Note: For now this only free the shading batches / vbo if any cd layers is
 * not needed anymore. */
void DRW_mesh_batch_cache_free_old(Mesh *me, int ctime)
{
  MeshBatchCache *cache = me->runtime.batch_cache;

  if (cache == NULL) {
    return;
  }

  if (mesh_cd_layers_type_equal(cache->cd_used_over_time, cache->cd_used)) {
    cache->lastmatch = ctime;
  }

  if (ctime - cache->lastmatch > U.vbotimeout) {
    mesh_batch_cache_discard_shaded_tri(cache);
  }

  mesh_cd_layers_type_clear(&cache->cd_used_over_time);
}

/* Can be called for any surface type. Mesh *me is the final mesh. */
void DRW_mesh_batch_cache_create_requested(
    Object *ob, Mesh *me, const Scene *scene, const bool is_paint_mode, const bool use_hide)
{
  const ToolSettings *ts = scene->toolsettings;
  MeshBatchCache *cache = mesh_batch_cache_get(me);

  /* Early out */
  if (cache->batch_requested == 0) {
#ifdef DEBUG
    goto check;
#endif
    return;
  }

  DRWBatchFlag batch_requested = cache->batch_requested;
  cache->batch_requested = 0;

  if (batch_requested & MBC_SURFACE_WEIGHTS) {
    /* Check vertex weights. */
    if ((cache->batch.surface_weights != NULL) && (ts != NULL)) {
      struct DRW_MeshWeightState wstate;
      BLI_assert(ob->type == OB_MESH);
      drw_mesh_weight_state_extract(ob, me, ts, is_paint_mode, &wstate);
      mesh_batch_cache_check_vertex_group(cache, &wstate);
      drw_mesh_weight_state_copy(&cache->weight_state, &wstate);
      drw_mesh_weight_state_clear(&wstate);
    }
  }

  if (batch_requested &
      (MBC_SURFACE | MBC_SURF_PER_MAT | MBC_WIRE_LOOPS_UVS | MBC_EDITUV_FACES_STRECH_AREA |
       MBC_EDITUV_FACES_STRECH_ANGLE | MBC_EDITUV_FACES | MBC_EDITUV_EDGES | MBC_EDITUV_VERTS)) {
    /* Modifiers will only generate an orco layer if the mesh is deformed. */
    if (cache->cd_needed.orco != 0) {
      if (CustomData_get_layer(&me->vdata, CD_ORCO) != NULL) {
        /* Orco layer is needed. */
      }
      else if (cache->cd_needed.tan_orco == 0) {
        /* Skip orco calculation if not needed by tangent generation.
         */
        cache->cd_needed.orco = 0;
      }
    }

    /* Verify that all surface batches have needed attribute layers.
     */
    /* TODO(fclem): We could be a bit smarter here and only do it per
     * material. */
    bool cd_overlap = mesh_cd_layers_type_overlap(cache->cd_used, cache->cd_needed);
    if (cd_overlap == false) {
      FOREACH_MESH_BUFFER_CACHE(cache, mbuffercache)
      {
        if ((cache->cd_used.uv & cache->cd_needed.uv) != cache->cd_needed.uv ||
            (cache->cd_used.tan & cache->cd_needed.tan) != cache->cd_needed.tan ||
            cache->cd_used.tan_orco != cache->cd_needed.tan_orco) {
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.uv_tan);
        }
        if (cache->cd_used.orco != cache->cd_needed.orco) {
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.orco);
        }
        if ((cache->cd_used.vcol & cache->cd_needed.vcol) != cache->cd_needed.vcol) {
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.vcol);
        }
      }
      /* We can't discard batches at this point as they have been
       * referenced for drawing. Just clear them in place. */
      for (int i = 0; i < cache->mat_len; ++i) {
        GPU_BATCH_CLEAR_SAFE(cache->surface_per_mat[i]);
      }
      GPU_BATCH_CLEAR_SAFE(cache->batch.surface);
      cache->batch_ready &= ~(MBC_SURFACE | MBC_SURF_PER_MAT);

      mesh_cd_layers_type_merge(&cache->cd_used, cache->cd_needed);
    }
    mesh_cd_layers_type_merge(&cache->cd_used_over_time, cache->cd_needed);
    mesh_cd_layers_type_clear(&cache->cd_needed);
  }

  if (batch_requested & MBC_EDITUV) {
    /* Discard UV batches if sync_selection changes */
    if (ts != NULL) {
      const bool is_uvsyncsel = (ts->uv_flag & UV_SYNC_SELECTION);
      if (cache->is_uvsyncsel != is_uvsyncsel) {
        cache->is_uvsyncsel = is_uvsyncsel;
        FOREACH_MESH_BUFFER_CACHE(cache, mbuffercache)
        {
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.data_edituv);
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.stretch_angle);
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.stretch_area);
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.uv_tan);
          GPU_VERTBUF_DISCARD_SAFE(mbuffercache->vbo.facedots_uv);
          GPU_INDEXBUF_DISCARD_SAFE(mbuffercache->ibo.edituv_tris);
          GPU_INDEXBUF_DISCARD_SAFE(mbuffercache->ibo.edituv_lines);
          GPU_INDEXBUF_DISCARD_SAFE(mbuffercache->ibo.edituv_points);
        }
        /* We only clear the batches as they may already have been
         * referenced. */
        GPU_BATCH_CLEAR_SAFE(cache->batch.wire_loops_uvs);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_faces_strech_area);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_faces_strech_angle);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_faces);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_edges);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_verts);
        GPU_BATCH_CLEAR_SAFE(cache->batch.edituv_facedots);
        cache->batch_ready &= ~MBC_EDITUV;
      }
    }
  }

  /* Second chance to early out */
  if ((batch_requested & ~cache->batch_ready) == 0) {
#ifdef DEBUG
    goto check;
#endif
    return;
  }

  cache->batch_ready |= batch_requested;

  const bool do_cage = (me->edit_mesh &&
                        (me->edit_mesh->mesh_eval_final != me->edit_mesh->mesh_eval_cage));

  MeshBufferCache *mbufcache = &cache->final;

  /* Init batches and request VBOs & IBOs */
  if (DRW_batch_requested(cache->batch.surface, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.surface, &mbufcache->ibo.tris);
    /* Order matters. First ones override latest vbos' attribs. */
    DRW_vbo_request(cache->batch.surface, &mbufcache->vbo.lnor);
    DRW_vbo_request(cache->batch.surface, &mbufcache->vbo.pos_nor);
    if (cache->cd_used.uv != 0) {
      DRW_vbo_request(cache->batch.surface, &mbufcache->vbo.uv_tan);
    }
    if (cache->cd_used.vcol != 0) {
      DRW_vbo_request(cache->batch.surface, &mbufcache->vbo.vcol);
    }
  }
  if (DRW_batch_requested(cache->batch.all_verts, GPU_PRIM_POINTS)) {
    DRW_vbo_request(cache->batch.all_verts, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.all_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.all_edges, &mbufcache->ibo.lines);
    DRW_vbo_request(cache->batch.all_edges, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.loose_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.loose_edges, &mbufcache->ibo.lines);
    DRW_vbo_request(cache->batch.loose_edges, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.edge_detection, GPU_PRIM_LINES_ADJ)) {
    DRW_ibo_request(cache->batch.edge_detection, &mbufcache->ibo.lines_adjacency);
    DRW_vbo_request(cache->batch.edge_detection, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.surface_weights, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.surface_weights, &mbufcache->ibo.tris);
    DRW_vbo_request(cache->batch.surface_weights, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.surface_weights, &mbufcache->vbo.weights);
  }
  if (DRW_batch_requested(cache->batch.wire_loops, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.wire_loops, &mbufcache->ibo.lines_paint_mask);
    DRW_vbo_request(cache->batch.wire_loops, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.wire_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.wire_edges, &mbufcache->ibo.lines);
    DRW_vbo_request(cache->batch.wire_edges, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.wire_edges, &mbufcache->vbo.edge_fac);
  }
  if (DRW_batch_requested(cache->batch.wire_loops_uvs, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.wire_loops_uvs, &mbufcache->ibo.edituv_lines);
    /* For paint overlay. Active layer should have been queried. */
    if (cache->cd_used.uv != 0) {
      DRW_vbo_request(cache->batch.wire_loops_uvs, &mbufcache->vbo.uv_tan);
    }
  }
  if (DRW_batch_requested(cache->batch.edit_mesh_analysis, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edit_mesh_analysis, &mbufcache->ibo.tris);
    DRW_vbo_request(cache->batch.edit_mesh_analysis, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_mesh_analysis, &mbufcache->vbo.mesh_analysis);
  }

  /* Per Material */
  for (int i = 0; i < cache->mat_len; ++i) {
    if (DRW_batch_requested(cache->surface_per_mat[i], GPU_PRIM_TRIS)) {
      DRW_ibo_request(cache->surface_per_mat[i], &mbufcache->ibo.tris);
      /* Order matters. First ones override latest vbos' attribs. */
      DRW_vbo_request(cache->surface_per_mat[i], &mbufcache->vbo.lnor);
      DRW_vbo_request(cache->surface_per_mat[i], &mbufcache->vbo.pos_nor);
      if ((cache->cd_used.uv != 0) || (cache->cd_used.tan != 0) ||
          (cache->cd_used.tan_orco != 0)) {
        DRW_vbo_request(cache->surface_per_mat[i], &mbufcache->vbo.uv_tan);
      }
      if (cache->cd_used.vcol != 0) {
        DRW_vbo_request(cache->surface_per_mat[i], &mbufcache->vbo.vcol);
      }
      if (cache->cd_used.orco != 0) {
        DRW_vbo_request(cache->surface_per_mat[i], &mbufcache->vbo.orco);
      }
    }
  }

  mbufcache = (do_cage) ? &cache->cage : &cache->final;

  /* Edit Mesh */
  if (DRW_batch_requested(cache->batch.edit_triangles, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edit_triangles, &mbufcache->ibo.tris);
    DRW_vbo_request(cache->batch.edit_triangles, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_triangles, &mbufcache->vbo.data);
  }
  if (DRW_batch_requested(cache->batch.edit_vertices, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_vertices, &mbufcache->ibo.points);
    DRW_vbo_request(cache->batch.edit_vertices, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_vertices, &mbufcache->vbo.data);
  }
  if (DRW_batch_requested(cache->batch.edit_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.edit_edges, &mbufcache->ibo.lines);
    DRW_vbo_request(cache->batch.edit_edges, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_edges, &mbufcache->vbo.data);
  }
  if (DRW_batch_requested(cache->batch.edit_vnor, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_vnor, &mbufcache->ibo.points);
    DRW_vbo_request(cache->batch.edit_vnor, &mbufcache->vbo.pos_nor);
  }
  if (DRW_batch_requested(cache->batch.edit_lnor, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_lnor, &mbufcache->ibo.tris);
    DRW_vbo_request(cache->batch.edit_lnor, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_lnor, &mbufcache->vbo.lnor);
  }
  if (DRW_batch_requested(cache->batch.edit_facedots, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_facedots, &mbufcache->ibo.facedots);
    DRW_vbo_request(cache->batch.edit_facedots, &mbufcache->vbo.facedots_pos);
    DRW_vbo_request(cache->batch.edit_facedots, &mbufcache->vbo.facedots_nor);
    DRW_vbo_request(cache->batch.edit_facedots, &mbufcache->vbo.facedots_data);
  }

  /* Selection */
  if (DRW_batch_requested(cache->batch.edit_selection_verts, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_selection_verts, &mbufcache->ibo.points);
    DRW_vbo_request(cache->batch.edit_selection_verts, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_selection_verts, &mbufcache->vbo.vert_idx);
  }
  if (DRW_batch_requested(cache->batch.edit_selection_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.edit_selection_edges, &mbufcache->ibo.lines);
    DRW_vbo_request(cache->batch.edit_selection_edges, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_selection_edges, &mbufcache->vbo.edge_idx);
  }
  if (DRW_batch_requested(cache->batch.edit_selection_faces, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edit_selection_faces, &mbufcache->ibo.tris);
    DRW_vbo_request(cache->batch.edit_selection_faces, &mbufcache->vbo.pos_nor);
    DRW_vbo_request(cache->batch.edit_selection_faces, &mbufcache->vbo.face_idx);
  }
  if (DRW_batch_requested(cache->batch.edit_selection_facedots, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edit_selection_facedots, &mbufcache->ibo.facedots);
    DRW_vbo_request(cache->batch.edit_selection_facedots, &mbufcache->vbo.facedots_pos);
    DRW_vbo_request(cache->batch.edit_selection_facedots, &mbufcache->vbo.facedots_idx);
  }

  /**
   * TODO: The code and data structure is ready to support modified UV display
   * but the selection code for UVs needs to support it first. So for now, only
   * display the cage in all cases.
   */
  /* FIXME We use the same cage as edit mesh cage now. We need to fix selection ASAP! */
  // mbufcache = &cache->cage;

  /* Edit UV */
  if (DRW_batch_requested(cache->batch.edituv_faces, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edituv_faces, &mbufcache->ibo.edituv_tris);
    DRW_vbo_request(cache->batch.edituv_faces, &mbufcache->vbo.uv_tan);
    DRW_vbo_request(cache->batch.edituv_faces, &mbufcache->vbo.data_edituv);
  }
  if (DRW_batch_requested(cache->batch.edituv_faces_strech_area, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edituv_faces_strech_area, &mbufcache->ibo.edituv_tris);
    DRW_vbo_request(cache->batch.edituv_faces_strech_area, &mbufcache->vbo.uv_tan);
    DRW_vbo_request(cache->batch.edituv_faces_strech_area, &mbufcache->vbo.data_edituv);
    DRW_vbo_request(cache->batch.edituv_faces_strech_area, &mbufcache->vbo.stretch_area);
  }
  if (DRW_batch_requested(cache->batch.edituv_faces_strech_angle, GPU_PRIM_TRIS)) {
    DRW_ibo_request(cache->batch.edituv_faces_strech_angle, &mbufcache->ibo.edituv_tris);
    DRW_vbo_request(cache->batch.edituv_faces_strech_angle, &mbufcache->vbo.uv_tan);
    DRW_vbo_request(cache->batch.edituv_faces_strech_angle, &mbufcache->vbo.data_edituv);
    DRW_vbo_request(cache->batch.edituv_faces_strech_angle, &mbufcache->vbo.stretch_angle);
  }
  if (DRW_batch_requested(cache->batch.edituv_edges, GPU_PRIM_LINES)) {
    DRW_ibo_request(cache->batch.edituv_edges, &mbufcache->ibo.edituv_lines);
    DRW_vbo_request(cache->batch.edituv_edges, &mbufcache->vbo.uv_tan);
    DRW_vbo_request(cache->batch.edituv_edges, &mbufcache->vbo.data_edituv);
  }
  if (DRW_batch_requested(cache->batch.edituv_verts, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edituv_verts, &mbufcache->ibo.edituv_points);
    DRW_vbo_request(cache->batch.edituv_verts, &mbufcache->vbo.uv_tan);
    DRW_vbo_request(cache->batch.edituv_verts, &mbufcache->vbo.data_edituv);
  }
  if (DRW_batch_requested(cache->batch.edituv_facedots, GPU_PRIM_POINTS)) {
    DRW_ibo_request(cache->batch.edituv_facedots, &mbufcache->ibo.edituv_facedots);
    DRW_vbo_request(cache->batch.edituv_facedots, &mbufcache->vbo.facedots_uv);
    DRW_vbo_request(cache->batch.edituv_facedots, &mbufcache->vbo.facedots_data_edituv);
  }

  /* Meh loose Scene const correctness here. */
  const bool use_subsurf_fdots = scene ? modifiers_usesSubsurfFacedots((Scene *)scene, ob) : false;

  mesh_buffer_cache_create_requested(
      cache, cache->final, me, true, use_subsurf_fdots, &cache->cd_used, ts, use_hide);

  /* Init index buffer subranges. */
  if (cache->surface_per_mat[0] && (cache->surface_per_mat[0]->elem == cache->final.ibo.tris)) {
    BLI_assert(cache->tri_mat_start && cache->tri_mat_end);
    for (int i = 0; i < cache->mat_len; ++i) {
      /* Multiply by 3 because these are triangle indices. */
      int start = cache->tri_mat_start[i] * 3;
      int len = cache->tri_mat_end[i] * 3 - cache->tri_mat_start[i] * 3;
      GPUIndexBuf *sub_ibo = GPU_indexbuf_create_subrange(cache->final.ibo.tris, start, len);
      GPU_batch_elembuf_set(cache->surface_per_mat[i], sub_ibo, true);
    }
  }
  if (cache->batch.loose_edges && (cache->batch.loose_edges->elem == cache->final.ibo.lines)) {
    for (int i = 0; i < cache->mat_len; ++i) {
      /* Multiply by 2 because these are triangle indices. */
      int start = cache->edge_loose_start * 2;
      int len = cache->edge_loose_end * 2 - cache->edge_loose_start * 2;
      GPUIndexBuf *sub_ibo = GPU_indexbuf_create_subrange(cache->final.ibo.lines, start, len);
      GPU_batch_elembuf_set(cache->batch.loose_edges, sub_ibo, true);
    }
  }

  if (do_cage) {
    mesh_buffer_cache_create_requested(
        cache, cache->cage, me, false, use_subsurf_fdots, &cache->cd_used, ts, true);
  }

#ifdef DEBUG
check:
  /* Make sure all requested batches have been setup. */
  for (int i = 0; i < sizeof(cache->batch) / sizeof(void *); ++i) {
    BLI_assert(!DRW_batch_requested(((GPUBatch **)&cache->batch)[i], 0));
  }
  for (int i = 0; i < sizeof(cache->final.vbo) / sizeof(void *); ++i) {
    BLI_assert(!DRW_vbo_requested(((GPUVertBuf **)&cache->final.vbo)[i]));
  }
  for (int i = 0; i < sizeof(cache->final.ibo) / sizeof(void *); ++i) {
    BLI_assert(!DRW_ibo_requested(((GPUIndexBuf **)&cache->final.ibo)[i]));
  }
  for (int i = 0; i < sizeof(cache->cage.vbo) / sizeof(void *); ++i) {
    BLI_assert(!DRW_vbo_requested(((GPUVertBuf **)&cache->cage.vbo)[i]));
  }
  for (int i = 0; i < sizeof(cache->cage.ibo) / sizeof(void *); ++i) {
    BLI_assert(!DRW_ibo_requested(((GPUIndexBuf **)&cache->cage.ibo)[i]));
  }
#endif
}

/** \} */
