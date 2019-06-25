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
 *
 * Contributor(s): Yiming Wu
 *
 */

/** \file
 * \ingroup DNA
 */

#ifndef __LANPR_ALL_H__
#define __LANPR_ALL_H__

#include "lanpr_util.h"
#include "lanpr_data_types.h"
#include "BLI_mempool.h"
#include "BLI_utildefines.h"
/* #include "GPU_framebuffer.h" */
#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_uniformbuffer.h"
#include "GPU_viewport.h"
#include "DNA_listBase.h"
#include "DRW_render.h"
#include "BKE_object.h"
#include "DNA_mesh_types.h"
#include "DNA_camera_types.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_framebuffer.h"
#include "DNA_lanpr_types.h"
#include "DNA_meshdata_types.h"
#include "BKE_customdata.h"
#include "DEG_depsgraph_query.h"
#include "GPU_draw.h"

#include "BLI_threads.h"

#include "bmesh.h"

#include "WM_types.h"
#include "WM_api.h"

#define LANPR_ENGINE "BLENDER_LANPR"

#define TNS_PI 3.1415926535897932384626433832795
#define deg(r) r / TNS_PI * 180.0
#define rad(d) d *TNS_PI / 180.0

#define tMatDist2v(p1, p2) \
  sqrt(((p1)[0] - (p2)[0]) * ((p1)[0] - (p2)[0]) + ((p1)[1] - (p2)[1]) * ((p1)[1] - (p2)[1]))

#define tnsLinearItp(l, r, T) ((l) * (1.0f - (T)) + (r) * (T))

extern struct RenderEngineType DRW_engine_viewport_lanpr_type;
extern struct DrawEngineType draw_engine_lanpr_type;

typedef struct LANPR_RenderBuffer LANPR_RenderBuffer;

typedef struct LANPR_SharedResource {

  /* We only allocate once for all */
  LANPR_RenderBuffer *render_buffer_shared;

  /* cache */
  BLI_mempool *mp_sample;
  BLI_mempool *mp_line_strip;
  BLI_mempool *mp_line_strip_point;
  BLI_mempool *mp_batch_list;

  /* Snake */
  GPUShader *multichannel_shader;
  GPUShader *edge_detect_shader;
  GPUShader *edge_thinning_shader;
  GPUShader *snake_connection_shader;

  /* DPIX */
  GPUShader *dpix_transform_shader;
  GPUShader *dpix_preview_shader;

  /* Software */
  GPUShader *software_shader;
  GPUShader *software_chaining_shader;

  void *ved_viewport;
  void *ved_render;

  int init_complete;

  SpinLock render_flag_lock;
  int during_render; /*  get/set using access funcion which uses render_flag_lock to lock. */
  /*  this prevents duplicate too much resource. (no render preview in viewport */
  /*  while rendering) */

} LANPR_SharedResource;

#define TNS_DPIX_TEXTURE_SIZE 2048

typedef struct LANPR_PassList {
  /* Snake */
  struct DRWPass *depth_pass;
  struct DRWPass *color_pass;
  struct DRWPass *normal_pass;
  struct DRWPass *edge_intermediate;
  struct DRWPass *edge_thinning;
  struct DRWPass *snake_pass;

  /* DPIX */
  struct DRWPass *dpix_transform_pass;
  struct DRWPass *dpix_preview_pass;

  /* SOFTWARE */
  struct DRWPass *software_pass;

} LANPR_PassList;

typedef struct LANPR_FramebufferList {

  /* Snake */
  struct GPUFrameBuffer *passes;
  struct GPUFrameBuffer *edge_intermediate;
  struct GPUFrameBuffer *edge_thinning;

  /* DPIX */
  struct GPUFrameBuffer *dpix_transform;
  struct GPUFrameBuffer *dpix_preview;

  /* Software */
  struct GPUFrameBuffer *software_ms;

} LANPR_FramebufferList;

