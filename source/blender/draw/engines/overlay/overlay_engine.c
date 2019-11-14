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
 *
 * Engine for drawing a selection map where the pixels indicate the selection indices.
 */

#include "DRW_engine.h"
#include "DRW_render.h"

#include "ED_view3d.h"

#include "BKE_object.h"

#include "overlay_engine.h"
#include "overlay_private.h"

/* -------------------------------------------------------------------- */
/** \name Engine Callbacks
 * \{ */

static void OVERLAY_engine_init(void *vedata)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_StorageList *stl = data->stl;
  const DRWContextState *draw_ctx = DRW_context_state_get();

  if (!stl->pd) {
    /* Alloc transient pointers */
    stl->pd = MEM_callocN(sizeof(*stl->pd), __func__);
  }

  stl->pd->ctx_mode = CTX_data_mode_enum_ex(
      draw_ctx->object_edit, draw_ctx->obact, draw_ctx->object_mode);

  switch (stl->pd->ctx_mode) {
    case CTX_MODE_EDIT_MESH:
      OVERLAY_edit_mesh_init(vedata);
      break;
    case CTX_MODE_EDIT_SURFACE:
    case CTX_MODE_EDIT_CURVE:
      break;
    case CTX_MODE_EDIT_TEXT:
      break;
    case CTX_MODE_EDIT_ARMATURE:
      break;
    case CTX_MODE_EDIT_METABALL:
      break;
    case CTX_MODE_EDIT_LATTICE:
      break;
    case CTX_MODE_PARTICLE:
      break;
    case CTX_MODE_POSE:
    case CTX_MODE_PAINT_WEIGHT:
      break;
    case CTX_MODE_SCULPT:
    case CTX_MODE_PAINT_VERTEX:
    case CTX_MODE_PAINT_TEXTURE:
    case CTX_MODE_OBJECT:
    case CTX_MODE_PAINT_GPENCIL:
    case CTX_MODE_EDIT_GPENCIL:
    case CTX_MODE_SCULPT_GPENCIL:
    case CTX_MODE_WEIGHT_GPENCIL:
      break;
    default:
      BLI_assert(!"Draw mode invalid");
      break;
  }
  OVERLAY_grid_init(vedata);
  OVERLAY_facing_init(vedata);
  OVERLAY_outline_init(vedata);
  OVERLAY_wireframe_init(vedata);
}

static void OVERLAY_cache_init(void *vedata)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_StorageList *stl = data->stl;
  OVERLAY_PrivateData *pd = stl->pd;

  const DRWContextState *draw_ctx = DRW_context_state_get();
  const RegionView3D *rv3d = draw_ctx->rv3d;
  const View3D *v3d = draw_ctx->v3d;

  if (v3d && !(v3d->flag2 & V3D_HIDE_OVERLAYS)) {
    pd->overlay = v3d->overlay;
    pd->v3d_flag = v3d->flag;
  }
  else {
    memset(&pd->overlay, 0, sizeof(pd->overlay));
    pd->v3d_flag = 0;
  }

  if (v3d->shading.type == OB_WIRE) {
    pd->overlay.flag |= V3D_OVERLAY_WIREFRAMES;
  }

  pd->clipping_state = (rv3d->rflag & RV3D_CLIPPING) ? DRW_STATE_CLIP_PLANES : 0;
  pd->xray_enabled = XRAY_ACTIVE(v3d);
  pd->xray_enabled_and_not_wire = pd->xray_enabled && v3d->shading.type > OB_WIRE;

  switch (pd->ctx_mode) {
    case CTX_MODE_EDIT_MESH:
      OVERLAY_edit_mesh_cache_init(vedata);
      break;
    case CTX_MODE_EDIT_SURFACE:
    case CTX_MODE_EDIT_CURVE:
      break;
    case CTX_MODE_EDIT_TEXT:
      break;
    case CTX_MODE_EDIT_ARMATURE:
      break;
    case CTX_MODE_EDIT_METABALL:
      break;
    case CTX_MODE_EDIT_LATTICE:
      break;
    case CTX_MODE_PARTICLE:
      break;
    case CTX_MODE_POSE:
    case CTX_MODE_PAINT_WEIGHT:
      break;
    case CTX_MODE_SCULPT:
    case CTX_MODE_PAINT_VERTEX:
    case CTX_MODE_PAINT_TEXTURE:
    case CTX_MODE_OBJECT:
    case CTX_MODE_PAINT_GPENCIL:
    case CTX_MODE_EDIT_GPENCIL:
    case CTX_MODE_SCULPT_GPENCIL:
    case CTX_MODE_WEIGHT_GPENCIL:
      break;
    default:
      BLI_assert(!"Draw mode invalid");
      break;
  }
  OVERLAY_armature_cache_init(vedata);
  OVERLAY_extra_cache_init(vedata);
  OVERLAY_facing_cache_init(vedata);
  OVERLAY_grid_cache_init(vedata);
  OVERLAY_image_cache_init(vedata);
  OVERLAY_outline_cache_init(vedata);
  OVERLAY_wireframe_cache_init(vedata);
}

