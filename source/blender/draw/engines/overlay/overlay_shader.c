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

extern char datatoc_depth_only_vert_glsl[];
extern char datatoc_edit_mesh_common_lib_glsl[];
extern char datatoc_edit_mesh_frag_glsl[];
extern char datatoc_edit_mesh_geom_glsl[];
extern char datatoc_edit_mesh_vert_glsl[];
extern char datatoc_edit_mesh_normal_geom_glsl[];
extern char datatoc_edit_mesh_normal_vert_glsl[];
extern char datatoc_edit_mesh_mix_occlude_frag_glsl[];
extern char datatoc_edit_mesh_skin_root_vert_glsl[];
extern char datatoc_edit_mesh_analysis_vert_glsl[];
extern char datatoc_edit_mesh_analysis_frag_glsl[];
extern char datatoc_facing_frag_glsl[];
extern char datatoc_facing_vert_glsl[];
extern char datatoc_grid_frag_glsl[];
extern char datatoc_grid_vert_glsl[];
extern char datatoc_outline_detect_frag_glsl[];
extern char datatoc_outline_expand_frag_glsl[];
extern char datatoc_outline_prepass_frag_glsl[];
extern char datatoc_outline_prepass_geom_glsl[];
extern char datatoc_outline_prepass_vert_glsl[];
extern char datatoc_outline_lightprobe_grid_vert_glsl[];
extern char datatoc_outline_resolve_frag_glsl[];
extern char datatoc_paint_weight_frag_glsl[];
extern char datatoc_paint_weight_vert_glsl[];
extern char datatoc_wireframe_vert_glsl[];
extern char datatoc_wireframe_geom_glsl[];
extern char datatoc_wireframe_frag_glsl[];

extern char datatoc_gpu_shader_depth_only_frag_glsl[];
extern char datatoc_gpu_shader_point_varying_color_frag_glsl[];
extern char datatoc_gpu_shader_3D_smooth_color_frag_glsl[];
extern char datatoc_gpu_shader_uniform_color_frag_glsl[];
extern char datatoc_gpu_shader_flat_color_frag_glsl[];

extern char datatoc_common_fullscreen_vert_glsl[];
extern char datatoc_common_fxaa_lib_glsl[];
extern char datatoc_common_globals_lib_glsl[];
extern char datatoc_common_view_lib_glsl[];

typedef struct OVERLAY_Shaders {
  GPUShader *depth_only;
  GPUShader *grid;
  GPUShader *facing;
  GPUShader *edit_mesh_vert;
  GPUShader *edit_mesh_edge;
  GPUShader *edit_mesh_edge_flat;
  GPUShader *edit_mesh_face;
  GPUShader *edit_mesh_facedot;
  GPUShader *edit_mesh_skin_root;
  GPUShader *edit_mesh_vnormals;
  GPUShader *edit_mesh_lnormals;
  GPUShader *edit_mesh_fnormals;
  GPUShader *edit_mesh_mix_occlude;
  GPUShader *edit_mesh_analysis;
  GPUShader *outline_prepass;
  GPUShader *outline_prepass_wire;
  GPUShader *outline_prepass_lightprobe_grid;
  GPUShader *outline_resolve;
  GPUShader *outline_fade;
  GPUShader *outline_fade_large;
  GPUShader *outline_detect;
  GPUShader *outline_detect_wire;
  GPUShader *paint_weight;
  GPUShader *wireframe_select;
  GPUShader *wireframe;
} OVERLAY_Shaders;

static struct {
  OVERLAY_Shaders sh_data[GPU_SHADER_CFG_LEN];
} e_data = {{{NULL}}};

GPUShader *OVERLAY_shader_depth_only(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->depth_only) {
    sh_data->depth_only = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_depth_only_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_depth_only_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->depth_only;
}

GPUShader *OVERLAY_shader_edit_mesh_vert(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_vert) {
    sh_data->edit_mesh_vert = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_common_lib_glsl,
                                 datatoc_edit_mesh_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_point_varying_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define VERT\n", NULL},
    });
  }
  return sh_data->edit_mesh_vert;
}

