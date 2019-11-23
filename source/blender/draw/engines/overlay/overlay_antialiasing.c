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

#include "overlay_private.h"

void OVERLAY_antialiasing_init(OVERLAY_Data *vedata)
{
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  OVERLAY_TextureList *txl = vedata->txl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  DefaultTextureList *dtxl = DRW_viewport_texture_list_get();

  if (!DRW_state_is_fbo()) {
    pd->use_antialiasing = false;
    return;
  }

  /* TODO Get real userpref option and remove MSAA buffer. */
  pd->use_antialiasing = dtxl->multisample_color != NULL;

  if (pd->use_antialiasing) {
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
        &fbl->overlay_color_only_history_fb,
        {GPU_ATTACHMENT_NONE, GPU_ATTACHMENT_TEXTURE(txl->overlay_color_history_tx)});
    GPU_framebuffer_ensure_config(&fbl->overlay_default_history_fb,
                                  {GPU_ATTACHMENT_TEXTURE(dtxl->depth),
                                   GPU_ATTACHMENT_TEXTURE(txl->overlay_color_history_tx)});
    GPU_framebuffer_ensure_config(&fbl->overlay_in_front_history_fb,
                                  {GPU_ATTACHMENT_TEXTURE(dtxl->depth_in_front),
                                   GPU_ATTACHMENT_TEXTURE(txl->overlay_color_history_tx)});
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
  }
}

void OVERLAY_antialiasing_cache_init(OVERLAY_Data *vedata)
{
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  OVERLAY_TextureList *txl = vedata->txl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  OVERLAY_PassList *psl = vedata->psl;
  DefaultTextureList *dtxl = DRW_viewport_texture_list_get();
  struct GPUShader *sh;
  DRWShadingGroup *grp;

  if (pd->use_antialiasing) {
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
}

void OVERLAY_antialiasing_start(OVERLAY_Data *vedata)
{
  OVERLAY_FramebufferList *fbl = vedata->fbl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;

  if (pd->use_antialiasing) {
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

  if (pd->use_antialiasing) {
    GPU_framebuffer_bind(fbl->overlay_color_only_fb);
    DRW_draw_pass(psl->antialiasing_merge_ps);

    GPU_framebuffer_bind(dfbl->default_fb);
    DRW_draw_pass(psl->antialiasing_ps);
  }
}