typedef struct LANPR_TextureList {

  struct GPUTexture *color;
  struct GPUTexture *normal;
  struct GPUTexture *depth;
  struct GPUTexture *edge_intermediate;

  struct GPUTexture *dpix_in_pl;        /* point l */
  struct GPUTexture *dpix_in_pr;        /* point r */
  struct GPUTexture *dpix_in_nl;        /* normal l */
  struct GPUTexture *dpix_in_nr;        /* normal r */
  struct GPUTexture *dpix_in_edge_mask; /* RGBA, r:Material, G: Freestyle Edge Mark, BA:Reserved
                                           for future usage */

  struct GPUTexture *dpix_out_pl;
  struct GPUTexture *dpix_out_pr;
  struct GPUTexture *dpix_out_length;

  /* multisample resolve */
  struct GPUTexture *ms_resolve_depth;
  struct GPUTexture *ms_resolve_color;

} LANPR_TextureList;

typedef struct LANPR_PrivateData {
  DRWShadingGroup *multipass_shgrp;
  DRWShadingGroup *edge_detect_shgrp;
  DRWShadingGroup *edge_thinning_shgrp;
  DRWShadingGroup *snake_shgrp;

  DRWShadingGroup *dpix_transform_shgrp;
  DRWShadingGroup *dpix_preview_shgrp;

  DRWShadingGroup *debug_shgrp;

  /*  snake */

  float normal_clamp;
  float normal_strength;
  float depth_clamp;
  float depth_strength;

  float zfar;
  float znear;

  int stage; /*  thinning */

  float *line_result;
  unsigned char *line_result_8bit;
  int width, height; /*  if not match recreate buffer. */
  void **sample_table;

  ListBase pending_samples;
  ListBase erased_samples;
  ListBase line_strips;

  /*  dpix data */

  void *atlas_pl;
  void *atlas_pr;
  void *atlas_nl;
  void *atlas_nr;
  void *atlas_edge_mask;

  int begin_index;

  int dpix_sample_step;
  int dpix_is_perspective;
  float dpix_viewport[4];
  float output_viewport[4];
  int dpix_buffer_width;
  float dpix_depth_offset;

  float dpix_znear;
  float dpix_zfar;

  /*  drawing */

  unsigned v_buf;
  unsigned i_buf;
  unsigned l_buf;

  GPUVertFormat snake_gwn_format;
  GPUBatch *snake_batch;

  ListBase dpix_batch_list;
} LANPR_PrivateData;

typedef struct LANPR_StorageList {
  LANPR_PrivateData *g_data;
} LANPR_StorageList;

typedef struct LANPR_BatchItem {
  Link item;
  GPUBatch *dpix_transform_batch;
  GPUBatch *dpix_preview_batch;
  Object *ob;
} LANPR_BatchItem;

typedef struct LANPR_Data {
  void *engine_type;
  LANPR_FramebufferList *fbl;
  LANPR_TextureList *txl;
  LANPR_PassList *psl;
  LANPR_StorageList *stl;
} LANPR_Data;

/* Below ported from NUL_TNS.h */

#define TNS_THREAD_LINE_COUNT 10000

#define TNS_CALCULATION_IDLE 0
#define TNS_CALCULATION_GEOMETRY 1
#define TNS_CALCULATION_CONTOUR 2
#define TNS_CALCULATION_INTERSECTION 3
#define TNS_CALCULATION_OCCLUTION 4
#define TNS_CALCULATION_FINISHED 100

typedef struct LANPR_RenderTaskInfo {
  /*  thrd_t           ThreadHandle; */

  int thread_id;

  LinkData *contour;
  ListBase contour_pointers;

  LinkData *intersection;
  ListBase intersection_pointers;

  LinkData *crease;
  ListBase crease_pointers;

  LinkData *material;
  ListBase material_pointers;

  LinkData *edge_mark;
  ListBase edge_mark_pointers;

} LANPR_RenderTaskInfo;

