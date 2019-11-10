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
 * Copyright 2019, Blender Foundation.
 */

/** \file
 * \ingroup draw_engine
 */

#include "DNA_mesh_types.h"
#include "DNA_view3d_types.h"

#include "BKE_editmesh.h"
#include "BKE_global.h"
#include "BKE_object.h"
#include "BKE_paint.h"

#include "BLI_hash.h"

#include "GPU_shader.h"
#include "DRW_render.h"

#include "ED_view3d.h"

#include "overlay_private.h"

void OVERLAY_wireframe_init(OVERLAY_Data *vedata)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  vedata->stl->pd->view_wires = DRW_view_create_with_zoffset(draw_ctx->rv3d, 0.5f);
}

void OVERLAY_wireframe_cache_init(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  const DRWContextState *draw_ctx = DRW_context_state_get();

  const bool use_select = (DRW_state_is_select() || DRW_state_is_depth());
  GPUShader *wires_sh = use_select ? OVERLAY_shader_wireframe_select() :
                                     OVERLAY_shader_wireframe();

  DRWState state = DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL |
                   DRW_STATE_STENCIL_EQUAL | DRW_STATE_FIRST_VERTEX_CONVENTION;
  DRW_PASS_CREATE(psl->wireframe_ps, state | pd->clipping_state);

  DRWState state_xray = DRW_STATE_WRITE_DEPTH | DRW_STATE_WRITE_STENCIL |
                        DRW_STATE_DEPTH_GREATER_EQUAL | DRW_STATE_STENCIL_NEQUAL |
                        DRW_STATE_FIRST_VERTEX_CONVENTION;
  DRW_PASS_CREATE(psl->wireframe_xray_ps, state_xray | pd->clipping_state);

  pd->wires_grp = DRW_shgroup_create(wires_sh, psl->wireframe_ps);
  pd->wires_xray_grp = DRW_shgroup_create(wires_sh, psl->wireframe_xray_ps);
  pd->clear_stencil = (draw_ctx->v3d->shading.type > OB_SOLID);
  pd->shdata.wire_step_param = pd->overlay.wireframe_threshold - 254.0f / 255.0f;
}

static void wire_color_get(const View3D *v3d,
                           const Object *ob,
                           const bool use_coloring,
                           float **rim_col,
                           float **wire_col)
{
#ifndef NDEBUG
  *rim_col = NULL;
  *wire_col = NULL;
#endif
  const DRWContextState *draw_ctx = DRW_context_state_get();

  if (UNLIKELY(ob->base_flag & BASE_FROM_SET)) {
    *rim_col = G_draw.block.colorDupli;
    *wire_col = G_draw.block.colorDupli;
  }
  else if (UNLIKELY(ob->base_flag & BASE_FROM_DUPLI)) {
    if (ob->base_flag & BASE_SELECTED) {
      if (G.moving & G_TRANSFORM_OBJ) {
        *rim_col = G_draw.block.colorTransform;
      }
      else {
        *rim_col = G_draw.block.colorDupliSelect;
      }
    }
    else {
      *rim_col = G_draw.block.colorDupli;
    }
    *wire_col = G_draw.block.colorDupli;
  }
  else if ((ob->base_flag & BASE_SELECTED) && use_coloring) {
    if (G.moving & G_TRANSFORM_OBJ) {
      *rim_col = G_draw.block.colorTransform;
    }
    else if (ob == draw_ctx->obact) {
      *rim_col = G_draw.block.colorActive;
    }
    else {
      *rim_col = G_draw.block.colorSelect;
    }
    *wire_col = G_draw.block.colorWire;
  }
  else {
    *rim_col = G_draw.block.colorWire;
    *wire_col = G_draw.block.colorBackground;
  }

  if (v3d->shading.type == OB_WIRE) {
    if (ELEM(v3d->shading.wire_color_type, V3D_SHADING_OBJECT_COLOR, V3D_SHADING_RANDOM_COLOR)) {
      /* Theses stays valid until next call. So we need to copy them when using them as uniform. */
      static float wire_col_val[3], rim_col_val[3];
      *wire_col = wire_col_val;
      *rim_col = rim_col_val;

      if (v3d->shading.wire_color_type == V3D_SHADING_OBJECT_COLOR) {
        linearrgb_to_srgb_v3_v3(*wire_col, ob->color);
        mul_v3_fl(*wire_col, 0.5f);
        copy_v3_v3(*rim_col, *wire_col);
      }
      else {
        uint hash = BLI_ghashutil_strhash_p_murmur(ob->id.name);
        if (ob->id.lib) {
          hash = (hash * 13) ^ BLI_ghashutil_strhash_p_murmur(ob->id.lib->name);
        }

        float hue = BLI_hash_int_01(hash);
        float hsv[3] = {hue, 0.75f, 0.8f};
        hsv_to_rgb_v(hsv, *wire_col);
        copy_v3_v3(*rim_col, *wire_col);
      }

      if ((ob->base_flag & BASE_SELECTED) && use_coloring) {
        /* "Normalize" color. */
        add_v3_fl(*wire_col, 1e-4f);
        float brightness = max_fff((*wire_col)[0], (*wire_col)[1], (*wire_col)[2]);
        mul_v3_fl(*wire_col, (0.5f / brightness));
        add_v3_fl(*rim_col, 0.75f);
      }
      else {
        mul_v3_fl(*rim_col, 0.5f);
        add_v3_fl(*wire_col, 0.5f);
      }
    }
  }
  BLI_assert(*rim_col && *wire_col);
}

