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
  struct GPUFrameBuffer *edit_mesh_occlude_wire_fb;
} OVERLAY_FramebufferList;

typedef struct OVERLAY_TextureList {
  struct GPUTexture *temp_depth_tx;
  struct GPUTexture *outlines_id_tx;
  struct GPUTexture *outlines_color_tx[2];
  struct GPUTexture *edit_mesh_occlude_wire_tx;
} OVERLAY_TextureList;

#define NOT_IN_FRONT 0
#define IN_FRONT 1

typedef struct OVERLAY_PassList {
  DRWPass *edit_mesh_depth_ps[2];
  DRWPass *edit_mesh_verts_ps[2];
  DRWPass *edit_mesh_edges_ps[2];
  DRWPass *edit_mesh_faces_ps[2];
  DRWPass *edit_mesh_faces_cage_ps[2];
  DRWPass *edit_mesh_analysis_ps;
  DRWPass *edit_mesh_mix_occlude_ps;
  DRWPass *edit_mesh_normals_ps;
  DRWPass *edit_mesh_weight_ps;
  DRWPass *extra_ps[2];
  DRWPass *facing_ps;
  DRWPass *grid_ps;
  DRWPass *outlines_prepass_ps;
  DRWPass *outlines_detect_ps;
  DRWPass *outlines_expand_ps;
  DRWPass *outlines_bleed_ps;
  DRWPass *outlines_resolve_ps;
  DRWPass *wireframe_ps;
  DRWPass *wireframe_xray_ps;
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
  /** Edit Mesh */
  int data_mask[4];
} OVERLAY_ShadingData;

typedef struct OVERLAY_ExtraCallBuffers {
  DRWCallBuffer *groundline;

  DRWCallBuffer *light_point;
  DRWCallBuffer *light_sun;
  DRWCallBuffer *light_spot;
  DRWCallBuffer *light_spot_volume_outside;
  DRWCallBuffer *light_spot_volume_inside;
  DRWCallBuffer *light_area[2];

  DRWCallBuffer *camera_frame;
  DRWCallBuffer *camera_tria[2];
  DRWCallBuffer *camera_distances;
  DRWCallBuffer *camera_volume;
  DRWCallBuffer *camera_volume_frame;
} OVERLAY_ExtraCallBuffers;

typedef struct OVERLAY_PrivateData {
  DRWShadingGroup *edit_mesh_depth_grp[2];
  DRWShadingGroup *edit_mesh_faces_grp[2];
  DRWShadingGroup *edit_mesh_faces_cage_grp[2];
  DRWShadingGroup *edit_mesh_verts_grp[2];
  DRWShadingGroup *edit_mesh_edges_grp[2];
  DRWShadingGroup *edit_mesh_facedots_grp[2];
  DRWShadingGroup *edit_mesh_skin_roots_grp[2];
  DRWShadingGroup *edit_mesh_fnormals_grp;
  DRWShadingGroup *edit_mesh_vnormals_grp;
  DRWShadingGroup *edit_mesh_lnormals_grp;
  DRWShadingGroup *edit_mesh_analysis_grp;
  DRWShadingGroup *edit_mesh_weight_grp;
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

  DRWView *view_wires;
  DRWView *view_edit_faces;
  DRWView *view_edit_faces_cage;
  DRWView *view_edit_edges;
  DRWView *view_edit_verts;

  /** Two instances for in_front option and without. */
  OVERLAY_ExtraCallBuffers extra_call_buffers[2];

  View3DOverlay overlay;
  enum eContextObjectMode ctx_mode;
  bool clear_stencil;
  bool xray_enabled;
  bool xray_enabled_and_not_wire;
  short v3d_flag;
  DRWState clipping_state;
  OVERLAY_ShadingData shdata;

  struct {
    int ghost_ob;
    int edit_ob;
    bool do_zbufclip;
    bool do_faces;
    bool do_edges;
    bool select_vert;
    bool select_face;
    bool select_edge;
    int flag; /** Copy of v3d->overlay.edit_flag.  */
  } edit_mesh;
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

typedef struct OVERLAY_InstanceFormats {
  struct GPUVertFormat *instance_screenspace;
  struct GPUVertFormat *instance_color;
  struct GPUVertFormat *instance_screen_aligned;
  struct GPUVertFormat *instance_scaled;
  struct GPUVertFormat *instance_sized;
  struct GPUVertFormat *instance_outline;
  struct GPUVertFormat *instance_camera;
  struct GPUVertFormat *instance_distance_lines;
  struct GPUVertFormat *instance_spot;
  struct GPUVertFormat *instance_bone;
  struct GPUVertFormat *instance_bone_dof;
  struct GPUVertFormat *instance_bone_stick;
  struct GPUVertFormat *instance_bone_outline;
  struct GPUVertFormat *instance_bone_envelope;
  struct GPUVertFormat *instance_bone_envelope_distance;
  struct GPUVertFormat *instance_bone_envelope_outline;
  struct GPUVertFormat *instance_mball_handles;
  struct GPUVertFormat *pos_color;
  struct GPUVertFormat *pos;

  struct GPUVertFormat *instance_pos;
  struct GPUVertFormat *instance_extra;
} OVERLAY_InstanceFormats;

void OVERLAY_edit_mesh_init(OVERLAY_Data *vedata);
void OVERLAY_edit_mesh_cache_init(OVERLAY_Data *vedata);
void OVERLAY_edit_mesh_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_edit_mesh_draw(OVERLAY_Data *vedata);

void OVERLAY_extra_cache_init(OVERLAY_Data *vedata);
void OVERLAY_extra_cache_populate(OVERLAY_Data *vedata,
                                  Object *ob,
                                  OVERLAY_DupliData *dupli,
                                  bool init_dupli);
void OVERLAY_extra_draw(OVERLAY_Data *vedata);

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

GPUShader *OVERLAY_shader_depth_only(void);
GPUShader *OVERLAY_shader_edit_mesh_vert(void);
GPUShader *OVERLAY_shader_edit_mesh_edge(bool use_flat_interp);
GPUShader *OVERLAY_shader_edit_mesh_face(void);
GPUShader *OVERLAY_shader_edit_mesh_facedot(void);
GPUShader *OVERLAY_shader_edit_mesh_skin_root(void);
GPUShader *OVERLAY_shader_edit_mesh_normal_face(void);
GPUShader *OVERLAY_shader_edit_mesh_normal_vert(void);
GPUShader *OVERLAY_shader_edit_mesh_normal_loop(void);
GPUShader *OVERLAY_shader_edit_mesh_mix_occlude(void);
GPUShader *OVERLAY_shader_edit_mesh_analysis(void);
GPUShader *OVERLAY_shader_extra(void);
GPUShader *OVERLAY_shader_extra_grounline(void);
GPUShader *OVERLAY_shader_facing(void);
GPUShader *OVERLAY_shader_grid(void);
GPUShader *OVERLAY_shader_outline_prepass(bool use_wire);
GPUShader *OVERLAY_shader_outline_prepass_grid(void);
GPUShader *OVERLAY_shader_outline_resolve(void);
GPUShader *OVERLAY_shader_outline_expand(bool high_dpi);
GPUShader *OVERLAY_shader_outline_detect(bool use_wire);
GPUShader *OVERLAY_shader_paint_weight(void);
GPUShader *OVERLAY_shader_wireframe(void);
GPUShader *OVERLAY_shader_wireframe_select(void);

OVERLAY_InstanceFormats *OVERLAY_shader_instance_formats_get(void);

void OVERLAY_shader_free(void);

#endif /* __OVERLAY_PRIVATE_H__ */