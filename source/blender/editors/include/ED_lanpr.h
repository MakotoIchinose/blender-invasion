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
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup editors
 */

#ifndef __ED_LANPR_H__
#define __ED_LANPR_H__


#include <string.h>
/* #include "lanpr_all.h" */
#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_threads.h"

#include <math.h>
#include "BLI_math.h"

#include "DNA_lanpr_types.h"

#define _CRT_SECURE_NO_WARNINGS

#ifndef BYTE
#  define BYTE unsigned char
#endif

typedef double real;
typedef unsigned long long u64bit;
typedef unsigned int u32bit;
typedef unsigned short u16bit;
typedef unsigned short ushort;
typedef unsigned char u8bit;
typedef char nShortBuf[16];

typedef float tnsMatrix44f[16];

typedef real tnsMatrix44d[16];
typedef real tnsVector2d[2];
typedef real tnsVector3d[3];
typedef real tnsVector4d[4];
typedef float tnsVector3f[3];
typedef float tnsVector4f[4];
typedef int tnsVector2i[2];

typedef struct LANPR_StaticMemPoolNode {
  Link item;
  int used_byte;
  /*   <----------- User Mem Start Here */
} LANPR_StaticMemPoolNode;

typedef struct LANPR_StaticMemPool {
  int each_size;
  ListBase pools;
  SpinLock cs_mem;
} LANPR_StaticMemPool;

typedef struct LANPR_TextureSample {
  struct LANPR_TextureSample *next, *prev;
  int X, Y;
  float Z; /*  for future usage */
} LANPR_TextureSample;

typedef struct LANPR_LineStripPoint {
  struct LANPR_LineStripPoint *next, *prev;
  float P[3];
} LANPR_LineStripPoint;

typedef struct LANPR_LineStrip {
  struct LANPR_LineStrip *next, *prev;
  ListBase points;
  int point_count;
  float total_length;
} LANPR_LineStrip;

typedef struct LANPR_RenderTriangle {
  struct LANPR_RenderTriangle *next, *prev;
  struct LANPR_RenderVert *v[3];
  struct LANPR_RenderLine *rl[3];
  real gn[3];
  real gc[3];
  /*  struct BMFace *F; */
  short material_id;
  ListBase intersecting_verts;
  char cull_status;
  struct LANPR_RenderTriangle *testing; /*  Should Be tRT** testing[NumOfThreads] */
} LANPR_RenderTriangle;

typedef struct LANPR_RenderTriangleThread {
  struct LANPR_RenderTriangle base;
  struct LANPR_RenderLine *testing[127]; /*  max thread support; */
} LANPR_RenderTriangleThread;

typedef struct LANPR_RenderElementLinkNode {
  struct LANPR_RenderElementLinkNode *next, *prev;
  void *pointer;
  int element_count;
  void *object_ref;
  char additional;
} LANPR_RenderElementLinkNode;

typedef struct LANPR_RenderLineSegment {
  struct LANPR_RenderLineSegment *next, *prev;
  real at;                  /*  at==0: left    at==1: right  (this is in 2D projected space) */
  real at_global;           /*  to reconstruct 3d stroke     (XXX: implement global space?) */
  u8bit occlusion;          /*  after "at" point */
  short material_mask_mark; /*  e.g. to determine lines beind a glass window material. */
} LANPR_RenderLineSegment;

typedef struct LANPR_RenderVert {
  struct LANPR_RenderVert *next, *prev;
  real gloc[4];
  real fbcoord[4];
  int fbcoordi[2];
  struct BMVert *v; /*  Used As r When Intersecting */
  struct LANPR_RenderLine *intersecting_line;
  struct LANPR_RenderLine *intersecting_line2;
  struct LANPR_RenderTriangle *intersecting_with; /*    positive 1         Negative 0 */
  /*  tnsRenderTriangle* IntersectingOnFace;       /*          <|               |> */
  char positive;  /*                  l---->|----->r	l---->|----->r */
  char edge_used; /*                       <|		          |> */
} LANPR_RenderVert;