typedef struct LANPR_RenderBuffer {
  struct LANPR_RenderBuffer *prev, *next;

  int is_copied; /*  for render. */

  int w, h;
  int tile_size_w, tile_size_h;
  int tile_count_x, tile_count_y;
  real width_per_tile, height_per_tile;
  tnsMatrix44d view_projection;
  tnsMatrix44d vp_inverse;

  int output_mode;
  int output_aa_level;

  struct LANPR_BoundingArea *initial_bounding_areas;
  u32bit bounding_area_count;

  ListBase vertex_buffer_pointers;
  ListBase line_buffer_pointers;
  ListBase triangle_buffer_pointers;
  ListBase all_render_lines;

  ListBase intersecting_vertex_buffer;

  struct GPUBatch *DPIXIntersectionTransformBatch;
  struct GPUBatch *DPIXIntersectionBatch;

  /* use own-implemented one */
  LANPR_StaticMemPool render_data_pool;

  Material *material_pointers[2048];

  /*  render status */

  int cached_for_frame;

  real view_vector[3];

  int triangle_size;

  u32bit contour_count;
  u32bit contour_processed;
  LinkData *contour_managed;
  ListBase contours;

  u32bit intersection_count;
  u32bit intersection_processed;
  LinkData *intersection_managed;
  ListBase intersection_lines;

  u32bit crease_count;
  u32bit crease_processed;
  LinkData *crease_managed;
  ListBase crease_lines;

  u32bit material_line_count;
  u32bit material_processed;
  LinkData *material_managed;
  ListBase material_lines;

  u32bit edge_mark_count;
  u32bit edge_mark_processed;
  LinkData *edge_mark_managed;
  ListBase edge_marks;

  ListBase chains;
  GPUBatch *chain_draw_batch;

  DRWShadingGroup *ChainShgrp;

  SpinLock cs_info;
  SpinLock cs_data;
  SpinLock cs_management;

  /*  settings */

  int max_occlusion_level;
  real crease_angle;
  real crease_cos;
  int thread_count;

  real overall_progress;
  int calculation_status;

  int draw_material_preview;
  real material_transparency;

  int show_line;
  int show_fast;
  int show_material;
  int override_display;

  struct Scene *scene;
  struct Object *camera;

  int enable_intersections;
  int _pad;

} LANPR_RenderBuffer;

#define TNS_CULL_DISCARD 2
#define TNS_CULL_USED 1

typedef struct LANPR_BoundingArea {
  real l, r, u, b;
  real cx, cy;

  struct LANPR_BoundingArea *child; /*  1,2,3,4 quadrant */

  ListBase lp;
  ListBase rp;
  ListBase up;
  ListBase bp;

  int triangle_count;
  ListBase linked_triangles;
  ListBase linked_lines;

  ListBase linked_chains; /*  reserved for image space reduction && multithread chainning */
} LANPR_BoundingArea;

#define TNS_COMMAND_LINE 0
#define TNS_COMMAND_MATERIAL 1
#define TNS_COMMAND_EDGE 2

#define TNS_TRANSPARENCY_DRAW_SIMPLE 0
#define TNS_TRANSPARENCY_DRAW_LAYERED 1

#define TNS_OVERRIDE_ONLY 0
#define TNS_OVERRIDE_EXCLUDE 1
/* #define TNS_OVERRIDE_ALL_OTHERS_OUTSIDE_GROUP 2 */
/* #define TNS_OVERRIDE_ALL_OTHERS_IN_GROUP      3 */
/* #define TNS_OVERRIDE_ALL_OTHERS               4 */

#define tnsLinearItp(l, r, T) ((l) * (1.0f - (T)) + (r) * (T))

#define TNS_TILE(tile, r, c, CCount) tile[r * CCount + c]

#define TNS_CLAMP(a, Min, Max) a = a < Min ? Min : (a > Max ? Max : a)

#define TNS_MAX2_INDEX(a, b) (a > b ? 0 : 1)

#define TNS_MIN2_INDEX(a, b) (a < b ? 0 : 1)

