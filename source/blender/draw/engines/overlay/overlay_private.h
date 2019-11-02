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
 * \ingroup DNA
 */

#ifndef __OVERLAY_PRIVATE_H__
#define __OVERLAY_PRIVATE_H__

#ifdef __APPLE__
#  define USE_GEOM_SHADER_WORKAROUND 1
#else
#  define USE_GEOM_SHADER_WORKAROUND 0
#endif

typedef struct OVERLAY_FramebufferList {
  struct GPUFrameBuffer *outlines_prepass_fb;
  struct GPUFrameBuffer *outlines_process_fb[2];
} OVERLAY_FramebufferList;

typedef struct OVERLAY_TextureList {
  struct GPUTexture *tx;
} OVERLAY_TextureList;

typedef struct OVERLAY_PassList {
  struct DRWPass *grid_ps;
  struct DRWPass *facing_ps;
  struct DRWPass *outlines_prepass_ps;
  struct DRWPass *outlines_detect_ps;
  struct DRWPass *outlines_expand_ps;
  struct DRWPass *outlines_bleed_ps;
  struct DRWPass *outlines_resolve_ps;
  struct DRWPass *wireframe_ps;
  struct DRWPass *wireframe_xray_ps;
} OVERLAY_PassList;

/* Data used by GLSL shader. To be used as UBO. */
typedef struct OVERLAY_ShadingData {
  /** Grid */
  float grid_axes[3], grid_distance;
  float zplane_axes[3], grid_mesh_size;
  float grid_steps[8];
  float inv_viewport_size[2];
  float grid_line_size;
  int grid_flag;
  int zpos_flag;
  int zneg_flag;
  /** Wireframe */
  float wire_step_param;
} OVERLAY_ShadingData;

typedef struct OVERLAY_PrivateData {
  DRWShadingGroup *facing_grp;
  DRWShadingGroup *wires_grp;
  DRWShadingGroup *wires_xray_grp;
  DRWShadingGroup *outlines_active_grp;
  DRWShadingGroup *outlines_select_grp;
  DRWShadingGroup *outlines_select_dupli_grp;
  DRWShadingGroup *outlines_transform_grp;
  DRWShadingGroup *outlines_probe_transform_grp;
  DRWShadingGroup *outlines_probe_select_grp;
  DRWShadingGroup *outlines_probe_select_dupli_grp;
  DRWShadingGroup *outlines_probe_active_grp;
  DRWShadingGroup *outlines_probe_grid_grp;

  /* Temp buffer textures */
  struct GPUTexture *outlines_depth_tx;
  struct GPUTexture *outlines_id_tx;
  struct GPUTexture *outlines_color_tx[2];

  DRWView *view_wires;
  View3DOverlay overlay;
  bool clear_stencil;
  bool xray_enabled;
  bool xray_enabled_and_not_wire;
  short v3d_flag;
  DRWState clipping_state;
  OVERLAY_ShadingData shdata;
} OVERLAY_PrivateData; /* Transient data */

typedef struct OVERLAY_StorageList {
  struct OVERLAY_PrivateData *pd;
} OVERLAY_StorageList;

typedef struct OVERLAY_Data {
  void *engine_type;
  OVERLAY_FramebufferList *fbl;
  OVERLAY_TextureList *txl;
  OVERLAY_PassList *psl;
  OVERLAY_StorageList *stl;
} OVERLAY_Data;

typedef struct OVERLAY_DupliData {
  DRWShadingGroup *wire_shgrp;
  DRWShadingGroup *outline_shgrp;
  DRWShadingGroup *extra_shgrp;
  struct GPUBatch *wire_geom;
  struct GPUBatch *outline_geom;
  struct GPUBatch *extra_geom;
  short base_flag;
} OVERLAY_DupliData;

void OVERLAY_facing_init(OVERLAY_Data *vedata);
void OVERLAY_facing_cache_init(OVERLAY_Data *vedata);
void OVERLAY_facing_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_facing_draw(OVERLAY_Data *vedata);

void OVERLAY_grid_init(OVERLAY_Data *vedata);
void OVERLAY_grid_cache_init(OVERLAY_Data *vedata);
void OVERLAY_grid_draw(OVERLAY_Data *vedata);

void OVERLAY_outline_init(OVERLAY_Data *vedata);
void OVERLAY_outline_cache_init(OVERLAY_Data *vedata);
void OVERLAY_outline_cache_populate(OVERLAY_Data *vedata,
                                    Object *ob,
                                    OVERLAY_DupliData *dupli,
                                    bool init_dupli);
void OVERLAY_outline_draw(OVERLAY_Data *vedata);

void OVERLAY_wireframe_init(OVERLAY_Data *vedata);
void OVERLAY_wireframe_cache_init(OVERLAY_Data *vedata);
void OVERLAY_wireframe_cache_populate(OVERLAY_Data *vedata,
                                      Object *ob,
                                      OVERLAY_DupliData *dupli,
                                      bool init_dupli);
void OVERLAY_wireframe_draw(OVERLAY_Data *vedata);

GPUShader *OVERLAY_shader_grid(void);
GPUShader *OVERLAY_shader_facing(void);
GPUShader *OVERLAY_shader_outline_prepass(bool use_wire);
GPUShader *OVERLAY_shader_outline_prepass_grid(void);
GPUShader *OVERLAY_shader_outline_resolve(void);
GPUShader *OVERLAY_shader_outline_expand(bool high_dpi);
GPUShader *OVERLAY_shader_outline_detect(bool use_wire);
GPUShader *OVERLAY_shader_wireframe(void);
GPUShader *OVERLAY_shader_wireframe_select(void);
void OVERLAY_shader_free(void);

#endif /* __OVERLAY_PRIVATE_H__ */