#define LANPR_EDGE_FLAG_EDGE_MARK 1
#define LANPR_EDGE_FLAG_CONTOUR 2
#define LANPR_EDGE_FLAG_CREASE 4
#define LANPR_EDGE_FLAG_MATERIAL 8
#define LANPR_EDGE_FLAG_INTERSECTION 16
#define LANPR_EDGE_FLAG_FLOATING 32 /*  floating edge, unimplemented yet */
#define LANPR_EDGE_FLAG_CHAIN_PICKED 64

#define LANPR_EDGE_FLAG_ALL_TYPE 0x3f

typedef struct LANPR_RenderLine {
  struct LANPR_RenderLine *next, *prev;
  struct LANPR_RenderVert *l, *r;
  struct LANPR_RenderTriangle *tl, *tr;
  ListBase segments;
  /*  tnsEdge*       Edge;/* should be edge material */
  /*  tnsRenderTriangle* testing;/* Should Be tRT** testing[NumOfThreads]	struct Materil */
  /*  *MaterialRef; */
  char min_occ;
  char flags; /*  also for line type determination on chainning */

  /*  still need this entry because culled lines will not add to object reln node */
  struct Object *object_ref;

  int edge_idx; /*  for gpencil stroke modifier */
} LANPR_RenderLine;

typedef struct LANPR_RenderLineChain {
  struct LANPR_RenderLineChain *next, *prev;
  ListBase chain;
  /*  int         SegmentCount;  /*  we count before draw cmd. */
  float length; /*  calculated before draw cmd. */
  char picked;  /*  used when re-connecting and gp stroke generation */
  char level;
  int type; /* Chain now only contains one type of segments */
  struct Object *object_ref;
} LANPR_RenderLineChain;

typedef struct LANPR_RenderLineChainItem {
  struct LANPR_RenderLineChainItem *next, *prev;
  float pos[3];  /*  need z value for fading */
  float gpos[3]; /*  for restore position to 3d space */
  float normal[3];
  char line_type; /*       style of [1]       style of [2] */
  char occlusion; /*  [1]--------------->[2]---------------->[3]--.... */
} LANPR_RenderLineChainItem;

typedef struct LANPR_ChainRegisterEntry {
  struct LANPR_ChainRegisterEntry *next, *prev;
  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  char picked;
  char is_left; /*  left/right mark. Because we revert list in chaining and we need the flag. */
} LANPR_ChainRegisterEntry;

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

  struct Material *material_pointers[2048];

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
  struct GPUBatch *chain_draw_batch;

  struct DRWShadingGroup *ChainShgrp;

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


typedef struct LANPR_SharedResource {

  /* We only allocate once for all */
  LANPR_RenderBuffer *render_buffer_shared;

  /* cache */
  struct BLI_mempool *mp_sample;
  struct BLI_mempool *mp_line_strip;
  struct BLI_mempool *mp_line_strip_point;
  struct BLI_mempool *mp_batch_list;

  /* Snake */
  struct GPUShader *multichannel_shader;
  struct GPUShader *edge_detect_shader;
  struct GPUShader *edge_thinning_shader;
  struct GPUShader *snake_connection_shader;

  /* DPIX */
  struct GPUShader *dpix_transform_shader;
  struct GPUShader *dpix_preview_shader;
  int dpix_shader_error;
  int texture_size;

  /* Software */
  struct GPUShader *software_shader;
  struct GPUShader *software_chaining_shader;

  void *ved_viewport;
  void *ved_render;

  int init_complete;

  SpinLock render_flag_lock;
  int during_render; /*  get/set using access funcion which uses render_flag_lock to lock. */
  /*  this prevents duplicate too much resource. (no render preview in viewport */
  /*  while rendering) */

} LANPR_SharedResource;

#define deg(r) r / M_PI * 180.0
#define rad(d) d *M_PI / 180.0

