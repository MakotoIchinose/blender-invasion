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

#include "ED_screen.h"

#include "overlay_private.h"
#include "smaa_textures.h"

struct {
  GPUTexture *smaa_search_tx;
  GPUTexture *smaa_area_tx;
} e_data = {NULL};

void OVERLAY_antialiasing_reset(OVERLAY_Data *vedata)
{
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  pd->antialiasing.sample = 0;
}

void OVERLAY_antialiasing_init(OVERLAY_Data *vedata)
{
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  OVERLAY_TextureList *txl = vedata->txl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  DefaultTextureList *dtxl = DRW_viewport_texture_list_get();

  if (!DRW_state_is_fbo()) {
    /* Use default view */
    pd->view_default = (DRWView *)DRW_view_default_get();
    pd->antialiasing.enabled = false;
    return;
  }

  /* TODO Get real userpref option and remove MSAA buffer. */
  pd->antialiasing.enabled = dtxl->multisample_color != NULL;

  if (pd->antialiasing.enabled) {

    if (e_data.smaa_search_tx == NULL) {
      e_data.smaa_search_tx = GPU_texture_create_nD(SEARCHTEX_WIDTH,
                                                    SEARCHTEX_HEIGHT,
                                                    0,
                                                    2,
                                                    searchTexBytes,
                                                    GPU_R8,
                                                    GPU_DATA_UNSIGNED_BYTE,
                                                    0,
                                                    false,
                                                    NULL);

      e_data.smaa_area_tx = GPU_texture_create_nD(AREATEX_WIDTH,
                                                  AREATEX_HEIGHT,
                                                  0,
                                                  2,
                                                  areaTexBytes,
                                                  GPU_RG8,
                                                  GPU_DATA_UNSIGNED_BYTE,
                                                  0,
                                                  false,
                                                  NULL);
      GPU_texture_bind(e_data.smaa_area_tx, 0);
      GPU_texture_filter_mode(e_data.smaa_area_tx, true);
      GPU_texture_unbind(e_data.smaa_area_tx);

      GPU_texture_bind(e_data.smaa_search_tx, 0);
      GPU_texture_filter_mode(e_data.smaa_search_tx, true);
      GPU_texture_unbind(e_data.smaa_search_tx);
    }

    pd->antialiasing.target_sample = 4;

    bool valid_history = true;
    if (txl->overlay_color_history_tx == NULL || pd->antialiasing.sample == 0) {
      valid_history = false;
    }

    DRW_texture_ensure_fullscreen_2d(&txl->temp_depth_tx, GPU_DEPTH24_STENCIL8, 0);
    DRW_texture_ensure_fullscreen_2d(&txl->overlay_color_tx, GPU_RGBA8, DRW_TEX_FILTER);
    DRW_texture_ensure_fullscreen_2d(&txl->overlay_color_history_tx, GPU_RGBA8, DRW_TEX_FILTER);
    DRW_texture_ensure_fullscreen_2d(&txl->smaa_edge_tx, GPU_RG8, DRW_TEX_FILTER);
    DRW_texture_ensure_fullscreen_2d(&txl->smaa_blend_tx, GPU_RGBA8, DRW_TEX_FILTER);

    GPU_framebuffer_ensure_config(
        &fbl->overlay_color_only_fb,
        {GPU_ATTACHMENT_NONE, GPU_ATTACHMENT_TEXTURE(txl->overlay_color_tx)});
    GPU_framebuffer_ensure_config(
        &fbl->overlay_default_fb,
        {GPU_ATTACHMENT_TEXTURE(dtxl->depth), GPU_ATTACHMENT_TEXTURE(txl->overlay_color_tx)});
    GPU_framebuffer_ensure_config(&fbl->overlay_in_front_fb,
                                  {GPU_ATTACHMENT_TEXTURE(dtxl->depth_in_front),
                                   GPU_ATTACHMENT_TEXTURE(txl->overlay_color_tx)});

    GPU_framebuffer_ensure_config(
        &fbl->overlay_color_history_fb,
        {GPU_ATTACHMENT_NONE, GPU_ATTACHMENT_TEXTURE(txl->overlay_color_history_tx)});
    GPU_framebuffer_ensure_config(
        &fbl->smaa_edge_fb,
        {GPU_ATTACHMENT_TEXTURE(txl->temp_depth_tx), GPU_ATTACHMENT_TEXTURE(txl->smaa_edge_tx)});
    GPU_framebuffer_ensure_config(
        &fbl->smaa_blend_fb,
        {GPU_ATTACHMENT_TEXTURE(txl->temp_depth_tx), GPU_ATTACHMENT_TEXTURE(txl->smaa_blend_tx)});

    /* Test if we can do TAA */
    if (valid_history) {
      float persmat[4][4];
      DRW_view_persmat_get(NULL, persmat, false);
      valid_history = compare_m4m4(persmat, pd->antialiasing.prev_persmat, FLT_MIN);
      copy_m4_m4(pd->antialiasing.prev_persmat, persmat);
    }

    const DRWContextState *draw_ctx = DRW_context_state_get();
    if (valid_history && draw_ctx->evil_C != NULL) {
      struct wmWindowManager *wm = CTX_wm_manager(draw_ctx->evil_C);
      valid_history = (ED_screen_animation_no_scrub(wm) == NULL);
    }

    if (valid_history) {
      /* TAA */
      float persmat[4][4], viewmat[4][4], winmat[4][4];
      DRW_view_persmat_get(NULL, persmat, false);
      DRW_view_viewmat_get(NULL, viewmat, false);
      DRW_view_winmat_get(NULL, winmat, false);

      const float *viewport_size = DRW_viewport_size_get();
      int nr = min_ii(pd->antialiasing.sample, pd->antialiasing.target_sample);
      /* x4 rotated grid. TODO(fclem) better patterns. */
      const float samples_pos[5][2] = {{0, 0}, {-2, -6}, {6, -2}, {-6, 2}, {2, 6}};
      /* For nr = 0,we should sample {0, 0} (goes through the FXAA branch). */
      float ofs[2] = {samples_pos[nr][0] / 6.0f, samples_pos[nr][1] / 6.0f};

      // window_translate_m4(winmat, persmat, ofs[0] / viewport_size[0], ofs[1] /
      // viewport_size[1]);

      const DRWView *default_view = DRW_view_default_get();
      pd->view_default = DRW_view_create_sub(default_view, viewmat, winmat);

      /* Avoid infinite sample count. */
      CLAMP_MAX(pd->antialiasing.sample, pd->antialiasing.target_sample + 1);
    }
    else {
      /* FXAA */
      OVERLAY_antialiasing_reset(vedata);
      /* Use default view */
      pd->view_default = (DRWView *)DRW_view_default_get();
    }
  }
  else {
    /* Just a copy of the defaults framebuffers. */
    GPU_framebuffer_ensure_config(&fbl->overlay_color_only_fb,
                                  {GPU_ATTACHMENT_NONE, GPU_ATTACHMENT_TEXTURE(dtxl->color)});
    GPU_framebuffer_ensure_config(
        &fbl->overlay_default_fb,
        {GPU_ATTACHMENT_TEXTURE(dtxl->depth), GPU_ATTACHMENT_TEXTURE(dtxl->color)});
    GPU_framebuffer_ensure_config(
        &fbl->overlay_in_front_fb,
        {GPU_ATTACHMENT_TEXTURE(dtxl->depth_in_front), GPU_ATTACHMENT_TEXTURE(dtxl->color)});

    /* Use default view */
    pd->view_default = (DRWView *)DRW_view_default_get();
  }
}

