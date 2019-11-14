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

#include "DRW_render.h"

#include "BKE_object.h"

#include "overlay_private.h"

void OVERLAY_image_cache_init(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;

  DRWState state = DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_UNDER_PREMUL;
  DRW_PASS_CREATE(psl->image_background_ps, state);

  state = DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS;
  DRW_PASS_CREATE(psl->image_empties_ps, state | pd->clipping_state);

  state = DRW_STATE_WRITE_COLOR | DRW_STATE_DEPTH_LESS_EQUAL | DRW_STATE_BLEND_ALPHA;
  DRW_PASS_CREATE(psl->image_empties_back_ps, state | pd->clipping_state);
  DRW_PASS_CREATE(psl->image_empties_blend_ps, state | pd->clipping_state);

  state = DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA;
  DRW_PASS_CREATE(psl->image_empties_front_ps, state);
  DRW_PASS_CREATE(psl->image_foreground_ps, state);
}

static void overlay_image_calc_aspect(Image *ima, const int size[2], float r_image_aspect[2])
{
  float ima_x, ima_y;
  if (ima) {
    ima_x = size[0];
    ima_y = size[1];
  }
  else {
    /* if no image, make it a 1x1 empty square, honor scale & offset */
    ima_x = ima_y = 1.0f;
  }
  /* Get the image aspect even if the buffer is invalid */
  float sca_x = 1.0f, sca_y = 1.0f;
  if (ima) {
    if (ima->aspx > ima->aspy) {
      sca_y = ima->aspy / ima->aspx;
    }
    else if (ima->aspx < ima->aspy) {
      sca_x = ima->aspx / ima->aspy;
    }
  }

  const float scale_x_inv = ima_x * sca_x;
  const float scale_y_inv = ima_y * sca_y;
  if (scale_x_inv > scale_y_inv) {
    r_image_aspect[0] = 1.0f;
    r_image_aspect[1] = scale_y_inv / scale_x_inv;
  }
  else {
    r_image_aspect[0] = scale_x_inv / scale_y_inv;
    r_image_aspect[1] = 1.0f;
  }
}

void OVERLAY_image_camera_cache_populate(OVERLAY_Data *vedata, Object *ob)
{
  // OVERLAY_PrivateData *pd = vedata->stl->pd;
  // DRWShadingGroup *grp = DRW_shgroup_create_sub(pd->image_background);
  // DRW_shgroup_uniform_texture(grp, "imgTexture", tex);
  // DRW_shgroup_call_obmat(grp, batch, NULL);
}

void OVERLAY_image_empty_cache_populate(OVERLAY_Data *vedata, Object *ob)
{
  OVERLAY_PassList *psl = vedata->psl;
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const RegionView3D *rv3d = draw_ctx->rv3d;
  GPUTexture *tex = NULL;
  Image *ima = ob->data;
  float mat[4][4];

  const bool show_frame = BKE_object_empty_image_frame_is_visible_in_view3d(ob, rv3d);
  const bool show_image = show_frame && BKE_object_empty_image_data_is_visible_in_view3d(ob, rv3d);
  const bool use_alpha_blend = (ob->empty_image_flag & OB_EMPTY_IMAGE_USE_ALPHA_BLEND) != 0;
  const bool use_alpha_premult = ima && (ima->alpha_mode == IMA_ALPHA_PREMUL);

  if (!show_frame) {
    return;
  }

  {
    /* Calling 'BKE_image_get_size' may free the texture. Get the size from 'tex' instead,
     * see: T59347 */
    int size[2] = {0};
    if (ima != NULL) {
      tex = GPU_texture_from_blender(ima, ob->iuser, GL_TEXTURE_2D);
      if (tex) {
        size[0] = GPU_texture_orig_width(tex);
        size[1] = GPU_texture_orig_height(tex);
      }
    }
    CLAMP_MIN(size[0], 1);
    CLAMP_MIN(size[1], 1);

    float image_aspect[2];
    overlay_image_calc_aspect(ob->data, size, image_aspect);

    copy_m4_m4(mat, ob->obmat);
    mul_v3_fl(mat[0], image_aspect[0] * 0.5f * ob->empty_drawsize);
    mul_v3_fl(mat[1], image_aspect[1] * 0.5f * ob->empty_drawsize);
    madd_v3_v3fl(mat[3], mat[0], ob->ima_ofs[0] * 2.0f + 1.0f);
    madd_v3_v3fl(mat[3], mat[1], ob->ima_ofs[1] * 2.0f + 1.0f);
  }

  /* Use the actual depth if we are doing depth tests to determine the distance to the object */
  char depth_mode = DRW_state_is_depth() ? OB_EMPTY_IMAGE_DEPTH_DEFAULT : ob->empty_image_depth;
  DRWPass *pass = NULL;
  switch (depth_mode) {
    case OB_EMPTY_IMAGE_DEPTH_DEFAULT:
      pass = (use_alpha_blend) ? psl->image_empties_blend_ps : psl->image_empties_ps;
      break;
    case OB_EMPTY_IMAGE_DEPTH_BACK:
      pass = psl->image_empties_back_ps;
      break;
    case OB_EMPTY_IMAGE_DEPTH_FRONT:
      pass = psl->image_empties_front_ps;
      break;
  }

  if (show_frame) {
    OVERLAY_ExtraCallBuffers *cb = OVERLAY_extra_call_buffer_get(vedata, ob);
    float *color;
    DRW_object_wire_theme_get(ob, draw_ctx->view_layer, &color);
    OVERLAY_empty_shape(cb, mat, 1.0f, OB_EMPTY_IMAGE, color);
  }

  if (show_image && tex && ((ob->color[3] > 0.0f) || !use_alpha_blend)) {
    GPUShader *sh = OVERLAY_shader_image();
    DRWShadingGroup *grp = DRW_shgroup_create(sh, pass);
    DRW_shgroup_uniform_texture(grp, "imgTexture", tex);
    DRW_shgroup_uniform_bool_copy(grp, "imgPremultiplied", use_alpha_premult);
    DRW_shgroup_uniform_bool_copy(grp, "imgAlphaBlend", use_alpha_blend);
    DRW_shgroup_uniform_bool_copy(grp, "imgLinear", false);
    DRW_shgroup_uniform_bool_copy(grp, "depthSet", depth_mode != OB_EMPTY_IMAGE_DEPTH_DEFAULT);
    DRW_shgroup_uniform_vec4_copy(grp, "color", ob->color);
    DRW_shgroup_call_obmat(grp, DRW_cache_empty_image_plane_get(), mat);
  }
}

void OVERLAY_image_cache_finish(OVERLAY_Data *vedata)
{
  /* Order by Z depth. */
}

void OVERLAY_image_draw(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;
  /* TODO better ordering with other passes. */
  DRW_draw_pass(psl->image_background_ps);
  DRW_draw_pass(psl->image_empties_back_ps);

  DRW_draw_pass(psl->image_empties_ps);
  DRW_draw_pass(psl->image_empties_blend_ps);

  DRW_draw_pass(psl->image_empties_front_ps);
  DRW_draw_pass(psl->image_foreground_ps);
}