#define TNS_MAX3_INDEX(a, b, c) (a > b ? (a > c ? 0 : (b > c ? 1 : 2)) : (b > c ? 1 : 2))

#define TNS_MIN3_INDEX(a, b, c) (a < b ? (a < c ? 0 : (b < c ? 1 : 2)) : (b < c ? 1 : 2))

#define TNS_MAX3_INDEX_ABC(x, y, z) (x > y ? (x > z ? a : (y > z ? b : c)) : (y > z ? b : c))

#define TNS_MIN3_INDEX_ABC(x, y, z) (x < y ? (x < z ? a : (y < z ? b : c)) : (y < z ? b : c))

#define TNS_ABC(index) (index == 0 ? a : (index == 1 ? b : c))

#define TNS_DOUBLE_CLOSE_ENOUGH(a, b) (((a) + DBL_EDGE_LIM) >= (b) && ((a)-DBL_EDGE_LIM) <= (b))

/* #define TNS_DOUBLE_CLOSE_ENOUGH(a,b)\ */
/*  //(((a)+0.00000000001)>=(b) && ((a)-0.0000000001)<=(b)) */

#define TNS_FLOAT_CLOSE_ENOUGH_WIDER(a, b) (((a) + 0.0000001) >= (b) && ((a)-0.0000001) <= (b))

#define TNS_IN_TILE_X(RenderTile, Fx) (RenderTile->FX <= Fx && RenderTile->FXLim >= Fx)

#define TNS_IN_TILE_Y(RenderTile, Fy) (RenderTile->FY <= Fy && RenderTile->FYLim >= Fy)

#define TNS_IN_TILE(RenderTile, Fx, Fy) \
  (TNS_IN_TILE_X(RenderTile, Fx) && TNS_IN_TILE_Y(RenderTile, Fy))

BLI_INLINE void tMatConvert44df(tnsMatrix44d from, tnsMatrix44f to)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
  to[4] = from[4];
  to[5] = from[5];
  to[6] = from[6];
  to[7] = from[7];
  to[8] = from[8];
  to[9] = from[9];
  to[10] = from[10];
  to[11] = from[11];
  to[12] = from[12];
  to[13] = from[13];
  to[14] = from[14];
  to[15] = from[15];
}

BLI_INLINE int lanpr_TrangleLineBoundBoxTest(LANPR_RenderTriangle *rt, LANPR_RenderLine *rl)
{
  if (MAX3(rt->v[0]->fbcoord[2], rt->v[1]->fbcoord[2], rt->v[2]->fbcoord[2]) >
      MIN2(rl->l->fbcoord[2], rl->r->fbcoord[2]))
    return 0;
  if (MAX3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]) <
      MIN2(rl->l->fbcoord[0], rl->r->fbcoord[0]))
    return 0;
  if (MIN3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]) >
      MAX2(rl->l->fbcoord[0], rl->r->fbcoord[0]))
    return 0;
  if (MAX3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]) <
      MIN2(rl->l->fbcoord[1], rl->r->fbcoord[1]))
    return 0;
  if (MIN3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]) >
      MAX2(rl->l->fbcoord[1], rl->r->fbcoord[1]))
    return 0;
  return 1;
}