BLI_INLINE OVERLAY_DupliData *OVERLAY_duplidata_get(Object *ob, void *vedata, bool *init)
{
  OVERLAY_DupliData **dupli_data = (OVERLAY_DupliData **)DRW_duplidata_get(vedata);
  *init = false;
  if (!ELEM(ob->type, OB_MESH, OB_SURF, OB_LATTICE, OB_CURVE, OB_FONT)) {
    return NULL;
  }

  if (dupli_data) {
    if (*dupli_data == NULL) {
      *dupli_data = MEM_callocN(sizeof(OVERLAY_DupliData), __func__);
      *init = true;
    }
    else if ((*dupli_data)->base_flag != ob->base_flag) {
      /* Select state might have change, reinit. */
      *init = true;
    }
    return *dupli_data;
  }
  return NULL;
}

static void OVERLAY_cache_populate(void *vedata, Object *ob)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_PrivateData *pd = data->stl->pd;
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const bool is_select = DRW_state_is_select();
  const bool renderable = DRW_object_is_renderable(ob);
  const bool in_pose_mode = ob->type == OB_ARMATURE && DRW_pose_mode_armature(ob, draw_ctx->obact);
  const bool in_edit_mode = BKE_object_is_in_editmode(ob);
  const bool in_paint_mode = (ob == draw_ctx->obact) &&
                             (draw_ctx->object_mode & OB_MODE_ALL_PAINT);
  const bool draw_surface = !((ob->dt < OB_WIRE) || (!renderable && (ob->dt != OB_WIRE)));
  const bool draw_facing = draw_surface && (pd->overlay.flag & V3D_OVERLAY_FACE_ORIENTATION);
  const bool draw_wires = draw_surface && ((pd->overlay.flag & V3D_OVERLAY_WIREFRAMES) ||
                                           (ob->dtx & OB_DRAWWIRE) || (ob->dt == OB_WIRE));
  const bool draw_outlines = !in_edit_mode && !in_paint_mode && renderable &&
                             (pd->v3d_flag & V3D_SELECT_OUTLINE) &&
                             ((ob->base_flag & BASE_SELECTED) ||
                              (is_select && ob->type == OB_LIGHTPROBE));
  const bool draw_extras =
      ((pd->overlay.flag & V3D_OVERLAY_HIDE_OBJECT_XTRAS) == 0) ||
      /* Show if this is the camera we're looking through since it's useful for selecting. */
      ((draw_ctx->rv3d->persp == RV3D_CAMOB) && ((ID *)draw_ctx->v3d->camera == ob->id.orig_id));

  bool init;
  OVERLAY_DupliData *dupli = OVERLAY_duplidata_get(ob, vedata, &init);

  if (draw_facing) {
    OVERLAY_facing_cache_populate(vedata, ob);
  }
  if (draw_wires) {
    OVERLAY_wireframe_cache_populate(vedata, ob, dupli, init);
  }
  if (draw_outlines) {
    OVERLAY_outline_cache_populate(vedata, ob, dupli, init);
  }
  if (in_edit_mode) {
    switch (ob->type) {
      case OB_MESH:
        OVERLAY_edit_mesh_cache_populate(vedata, ob);
        break;
      case OB_ARMATURE:
        OVERLAY_edit_armature_cache_populate(vedata, ob);
        break;
      case OB_CURVE:
        break;
      case OB_SURF:
        break;
      case OB_LATTICE:
        break;
      case OB_MBALL:
        break;
      case OB_FONT:
        break;
    }
  }

  if (in_pose_mode) {
    OVERLAY_pose_armature_cache_populate(vedata, ob);
  }

  // if (is_geom) {
  //   if (paint_vertex_mode) {
  //     OVERLAY_paint_vertex_cache_populate();
  //   }
  //   if (paint_texture_mode) {
  //     OVERLAY_paint_texture_cache_populate();
  //   }
  //   if (scuplt_mode) {
  //     OVERLAY_sculpt_cache_populate();
  //   }
  // }

  switch (ob->type) {
    case OB_ARMATURE:
      if ((!in_edit_mode && !in_pose_mode) || is_select) {
        OVERLAY_armature_cache_populate(vedata, ob);
      }
      break;
    // case OB_MBALL:
    //   OVERLAY_mball_cache_populate();
    //   break;
    // case OB_FONT:
    //   OVERLAY_font_cache_populate();
    //   break;
    case OB_GPENCIL:
      OVERLAY_gpencil_cache_populate(vedata, ob);
      break;
  }
  /* Non-Meshes */
  if (draw_extras) {
    switch (ob->type) {
      case OB_EMPTY:
        OVERLAY_empty_cache_populate(vedata, ob);
        break;
      case OB_LAMP:
        OVERLAY_light_cache_populate(vedata, ob);
        break;
      case OB_CAMERA:
        OVERLAY_camera_cache_populate(vedata, ob);
        break;
      case OB_SPEAKER:
        OVERLAY_speaker_cache_populate(vedata, ob);
        break;
      case OB_LIGHTPROBE:
        OVERLAY_lightprobe_cache_populate(vedata, ob);
        break;
    }
  }

  /* Relationship, object center, bounbox ... */
  OVERLAY_extra_cache_populate(vedata, ob);

  if (dupli) {
    dupli->base_flag = ob->base_flag;
  }
}