GPUShader *OVERLAY_shader_edit_mesh_edge(bool use_flat_interp)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  GPUShader **sh = use_flat_interp ? &sh_data->edit_mesh_edge_flat : &sh_data->edit_mesh_edge;
  if (*sh == NULL) {
    /* Use geometry shader to draw edge wire-frame. This ensure us
     * the same result across platforms and more flexibility.
     * But we pay the cost of running a geometry shader.
     * In the future we might consider using only the vertex shader
     * and loading data manually with buffer textures. */
    const bool use_geom_shader = true;
    const bool use_smooth_wires = (U.gpu_flag & USER_GPU_FLAG_NO_EDIT_MODE_SMOOTH_WIRE) == 0;
    const char *geom_sh_code[] = {use_geom_shader ? sh_cfg->lib : NULL,
                                  datatoc_common_globals_lib_glsl,
                                  datatoc_common_view_lib_glsl,
                                  datatoc_edit_mesh_geom_glsl,
                                  NULL};
    const char *vert_sh_code[] = {sh_cfg->lib,
                                  datatoc_common_globals_lib_glsl,
                                  datatoc_common_view_lib_glsl,
                                  datatoc_edit_mesh_common_lib_glsl,
                                  datatoc_edit_mesh_vert_glsl,
                                  NULL};
    const char *frag_sh_code[] = {sh_cfg->lib,
                                  datatoc_common_globals_lib_glsl,
                                  datatoc_common_view_lib_glsl,
                                  datatoc_edit_mesh_common_lib_glsl,
                                  datatoc_edit_mesh_frag_glsl,
                                  NULL};
    const char *defs[] = {sh_cfg->def,
                          use_geom_shader ? "#define USE_GEOM_SHADER\n" : "",
                          use_smooth_wires ? "#define USE_SMOOTH_WIRE\n" : "",
                          use_flat_interp ? "#define FLAT\n" : "",
                          "#define EDGE\n",
                          NULL};

    *sh = GPU_shader_create_from_arrays({
        .vert = vert_sh_code,
        .frag = frag_sh_code,
        .geom = geom_sh_code,
        .defs = defs,
    });
  }
  return *sh;
}

GPUShader *OVERLAY_shader_edit_mesh_face(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_face) {
    sh_data->edit_mesh_face = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_common_lib_glsl,
                                 datatoc_edit_mesh_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_3D_smooth_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define FACE\n", NULL},
    });
  }
  return sh_data->edit_mesh_face;
}

GPUShader *OVERLAY_shader_edit_mesh_facedot(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_facedot) {
    sh_data->edit_mesh_facedot = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_common_lib_glsl,
                                 datatoc_edit_mesh_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_point_varying_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define FACEDOT\n", NULL},
    });
  }
  return sh_data->edit_mesh_facedot;
}

GPUShader *OVERLAY_shader_edit_mesh_normal_face(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];

  if (!sh_data->edit_mesh_fnormals) {
    sh_data->edit_mesh_fnormals = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_vert_glsl,
                                 NULL},
        .geom = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_geom_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_uniform_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define FACE_NORMALS\n", NULL},
    });
  }
  return sh_data->edit_mesh_fnormals;
}

GPUShader *OVERLAY_shader_edit_mesh_normal_vert(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_vnormals) {
    sh_data->edit_mesh_vnormals = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_vert_glsl,
                                 NULL},
        .geom = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_geom_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_uniform_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define VERT_NORMALS\n", NULL},
    });
  }
  return sh_data->edit_mesh_vnormals;
}

GPUShader *OVERLAY_shader_edit_mesh_normal_loop(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_lnormals) {
    sh_data->edit_mesh_lnormals = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_vert_glsl,
                                 NULL},
        .geom = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_normal_geom_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_uniform_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define LOOP_NORMALS\n", NULL},
    });
  }
  return sh_data->edit_mesh_lnormals;
}

GPUShader *OVERLAY_shader_edit_mesh_mix_occlude(void)
{
  OVERLAY_Shaders *sh_data = &e_data.sh_data[0];
  if (!sh_data->edit_mesh_mix_occlude) {
    sh_data->edit_mesh_mix_occlude = DRW_shader_create_fullscreen(
        datatoc_edit_mesh_mix_occlude_frag_glsl, NULL);
  }
  return sh_data->edit_mesh_mix_occlude;
}

GPUShader *OVERLAY_shader_edit_mesh_analysis(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_analysis) {
    sh_data->edit_mesh_analysis = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_analysis_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_edit_mesh_analysis_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->edit_mesh_analysis;
}

GPUShader *OVERLAY_shader_edit_mesh_skin_root(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->edit_mesh_skin_root) {
    sh_data->edit_mesh_skin_root = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_edit_mesh_common_lib_glsl,
                                 datatoc_edit_mesh_skin_root_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_gpu_shader_flat_color_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->edit_mesh_skin_root;
}