#define DBL_TRIANGLE_LIM 1e-8
#define DBL_EDGE_LIM 1e-9

#define NUL_MEMORY_POOL_1MB 1048576
#define NUL_MEMORY_POOL_128MB 134217728
#define NUL_MEMORY_POOL_256MB 268435456
#define NUL_MEMORY_POOL_512MB 536870912

#define LANPR_CULL_DISCARD 2
#define LANPR_CULL_USED 1


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
      return M_PI * 2 - arcc;
  }
  else {
    if (Dir[1] >= 0)
      return arcs + M_PI / 2;
    else
      return M_PI + arcs;
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

int ED_lanpr_point_inside_triangled(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2);

#define tnsLinearItp(l, r, T) ((l) * (1.0f - (T)) + (r) * (T))

#define CreateNew(Type) MEM_callocN(sizeof(Type), "VOID") /*  nutCalloc(sizeof(Type),1) */

#define CreateNew_Size(size) nutCalloc(size, 1)

#define CreateNewBuffer(Type, Num) \
  MEM_callocN(sizeof(Type) * Num, "VOID BUFFER") /*  nutCalloc(sizeof(Type),Num); */

void list_handle_empty(ListBase *h);

void list_clear_prev_next(Link *li);

void list_insert_item_before(ListBase *Handle, Link *toIns, Link *pivot);
void list_insert_item_after(ListBase *Handle, Link *toIns, Link *pivot);
void list_insert_segment_before(ListBase *Handle, Link *Begin, Link *End, Link *pivot);
void lstInsertSegmentAfter(ListBase *Handle, Link *Begin, Link *End, Link *pivot);
int lstHaveItemInList(ListBase *Handle);
void *lst_get_top(ListBase *Handle);

void *list_append_pointer_only(ListBase *h, void *p);
void *list_append_pointer_sized_only(ListBase *h, void *p, int size);
void *list_push_pointer_only(ListBase *h, void *p);
void *list_push_pointer_sized_only(ListBase *h, void *p, int size);

void *list_append_pointer(ListBase *h, void *p);
void *list_append_pointer_sized(ListBase *h, void *p, int size);
void *list_push_pointer(ListBase *h, void *p);
void *list_push_pointer_sized(ListBase *h, void *p, int size);

void *list_append_pointer_static(ListBase *h, LANPR_StaticMemPool *smp, void *p);
void *list_append_pointer_static_sized(ListBase *h, LANPR_StaticMemPool *smp, void *p, int size);
void *list_push_pointer_static(ListBase *h, LANPR_StaticMemPool *smp, void *p);
void *list_push_pointer_static_sized(ListBase *h, LANPR_StaticMemPool *smp, void *p, int size);

void *list_pop_pointer_only(ListBase *h);
void list_remove_pointer_item_only(ListBase *h, LinkData *lip);
void list_remove_pointer_only(ListBase *h, void *p);
void list_clear_pointer_only(ListBase *h);
void list_generate_pointer_list_only(ListBase *from1, ListBase *from2, ListBase *to);

void *list_pop_pointer(ListBase *h);
void list_remove_pointer_item(ListBase *h, LinkData *lip);
void list_remove_pointer(ListBase *h, void *p);
void list_clear_pointer(ListBase *h);
void list_generate_pointer_list(ListBase *from1, ListBase *from2, ListBase *to);

void list_copy_handle(ListBase *target, ListBase *src);

void *list_append_pointer_static_pool(LANPR_StaticMemPool *mph, ListBase *h, void *p);
void *list_pop_pointer_no_free(ListBase *h);
void list_remove_pointer_item_no_free(ListBase *h, LinkData *lip);

void list_move_up(ListBase *h, Link *li);
void list_move_down(ListBase *h, Link *li);

void lstAddElement(ListBase *hlst, void *ext);
void lstDestroyElementList(ListBase *hlst);

LANPR_StaticMemPoolNode *mem_new_static_pool(LANPR_StaticMemPool *smp);
void *mem_static_aquire(LANPR_StaticMemPool *smp, int size);
void *mem_static_aquire_thread(LANPR_StaticMemPool *smp, int size);
void *mem_static_destroy(LANPR_StaticMemPool *smp);

void tmat_obmat_to_16d(float obmat[4][4], tnsMatrix44d out);

real tmat_dist_idv2(real x1, real y1, real x2, real y2);
real tmat_dist_3dv(tnsVector3d l, tnsVector3d r);
real tmat_dist_2dv(tnsVector2d l, tnsVector2d r);

real tmat_length_3d(tnsVector3d l);
real tmat_length_2d(tnsVector3d l);
void tmat_normalize_3d(tnsVector3d result, tnsVector3d l);
void tmat_normalize_3f(tnsVector3f result, tnsVector3f l);
void tmat_normalize_self_3d(tnsVector3d result);
real tmat_dot_3d(tnsVector3d l, tnsVector3d r, int normalize);
real tmat_dot_3df(tnsVector3d l, tnsVector3f r, int normalize);
real tmat_dot_2d(tnsVector2d l, tnsVector2d r, int normalize);
real tmat_vector_cross_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);
real tmat_angle_rad_3d(tnsVector3d from, tnsVector3d to, tnsVector3d PositiveReference);
void tmat_apply_rotation_33d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_rotation_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_transform_43dfND(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_normal_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_normal_transform_43df(tnsVector3d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_transform_44d(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v);
void tmat_apply_transform_43df(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_transform_44dTrue(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v);

void tmat_load_identity_44d(tnsMatrix44d m);
void tmat_make_ortho_matrix_44d(
    tnsMatrix44d mProjection, real xMin, real xMax, real yMin, real yMax, real zMin, real zMax);
void tmat_make_perspective_matrix_44d(
    tnsMatrix44d mProjection, real fFov_rad, real fAspect, real zMin, real zMax);
void tmat_make_translation_matrix_44d(tnsMatrix44d mTrans, real x, real y, real z);
void tmat_make_rotation_matrix_44d(tnsMatrix44d m, real angle_rad, real x, real y, real z);
void tmat_make_scale_matrix_44d(tnsMatrix44d m, real x, real y, real z);
void tmat_make_viewport_matrix_44d(tnsMatrix44d m, real w, real h, real Far, real Near);
void tmat_multiply_44d(tnsMatrix44d result, tnsMatrix44d l, tnsMatrix44d r);
void tmat_inverse_44d(tnsMatrix44d inverse, tnsMatrix44d mat);
void tmat_make_rotation_x_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_make_rotation_y_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_make_rotation_z_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_remove_translation_44d(tnsMatrix44d result, tnsMatrix44d mat);
void tmat_clear_translation_44d(tnsMatrix44d mat);

real tmat_angle_rad_3d(tnsVector3d from, tnsVector3d to, tnsVector3d PositiveReference);
real tmat_length_3d(tnsVector3d l);
void tmat_normalize_2d(tnsVector2d result, tnsVector2d l);
void tmat_normalize_3d(tnsVector3d result, tnsVector3d l);
void tmat_normalize_self_3d(tnsVector3d result);
real tmat_dot_3d(tnsVector3d l, tnsVector3d r, int normalize);
real tmat_vector_cross_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);
void tmat_vector_cross_only_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);

int lanpr_count_this_line(LANPR_RenderLine *rl, LANPR_LineLayer *ll);
long lanpr_count_leveled_edge_segment_count(ListBase *LineList, LANPR_LineLayer *ll);
long lanpr_count_intersection_segment_count(LANPR_RenderBuffer *rb);
void *lanpr_make_leveled_edge_vertex_array(LANPR_RenderBuffer *rb,
                                           ListBase *LineList,
                                           float *vertexArray,
                                           float *NormalArray,
                                           float **NextNormal,
                                           LANPR_LineLayer *ll,
                                           float componet_id);

#endif /* __ED_LANPR_H__ */
