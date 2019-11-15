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
  DRWPass *armature_transp_ps;
  DRWPass *armature_ps[2];
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
  DRWPass *extra_blend_ps;
  DRWPass *extra_centers_ps;
  DRWPass *facing_ps;
  DRWPass *grid_ps;
  DRWPass *image_background_ps;
  DRWPass *image_empties_ps;
  DRWPass *image_empties_back_ps;
  DRWPass *image_empties_blend_ps;
  DRWPass *image_empties_front_ps;
  DRWPass *image_foreground_ps;
  DRWPass *motion_paths_ps;
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
  DRWCallBuffer *camera_frame;
  DRWCallBuffer *camera_tria[2];
  DRWCallBuffer *camera_distances;
  DRWCallBuffer *camera_volume;
  DRWCallBuffer *camera_volume_frame;

  DRWCallBuffer *center_active;
  DRWCallBuffer *center_selected;
  DRWCallBuffer *center_deselected;
  DRWCallBuffer *center_selected_lib;
  DRWCallBuffer *center_deselected_lib;

  DRWCallBuffer *empty_axes;
  DRWCallBuffer *empty_capsule_body;
  DRWCallBuffer *empty_capsule_cap;
  DRWCallBuffer *empty_circle;
  DRWCallBuffer *empty_cone;
  DRWCallBuffer *empty_cube;
  DRWCallBuffer *empty_cylinder;
  DRWCallBuffer *empty_image_frame;
  DRWCallBuffer *empty_plain_axes;
  DRWCallBuffer *empty_single_arrow;
  DRWCallBuffer *empty_sphere;
  DRWCallBuffer *empty_sphere_solid;

  DRWCallBuffer *extra_dashed_lines;
  DRWCallBuffer *extra_lines;

  DRWCallBuffer *field_curve;
  DRWCallBuffer *field_force;
  DRWCallBuffer *field_vortex;
  DRWCallBuffer *field_wind;
  DRWCallBuffer *field_cone_limit;
  DRWCallBuffer *field_sphere_limit;
  DRWCallBuffer *field_tube_limit;

  DRWCallBuffer *groundline;

  DRWCallBuffer *light_point;
  DRWCallBuffer *light_sun;
  DRWCallBuffer *light_spot;
  DRWCallBuffer *light_spot_cone_back;
  DRWCallBuffer *light_spot_cone_front;
  DRWCallBuffer *light_area[2];

  DRWCallBuffer *origin_xform;

  DRWCallBuffer *probe_planar;
  DRWCallBuffer *probe_cube;
  DRWCallBuffer *probe_grid;

  DRWCallBuffer *speaker;
} OVERLAY_ExtraCallBuffers;

typedef struct OVERLAY_ArmatureCallBuffers {
  DRWCallBuffer *box_outline;
  DRWCallBuffer *box_solid;

  DRWCallBuffer *dof_lines;
  DRWCallBuffer *dof_sphere;

  DRWCallBuffer *envelope_distance;
  DRWCallBuffer *envelope_outline;
  DRWCallBuffer *envelope_solid;

  DRWCallBuffer *octa_outline;
  DRWCallBuffer *octa_solid;

  DRWCallBuffer *point_outline;
  DRWCallBuffer *point_solid;

  DRWCallBuffer *stick;

  DRWCallBuffer *wire;

  DRWShadingGroup *custom_solid;
  DRWShadingGroup *custom_outline;
} OVERLAY_ArmatureCallBuffers;

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
  DRWShadingGroup *motion_path_lines_grp;
  DRWShadingGroup *motion_path_points_grp;
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

  /** TODO get rid of this. */
  ListBase smoke_domains;
  ListBase bg_movie_clips;

  /** Two instances for in_front option and without. */
  OVERLAY_ExtraCallBuffers extra_call_buffers[2];

  OVERLAY_ArmatureCallBuffers armature_call_buffers[2];

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
  struct {
    bool transparent;
    bool show_relations;
  } armature;
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
  // struct GPUVertFormat *instance_mball_handles;

  struct GPUVertFormat *instance_pos;
  struct GPUVertFormat *instance_extra;
  struct GPUVertFormat *instance_bone;
  struct GPUVertFormat *instance_bone_outline;
  struct GPUVertFormat *instance_bone_envelope;
  struct GPUVertFormat *instance_bone_envelope_distance;
  struct GPUVertFormat *instance_bone_envelope_outline;
  struct GPUVertFormat *instance_bone_stick;
  struct GPUVertFormat *pos;
  struct GPUVertFormat *pos_color;
  struct GPUVertFormat *wire_extra;
  struct GPUVertFormat *wire_dashed_extra;
} OVERLAY_InstanceFormats;