BLI_INLINE double tMatGetLinearRatio(real l, real r, real FromL);
BLI_INLINE int lanpr_LineIntersectTest2d(
    const double *a1, const double *a2, const double *b1, const double *b2, double *aRatio)
{
  double k1, k2;
  double x;
  double y;
  double Ratio;
  double xDiff = (a2[0] - a1[0]); /*  +DBL_EPSILON; */
  double xDiff2 = (b2[0] - b1[0]);

  if (xDiff == 0) {
    if (xDiff2 == 0) {
      *aRatio = 0;
      return 0;
    }
    double r2 = tMatGetLinearRatio(b1[0], b2[0], a1[0]);
    x = tnsLinearItp(b1[0], b2[0], r2);
    y = tnsLinearItp(b1[1], b2[1], r2);
    *aRatio = Ratio = tMatGetLinearRatio(a1[1], a2[1], y);
  }
  else {
    if (xDiff2 == 0) {
      Ratio = tMatGetLinearRatio(a1[0], a2[0], b1[0]);
      x = tnsLinearItp(a1[0], a2[0], Ratio);
      *aRatio = Ratio;
    }
    else {
      k1 = (a2[1] - a1[1]) / xDiff;
      k2 = (b2[1] - b1[1]) / xDiff2;

      if ((k1 == k2))
        return 0;

      x = (a1[1] - b1[1] - k1 * a1[0] + k2 * b1[0]) / (k2 - k1);

      Ratio = (x - a1[0]) / xDiff;

      *aRatio = Ratio;
    }
  }

  if (b1[0] == b2[0]) {
    y = tnsLinearItp(a1[1], a2[1], Ratio);
    if (y > MAX2(b1[1], b2[1]) || y < MIN2(b1[1], b2[1]))
      return 0;
  }
  else if (Ratio <= 0 || Ratio > 1 || (b1[0] > b2[0] && x > b1[0]) ||
           (b1[0] < b2[0] && x < b1[0]) || (b2[0] > b1[0] && x > b2[0]) ||
           (b2[0] < b1[0] && x < b2[0]))
    return 0;

  return 1;
}
BLI_INLINE double lanpr_GetLineZ(tnsVector3d l, tnsVector3d r, real Ratio)
{
  /*  double z = 1 / tnsLinearItp(1 / l[2], 1 / r[2], Ratio); */
  double z = tnsLinearItp(l[2], r[2], Ratio);
  return z;
}
BLI_INLINE double lanpr_GetLineZPoint(tnsVector3d l, tnsVector3d r, tnsVector3d FromL)
{
  double ra = (FromL[0] - l[0]) / (r[0] - l[0]);
  return tnsLinearItp(l[2], r[2], ra);
  /*  return 1 / tnsLinearItp(1 / l[2], 1 / r[2], r); */
}
BLI_INLINE double lanpr_GetLinearRatio(tnsVector3d l, tnsVector3d r, tnsVector3d FromL)
{
  double ra = (FromL[0] - l[0]) / (r[0] - l[0]);
  return ra;
}

BLI_INLINE double tMatGetLinearRatio(real l, real r, real FromL)
{
  double ra = (FromL - l) / (r - l);
  return ra;
}
BLI_INLINE void tMatVectorMinus2d(tnsVector2d result, tnsVector2d l, tnsVector2d r)
{
  result[0] = l[0] - r[0];
  result[1] = l[1] - r[1];
}

BLI_INLINE void tMatVectorMinus3d(tnsVector3d result, tnsVector3d l, tnsVector3d r)
{
  result[0] = l[0] - r[0];
  result[1] = l[1] - r[1];
  result[2] = l[2] - r[2];
}
BLI_INLINE void tMatVectorSubtract3d(tnsVector3d l, tnsVector3d r)
{
  l[0] = l[0] - r[0];
  l[1] = l[1] - r[1];
  l[2] = l[2] - r[2];
}
BLI_INLINE void tMatVectorPlus3d(tnsVector3d result, tnsVector3d l, tnsVector3d r)
{
  result[0] = l[0] + r[0];
  result[1] = l[1] + r[1];
  result[2] = l[2] + r[2];
}
BLI_INLINE void tMatVectorAccum3d(tnsVector3d l, tnsVector3d r)
{
  l[0] = l[0] + r[0];
  l[1] = l[1] + r[1];
  l[2] = l[2] + r[2];
}
BLI_INLINE void tMatVectorAccum2d(tnsVector2d l, tnsVector2d r)
{
  l[0] = l[0] + r[0];
  l[1] = l[1] + r[1];
}
BLI_INLINE void tMatVectorNegate3d(tnsVector3d result, tnsVector3d l)
{
  result[0] = -l[0];
  result[1] = -l[1];
  result[2] = -l[2];
}
BLI_INLINE void tMatVectorNegateSelf3d(tnsVector3d l)
{
  l[0] = -l[0];
  l[1] = -l[1];
  l[2] = -l[2];
}
BLI_INLINE void tMatVectorCopy2d(tnsVector2d from, tnsVector2d to)
{
  to[0] = from[0];
  to[1] = from[1];
}
BLI_INLINE void tMatVectorCopy3d(tnsVector3d from, tnsVector3d to)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
}
BLI_INLINE void tMatVectorCopy4d(tnsVector4d from, tnsVector4d to)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
}
BLI_INLINE void tMatVectorMultiSelf4d(tnsVector3d from, real num)
{
  from[0] *= num;
  from[1] *= num;
  from[2] *= num;
  from[3] *= num;
}
BLI_INLINE void tMatVectorMultiSelf3d(tnsVector3d from, real num)
{
  from[0] *= num;
  from[1] *= num;
  from[2] *= num;
}
BLI_INLINE void tMatVectorMultiSelf2d(tnsVector3d from, real num)
{
  from[0] *= num;
  from[1] *= num;
}

