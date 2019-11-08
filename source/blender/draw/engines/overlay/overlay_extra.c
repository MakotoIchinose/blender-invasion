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

#include "UI_resources.h"

#include "BKE_camera.h"

#include "DNA_camera_types.h"

#include "DEG_depsgraph_query.h"

#include "overlay_private.h"

#include "draw_common.h"

void OVERLAY_extra_cache_init(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;
  OVERLAY_PrivateData *pd = vedata->stl->pd;

  for (int i = 0; i < 2; i++) {
    /* Non Meshes Pass (Camera, empties, lights ...) */
    // struct GPUBatch *geom;
    struct GPUShader *sh;
    struct GPUVertFormat *format;
    DRWShadingGroup *grp, *grp_sub;
    float pixelsize = *DRW_viewport_pixelsize_get() * U.pixelsize;

    OVERLAY_InstanceFormats *formats = OVERLAY_shader_instance_formats_get();
    OVERLAY_ExtraCallBuffers *cb = &pd->extra_call_buffers[i];
    DRWPass **p_extra_ps = &psl->extra_ps[i];

    DRWState state = DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL;
    DRW_PASS_CREATE(*p_extra_ps, state | pd->clipping_state);

    DRWPass *extra_ps = *p_extra_ps;

#if 0
    /* Empties */
    empties_callbuffers_create(cb->non_meshes, &cb->empties, draw_ctx->sh_cfg);

    /* Force Field */
    geom = DRW_cache_field_wind_get();
    cb->field_wind = buffer_instance_scaled(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_field_force_get();
    cb->field_force = buffer_instance_screen_aligned(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_field_vortex_get();
    cb->field_vortex = buffer_instance_scaled(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_screenspace_circle_get();
    cb->field_curve_sta = buffer_instance_screen_aligned(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Grease Pencil */
    geom = DRW_cache_gpencil_axes_get();
    cb->gpencil_axes = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Speaker */
    geom = DRW_cache_speaker_get();
    cb->speaker = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Probe */
    static float probeSize = 14.0f;
    geom = DRW_cache_lightprobe_cube_get();
    cb->probe_cube = buffer_instance_screenspace(
        cb->non_meshes, geom, &probeSize, draw_ctx->sh_cfg);

    geom = DRW_cache_lightprobe_grid_get();
    cb->probe_grid = buffer_instance_screenspace(
        cb->non_meshes, geom, &probeSize, draw_ctx->sh_cfg);

    static float probePlanarSize = 20.0f;
    geom = DRW_cache_lightprobe_planar_get();
    cb->probe_planar = buffer_instance_screenspace(
        cb->non_meshes, geom, &probePlanarSize, draw_ctx->sh_cfg);

    /* Camera */
    geom = DRW_cache_camera_get();
    cb->camera = buffer_camera_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_camera_frame_get();
    cb->camera_frame = buffer_camera_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_camera_tria_get();
    cb->camera_tria = buffer_camera_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_plain_axes_get();
    cb->camera_focus = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_single_line_get();
    cb->camera_clip = buffer_distance_lines_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);
    cb->camera_mist = buffer_distance_lines_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_single_line_endpoints_get();
    cb->camera_clip_points = buffer_distance_lines_instance(
        cb->non_meshes, geom, draw_ctx->sh_cfg);
    cb->camera_mist_points = buffer_distance_lines_instance(
        cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_quad_wires_get();
    cb->camera_stereo_plane_wires = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    geom = DRW_cache_empty_cube_get();
    cb->camera_stereo_volume_wires = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    BLI_listbase_clear(&cb->camera_path);

    /* Texture Space */
    geom = DRW_cache_empty_cube_get();
    cb->texspace = buffer_instance(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Wires (for loose edges) */
    sh = GPU_shader_get_builtin_shader_with_config(GPU_SHADER_3D_UNIFORM_COLOR, draw_ctx->sh_cfg);
    cb->wire = shgroup_wire(cb->non_meshes, gb->colorWire, sh, draw_ctx->sh_cfg);
    cb->wire_select = shgroup_wire(cb->non_meshes, gb->colorSelect, sh, draw_ctx->sh_cfg);
    cb->wire_transform = shgroup_wire(cb->non_meshes, gb->colorTransform, sh, draw_ctx->sh_cfg);
    cb->wire_active = shgroup_wire(cb->non_meshes, gb->colorActive, sh, draw_ctx->sh_cfg);
    /* Wire (duplicator) */
    cb->wire_dupli = shgroup_wire(cb->non_meshes, gb->colorDupli, sh, draw_ctx->sh_cfg);
    cb->wire_dupli_select = shgroup_wire(
        cb->non_meshes, gb->colorDupliSelect, sh, draw_ctx->sh_cfg);

    /* Points (loose points) */
    sh = sh_data->loose_points;
    cb->points = shgroup_points(cb->non_meshes, gb->colorWire, sh, draw_ctx->sh_cfg);
    cb->points_select = shgroup_points(cb->non_meshes, gb->colorSelect, sh, draw_ctx->sh_cfg);
    cb->points_transform = shgroup_points(
        cb->non_meshes, gb->colorTransform, sh, draw_ctx->sh_cfg);
    cb->points_active = shgroup_points(cb->non_meshes, gb->colorActive, sh, draw_ctx->sh_cfg);
    /* Points (duplicator) */
    cb->points_dupli = shgroup_points(cb->non_meshes, gb->colorDupli, sh, draw_ctx->sh_cfg);
    cb->points_dupli_select = shgroup_points(
        cb->non_meshes, gb->colorDupliSelect, sh, draw_ctx->sh_cfg);
    DRW_shgroup_state_disable(cb->points, DRW_STATE_BLEND_ALPHA);
    DRW_shgroup_state_disable(cb->points_select, DRW_STATE_BLEND_ALPHA);
    DRW_shgroup_state_disable(cb->points_transform, DRW_STATE_BLEND_ALPHA);
    DRW_shgroup_state_disable(cb->points_active, DRW_STATE_BLEND_ALPHA);
    DRW_shgroup_state_disable(cb->points_dupli, DRW_STATE_BLEND_ALPHA);
    DRW_shgroup_state_disable(cb->points_dupli_select, DRW_STATE_BLEND_ALPHA);

    /* Metaballs Handles */
    cb->mball_handle = buffer_instance_mball_handles(cb->non_meshes, draw_ctx->sh_cfg);

#endif

#define BUF_INSTANCE DRW_shgroup_call_buffer_instance
#define BUF_POINT(grp, format) DRW_shgroup_call_buffer(grp, format, GPU_PRIM_POINTS)

    /* Sorted by shader to avoid state changes during render. */
    {
      format = formats->instance_extra;
      sh = OVERLAY_shader_extra();

      grp = DRW_shgroup_create(sh, extra_ps);
      DRW_shgroup_uniform_float_copy(grp, "pixel_size", pixelsize);
      DRW_shgroup_uniform_vec3(grp, "screen_vecs[0]", DRW_viewport_screenvecs_get(), 2);
      DRW_shgroup_uniform_block_persistent(grp, "globalsBlock", G_draw.block_ubo);

      grp_sub = DRW_shgroup_create_sub(grp);
      cb->light_point = BUF_INSTANCE(grp_sub, format, DRW_cache_light_point_lines_get());
      cb->light_sun = BUF_INSTANCE(grp_sub, format, DRW_cache_light_sun_lines_get());
      cb->light_area[0] = BUF_INSTANCE(grp_sub, format, DRW_cache_light_area_disk_lines_get());
      cb->light_area[1] = BUF_INSTANCE(grp_sub, format, DRW_cache_light_area_square_lines_get());
      cb->light_spot = BUF_INSTANCE(grp_sub, format, DRW_cache_light_spot_lines_get());

      cb->camera_frame = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_frame_get());
      cb->camera_tria[0] = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_tria_wire_get());
      cb->camera_tria[1] = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_tria_get());
      cb->camera_distances = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_distances_get());

      cb->empty_axes = BUF_INSTANCE(grp_sub, format, DRW_cache_bone_arrows_get());
      cb->empty_capsule_body = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_capsule_body_get());
      cb->empty_capsule_cap = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_capsule_cap_get());
      cb->empty_circle = BUF_INSTANCE(grp_sub, format, DRW_cache_circle_get());
      cb->empty_cone = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_cone_get());
      cb->empty_cube = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_cube_get());
      cb->empty_cylinder = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_cylinder_get());
      cb->empty_plain_axes = BUF_INSTANCE(grp_sub, format, DRW_cache_plain_axes_get());
      cb->empty_single_arrow = BUF_INSTANCE(grp_sub, format, DRW_cache_single_arrow_get());
      cb->empty_sphere = BUF_INSTANCE(grp_sub, format, DRW_cache_empty_sphere_get());
      cb->empty_sphere_solid = BUF_INSTANCE(grp_sub, format, DRW_cache_sphere_get());

      /* TODO Own move to transparent pass. */
      grp_sub = DRW_shgroup_create_sub(grp);
      DRW_shgroup_state_enable(grp_sub, DRW_STATE_CULL_BACK | DRW_STATE_BLEND_ALPHA);
      DRW_shgroup_state_disable(grp_sub, DRW_STATE_WRITE_DEPTH);
      cb->light_spot_volume_outside = BUF_INSTANCE(
          grp_sub, format, DRW_cache_light_spot_volume_get());

      grp_sub = DRW_shgroup_create_sub(grp);
      DRW_shgroup_state_enable(grp_sub, DRW_STATE_CULL_FRONT | DRW_STATE_BLEND_ALPHA);
      DRW_shgroup_state_disable(grp_sub, DRW_STATE_WRITE_DEPTH);
      cb->light_spot_volume_inside = BUF_INSTANCE(
          grp_sub, format, DRW_cache_light_spot_volume_get());

      grp_sub = DRW_shgroup_create_sub(grp);
      DRW_shgroup_state_enable(grp_sub, DRW_STATE_CULL_BACK | DRW_STATE_BLEND_ALPHA);
      DRW_shgroup_state_disable(grp_sub, DRW_STATE_WRITE_DEPTH);
      cb->camera_volume = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_volume_get());
      cb->camera_volume_frame = BUF_INSTANCE(grp_sub, format, DRW_cache_camera_volume_wire_get());
    }
    {
      format = formats->instance_pos;
      sh = OVERLAY_shader_extra_grounline();

      grp = DRW_shgroup_create(sh, extra_ps);
      DRW_shgroup_uniform_float_copy(grp, "pixel_size", pixelsize);
      DRW_shgroup_uniform_vec3(grp, "screen_vecs[0]", DRW_viewport_screenvecs_get(), 2);
      DRW_shgroup_uniform_block_persistent(grp, "globalsBlock", G_draw.block_ubo);
      DRW_shgroup_state_enable(grp, DRW_STATE_BLEND_ALPHA);

      cb->groundline = BUF_INSTANCE(grp, format, DRW_cache_groundline_get());
    }

