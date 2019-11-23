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
    pd->antialiasing.target_sample = 3;

    bool valid_history = true;
    if (txl->overlay_color_history_tx == NULL || pd->antialiasing.sample == 0) {
      valid_history = false;
    }

    DRW_texture_ensure_fullscreen_2d(&txl->overlay_color_tx, GPU_RGBA8, DRW_TEX_FILTER);
    DRW_texture_ensure_fullscreen_2d(&txl->overlay_color_history_tx, GPU_RGBA8, DRW_TEX_FILTER);

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
      const float samples_pos[4][2] = {{-2, 6}, {6, 2}, {-6, 2}, {2, 6}};
      float ofs[2] = {0.5f + samples_pos[nr][0] / 8.0f, 0.5f + samples_pos[nr][1] / 8.0f};

      window_translate_m4(winmat, persmat, ofs[0] / viewport_size[0], ofs[1] / viewport_size[1]);

      const DRWView *default_view = DRW_view_default_get();
      pd->view_default = DRW_view_create_sub(default_view, viewmat, winmat);
    }
    else {
      /* FXAA */
      pd->antialiasing.sample = 0;
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
    bool do_fxaa = (pd->antialiasing.sample == 0);
    if (do_fxaa) {
      DRW_PASS_CREATE(psl->antialiasing_merge_ps,
                      DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_UNDER_PREMUL);

      sh = OVERLAY_shader_antialiasing_merge();
      grp = DRW_shgroup_create(sh, psl->antialiasing_merge_ps);
      DRW_shgroup_uniform_texture_ref(grp, "srcTexture", &dtxl->color);
      DRW_shgroup_uniform_float_copy(grp, "alpha", 1.0f);
      DRW_shgroup_call_procedural_triangles(grp, NULL, 1);

      DRW_PASS_CREATE(psl->antialiasing_ps, DRW_STATE_WRITE_COLOR);

      sh = OVERLAY_shader_antialiasing();
      grp = DRW_shgroup_create(sh, psl->antialiasing_ps);
      DRW_shgroup_uniform_texture_ref(grp, "srcTexture", &txl->overlay_color_tx);
      DRW_shgroup_call_procedural_triangles(grp, NULL, 1);
    }
    else {
      DRW_PASS_CREATE(psl->antialiasing_merge_ps, DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_CUSTOM);

      /* TODO do not even render if not necessary. */
      float alpha = 0.0f;
      if (pd->antialiasing.sample < pd->antialiasing.target_sample) {
        alpha = 1.0f / (pd->antialiasing.sample + 1);
      }

      sh = OVERLAY_shader_antialiasing_accum();
      grp = DRW_shgroup_create(sh, psl->antialiasing_merge_ps);
      DRW_shgroup_uniform_texture_ref(grp, "srcTexture", &txl->overlay_color_tx);
      DRW_shgroup_uniform_float_copy(grp, "alpha", alpha);
      DRW_shgroup_call_procedural_triangles(grp, NULL, 1);

      DRW_PASS_CREATE(psl->antialiasing_ps, DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_PREMUL);

      /* Reuse merge shader. */
      sh = OVERLAY_shader_antialiasing_merge();
      grp = DRW_shgroup_create(sh, psl->antialiasing_ps);
      DRW_shgroup_uniform_texture_ref(grp, "srcTexture", &txl->overlay_color_history_tx);
      DRW_shgroup_uniform_float_copy(grp, "alpha", 1.0f);
      DRW_shgroup_call_procedural_triangles(grp, NULL, 1);
    }
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
    if (pd->antialiasing.sample == 0) {
      /* First sample: Copy to history buffer and apply FXAA. */
      GPU_framebuffer_blit(
          fbl->overlay_color_only_fb, 0, fbl->overlay_color_history_fb, 0, GPU_COLOR_BIT);
      /* We merge default framebuffer with the overlay framebuffer to apply
       * FXAA pass on the final image */
      GPU_framebuffer_bind(fbl->overlay_color_only_fb);
      DRW_draw_pass(psl->antialiasing_merge_ps);
      /* Do FXAA */
      GPU_framebuffer_bind(dfbl->default_fb);
      DRW_draw_pass(psl->antialiasing_ps);

      pd->antialiasing.sample++;

      DRW_viewport_request_redraw();
    }
    else {
      /* We merge into history buffer with the right weight. */
      GPU_framebuffer_bind(fbl->overlay_color_history_fb);
      DRW_draw_pass(psl->antialiasing_merge_ps);
      /* Composite the AA'ed overlays on top of the default framebuffer  */
      GPU_framebuffer_bind(dfbl->default_fb);
      DRW_draw_pass(psl->antialiasing_ps);

      if (pd->antialiasing.sample < pd->antialiasing.target_sample) {
        pd->antialiasing.sample++;
        DRW_viewport_request_redraw();
      }
    }
  }
}