BLI_INLINE real tMatDirectionToRad(tnsVector2d Dir)
{
  real arcc = acos(Dir[0]);
  real arcs = asin(Dir[1]);

  if (Dir[0] >= 0) {
    if (Dir[1] >= 0)
      return arcc;
    else
      return TNS_PI * 2 - arcc;
  }
  else {
    if (Dir[1] >= 0)
      return arcs + TNS_PI / 2;
    else
      return TNS_PI + arcs;
  }
}

BLI_INLINE void tMatVectorConvert4fd(tnsVector4f from, tnsVector4d to)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
  to[3] = from[3];
}

BLI_INLINE void tMatVectorConvert3fd(tnsVector3f from, tnsVector3d to)
{
  to[0] = from[0];
  to[1] = from[1];
  to[2] = from[2];
}

int lanpr_point_inside_triangled(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2);
real lanpr_LinearInterpolate(real l, real r, real T);
void lanpr_LinearInterpolate2dv(real *l, real *r, real T, real *Result);
void lanpr_LinearInterpolate3dv(real *l, real *r, real T, real *Result);

/*  functions */

/*  dpix */

void lanpr_init_atlas_inputs(void *ved);
void lanpr_destroy_atlas(void *ved);
int lanpr_feed_atlas_data_obj(void *vedata,
                              float *AtlasPointsL,
                              float *AtlasPointsR,
                              float *AtlasFaceNormalL,
                              float *AtlasFaceNormalR,
                              float *AtlasEdgeMask,
                              Object *ob,
                              int begin_index);

int lanpr_feed_atlas_data_intersection_cache(void *vedata,
                                             float *AtlasPointsL,
                                             float *AtlasPointsR,
                                             float *AtlasFaceNormalL,
                                             float *AtlasFaceNormalR,
                                             float *AtlasEdgeMask,
                                             int begin_index);

int lanpr_feed_atlas_trigger_preview_obj(void *vedata, Object *ob, int begin_index);
void lanpr_create_atlas_intersection_preview(void *vedata, int begin_index);

void lanpr_dpix_draw_scene(LANPR_TextureList *txl,
                           LANPR_FramebufferList *fbl,
                           LANPR_PassList *psl,
                           LANPR_PrivateData *pd,
                           SceneLANPR *lanpr,
                           GPUFrameBuffer *DefaultFB,
                           int is_render);

void lanpr_snake_draw_scene(LANPR_TextureList *txl,
                            LANPR_FramebufferList *fbl,
                            LANPR_PassList *psl,
                            LANPR_PrivateData *pd,
                            SceneLANPR *lanpr,
                            GPUFrameBuffer *DefaultFB,
                            int is_render);

void lanpr_software_draw_scene(void *vedata, GPUFrameBuffer *dfb, int is_render);

void lanpr_set_render_flag();
void lanpr_clear_render_flag();
int lanpr_during_render();

#endif