static void OVERLAY_cache_finish(void *vedata)
{
  OVERLAY_image_cache_finish(vedata);
}

static void OVERLAY_draw_scene(void *vedata)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_PrivateData *pd = data->stl->pd;

  OVERLAY_image_draw(vedata);
  OVERLAY_facing_draw(vedata);
  OVERLAY_wireframe_draw(vedata);
  OVERLAY_extra_draw(vedata);
  OVERLAY_armature_draw(vedata);
  OVERLAY_grid_draw(vedata);
  OVERLAY_outline_draw(vedata);

  switch (pd->ctx_mode) {
    case CTX_MODE_EDIT_MESH:
      OVERLAY_edit_mesh_draw(vedata);
      break;
    default:
      break;
  }
}

static void OVERLAY_engine_free(void)
{
  OVERLAY_shader_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Engine Type
 * \{ */

static const DrawEngineDataSize overlay_data_size = DRW_VIEWPORT_DATA_SIZE(OVERLAY_Data);

DrawEngineType draw_engine_overlay_type = {
    NULL,
    NULL,
    N_("Overlay"),
    &overlay_data_size,
    &OVERLAY_engine_init,
    &OVERLAY_engine_free,
    &OVERLAY_cache_init,
    &OVERLAY_cache_populate,
    &OVERLAY_cache_finish,
    NULL,
    &OVERLAY_draw_scene,
    NULL,
    NULL,
    NULL,
};

/** \} */

#undef SELECT_ENGINE