void OVERLAY_wireframe_cache_populate(OVERLAY_Data *vedata,
                                      Object *ob,
                                      OVERLAY_DupliData *dupli,
                                      bool init_dupli)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_StorageList *stl = data->stl;
  OVERLAY_PrivateData *pd = stl->pd;
  const bool use_wire = (pd->overlay.flag & V3D_OVERLAY_WIREFRAMES) != 0;
  const DRWContextState *draw_ctx = DRW_context_state_get();
  View3D *v3d = draw_ctx->v3d;

  /* Fast path for duplis. */
  if (dupli && !init_dupli) {
    if (dupli->wire_shgrp && dupli->wire_geom) {
      if (dupli->base_flag == ob->base_flag) {
        DRW_shgroup_call(dupli->wire_shgrp, dupli->wire_geom, ob);
        return;
      }
    }
    else {
      /* Nothing to draw for this dupli. */
      return;
    }
  }

  const bool is_edit_mode = BKE_object_is_in_editmode(ob);
  bool has_edit_mesh_cage = false;
  if (ob->type == OB_MESH) {
    /* TODO: Should be its own function. */
    Mesh *me = (Mesh *)ob->data;
    BMEditMesh *embm = me->edit_mesh;
    if (embm) {
      has_edit_mesh_cage = embm->mesh_eval_cage && (embm->mesh_eval_cage != embm->mesh_eval_final);
    }
  }

  /* Don't do that in edit Mesh mode, unless there is a modifier preview. */
  if (!use_wire || (((ob != draw_ctx->object_edit) && !is_edit_mode) || has_edit_mesh_cage) ||
      ob->type != OB_MESH) {
    const bool use_sculpt_pbvh = BKE_sculptsession_use_pbvh_draw(ob, draw_ctx->v3d) &&
                                 !DRW_state_is_image_render();
    const bool all_wires = (ob->dtx & OB_DRAW_ALL_EDGES);
    const bool is_wire = (ob->dt < OB_SOLID);
    const bool is_xray = (ob->dtx & OB_DRAWXRAY);
    const bool use_coloring = (use_wire && !is_edit_mode && !use_sculpt_pbvh &&
                               !has_edit_mesh_cage);
    DRWShadingGroup *shgrp = NULL;
    struct GPUBatch *geom = DRW_cache_object_face_wireframe_get(ob);

    if (geom || use_sculpt_pbvh) {
      if (is_wire && is_xray) {
        shgrp = DRW_shgroup_create_sub(pd->wires_xray_grp);
      }
      else {
        shgrp = DRW_shgroup_create_sub(pd->wires_grp);
      }

      float wire_step_param = 10.0f;
      if (!use_sculpt_pbvh) {
        wire_step_param = (all_wires) ? 1.0f : pd->shdata.wire_step_param;
      }
      DRW_shgroup_uniform_float_copy(shgrp, "wireStepParam", wire_step_param);

      if (!(DRW_state_is_select() || DRW_state_is_depth())) {
        float *rim_col, *wire_col;
        wire_color_get(v3d, ob, use_coloring, &rim_col, &wire_col);
        DRW_shgroup_uniform_vec3_copy(shgrp, "wireColor", wire_col);
        DRW_shgroup_uniform_vec3_copy(shgrp, "rimColor", rim_col);
        DRW_shgroup_stencil_mask(shgrp,
                                 (is_xray && (is_wire || !pd->clear_stencil)) ? 0x00 : 0xFF);
      }

      if (use_sculpt_pbvh) {
        DRW_shgroup_call_sculpt(shgrp, ob, true, false, false);
      }
      else {
        DRW_shgroup_call(shgrp, geom, ob);
      }
    }

    if (dupli) {
      dupli->wire_shgrp = shgrp;
      dupli->wire_geom = geom;
    }
  }
}

void OVERLAY_wireframe_draw(OVERLAY_Data *data)
{
  OVERLAY_PassList *psl = data->psl;
  OVERLAY_StorageList *stl = data->stl;

  DRW_view_set_active(stl->pd->view_wires);
  DRW_draw_pass(psl->wireframe_xray_ps);
  DRW_draw_pass(psl->wireframe_ps);
  DRW_view_set_active(NULL);
}