void OVERLAY_armature_cache_init(OVERLAY_Data *vedata);
void OVERLAY_armature_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_edit_armature_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_pose_armature_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_armature_draw(OVERLAY_Data *vedata);

void OVERLAY_edit_mesh_init(OVERLAY_Data *vedata);
void OVERLAY_edit_mesh_cache_init(OVERLAY_Data *vedata);
void OVERLAY_edit_mesh_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_edit_mesh_draw(OVERLAY_Data *vedata);

void OVERLAY_extra_cache_init(OVERLAY_Data *vedata);
void OVERLAY_extra_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_extra_draw(OVERLAY_Data *vedata);

void OVERLAY_camera_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_empty_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_gpencil_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_light_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_lightprobe_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_speaker_cache_populate(OVERLAY_Data *vedata, Object *ob);

OVERLAY_ExtraCallBuffers *OVERLAY_extra_call_buffer_get(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_extra_line_dashed(OVERLAY_ExtraCallBuffers *cb,
                               const float start[3],
                               const float end[3],
                               const float color[4]);
void OVERLAY_extra_line(OVERLAY_ExtraCallBuffers *cb,
                        const float start[3],
                        const float end[3],
                        const int color_id);
void OVERLAY_empty_shape(OVERLAY_ExtraCallBuffers *cb,
                         const float mat[4][4],
                         const float draw_size,
                         const char draw_type,
                         const float color[4]);

void OVERLAY_facing_init(OVERLAY_Data *vedata);
void OVERLAY_facing_cache_init(OVERLAY_Data *vedata);
void OVERLAY_facing_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_facing_draw(OVERLAY_Data *vedata);

void OVERLAY_grid_init(OVERLAY_Data *vedata);
void OVERLAY_grid_cache_init(OVERLAY_Data *vedata);
void OVERLAY_grid_draw(OVERLAY_Data *vedata);

void OVERLAY_image_cache_init(OVERLAY_Data *vedata);
void OVERLAY_image_camera_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_image_empty_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_image_cache_finish(OVERLAY_Data *vedata);
void OVERLAY_image_draw(OVERLAY_Data *vedata);

void OVERLAY_motion_path_cache_init(OVERLAY_Data *vedata);
void OVERLAY_motion_path_cache_populate(OVERLAY_Data *vedata, Object *ob);
void OVERLAY_motion_path_draw(OVERLAY_Data *vedata);

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

GPUShader *OVERLAY_shader_armature_degrees_of_freedom(void);
GPUShader *OVERLAY_shader_armature_envelope(bool use_outline);
GPUShader *OVERLAY_shader_armature_shape(bool use_outline);
GPUShader *OVERLAY_shader_armature_sphere(bool use_outline);
GPUShader *OVERLAY_shader_armature_stick(void);
GPUShader *OVERLAY_shader_armature_wire(void);
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
GPUShader *OVERLAY_shader_extra_groundline(void);
GPUShader *OVERLAY_shader_extra_wire(void);
GPUShader *OVERLAY_shader_extra_point(void);
GPUShader *OVERLAY_shader_facing(void);
GPUShader *OVERLAY_shader_grid(void);
GPUShader *OVERLAY_shader_image(void);
GPUShader *OVERLAY_shader_motion_path_line(void);
GPUShader *OVERLAY_shader_motion_path_vert(void);
GPUShader *OVERLAY_shader_outline_prepass(bool use_wire);
GPUShader *OVERLAY_shader_outline_prepass_grid(void);
GPUShader *OVERLAY_shader_outline_resolve(void);
GPUShader *OVERLAY_shader_outline_expand(bool high_dpi);
GPUShader *OVERLAY_shader_outline_detect(bool use_wire);
GPUShader *OVERLAY_shader_paint_weight(void);
GPUShader *OVERLAY_shader_volume_velocity(bool use_needle);
GPUShader *OVERLAY_shader_wireframe(void);
GPUShader *OVERLAY_shader_wireframe_select(void);

OVERLAY_InstanceFormats *OVERLAY_shader_instance_formats_get(void);

void OVERLAY_shader_free(void);

#endif /* __OVERLAY_PRIVATE_H__ */