void OVERLAY_antialiasing_cache_init(OVERLAY_Data *vedata)
{
  OVERLAY_TextureList *txl = vedata->txl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  OVERLAY_PassList *psl = vedata->psl;
  DefaultTextureList *dtxl = DRW_viewport_texture_list_get();
  struct GPUShader *sh;
  DRWShadingGroup *grp;

  if (pd->antialiasing.enabled) {
    DRW_PASS_CREATE(psl->smaa_edge_detect_ps,
                    DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_STENCIL | DRW_STATE_STENCIL_ALWAYS);
    DRW_PASS_CREATE(psl->smaa_blend_weight_ps, DRW_STATE_WRITE_COLOR | DRW_STATE_STENCIL_EQUAL);
    DRW_PASS_CREATE(psl->smaa_resolve_ps, DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_PREMUL);

    sh = OVERLAY_shader_antialiasing(0);
    grp = DRW_shgroup_create(sh, psl->smaa_edge_detect_ps);
    DRW_shgroup_uniform_block(grp, "globalsBlock", G_draw.block_ubo);
    DRW_shgroup_uniform_texture_ref(grp, "colorTex", &txl->overlay_color_tx);
    DRW_shgroup_stencil_mask(grp, 0xFF);
    DRW_shgroup_call_procedural_triangles(grp, NULL, 1);

    sh = OVERLAY_shader_antialiasing(1);
    grp = DRW_shgroup_create(sh, psl->smaa_blend_weight_ps);
    DRW_shgroup_uniform_block(grp, "globalsBlock", G_draw.block_ubo);
    DRW_shgroup_uniform_texture_ref(grp, "edgesTex", &txl->smaa_edge_tx);
    DRW_shgroup_uniform_texture(grp, "areaTex", e_data.smaa_area_tx);
    DRW_shgroup_uniform_texture(grp, "searchTex", e_data.smaa_search_tx);
    DRW_shgroup_stencil_mask(grp, 0xFF);
    DRW_shgroup_call_procedural_triangles(grp, NULL, 1);

    sh = OVERLAY_shader_antialiasing(2);
    grp = DRW_shgroup_create(sh, psl->smaa_resolve_ps);
    DRW_shgroup_uniform_block(grp, "globalsBlock", G_draw.block_ubo);
    DRW_shgroup_uniform_texture_ref(grp, "blendTex", &txl->smaa_blend_tx);
    DRW_shgroup_uniform_texture_ref(grp, "colorTex", &txl->overlay_color_tx);
    DRW_shgroup_call_procedural_triangles(grp, NULL, 1);
  }
}