GPUShader *OVERLAY_shader_facing(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->facing) {
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

GPUShader *OVERLAY_shader_grid(void)
{
  OVERLAY_Shaders *sh_data = &e_data.sh_data[0];
  if (!sh_data->grid) {
    sh_data->grid = GPU_shader_create_from_arrays({
        .vert = (const char *[]){datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_grid_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_grid_frag_glsl,
                                 NULL},
    });
  }
  return sh_data->grid;
}

GPUShader *OVERLAY_shader_outline_prepass(bool use_wire)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (use_wire && !sh_data->outline_prepass_wire) {
    sh_data->outline_prepass_wire = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_outline_prepass_vert_glsl,
                                 NULL},
        .geom = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_outline_prepass_geom_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_outline_prepass_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, "#define USE_GEOM\n", NULL},
    });
  }
  else if (!sh_data->outline_prepass) {
    sh_data->outline_prepass = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_outline_prepass_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_outline_prepass_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return use_wire ? sh_data->outline_prepass_wire : sh_data->outline_prepass;
}

GPUShader *OVERLAY_shader_outline_prepass_grid(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->outline_prepass_lightprobe_grid) {
    sh_data->outline_prepass_lightprobe_grid = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_outline_lightprobe_grid_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_outline_prepass_frag_glsl, NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->outline_prepass_lightprobe_grid;
}

GPUShader *OVERLAY_shader_outline_resolve(void)
{
  OVERLAY_Shaders *sh_data = &e_data.sh_data[0];
  if (!sh_data->outline_resolve) {
    sh_data->outline_resolve = DRW_shader_create_with_lib(datatoc_common_fullscreen_vert_glsl,
                                                          NULL,
                                                          datatoc_outline_resolve_frag_glsl,
                                                          datatoc_common_fxaa_lib_glsl,
                                                          "#define FXAA_ALPHA\n"
                                                          "#define USE_FXAA\n");
  }
  return sh_data->outline_resolve;
}

GPUShader *OVERLAY_shader_outline_expand(bool high_dpi)
{
  OVERLAY_Shaders *sh_data = &e_data.sh_data[0];
  if (high_dpi && !sh_data->outline_fade_large) {
    sh_data->outline_fade_large = DRW_shader_create_fullscreen(datatoc_outline_expand_frag_glsl,
                                                               "#define LARGE_OUTLINE\n");
  }
  else if (!sh_data->outline_fade) {
    sh_data->outline_fade = DRW_shader_create_fullscreen(datatoc_outline_expand_frag_glsl, NULL);
  }
  return (high_dpi) ? sh_data->outline_fade_large : sh_data->outline_fade;
}

GPUShader *OVERLAY_shader_outline_detect(bool use_wire)
{
  OVERLAY_Shaders *sh_data = &e_data.sh_data[0];
  if (use_wire && !sh_data->outline_detect_wire) {
    sh_data->outline_detect_wire = DRW_shader_create_with_lib(datatoc_common_fullscreen_vert_glsl,
                                                              NULL,
                                                              datatoc_outline_detect_frag_glsl,
                                                              datatoc_common_globals_lib_glsl,
                                                              "#define WIRE\n");
  }
  else if (!sh_data->outline_detect) {
    sh_data->outline_detect = DRW_shader_create_with_lib(datatoc_common_fullscreen_vert_glsl,
                                                         NULL,
                                                         datatoc_outline_detect_frag_glsl,
                                                         datatoc_common_globals_lib_glsl,
                                                         NULL);
  }
  return (use_wire) ? sh_data->outline_detect_wire : sh_data->outline_detect;
}

GPUShader *OVERLAY_shader_paint_weight(void)
{
  const DRWContextState *draw_ctx = DRW_context_state_get();
  const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  OVERLAY_Shaders *sh_data = &e_data.sh_data[draw_ctx->sh_cfg];
  if (!sh_data->paint_weight) {
    sh_data->paint_weight = GPU_shader_create_from_arrays({
        .vert = (const char *[]){sh_cfg->lib,
                                 datatoc_common_globals_lib_glsl,
                                 datatoc_common_view_lib_glsl,
                                 datatoc_paint_weight_vert_glsl,
                                 NULL},
        .frag = (const char *[]){datatoc_common_globals_lib_glsl,
                                 datatoc_paint_weight_frag_glsl,
                                 NULL},
        .defs = (const char *[]){sh_cfg->def, NULL},
    });
  }
  return sh_data->paint_weight;
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