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

#include "overlay_engine.h"
#include "overlay_private.h"

/* -------------------------------------------------------------------- */
/** \name Engine Callbacks
 * \{ */

static void OVERLAY_engine_init(void *vedata)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_StorageList *stl = data->stl;

  // const enum eContextObjectMode mode = CTX_data_mode_enum_ex(
  //     DST.draw_ctx.object_edit, obact, DST.draw_ctx.object_mode);

  if (!stl->pd) {
    /* Alloc transient pointers */
    stl->pd = MEM_callocN(sizeof(*stl->pd), __func__);
  }
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
  }
  else {
    memset(&pd->overlay, 0, sizeof(pd->overlay));
  }

  if (v3d->shading.type == OB_WIRE) {
    pd->overlay.flag |= V3D_OVERLAY_WIREFRAMES;
  }

  pd->clipping_state = (rv3d->rflag & RV3D_CLIPPING) ? DRW_STATE_CLIP_PLANES : 0;

  OVERLAY_facing_cache_init(vedata);
  OVERLAY_wireframe_cache_init(vedata);
}

static void OVERLAY_cache_populate(void *vedata, Object *ob)
{
  OVERLAY_Data *data = vedata;
  OVERLAY_PrivateData *pd = data->stl->pd;
  const bool renderable = DRW_object_is_renderable(ob);
  const bool draw_surface = !((ob->dt < OB_WIRE) || (!renderable && (ob->dt != OB_WIRE)));
  const bool draw_facing = draw_surface && (pd->overlay.flag & V3D_OVERLAY_FACE_ORIENTATION);
  const bool draw_wires = draw_surface && ((pd->overlay.flag & V3D_OVERLAY_WIREFRAMES) ||
                                           (ob->dtx & OB_DRAWWIRE) || (ob->dt == OB_WIRE));
  if (draw_facing) {
    OVERLAY_facing_cache_populate(vedata, ob);
  }
  if (draw_wires) {
    OVERLAY_wireframe_cache_populate(vedata, ob);
  }
  // if (is_geom) {
  //   if (edit_mode) {
  //     switch (ob->type) {
  //       case OB_MESH:
  //         OVERLAY_edit_mesh_cache_populate();
  //         break;
  //       case OB_CURVE:
  //         OVERLAY_edit_curve_cache_populate();
  //         break;
  //       case OB_SURF:
  //         OVERLAY_edit_surf_cache_populate();
  //         break;
  //       case OB_LATTICE:
  //         OVERLAY_edit_lattive_cache_populate();
  //         break;
  //       case OB_MBALL:
  //         OVERLAY_edit_metaball_cache_populate();
  //         break;
  //       case OB_FONT:
  //         OVERLAY_edit_font_cache_populate();
  //         break;
  //     }
  //   }
  //   if (paint_vertex_mode) {
  //     OVERLAY_paint_vertex_cache_populate();
  //   }
  //   if (paint_texture_mode) {
  //     OVERLAY_paint_texture_cache_populate();
  //   }
  //   if (scuplt_mode) {
  //     OVERLAY_sculpt_cache_populate();
  //   }
  //   if (draw_wires) {
  //     OVERLAY_wireframe_cache_populate();
  //   }
  // }
  // /* Non-Meshes */
  // switch (ob->type) {
  //   case OB_ARMATURE:
  //     OVERLAY_armature_cache_populate();
  //     break;
  //   case OB_EMPTY:
  //     OVERLAY_empty_cache_populate();
  //     break;
  //   case OB_MBALL:
  //     OVERLAY_mball_cache_populate();
  //     break;
  //   case OB_LAMP:
  //     OVERLAY_lamp_cache_populate();
  //     break;
  //   case OB_CAMERA:
  //     OVERLAY_camera_cache_populate();
  //     break;
  //   case OB_SPEAKER:
  //     OVERLAY_speaker_cache_populate();
  //     break;
  //   case OB_LIGHTPROBE:
  //     OVERLAY_lightprobe_cache_populate();
  //     break;
  //   case OB_FONT:
  //     OVERLAY_font_cache_populate();
  //     break;
  // }
  // if (draw_extra) {
  //   OVERLAY_extra_cache_populate();
  // }
}

static void OVERLAY_draw_scene(void *vedata)
{
  OVERLAY_facing_draw(vedata);
  OVERLAY_wireframe_draw(vedata);
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
    NULL,
    NULL,
    &OVERLAY_draw_scene,
    NULL,
    NULL,
    NULL,
};

/** \} */

#undef SELECT_ENGINE
