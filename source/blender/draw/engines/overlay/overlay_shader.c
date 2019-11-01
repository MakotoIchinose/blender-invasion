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
 * Copyright 2016, Blender Foundation.
 */

/** \file
 * \ingroup draw_engine
 */

#include "DRW_render.h"

#include "GPU_shader.h"

#include "overlay_private.h"

extern char datatoc_facing_frag_glsl[];
extern char datatoc_facing_vert_glsl[];
extern char datatoc_wireframe_vert_glsl[];
extern char datatoc_wireframe_geom_glsl[];
extern char datatoc_wireframe_frag_glsl[];
extern char datatoc_gpu_shader_depth_only_frag_glsl[];

extern char datatoc_common_view_lib_glsl[];

typedef struct OVERLAY_Shaders {
  struct GPUShader *facing;
  struct GPUShader *wireframe_select;
  struct GPUShader *wireframe;
} OVERLAY_Shaders;

static struct {
  OVERLAY_Shaders sh_data[GPU_SHADER_CFG_LEN];
} e_data = {{{NULL}}};

GPUShader *OVERLAY_shader_facing(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->facing) {
    /* Face orientation */
    sh_data->facing = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_facing_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_facing_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->facing;
}

GPUShader *OVERLAY_shader_wireframe_select(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->wireframe_select) {
    sh_data->wireframe_select = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_wireframe_vert_glsl,
                                 NULL},
        .geom = (const char *[]){sh_cfg->lib, datatoc_wireframe_geom_glsl, NULL},
        .frag = (const char *[]){datatoc_gpu_shader_depth_only_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define SELECT_EDGES\n", NULL},
    });
  }
  return sh_data->wireframe_select;
}

GPUShader *OVERLAY_shader_wireframe(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->wireframe) {
    sh_data->wireframe = GPU_shader_create_from_arrays({
      .vert = (const char *[]){sh_cfg->lib,
                               datatoc_common_view_lib_glsl,
                               datatoc_wireframe_vert_glsl,
                               NULL},
      .frag = (const char *[]){datatoc_wireframe_frag_glsl, NULL},
      /* Apple drivers does not support wide wires. Use geometry shader as a workaround. */
#if USE_GEOM_SHADER_WORKAROUND
      .geom = (const char *[]){sh_cfg->lib, datatoc_wireframe_geom_glsl, NULL},
      .defs = (const char *[]){sh_cfg->def, "#define USE_GEOM\n", NULL},
#else
      .defs = (const char *[]){sh_cfg->def, NULL},
#endif
    });
  }
  return sh_data->wireframe;
}

void OVERLAY_shader_free(void)
{
  for (int sh_data_index = 0; sh_data_index < ARRAY_SIZE(e_data.sh_data); sh_data_index++) {
    OVERLAY_Shaders *sh_data = &e_data.sh_data[sh_data_index];
    GPUShader **sh_data_as_array = (GPUShader **)sh_data;
    for (int i = 0; i < (sizeof(OVERLAY_Shaders) / sizeof(GPUShader *)); i++) {
      DRW_SHADER_FREE_SAFE(sh_data_as_array[i]);
    }
  }
}