#if 0
    /* -------- STIPPLES ------- */
    /* Relationship Lines */
    cb->relationship_lines = buffer_dynlines_dashed_uniform_color(
        cb->non_meshes, gb->colorWire, draw_ctx->sh_cfg);
    cb->constraint_lines = buffer_dynlines_dashed_uniform_color(
        cb->non_meshes, gb->colorGridAxisZ, draw_ctx->sh_cfg);

    {
      DRWShadingGroup *grp_axes;
      cb->origin_xform = buffer_instance_color_axes(
          cb->non_meshes, DRW_cache_bone_arrows_get(), &grp_axes, draw_ctx->sh_cfg);

      DRW_shgroup_state_disable(grp_axes, DRW_STATE_DEPTH_LESS_EQUAL);
      DRW_shgroup_state_enable(grp_axes, DRW_STATE_DEPTH_ALWAYS);
      DRW_shgroup_state_enable(grp_axes, DRW_STATE_WIRE_SMOOTH);
    }

    /* Force Field Curve Guide End (here because of stipple) */
    /* TODO port to shader stipple */
    geom = DRW_cache_screenspace_circle_get();
    cb->field_curve_end = buffer_instance_screen_aligned(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Force Field Limits */
    /* TODO port to shader stipple */
    geom = DRW_cache_field_tube_limit_get();
    cb->field_tube_limit = buffer_instance_scaled(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* TODO port to shader stipple */
    geom = DRW_cache_field_cone_limit_get();
    cb->field_cone_limit = buffer_instance_scaled(cb->non_meshes, geom, draw_ctx->sh_cfg);

    /* Transparent Shapes */
    state = DRW_STATE_WRITE_COLOR | DRW_STATE_DEPTH_LESS_EQUAL | DRW_STATE_BLEND_ALPHA |
            DRW_STATE_CULL_FRONT;
    cb->transp_shapes = psl->transp_shapes[i] = DRW_pass_create("Transparent Shapes", state);

    sh = GPU_shader_get_builtin_shader_with_config(
        GPU_SHADER_INSTANCE_VARIYING_COLOR_VARIYING_SIZE, draw_ctx->sh_cfg);

    DRWShadingGroup *grp_transp = DRW_shgroup_create(sh, cb->transp_shapes);
    if (draw_ctx->sh_cfg == GPU_SHADER_CFG_CLIPPED) {
      DRW_shgroup_state_enable(grp_transp, DRW_STATE_CLIP_PLANES);
    }

    DRWShadingGroup *grp_cull_back = DRW_shgroup_create_sub(grp_transp);
    DRW_shgroup_state_disable(grp_cull_back, DRW_STATE_CULL_FRONT);
    DRW_shgroup_state_enable(grp_cull_back, DRW_STATE_CULL_BACK);

    DRWShadingGroup *grp_cull_none = DRW_shgroup_create_sub(grp_transp);
    DRW_shgroup_state_disable(grp_cull_none, DRW_STATE_CULL_FRONT);

    /* Spot cones */
    geom = DRW_cache_light_spot_volume_get();
    cb->light_spot_volume = buffer_instance_alpha(grp_transp, geom);

    geom = DRW_cache_light_spot_square_volume_get();
    cb->light_spot_volume_rect = buffer_instance_alpha(grp_transp, geom);

    geom = DRW_cache_light_spot_volume_get();
    cb->light_spot_volume_outside = buffer_instance_alpha(grp_cull_back, geom);

    geom = DRW_cache_light_spot_square_volume_get();
    cb->light_spot_volume_rect_outside = buffer_instance_alpha(grp_cull_back, geom);

    /* Camera stereo volumes */
    geom = DRW_cache_cube_get();
    cb->camera_stereo_volume = buffer_instance_alpha(grp_transp, geom);

    geom = DRW_cache_quad_get();
    cb->camera_stereo_plane = buffer_instance_alpha(grp_cull_none, geom);
#endif
  }
}