void OVERLAY_antialiasing_start(OVERLAY_Data *vedata)
{
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;

  if (pd->antialiasing.enabled) {
    float clear_col[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    GPU_framebuffer_bind(fbl->overlay_default_fb);
    GPU_framebuffer_clear_color(fbl->overlay_default_fb, clear_col);
  }
}

void OVERLAY_antialiasing_end(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  DefaultFramebufferList *dfbl = DRW_viewport_framebuffer_list_get();

  if (pd->antialiasing.enabled) {
    float clear_col[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    GPU_framebuffer_bind(fbl->smaa_edge_fb);
    GPU_framebuffer_clear_color(fbl->smaa_edge_fb, clear_col);
    GPU_framebuffer_clear_stencil(fbl->smaa_edge_fb, 0x00);
    DRW_draw_pass(psl->smaa_edge_detect_ps);

    GPU_framebuffer_bind(fbl->smaa_blend_fb);
    GPU_framebuffer_clear_color(fbl->smaa_blend_fb, clear_col);
    DRW_draw_pass(psl->smaa_blend_weight_ps);

    GPU_framebuffer_bind(dfbl->default_fb);
    DRW_draw_pass(psl->smaa_resolve_ps);
  }
}

void OVERLAY_antialiasing_free(void)
{
  DRW_TEXTURE_FREE_SAFE(e_data.smaa_area_tx);
  DRW_TEXTURE_FREE_SAFE(e_data.smaa_search_tx);
}