/* -------------------------------------------------------------------- */
/** \name Empties
 * \{ */

static void DRW_shgroup_empty_ex(OVERLAY_ExtraCallBuffers *cb,
                                 const float mat[4][4],
                                 const float *draw_size,
                                 char draw_type,
                                 const float color[4])
{
  float instdata[4][4];
  copy_m4_m4(instdata, mat);
  instdata[3][3] = *draw_size;

  switch (draw_type) {
    case OB_PLAINAXES:
      DRW_buffer_add_entry(cb->empty_plain_axes, color, instdata);
      break;
    case OB_SINGLE_ARROW:
      DRW_buffer_add_entry(cb->empty_single_arrow, color, instdata);
      break;
    case OB_CUBE:
      DRW_buffer_add_entry(cb->empty_cube, color, instdata);
      break;
    case OB_CIRCLE:
      DRW_buffer_add_entry(cb->empty_circle, color, instdata);
      break;
    case OB_EMPTY_SPHERE:
      DRW_buffer_add_entry(cb->empty_sphere, color, instdata);
      break;
    case OB_EMPTY_CONE:
      DRW_buffer_add_entry(cb->empty_cone, color, instdata);
      break;
    case OB_ARROWS:
      DRW_buffer_add_entry(cb->empty_axes, color, instdata);
      break;
    case OB_EMPTY_IMAGE:
      BLI_assert(!"Should never happen, use DRW_shgroup_empty instead.");
      break;
  }
}

static void DRW_shgroup_empty(OVERLAY_ExtraCallBuffers *cb, Object *ob, ViewLayer *view_layer)
{
  float *color;
  DRW_object_wire_theme_get(ob, view_layer, &color);

  switch (ob->empty_drawtype) {
    case OB_PLAINAXES:
    case OB_SINGLE_ARROW:
    case OB_CUBE:
    case OB_CIRCLE:
    case OB_EMPTY_SPHERE:
    case OB_EMPTY_CONE:
    case OB_ARROWS:
      DRW_shgroup_empty_ex(cb, ob->obmat, &ob->empty_drawsize, ob->empty_drawtype, color);
      break;
    case OB_EMPTY_IMAGE:
      // DRW_shgroup_empty_image(sh_data, sgl, ob, color, rv3d, sh_cfg);
      break;
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lights
 * \{ */

static void DRW_shgroup_light(OVERLAY_ExtraCallBuffers *cb, Object *ob, ViewLayer *view_layer)
{
  Light *la = ob->data;
  float *color_p;
  DRW_object_wire_theme_get(ob, view_layer, &color_p);
  /* Remove the alpha. */
  float color[4] = {color_p[0], color_p[1], color_p[2], 1.0f};
  /* Pack render data into object matrix. */
  union {
    float mat[4][4];
    struct {
      float _pad00[3];
      union {
        float area_size_x;
        float spot_cosine;
      };
      float _pad01[3];
      union {
        float area_size_y;
        float spot_blend;
      };
      float _pad02[3], clip_sta;
      float pos[3], clip_end;
    };
  } instdata;

  copy_m4_m4(instdata.mat, ob->obmat);
  /* FIXME / TODO: clipend has no meaning nowadays.
   * In EEVEE, Only clipsta is used shadowmaping.
   * Clip end is computed automatically based on light power. */
  instdata.clip_end = la->clipend;
  instdata.clip_sta = la->clipsta;

  DRW_buffer_add_entry(cb->groundline, instdata.pos);

  if (la->type == LA_LOCAL) {
    instdata.area_size_x = instdata.area_size_y = la->area_size;
    DRW_buffer_add_entry(cb->light_point, color, &instdata);
  }
  else if (la->type == LA_SUN) {
    DRW_buffer_add_entry(cb->light_sun, color, &instdata);
  }
  else if (la->type == LA_SPOT) {
    /* For cycles and eevee the spot attenuation is
     * y = (1/(1 + x^2) - a)/((1 - a) b)
     * We solve the case where spot attenuation y = 1 and y = 0
     * root for y = 1 is  (-1 - c) / c
     * root for y = 0 is  (1 - a) / a
     * and use that to position the blend circle. */
    float a = cosf(la->spotsize * 0.5f);
    float b = la->spotblend;
    float c = a * b - a - b;
    /* Optimized version or root1 / root0 */
    instdata.spot_blend = sqrtf((-a - c * a) / (c - c * a));
    instdata.spot_cosine = a;
    /* HACK: We pack the area size in alpha color. This is decoded by the shader. */
    color[3] = -max_ff(la->area_size, FLT_MIN);
    DRW_buffer_add_entry(cb->light_spot, color, &instdata);

    if ((la->mode & LA_SHOW_CONE) && !DRW_state_is_select()) {
      float color_inside[4] = {0.0f, 0.0f, 0.0f, 0.5f};
      float color_outside[4] = {1.0f, 1.0f, 1.0f, 0.3f};
      DRW_buffer_add_entry(cb->light_spot_volume_inside, color_inside, &instdata);
      DRW_buffer_add_entry(cb->light_spot_volume_outside, color_outside, &instdata);
    }
  }
  else if (la->type == LA_AREA) {
    bool uniform_scale = !ELEM(la->area_shape, LA_AREA_RECT, LA_AREA_ELLIPSE);
    int sqr = ELEM(la->area_shape, LA_AREA_SQUARE, LA_AREA_RECT);
    instdata.area_size_x = la->area_size;
    instdata.area_size_y = uniform_scale ? la->area_size : la->area_sizey;
    DRW_buffer_add_entry(cb->light_area[sqr], color, &instdata);
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Camera
 * \{ */

typedef union OVERLAY_CameraInstanceData {
  /* Pack render data into object matrix and object color. */
  struct {
    float color[4];
    float mat[4][4];
  };
  struct {
    float _pad0[2];
    float volume_sta;
    union {
      float depth;
      float focus;
      float volume_end;
    };
    float _pad00[3];
    union {
      float corner_x;
      float dist_color_id;
    };
    float _pad01[3];
    union {
      float corner_y;
    };
    float _pad02[3];
    union {
      float center_x;
      float clip_sta;
      float mist_sta;
    };
    float pos[3];
    union {
      float center_y;
      float clip_end;
      float mist_end;
    };
  };
} OVERLAY_CameraInstanceData;

static float camera_offaxis_shiftx_get(Scene *scene,
                                       Object *ob,
                                       const OVERLAY_CameraInstanceData *instdata,
                                       bool right_eye)
{
  Camera *cam = ob->data;
  if (cam->stereo.convergence_mode == CAM_S3D_OFFAXIS) {
    const char *viewnames[2] = {STEREO_LEFT_NAME, STEREO_RIGHT_NAME};
    const float shiftx = BKE_camera_multiview_shift_x(&scene->r, ob, viewnames[right_eye]);
    const float delta_shiftx = shiftx - cam->shiftx;
    const float width = instdata->corner_x * 2.0f;
    return delta_shiftx * width;
  }
  else {
    return 0.0;
  }
}
/**
 * Draw the stereo 3d support elements (cameras, plane, volume).
 * They are only visible when not looking through the camera:
 */
static void camera_stereoscopy_extra(OVERLAY_ExtraCallBuffers *cb,
                                     Scene *scene,
                                     View3D *v3d,
                                     Object *ob,
                                     const OVERLAY_CameraInstanceData *instdata)
{
  OVERLAY_CameraInstanceData stereodata = *instdata;
  Camera *cam = ob->data;
  const bool is_select = DRW_state_is_select();
  const char *viewnames[2] = {STEREO_LEFT_NAME, STEREO_RIGHT_NAME};

  const bool is_stereo3d_cameras = (v3d->stereo3d_flag & V3D_S3D_DISPCAMERAS) != 0;
  const bool is_stereo3d_plane = (v3d->stereo3d_flag & V3D_S3D_DISPPLANE) != 0;
  const bool is_stereo3d_volume = (v3d->stereo3d_flag & V3D_S3D_DISPVOLUME) != 0;

  for (int eye = 0; eye < 2; eye++) {
    ob = BKE_camera_multiview_render(scene, ob, viewnames[eye]);
    BKE_camera_multiview_model_matrix(&scene->r, ob, viewnames[eye], stereodata.mat);

    stereodata.corner_x = instdata->corner_x;
    stereodata.corner_y = instdata->corner_y;
    stereodata.center_x = instdata->center_x + camera_offaxis_shiftx_get(scene, ob, instdata, eye);
    stereodata.center_y = instdata->center_y;
    stereodata.depth = instdata->depth;

    if (is_stereo3d_cameras) {
      DRW_buffer_add_entry_struct(cb->camera_frame, &stereodata);

      /* Connecting line between cameras. */
      /* TODO */
      // DRW_buffer_add_entry(cb->relationship_lines, obmat[3]);
    }

    if (is_stereo3d_volume && !is_select) {
      float r = (eye == 1) ? 2.0f : 1.0f;

      stereodata.volume_sta = -cam->clip_start;
      stereodata.volume_end = -cam->clip_end;
      /* Encode eye + intensity and alpha (see shader) */
      copy_v2_fl2(stereodata.color, r + 0.15f, 1.0f);
      DRW_buffer_add_entry_struct(cb->camera_volume_frame, &stereodata);

      if (v3d->stereo3d_volume_alpha > 0.0f) {
        /* Encode eye + intensity and alpha (see shader) */
        copy_v2_fl2(stereodata.color, r + 0.999f, v3d->stereo3d_volume_alpha);
        DRW_buffer_add_entry_struct(cb->camera_volume, &stereodata);
      }
      /* restore */
      copy_v3_v3(stereodata.color, instdata->color);
    }
  }

  if (is_stereo3d_plane && !is_select) {
    if (cam->stereo.convergence_mode == CAM_S3D_TOE) {
      /* There is no real convergence plane but we highlight the center
       * point where the views are pointing at. */
      // zero_v3(stereodata.mat[0]); /* We reconstruct from Z and Y */
      // zero_v3(stereodata.mat[1]); /* Y doesn't change */
      zero_v3(stereodata.mat[2]);
      zero_v3(stereodata.mat[3]);
      for (int i = 0; i < 2; i++) {
        float mat[4][4];
        /* Need normalized version here. */
        BKE_camera_multiview_model_matrix(&scene->r, ob, viewnames[i], mat);
        add_v3_v3(stereodata.mat[2], mat[2]);
        madd_v3_v3fl(stereodata.mat[3], mat[3], 0.5f);
      }
      normalize_v3(stereodata.mat[2]);
      cross_v3_v3v3(stereodata.mat[0], stereodata.mat[1], stereodata.mat[2]);
    }
    else if (cam->stereo.convergence_mode == CAM_S3D_PARALLEL) {
      /* Show plane at the given distance between the views even if it makes no sense. */
      zero_v3(stereodata.pos);
      for (int i = 0; i < 2; i++) {
        float mat[4][4];
        BKE_camera_multiview_model_matrix_scaled(&scene->r, ob, viewnames[i], mat);
        madd_v3_v3fl(stereodata.pos, mat[3], 0.5f);
      }
    }
    else if (cam->stereo.convergence_mode == CAM_S3D_OFFAXIS) {
      /* Nothing to do. Everything is already setup. */
    }
    stereodata.volume_sta = -cam->stereo.convergence_distance;
    stereodata.volume_end = -cam->stereo.convergence_distance;
    /* Encode eye + intensity and alpha (see shader) */
    copy_v2_fl2(stereodata.color, 0.1f, 1.0f);
    DRW_buffer_add_entry_struct(cb->camera_volume_frame, &stereodata);

    if (v3d->stereo3d_convergence_alpha > 0.0f) {
      /* Encode eye + intensity and alpha (see shader) */
      copy_v2_fl2(stereodata.color, 0.0f, v3d->stereo3d_convergence_alpha);
      DRW_buffer_add_entry_struct(cb->camera_volume, &stereodata);
    }
  }
}

static void DRW_shgroup_camera(OVERLAY_ExtraCallBuffers *cb, Object *ob, ViewLayer *view_layer)
{
  OVERLAY_CameraInstanceData instdata;

  const DRWContextState *draw_ctx = DRW_context_state_get();
  View3D *v3d = draw_ctx->v3d;
  Scene *scene = draw_ctx->scene;
  RegionView3D *rv3d = draw_ctx->rv3d;

  Camera *cam = ob->data;
  Object *camera_object = DEG_get_evaluated_object(draw_ctx->depsgraph, v3d->camera);
  const bool is_select = DRW_state_is_select();
  const bool is_active = (ob == camera_object);
  const bool look_through = (is_active && (rv3d->persp == RV3D_CAMOB));

  const bool is_multiview = (scene->r.scemode & R_MULTIVIEW) != 0;
  const bool is_stereo3d_view = (scene->r.views_format == SCE_VIEWS_FORMAT_STEREO_3D);
  const bool is_stereo3d_display_extra = is_active && is_multiview && (!look_through) &&
                                         ((v3d->stereo3d_flag) != 0);
  const bool is_selection_camera_stereo = is_select && look_through && is_multiview &&
                                          is_stereo3d_view;

  float vec[4][3], asp[2], shift[2], scale[3], drawsize, center[2], corner[2];

  float *color_p;
  DRW_object_wire_theme_get(ob, view_layer, &color_p);
  copy_v4_v4(instdata.color, color_p);

  normalize_m4_m4(instdata.mat, ob->obmat);

  /* BKE_camera_multiview_model_matrix already accounts for scale, don't do it here. */
  if (is_selection_camera_stereo) {
    copy_v3_fl(scale, 1.0f);
  }
  else {
    copy_v3_fl3(scale, len_v3(ob->obmat[0]), len_v3(ob->obmat[1]), len_v3(ob->obmat[2]));
    invert_v3(scale);
  }

  BKE_camera_view_frame_ex(
      scene, cam, cam->drawsize, look_through, scale, asp, shift, &drawsize, vec);

  /* Apply scale to simplify the rest of the drawing. */
  invert_v3(scale);
  for (int i = 0; i < 4; i++) {
    mul_v3_v3(vec[i], scale);
    /* Project to z=-1 plane. Makes positionning / scaling easier. (see shader) */
    mul_v2_fl(vec[i], 1.0f / fabsf(vec[i][2]));
  }

  /* Frame coords */
  mid_v2_v2v2(center, vec[0], vec[2]);
  sub_v2_v2v2(corner, vec[0], center);
  instdata.corner_x = corner[0];
  instdata.corner_y = corner[1];
  instdata.center_x = center[0];
  instdata.center_y = center[1];
  instdata.depth = vec[0][2];

  if (look_through) {
    if (!DRW_state_is_image_render()) {
      /* Only draw the frame. */
      if (is_multiview) {
        float mat[4][4];
        const bool is_right = v3d->multiview_eye == STEREO_RIGHT_ID;
        const char *view_name = is_right ? STEREO_RIGHT_NAME : STEREO_LEFT_NAME;
        BKE_camera_multiview_model_matrix(&scene->r, ob, view_name, mat);
        instdata.center_x += camera_offaxis_shiftx_get(scene, ob, &instdata, is_right);
        for (int i = 0; i < 4; i++) {
          /* Partial copy to avoid overriding packed data. */
          copy_v3_v3(instdata.mat[i], mat[i]);
        }
      }
      instdata.depth = -instdata.depth; /* Hides the back of the camera wires (see shader). */
      DRW_buffer_add_entry_struct(cb->camera_frame, &instdata);
    }
  }
  else {
    /* Stereo cameras, volumes, plane drawing. */
    if (is_stereo3d_display_extra) {
      camera_stereoscopy_extra(cb, scene, v3d, ob, &instdata);
    }
    else {
      DRW_buffer_add_entry_struct(cb->camera_frame, &instdata);
    }
  }

  {
    /* Triangle. */
    float tria_size = 0.7f * drawsize / fabsf(instdata.depth);
    float tria_margin = 0.1f * drawsize / fabsf(instdata.depth);
    instdata.center_x = center[0];
    instdata.center_y = center[1] + instdata.corner_y + tria_margin + tria_size;
    instdata.corner_x = instdata.corner_y = -tria_size;
    DRW_buffer_add_entry_struct(cb->camera_tria[is_active], &instdata);
  }

  if (cam->flag & CAM_SHOWLIMITS) {
    /* Scale focus point. */
    mul_v3_fl(instdata.mat[0], cam->drawsize);
    mul_v3_fl(instdata.mat[1], cam->drawsize);

    instdata.dist_color_id = (is_active) ? 3 : 2;
    instdata.focus = -BKE_camera_object_dof_distance(ob);
    instdata.clip_sta = cam->clip_start;
    instdata.clip_end = cam->clip_end;
    DRW_buffer_add_entry_struct(cb->camera_distances, &instdata);
  }

  if (cam->flag & CAM_SHOWMIST) {
    World *world = scene->world;
    if (world) {
      instdata.dist_color_id = (is_active) ? 1 : 0;
      instdata.focus = 1.0f; /* Disable */
      instdata.mist_sta = world->miststa;
      instdata.mist_end = world->miststa + world->mistdist;
      DRW_buffer_add_entry_struct(cb->camera_distances, &instdata);
    }
  }

#if 0
  /* Motion Tracking. */
  camera_view3d_reconstruction(sgl, scene, v3d, camera_object, ob, color, is_select);
#endif
}

void OVERLAY_extra_cache_populate(OVERLAY_Data *vedata,
                                  Object *ob,
                                  OVERLAY_DupliData *UNUSED(dupli),
                                  bool UNUSED(init_dupli))
{
  bool do_in_front = (ob->dtx & OB_DRAWXRAY) != 0;
  OVERLAY_PrivateData *pd = vedata->stl->pd;
  OVERLAY_ExtraCallBuffers *cb = &pd->extra_call_buffers[do_in_front];
  const DRWContextState *draw_ctx = DRW_context_state_get();
  ViewLayer *view_layer = draw_ctx->view_layer;

  // if (dupli && !init_dupli) {
  //   if (dupli->extra_shgrp && dupli->extra_geom) {
  //     DRW_shgroup_call(dupli->extra_shgrp, dupli->extra_geom, ob);
  //     return;
  //   }
  // }

  // int theme_id = DRW_object_wire_theme_get(ob, view_layer, NULL);

  // struct GPUBatch *geom = NULL;
  // DRWShadingGroup *shgroup = NULL;
  switch (ob->type) {
    case OB_SURF:
    case OB_LATTICE:
    case OB_FONT:
    case OB_CURVE: {
      // switch (ob->type) {
      //   case OB_SURF:
      //     geom = (!is_edit_mode) ? DRW_cache_surf_edge_wire_get(ob) : NULL;
      //     break;
      //   case OB_LATTICE:
      //     geom = (!is_edit_mode) ? DRW_cache_lattice_wire_get(ob, false) : NULL;
      //     break;
      //   case OB_CURVE:
      //     geom = (!is_edit_mode) ? DRW_cache_curve_edge_wire_get(ob) : NULL;
      //     break;
      //   case OB_FONT:
      //     geom = (!is_edit_mode) ? DRW_cache_text_edge_wire_get(ob) : NULL;
      //     break;
      // }
      // if (geom == NULL) {
      //   shgroup = shgroup_theme_id_to_wire(sgl, theme_id, ob->base_flag);
      //   DRW_shgroup_call(shgroup, geom, ob);
      // }
      break;
    }
    case OB_MBALL:
      // if (!is_edit_mode) {
      //   DRW_shgroup_mball_handles(sgl, ob, view_layer);
      // }
      break;
    case OB_LAMP:
      DRW_shgroup_light(cb, ob, view_layer);
      break;
    case OB_CAMERA:
      DRW_shgroup_camera(cb, ob, view_layer);
      // DRW_shgroup_camera_background_images(sh_data, psl, ob, rv3d);
      break;
    case OB_SPEAKER:
      // DRW_shgroup_speaker(sgl, ob, view_layer);
      break;
    case OB_LIGHTPROBE:
      // DRW_shgroup_lightprobe(sh_data, stl, psl, ob, view_layer, draw_ctx->sh_cfg);
      break;
    case OB_EMPTY:
      if (!(((ob->base_flag & BASE_FROM_DUPLI) != 0) &&
            ((ob->transflag & OB_DUPLICOLLECTION) != 0) && ob->instance_collection)) {
        DRW_shgroup_empty(cb, ob, view_layer);
      }
      break;
  }

    // if (init_duplidata) {
    //   dupli_data->extra_shgrp = shgroup;
    //   dupli_data->extra_geom = geom;
    //   dupli_data->base_flag = ob->base_flag;
    // }
#if 0
  if (ob->pd && ob->pd->forcefield) {
    DRW_shgroup_forcefield(sgl, ob, view_layer);
  }

  if ((ob->dt == OB_BOUNDBOX) &&
      !ELEM(ob->type, OB_LAMP, OB_CAMERA, OB_EMPTY, OB_SPEAKER, OB_LIGHTPROBE)) {

    DRW_shgroup_bounds(sgl, ob, theme_id, ob->boundtype, false);
  }

  /* Helpers for when we're transforming origins. */
  if (draw_ctx->object_mode == OB_MODE_OBJECT) {
    if (scene->toolsettings->transform_flag & SCE_XFORM_DATA_ORIGIN) {
      if (ob->base_flag & BASE_SELECTED) {
        if (!DRW_state_is_select()) {
          const float color[4] = {0.75, 0.75, 0.75, 0.5};
          float axes_size = 1.0f;
          DRW_buffer_add_entry(sgl->origin_xform, color, &axes_size, ob->obmat);
        }
      }
    }
  }

  /* don't show object extras in set's */
  if ((ob->base_flag & (BASE_FROM_SET | BASE_FROM_DUPLI)) == 0) {
    if ((draw_ctx->object_mode & (OB_MODE_ALL_PAINT | OB_MODE_ALL_PAINT_GPENCIL)) == 0) {
      DRW_shgroup_object_center(stl, ob, view_layer, v3d);
    }

    if (show_relations && !DRW_state_is_select()) {
      DRW_shgroup_relationship_lines(sgl, draw_ctx->depsgraph, scene, ob);
    }

    const bool draw_extra = ob->dtx & (OB_DRAWNAME | OB_TEXSPACE | OB_DRAWBOUNDOX);

    if ((ob->dtx & OB_DRAWNAME) && DRW_state_show_text()) {
      struct DRWTextStore *dt = DRW_text_cache_ensure();

      uchar color[4];
      UI_GetThemeColor4ubv(theme_id, color);

      DRW_text_cache_add(dt,
                         ob->obmat[3],
                         ob->id.name + 2,
                         strlen(ob->id.name + 2),
                         10,
                         0,
                         DRW_TEXT_CACHE_GLOBALSPACE | DRW_TEXT_CACHE_STRING_PTR,
                         color);

      /* draw grease pencil stroke names */
      if (ob->type == OB_GPENCIL) {
        OBJECT_gpencil_color_names(ob, dt, color);
      }
    }

    if ((ob->dtx & OB_TEXSPACE) && ELEM(ob->type, OB_MESH, OB_CURVE, OB_MBALL)) {
      DRW_shgroup_texture_space(sgl, ob, theme_id);
    }

    /* Don't draw bounding box again if draw type is bound box. */
    if ((ob->dtx & OB_DRAWBOUNDOX) && (ob->dt != OB_BOUNDBOX) &&
        !ELEM(ob->type, OB_LAMP, OB_CAMERA, OB_EMPTY, OB_SPEAKER, OB_LIGHTPROBE)) {
      DRW_shgroup_bounds(sgl, ob, theme_id, ob->boundtype, false);
    }

    if (ob->dtx & OB_AXIS) {
      float *color, axes_size = 1.0f;
      DRW_object_wire_theme_get(ob, view_layer, &color);

      DRW_buffer_add_entry(sgl->empties.empty_axes, color, &axes_size, ob->obmat);
    }

    if (ob->rigidbody_object) {
      DRW_shgroup_collision(sgl, ob, theme_id);
    }

    if ((md = modifiers_findByType(ob, eModifierType_Smoke)) &&
        (modifier_isEnabled(scene, md, eModifierMode_Realtime)) &&
        (((SmokeModifierData *)md)->domain != NULL)) {
      DRW_shgroup_volume_extra(sgl, ob, view_layer, scene, md);
    }
  }
#endif
}

void OVERLAY_extra_draw(OVERLAY_Data *vedata)
{
  OVERLAY_PassList *psl = vedata->psl;

  DRW_draw_pass(psl->extra_ps[0]);
  DRW_draw_pass(psl->extra_ps[1]);
}
