#include "DRW_engine.h"
#include "DRW_render.h"
#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_math_matrix.h"
#include "BLI_task.h"
#include "BLI_utildefines.h"
#include "lanpr_all.h"
#include "lanpr_util.h"
#include "DRW_render.h"
#include "BKE_object.h"
#include "DNA_mesh_types.h"
#include "DNA_camera_types.h"
#include "DNA_modifier_types.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_framebuffer.h"
#include "DNA_lanpr_types.h"
#include "DNA_meshdata_types.h"
#include "BKE_customdata.h"
#include "DEG_depsgraph_query.h"
#include "BKE_camera.h"
#include "BKE_collection.h"
#include "BKE_report.h"
#include "GPU_draw.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_uniformbuffer.h"
#include "GPU_viewport.h"
#include "bmesh.h"
#include "bmesh_class.h"
#include "bmesh_tools.h"

#include "WM_types.h"
#include "WM_api.h"

#include <math.h>
#include "RNA_access.h"
#include "RNA_define.h"

/*

   Ported from NUL4.0

   Author(s):WuYiming - xp8110@outlook.com

 */

struct Object;

int lanpr_triangle_line_imagespace_intersection_v2(SpinLock *spl,
                                                   LANPR_RenderTriangle *rt,
                                                   LANPR_RenderLine *rl,
                                                   Object *cam,
                                                   tnsMatrix44d vp,
                                                   real *CameraDir,
                                                   double *From,
                                                   double *To);
void lanpr_compute_view_Vector(LANPR_RenderBuffer *rb);

int use_smooth_contour_modifier_contour = 1;  // debug purpose

/* ====================================== base structures
 * =========================================== */

#define TNS_BOUND_AREA_CROSSES(b1, b2) \
  ((b1)[0] < (b2)[1] && (b1)[1] > (b2)[0] && (b1)[3] < (b2)[2] && (b1)[2] > (b2)[3])

void lanpr_make_initial_bounding_areas(LANPR_RenderBuffer *rb)
{
  int sp_w = 4;  // 20;
  int sp_h = 4;  // rb->H / (rb->W / sp_w);
  int row, col;
  LANPR_BoundingArea *ba;
  real W = (real)rb->w;
  real H = (real)rb->h;
  real span_w = (real)1 / sp_w * 2.0;
  real span_h = (real)1 / sp_h * 2.0;

  rb->tile_count_x = sp_w;
  rb->tile_count_y = sp_h;
  rb->width_per_tile = span_w;
  rb->height_per_tile = span_h;

  rb->bounding_area_count = sp_w * sp_h;
  rb->initial_bounding_areas = mem_static_aquire(
      &rb->render_data_pool, sizeof(LANPR_BoundingArea) * rb->bounding_area_count);

  for (row = 0; row < sp_h; row++) {
    for (col = 0; col < sp_w; col++) {
      ba = &rb->initial_bounding_areas[row * 4 + col];

      ba->l = span_w * col - 1.0;
      ba->r = (col == sp_w - 1) ? 1.0 : (span_w * (col + 1) - 1.0);
      ba->u = 1.0 - span_h * row;
      ba->b = (row == sp_h - 1) ? -1.0 : (1.0 - span_h * (row + 1));

      ba->cx = (ba->l + ba->r) / 2;
      ba->cy = (ba->u + ba->b) / 2;

      if (row) {
        list_append_pointer_static(
            &ba->up, &rb->render_data_pool, &rb->initial_bounding_areas[(row - 1) * 4 + col]);
      }
      if (col) {
        list_append_pointer_static(
            &ba->lp, &rb->render_data_pool, &rb->initial_bounding_areas[row * 4 + col - 1]);
      }
      if (row != sp_h - 1) {
        list_append_pointer_static(
            &ba->bp, &rb->render_data_pool, &rb->initial_bounding_areas[(row + 1) * 4 + col]);
      }
      if (col != sp_w - 1) {
        list_append_pointer_static(
            &ba->rp, &rb->render_data_pool, &rb->initial_bounding_areas[row * 4 + col + 1]);
      }
    }
  }
}
void lanpr_connect_new_bounding_areas(LANPR_RenderBuffer *rb, LANPR_BoundingArea *Root)
{
  LANPR_BoundingArea *ba = Root->child, *tba;
  LinkData *lip, *lip2, *next_lip;
  LANPR_StaticMemPool *mph = &rb->render_data_pool;

  list_append_pointer_static_pool(mph, &ba[1].rp, &ba[0]);
  list_append_pointer_static_pool(mph, &ba[0].lp, &ba[1]);
  list_append_pointer_static_pool(mph, &ba[1].bp, &ba[2]);
  list_append_pointer_static_pool(mph, &ba[2].up, &ba[1]);
  list_append_pointer_static_pool(mph, &ba[2].rp, &ba[3]);
  list_append_pointer_static_pool(mph, &ba[3].lp, &ba[2]);
  list_append_pointer_static_pool(mph, &ba[3].up, &ba[0]);
  list_append_pointer_static_pool(mph, &ba[0].bp, &ba[3]);

  for (lip = Root->lp.first; lip; lip = lip->next) {
    tba = lip->data;
    if (ba[1].u > tba->b && ba[1].b < tba->u) {
      list_append_pointer_static_pool(mph, &ba[1].lp, tba);
      list_append_pointer_static_pool(mph, &tba->rp, &ba[1]);
    }
    if (ba[2].u > tba->b && ba[2].b < tba->u) {
      list_append_pointer_static_pool(mph, &ba[2].lp, tba);
      list_append_pointer_static_pool(mph, &tba->rp, &ba[2]);
    }
  }
  for (lip = Root->rp.first; lip; lip = lip->next) {
    tba = lip->data;
    if (ba[0].u > tba->b && ba[0].b < tba->u) {
      list_append_pointer_static_pool(mph, &ba[0].rp, tba);
      list_append_pointer_static_pool(mph, &tba->lp, &ba[0]);
    }
    if (ba[3].u > tba->b && ba[3].b < tba->u) {
      list_append_pointer_static_pool(mph, &ba[3].rp, tba);
      list_append_pointer_static_pool(mph, &tba->lp, &ba[3]);
    }
  }
  for (lip = Root->up.first; lip; lip = lip->next) {
    tba = lip->data;
    if (ba[0].r > tba->l && ba[0].l < tba->r) {
      list_append_pointer_static_pool(mph, &ba[0].up, tba);
      list_append_pointer_static_pool(mph, &tba->bp, &ba[0]);
    }
    if (ba[1].r > tba->l && ba[1].l < tba->r) {
      list_append_pointer_static_pool(mph, &ba[1].up, tba);
      list_append_pointer_static_pool(mph, &tba->bp, &ba[1]);
    }
  }
  for (lip = Root->bp.first; lip; lip = lip->next) {
    tba = lip->data;
    if (ba[2].r > tba->l && ba[2].l < tba->r) {
      list_append_pointer_static_pool(mph, &ba[2].bp, tba);
      list_append_pointer_static_pool(mph, &tba->up, &ba[2]);
    }
    if (ba[3].r > tba->l && ba[3].l < tba->r) {
      list_append_pointer_static_pool(mph, &ba[3].bp, tba);
      list_append_pointer_static_pool(mph, &tba->up, &ba[3]);
    }
  }
  for (lip = Root->lp.first; lip; lip = lip->next) {
    for (lip2 = ((LANPR_BoundingArea *)lip->data)->rp.first; lip2; lip2 = next_lip) {
      next_lip = lip2->next;
      tba = lip2->data;
      if (tba == Root) {
        list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->rp, lip2);
        if (ba[1].u > tba->b && ba[1].b < tba->u)
          list_append_pointer_static_pool(mph, &tba->rp, &ba[1]);
        if (ba[2].u > tba->b && ba[2].b < tba->u)
          list_append_pointer_static_pool(mph, &tba->rp, &ba[2]);
      }
    }
  }
  for (lip = Root->rp.first; lip; lip = lip->next) {
    for (lip2 = ((LANPR_BoundingArea *)lip->data)->lp.first; lip2; lip2 = next_lip) {
      next_lip = lip2->next;
      tba = lip2->data;
      if (tba == Root) {
        list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->lp, lip2);
        if (ba[0].u > tba->b && ba[0].b < tba->u)
          list_append_pointer_static_pool(mph, &tba->lp, &ba[0]);
        if (ba[3].u > tba->b && ba[3].b < tba->u)
          list_append_pointer_static_pool(mph, &tba->lp, &ba[3]);
      }
    }
  }
  for (lip = Root->up.first; lip; lip = lip->next) {
    for (lip2 = ((LANPR_BoundingArea *)lip->data)->bp.first; lip2; lip2 = next_lip) {
      next_lip = lip2->next;
      tba = lip2->data;
      if (tba == Root) {
        list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->bp, lip2);
        if (ba[0].r > tba->l && ba[0].l < tba->r)
          list_append_pointer_static_pool(mph, &tba->up, &ba[0]);
        if (ba[1].r > tba->l && ba[1].l < tba->r)
          list_append_pointer_static_pool(mph, &tba->up, &ba[1]);
      }
    }
  }
  for (lip = Root->bp.first; lip; lip = lip->next) {
    for (lip2 = ((LANPR_BoundingArea *)lip->data)->up.first; lip2; lip2 = next_lip) {
      next_lip = lip2->next;
      tba = lip2->data;
      if (tba == Root) {
        list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->up, lip2);
        if (ba[2].r > tba->l && ba[2].l < tba->r)
          list_append_pointer_static_pool(mph, &tba->bp, &ba[2]);
        if (ba[3].r > tba->l && ba[3].l < tba->r)
          list_append_pointer_static_pool(mph, &tba->bp, &ba[3]);
      }
    }
  }
  while (list_pop_pointer_no_free(&Root->lp))
    ;
  while (list_pop_pointer_no_free(&Root->rp))
    ;
  while (list_pop_pointer_no_free(&Root->up))
    ;
  while (list_pop_pointer_no_free(&Root->bp))
    ;
}
void lanpr_link_triangle_with_bounding_area(LANPR_RenderBuffer *rb,
                                            LANPR_BoundingArea *RootBoundingArea,
                                            LANPR_RenderTriangle *rt,
                                            real *LRUB,
                                            int Recursive);
void lanpr_triangle_enable_intersections_in_bounding_area(LANPR_RenderBuffer *rb,
                                                          LANPR_RenderTriangle *rt,
                                                          LANPR_BoundingArea *ba);

void lanpr_split_bounding_area(LANPR_RenderBuffer *rb, LANPR_BoundingArea *Root)
{
  LANPR_BoundingArea *ba = mem_static_aquire(&rb->render_data_pool,
                                             sizeof(LANPR_BoundingArea) * 4);
  LANPR_RenderTriangle *rt;

  ba[0].l = Root->cx;
  ba[0].r = Root->r;
  ba[0].u = Root->u;
  ba[0].b = Root->cy;
  ba[0].cx = (ba[0].l + ba[0].r) / 2;
  ba[0].cy = (ba[0].u + ba[0].b) / 2;

  ba[1].l = Root->l;
  ba[1].r = Root->cx;
  ba[1].u = Root->u;
  ba[1].b = Root->cy;
  ba[1].cx = (ba[1].l + ba[1].r) / 2;
  ba[1].cy = (ba[1].u + ba[1].b) / 2;

  ba[2].l = Root->l;
  ba[2].r = Root->cx;
  ba[2].u = Root->cy;
  ba[2].b = Root->b;
  ba[2].cx = (ba[2].l + ba[2].r) / 2;
  ba[2].cy = (ba[2].u + ba[2].b) / 2;

  ba[3].l = Root->cx;
  ba[3].r = Root->r;
  ba[3].u = Root->cy;
  ba[3].b = Root->b;
  ba[3].cx = (ba[3].l + ba[3].r) / 2;
  ba[3].cy = (ba[3].u + ba[3].b) / 2;

  Root->child = ba;

  lanpr_connect_new_bounding_areas(rb, Root);

  while (rt = list_pop_pointer_no_free(&Root->linked_triangles)) {
    LANPR_BoundingArea *ba = Root->child;
    real b[4];
    b[0] = MIN3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
    b[1] = MAX3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
    b[2] = MAX3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);
    b[3] = MIN3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);
    if (TNS_BOUND_AREA_CROSSES(b, &ba[0].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[0], rt, b, 0);
    if (TNS_BOUND_AREA_CROSSES(b, &ba[1].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[1], rt, b, 0);
    if (TNS_BOUND_AREA_CROSSES(b, &ba[2].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[2], rt, b, 0);
    if (TNS_BOUND_AREA_CROSSES(b, &ba[3].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[3], rt, b, 0);
  }

  rb->bounding_area_count += 3;
}
int lanpr_line_crosses_bounding_area(LANPR_RenderBuffer *fb,
                                     tnsVector2d l,
                                     tnsVector2d r,
                                     LANPR_BoundingArea *ba)
{
  real vx, vy;
  tnsVector4d converted;
  real c1, c;

  if ((converted[0] = (real)ba->l) > MAX2(l[0], r[0]))
    return 0;
  if ((converted[1] = (real)ba->r) < MIN2(l[0], r[0]))
    return 0;
  if ((converted[2] = (real)ba->b) > MAX2(l[1], r[1]))
    return 0;
  if ((converted[3] = (real)ba->u) < MIN2(l[1], r[1]))
    return 0;

  vx = l[0] - r[0];
  vy = l[1] - r[1];

  c1 = vx * (converted[2] - l[1]) - vy * (converted[0] - l[0]);
  c = c1;

  c1 = vx * (converted[2] - l[1]) - vy * (converted[1] - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  c1 = vx * (converted[3] - l[1]) - vy * (converted[0] - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  c1 = vx * (converted[3] - l[1]) - vy * (converted[1] - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  return 0;
}
int lanpr_triangle_covers_bounding_area(LANPR_RenderBuffer *fb,
                                        LANPR_RenderTriangle *rt,
                                        LANPR_BoundingArea *ba)
{
  tnsVector2d p1, p2, p3, p4;
  real *FBC1 = rt->v[0]->fbcoord, *FBC2 = rt->v[1]->fbcoord, *FBC3 = rt->v[2]->fbcoord;

  p3[0] = p1[0] = (real)ba->l;
  p2[1] = p1[1] = (real)ba->b;
  p2[0] = p4[0] = (real)ba->r;
  p3[1] = p4[1] = (real)ba->u;

  if (FBC1[0] >= p1[0] && FBC1[0] <= p2[0] && FBC1[1] >= p1[1] && FBC1[1] <= p3[1])
    return 1;
  if (FBC2[0] >= p1[0] && FBC2[0] <= p2[0] && FBC2[1] >= p1[1] && FBC2[1] <= p3[1])
    return 1;
  if (FBC3[0] >= p1[0] && FBC3[0] <= p2[0] && FBC3[1] >= p1[1] && FBC3[1] <= p3[1])
    return 1;

  if (lanpr_point_inside_triangled(p1, FBC1, FBC2, FBC3) ||
      lanpr_point_inside_triangled(p2, FBC1, FBC2, FBC3) ||
      lanpr_point_inside_triangled(p3, FBC1, FBC2, FBC3) ||
      lanpr_point_inside_triangled(p4, FBC1, FBC2, FBC3))
    return 1;

  if (lanpr_line_crosses_bounding_area(fb, FBC1, FBC2, ba))
    return 1;
  else if (lanpr_line_crosses_bounding_area(fb, FBC2, FBC3, ba))
    return 1;
  else if (lanpr_line_crosses_bounding_area(fb, FBC3, FBC1, ba))
    return 1;

  return 0;
}
void lanpr_link_triangle_with_bounding_area(LANPR_RenderBuffer *rb,
                                            LANPR_BoundingArea *RootBoundingArea,
                                            LANPR_RenderTriangle *rt,
                                            real *LRUB,
                                            int Recursive)
{
  if (!lanpr_triangle_covers_bounding_area(rb, rt, RootBoundingArea))
    return;
  if (!RootBoundingArea->child) {
    list_append_pointer_static_pool(
        &rb->render_data_pool, &RootBoundingArea->linked_triangles, rt);
    RootBoundingArea->triangle_count++;
    if (RootBoundingArea->triangle_count > 200 && Recursive) {
      lanpr_split_bounding_area(rb, RootBoundingArea);
    }
    if (Recursive && rb->enable_intersections)
      lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, RootBoundingArea);
  }
  else {
    LANPR_BoundingArea *ba = RootBoundingArea->child;
    real *B1 = LRUB;
    real b[4];
    if (!LRUB) {
      b[0] = MIN3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
      b[1] = MAX3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
      b[2] = MAX3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);
      b[3] = MIN3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);
      B1 = b;
    }
    if (TNS_BOUND_AREA_CROSSES(B1, &ba[0].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[0], rt, B1, Recursive);
    if (TNS_BOUND_AREA_CROSSES(B1, &ba[1].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[1], rt, B1, Recursive);
    if (TNS_BOUND_AREA_CROSSES(B1, &ba[2].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[2], rt, B1, Recursive);
    if (TNS_BOUND_AREA_CROSSES(B1, &ba[3].l))
      lanpr_link_triangle_with_bounding_area(rb, &ba[3], rt, B1, Recursive);
  }
}
void lanpr_link_line_with_bounding_area(LANPR_RenderBuffer *rb,
                                        LANPR_BoundingArea *RootBoundingArea,
                                        LANPR_RenderLine *rl)
{
  list_append_pointer_static_pool(&rb->render_data_pool, &RootBoundingArea->linked_lines, rl);
}
int lanpr_get_triangle_bounding_areas(LANPR_RenderBuffer *rb,
                                      LANPR_RenderTriangle *rt,
                                      int *rowBegin,
                                      int *rowEnd,
                                      int *colBegin,
                                      int *colEnd)
{
  real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
  real b[4];

  if (!rt->v[0] || !rt->v[1] || !rt->v[2])
    return 0;

  b[0] = MIN3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
  b[1] = MAX3(rt->v[0]->fbcoord[0], rt->v[1]->fbcoord[0], rt->v[2]->fbcoord[0]);
  b[2] = MIN3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);
  b[3] = MAX3(rt->v[0]->fbcoord[1], rt->v[1]->fbcoord[1], rt->v[2]->fbcoord[1]);

  if (b[0] > 1 || b[1] < -1 || b[2] > 1 || b[3] < -1)
    return 0;

  (*colBegin) = (int)((b[0] + 1.0) / sp_w);
  (*colEnd) = (int)((b[1] + 1.0) / sp_w);
  (*rowEnd) = rb->tile_count_y - (int)((b[2] + 1.0) / sp_h) - 1;
  (*rowBegin) = rb->tile_count_y - (int)((b[3] + 1.0) / sp_h) - 1;

  if ((*colEnd) >= rb->tile_count_x)
    (*colEnd) = rb->tile_count_x - 1;
  if ((*rowEnd) >= rb->tile_count_y)
    (*rowEnd) = rb->tile_count_y - 1;
  if ((*colBegin) < 0)
    (*colBegin) = 0;
  if ((*rowBegin) < 0)
    (*rowBegin) = 0;

  return 1;
}
int lanpr_get_line_bounding_areas(LANPR_RenderBuffer *rb,
                                  LANPR_RenderLine *rl,
                                  int *rowBegin,
                                  int *rowEnd,
                                  int *colBegin,
                                  int *colEnd)
{
  real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
  real b[4];

  if (!rl->l || !rl->r)
    return 0;

  if (rl->l->fbcoord[0] != rl->l->fbcoord[0] || rl->r->fbcoord[0] != rl->r->fbcoord[0])
    return 0;

  b[0] = MIN2(rl->l->fbcoord[0], rl->r->fbcoord[0]);
  b[1] = MAX2(rl->l->fbcoord[0], rl->r->fbcoord[0]);
  b[2] = MIN2(rl->l->fbcoord[1], rl->r->fbcoord[1]);
  b[3] = MAX2(rl->l->fbcoord[1], rl->r->fbcoord[1]);

  if (b[0] > 1 || b[1] < -1 || b[2] > 1 || b[3] < -1)
    return 0;

  (*colBegin) = (int)((b[0] + 1.0) / sp_w);
  (*colEnd) = (int)((b[1] + 1.0) / sp_w);
  (*rowEnd) = rb->tile_count_y - (int)((b[2] + 1.0) / sp_h) - 1;
  (*rowBegin) = rb->tile_count_y - (int)((b[3] + 1.0) / sp_h) - 1;

  if ((*colEnd) >= rb->tile_count_x)
    (*colEnd) = rb->tile_count_x - 1;
  if ((*rowEnd) >= rb->tile_count_y)
    (*rowEnd) = rb->tile_count_y - 1;
  if ((*colBegin) < 0)
    (*colBegin) = 0;
  if ((*rowBegin) < 0)
    (*rowBegin) = 0;

  return 1;
}
LANPR_BoundingArea *lanpr_get_point_bounding_area(LANPR_RenderBuffer *rb, real x, real y)
{
  real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
  int col, row;

  if (x > 1 || x < -1 || y > 1 || y < -1)
    return 0;

  col = (int)((x + 1.0) / sp_w);
  row = rb->tile_count_y - (int)((y + 1.0) / sp_h) - 1;

  if (col >= rb->tile_count_x)
    col = rb->tile_count_x - 1;
  if (row >= rb->tile_count_y)
    row = rb->tile_count_y - 1;
  if (col < 0)
    col = 0;
  if (row < 0)
    row = 0;

  return &rb->initial_bounding_areas[row * 4 + col];
}
void lanpr_add_triangles(LANPR_RenderBuffer *rb)
{
  LANPR_RenderElementLinkNode *reln;
  LANPR_RenderTriangle *rt;
  // tnsMatrix44d vP;
  Camera *c = ((Camera *)rb->scene->camera);
  int i, lim;
  int x1, x2, y1, y2;
  int r, co;
  // tnsMatrix44d proj, view, result, inv;
  // tmat_make_perspective_matrix_44d(proj, c->FOv, (real)fb->W / (real)fb->H, c->clipsta,
  // c->clipend); tmat_load_identity_44d(view); tObjApplyself_transformMatrix(c, 0);
  // tObjApplyGlobalTransformMatrixReverted(c);
  // tmat_inverse_44d(inv, c->Base.GlobalTransform);
  // tmat_multiply_44d(result, proj, inv);
  // memcpy(proj, result, sizeof(tnsMatrix44d));

  // tnsglobal_TriangleIntersectionCount = 0;

  // tnsset_RenderOverallProgress(rb, NUL_MH2);
  rb->calculation_status = TNS_CALCULATION_INTERSECTION;
  // nulThreadNotifyUsers("tns.render_buffer_list.calculation_status");

  for (reln = rb->triangle_buffer_pointers.first; reln; reln = reln->item.next) {
    rt = reln->pointer;
    lim = reln->element_count;
    for (i = 0; i < lim; i++) {
      if (rt->cull_status) {
        rt = (void *)(((BYTE *)rt) + rb->triangle_size);
        continue;
      }
      if (lanpr_get_triangle_bounding_areas(rb, rt, &y1, &y2, &x1, &x2)) {
        for (co = x1; co <= x2; co++) {
          for (r = y1; r <= y2; r++) {
            lanpr_link_triangle_with_bounding_area(
                rb, &rb->initial_bounding_areas[r * 4 + co], rt, 0, 1);
          }
        }
      }
      else {
        ;  // throw away.
      }
      rt = (void *)(((BYTE *)rt) + rb->triangle_size);
      // if (tnsglobal_TriangleIntersectionCount >= 2000) {
      // tnsset_PlusRenderIntersectionCount(rb, tnsglobal_TriangleIntersectionCount);
      // tnsglobal_TriangleIntersectionCount = 0;
      //}
    }
  }
  // tnsset_PlusRenderIntersectionCount(rb, tnsglobal_TriangleIntersectionCount);
}
LANPR_BoundingArea *lanpr_get_next_bounding_area(LANPR_BoundingArea *This,
                                                 LANPR_RenderLine *rl,
                                                 real x,
                                                 real y,
                                                 real k,
                                                 int PositiveX,
                                                 int PositiveY,
                                                 real *NextX,
                                                 real *NextY)
{
  real rx, ry, ux, uy, lx, ly, bx, by;
  real r1, r2;
  LANPR_BoundingArea *ba;
  LinkData *lip;
  if (PositiveX > 0) {
    rx = This->r;
    ry = y + k * (rx - x);
    if (PositiveY > 0) {
      uy = This->u;
      ux = x + (uy - y) / k;
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], rx);
      r2 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], ux);
      if (MIN2(r1, r2) > 1)
        return 0;
      if (r1 <= r2) {
        for (lip = This->rp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->u >= ry && ba->b < ry) {
            *NextX = rx;
            *NextY = ry;
            return ba;
          }
        }
      }
      else {
        for (lip = This->up.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->r >= ux && ba->l < ux) {
            *NextX = ux;
            *NextY = uy;
            return ba;
          }
        }
      }
    }
    else if (PositiveY < 0) {
      by = This->b;
      bx = x + (by - y) / k;
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], rx);
      r2 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], bx);
      if (MIN2(r1, r2) > 1)
        return 0;
      if (r1 <= r2) {
        for (lip = This->rp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->u >= ry && ba->b < ry) {
            *NextX = rx;
            *NextY = ry;
            return ba;
          }
        }
      }
      else {
        for (lip = This->bp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->r >= bx && ba->l < bx) {
            *NextX = bx;
            *NextY = by;
            return ba;
          }
        }
      }
    }
    else {  // Y diffence == 0
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], This->r);
      if (r1 > 1)
        return 0;
      for (lip = This->rp.first; lip; lip = lip->next) {
        ba = lip->data;
        if (ba->u >= y && ba->b < y) {
          *NextX = This->r;
          *NextY = y;
          return ba;
        }
      }
    }
  }
  else if (PositiveX < 0) {  // X diffence < 0
    lx = This->l;
    ly = y + k * (lx - x);
    if (PositiveY > 0) {
      uy = This->u;
      ux = x + (uy - y) / k;
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], lx);
      r2 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], ux);
      if (MIN2(r1, r2) > 1)
        return 0;
      if (r1 <= r2) {
        for (lip = This->lp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->u >= ly && ba->b < ly) {
            *NextX = lx;
            *NextY = ly;
            return ba;
          }
        }
      }
      else {
        for (lip = This->up.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->r >= ux && ba->l < ux) {
            *NextX = ux;
            *NextY = uy;
            return ba;
          }
        }
      }
    }
    else if (PositiveY < 0) {
      by = This->b;
      bx = x + (by - y) / k;
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], lx);
      r2 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], bx);
      if (MIN2(r1, r2) > 1)
        return 0;
      if (r1 <= r2) {
        for (lip = This->lp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->u >= ly && ba->b < ly) {
            *NextX = lx;
            *NextY = ly;
            return ba;
          }
        }
      }
      else {
        for (lip = This->bp.first; lip; lip = lip->next) {
          ba = lip->data;
          if (ba->r >= bx && ba->l < bx) {
            *NextX = bx;
            *NextY = by;
            return ba;
          }
        }
      }
    }
    else {  // Y diffence == 0
      r1 = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], This->l);
      if (r1 > 1)
        return 0;
      for (lip = This->lp.first; lip; lip = lip->next) {
        ba = lip->data;
        if (ba->u >= y && ba->b < y) {
          *NextX = This->l;
          *NextY = y;
          return ba;
        }
      }
    }
  }
  else {  // X difference == 0;
    if (PositiveY > 0) {
      r1 = tMatGetLinearRatio(rl->l->fbcoord[1], rl->r->fbcoord[1], This->u);
      if (r1 > 1)
        return 0;
      for (lip = This->up.first; lip; lip = lip->next) {
        ba = lip->data;
        if (ba->r > x && ba->l <= x) {
          *NextX = x;
          *NextY = This->u;
          return ba;
        }
      }
    }
    else if (PositiveY < 0) {
      r1 = tMatGetLinearRatio(rl->l->fbcoord[1], rl->r->fbcoord[1], This->b);
      if (r1 > 1)
        return 0;
      for (lip = This->bp.first; lip; lip = lip->next) {
        ba = lip->data;
        if (ba->r > x && ba->l <= x) {
          *NextX = x;
          *NextY = This->b;
          return ba;
        }
      }
    }
    else
      return 0;  // segment has no length
  }
  return 0;
}

LANPR_BoundingArea *lanpr_get_bounding_area(LANPR_RenderBuffer *rb, real x, real y)
{
  LANPR_BoundingArea *iba;
  real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
  int c = (int)((x + 1.0) / sp_w);
  int r = rb->tile_count_y - (int)((y + 1.0) / sp_h) - 1;
  if (r < 0)
    r = 0;
  if (c < 0)
    c = 0;
  if (r >= rb->tile_count_y)
    r = rb->tile_count_y - 1;
  if (c >= rb->tile_count_x)
    c = rb->tile_count_x - 1;

  iba = &rb->initial_bounding_areas[r * 4 + c];
  while (iba->child) {
    if (x > iba->cx) {
      if (y > iba->cy)
        iba = &iba->child[0];
      else
        iba = &iba->child[3];
    }
    else {
      if (y > iba->cy)
        iba = &iba->child[1];
      else
        iba = &iba->child[2];
    }
  }
  return iba;
}
LANPR_BoundingArea *lanpr_get_first_possible_bounding_area(LANPR_RenderBuffer *rb,
                                                           LANPR_RenderLine *rl)
{
  LANPR_BoundingArea *iba;
  real data[2] = {rl->l->fbcoord[0], rl->l->fbcoord[1]};
  tnsVector2d LU = {-1, 1}, RU = {1, 1}, LB = {-1, -1}, RB = {1, -1};
  real r = 1, sr = 1;

  if (data[0] > -1 && data[0] < 1 && data[1] > -1 && data[1] < 1) {
    return lanpr_get_bounding_area(rb, data[0], data[1]);
  }
  else {
    if (lanpr_LineIntersectTest2d(rl->l->fbcoord, rl->r->fbcoord, LU, RU, &sr) && sr < r && sr > 0)
      r = sr;
    if (lanpr_LineIntersectTest2d(rl->l->fbcoord, rl->r->fbcoord, LB, RB, &sr) && sr < r && sr > 0)
      r = sr;
    if (lanpr_LineIntersectTest2d(rl->l->fbcoord, rl->r->fbcoord, LB, LU, &sr) && sr < r && sr > 0)
      r = sr;
    if (lanpr_LineIntersectTest2d(rl->l->fbcoord, rl->r->fbcoord, RB, RU, &sr) && sr < r && sr > 0)
      r = sr;
    lanpr_LinearInterpolate2dv(rl->l->fbcoord, rl->r->fbcoord, r, data);

    return lanpr_get_bounding_area(rb, data[0], data[1]);
  }

  real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;

  return iba;
}

/* ======================================= geometry ============================================ */

void lanpr_cut_render_line(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl, real Begin, real End)
{
  LANPR_RenderLineSegment *rls = rl->segments.first, *irls;
  LANPR_RenderLineSegment *begin_segment = 0, *end_segment = 0;
  LANPR_RenderLineSegment *ns = 0, *ns2 = 0;
  int untouched = 0;

  if (TNS_DOUBLE_CLOSE_ENOUGH(Begin, End))
    return;

  if (Begin != Begin)
    Begin = 0;
  if (End != End)
    End = 0;

  if (Begin > End) {
    real t = Begin;
    Begin = End;
    End = t;
  }

  for (rls = rl->segments.first; rls; rls = rls->item.next) {
    if (TNS_DOUBLE_CLOSE_ENOUGH(rls->at, Begin)) {
      begin_segment = rls;
      ns = begin_segment;
      break;
    }
    if (!rls->item.next) {
      break;
    }
    irls = rls->item.next;
    if (irls->at > Begin + 1e-09 && Begin > rls->at) {
      begin_segment = irls;
      ns = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
      break;
    }
  }
  if (!begin_segment && TNS_DOUBLE_CLOSE_ENOUGH(1, End)) {
    untouched = 1;
  }
  for (rls = begin_segment; rls; rls = rls->item.next) {
    if (TNS_DOUBLE_CLOSE_ENOUGH(rls->at, End)) {
      end_segment = rls;
      ns2 = end_segment;
      break;
    }
    // irls = rls->item.next;
    // added this to prevent rls->at == 1.0 (we don't need an end point for this)
    if (!rls->item.next && TNS_DOUBLE_CLOSE_ENOUGH(1, End)) {
      end_segment = rls;
      ns2 = end_segment;
      untouched = 1;
      break;
    }
    else if (rls->at > End) {
      end_segment = rls;
      ns2 = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
      break;
    }
  }

  if (!ns)
    ns = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
  if (!ns2) {
    if (untouched) {
      ns2 = ns;
      end_segment = ns2;
    }
    else
      ns2 = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
  }

  if (begin_segment) {
    if (begin_segment != ns) {
      ns->occlusion = begin_segment->item.prev ? (irls = begin_segment->item.prev)->occlusion : 0;
      list_insert_item_before(&rl->segments, (void *)ns, (void *)begin_segment);
    }
  }
  else {
    ns->occlusion = (irls = rl->segments.last)->occlusion;
    BLI_addtail(&rl->segments, ns);
  }
  if (end_segment) {
    if (end_segment != ns2) {
      ns2->occlusion = end_segment->item.prev ? (irls = end_segment->item.prev)->occlusion : 0;
      list_insert_item_before(&rl->segments, (void *)ns2, (void *)end_segment);
    }
  }
  else {
    ns2->occlusion = (irls = rl->segments.last)->occlusion;
    BLI_addtail(&rl->segments, ns2);
  }

  ns->at = Begin;
  if (!untouched)
    ns2->at = End;
  else
    ns2 = ns2->item.next;

  for (rls = ns; rls && rls != ns2; rls = rls->item.next) {
    rls->occlusion++;
  }
}

int lanpr_make_next_occlusion_task_info(LANPR_RenderBuffer *rb, LANPR_RenderTaskInfo *rti)
{
  LinkData *data;
  int i;
  int res = 0;

  BLI_spin_lock(&rb->cs_management);

  if (rb->contour_managed) {
    data = rb->contour_managed;
    rti->contour = (void *)data;
    rti->contour_pointers.first = data;
    for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
      data = data->next;
    }
    rb->contour_managed = data;
    rti->contour_pointers.last = data ? data->prev : rb->contours.last;
    res = 1;
  }
  else {
    list_handle_empty(&rti->contour_pointers);
    rti->contour = 0;
  }

  if (rb->intersection_managed) {
    data = rb->intersection_managed;
    rti->intersection = (void *)data;
    rti->intersection_pointers.first = data;
    for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
      data = data->next;
    }
    rb->intersection_managed = data;
    rti->intersection_pointers.last = data ? data->prev : rb->intersection_lines.last;
    res = 1;
  }
  else {
    list_handle_empty(&rti->intersection_pointers);
    rti->intersection = 0;
  }

  if (rb->crease_managed) {
    data = rb->crease_managed;
    rti->crease = (void *)data;
    rti->crease_pointers.first = data;
    for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
      data = data->next;
    }
    rb->crease_managed = data;
    rti->crease_pointers.last = data ? data->prev : rb->crease_lines.last;
    res = 1;
  }
  else {
    list_handle_empty(&rti->crease_pointers);
    rti->crease = 0;
  }

  if (rb->material_managed) {
    data = rb->material_managed;
    rti->material = (void *)data;
    rti->material_pointers.first = data;
    for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
      data = data->next;
    }
    rb->material_managed = data;
    rti->material_pointers.last = data ? data->prev : rb->material_lines.last;
    res = 1;
  }
  else {
    list_handle_empty(&rti->material_pointers);
    rti->material = 0;
  }

  if (rb->edge_mark_managed) {
    data = rb->edge_mark_managed;
    rti->edge_mark = (void *)data;
    rti->edge_mark_pointers.first = data;
    for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
      data = data->next;
    }
    rb->edge_mark_managed = data;
    rti->edge_mark_pointers.last = data ? data->prev : rb->edge_marks.last;
    res = 1;
  }
  else {
    list_handle_empty(&rti->edge_mark_pointers);
    rti->edge_mark = 0;
  }

  BLI_spin_unlock(&rb->cs_management);

  return res;
}
void lanpr_calculate_single_line_occlusion(LANPR_RenderBuffer *rb,
                                           LANPR_RenderLine *rl,
                                           int thread_id)
{
  real x = rl->l->fbcoord[0], y = rl->l->fbcoord[1];
  LANPR_BoundingArea *ba = lanpr_get_first_possible_bounding_area(rb, rl);
  LANPR_BoundingArea *nba = ba;
  LANPR_RenderTriangleThread *rt;
  LinkData *lip;
  Object *c = rb->scene->camera;
  real l, r;
  real k = (rl->r->fbcoord[1] - rl->l->fbcoord[1]) /
           (rl->r->fbcoord[0] - rl->l->fbcoord[0] + 1e-30);
  int PositiveX = (rl->r->fbcoord[0] - rl->l->fbcoord[0]) > 0 ?
                      1 :
                      (rl->r->fbcoord[0] == rl->l->fbcoord[0] ? 0 : -1);
  int PositiveY = (rl->r->fbcoord[1] - rl->l->fbcoord[1]) > 0 ?
                      1 :
                      (rl->r->fbcoord[1] == rl->l->fbcoord[1] ? 0 : -1);

  // printf("PX %d %lf   PY %d %lf\n", PositiveX, rl->r->fbcoord[0] -
  // rl->l->fbcoord[0], PositiveY, rl->r->fbcoord[1] -
  // rl->l->fbcoord[1]);

  while (nba) {

    for (lip = nba->linked_triangles.first; lip; lip = lip->next) {
      rt = lip->data;
      if (rt->testing[thread_id] == rl || rl->l->intersecting_with == (void *)rt ||
          rl->r->intersecting_with == (void *)rt)
        continue;
      rt->testing[thread_id] = rl;
      if (lanpr_triangle_line_imagespace_intersection_v2(&rb->cs_management,
                                                         (void *)rt,
                                                         rl,
                                                         c,
                                                         rb->view_projection,
                                                         rb->view_vector,
                                                         &l,
                                                         &r)) {
        lanpr_cut_render_line(rb, rl, l, r);
      }
    }

    nba = lanpr_get_next_bounding_area(nba, rl, x, y, k, PositiveX, PositiveY, &x, &y);
  }
}
void lanpr_THREAD_calculate_line_occlusion(TaskPool *__restrict pool,
                                           LANPR_RenderTaskInfo *rti,
                                           int threadid)
{
  LANPR_RenderBuffer *rb = rti->render_buffer;
  int thread_id = rti->thread_id;
  LinkData *lip;
  int count = 0;

  while (lanpr_make_next_occlusion_task_info(rb, rti)) {

    for (lip = (void *)rti->contour; lip && lip->prev != rti->contour_pointers.last;
         lip = lip->next) {
      lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
    }

    for (lip = (void *)rti->crease; lip && lip->prev != rti->crease_pointers.last;
         lip = lip->next) {
      lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
    }

    for (lip = (void *)rti->intersection; lip && lip->prev != rti->intersection_pointers.last;
         lip = lip->next) {
      lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
    }

    for (lip = (void *)rti->material; lip && lip->prev != rti->material_pointers.last;
         lip = lip->next) {
      lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
    }

    for (lip = (void *)rti->edge_mark; lip && lip->prev != rti->edge_mark_pointers.last;
         lip = lip->next) {
      lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
    }
  }
}
void lanpr_THREAD_calculate_line_occlusion_begin(LANPR_RenderBuffer *rb)
{
  int thread_count = rb->thread_count;
  LANPR_RenderTaskInfo *rti = MEM_callocN(sizeof(LANPR_RenderTaskInfo) * thread_count,
                                          "render task info");
  TaskScheduler *scheduler = BLI_task_scheduler_get();
  int i;

  rb->contour_managed = rb->contours.first;
  rb->crease_managed = rb->crease_lines.first;
  rb->intersection_managed = rb->intersection_lines.first;
  rb->material_managed = rb->material_lines.first;
  rb->edge_mark_managed = rb->edge_marks.first;

  TaskPool *tp = BLI_task_pool_create(scheduler, 0);

  for (i = 0; i < thread_count; i++) {
    rti[i].thread_id = i;
    rti[i].render_buffer = rb;
    BLI_task_pool_push(tp, lanpr_THREAD_calculate_line_occlusion, &rti[i], 0, TASK_PRIORITY_HIGH);
  }
  BLI_task_pool_work_and_wait(tp);

  MEM_freeN(rti);
}

void lanpr_NO_THREAD_calculate_line_occlusion(LANPR_RenderBuffer *rb)
{
  LinkData *lip;

  for (lip = rb->contours.first; lip; lip = lip->next) {
    lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
  }

  for (lip = rb->crease_lines.first; lip; lip = lip->next) {
    lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
  }

  for (lip = rb->intersection_lines.first; lip; lip = lip->next) {
    lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
  }

  for (lip = rb->material_lines.first; lip; lip = lip->next) {
    lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
  }

  for (lip = rb->edge_marks.first; lip; lip = lip->next) {
    lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
  }
}

int lanpr_get_normal(
    tnsVector3d v1, tnsVector3d v2, tnsVector3d v3, tnsVector3d n, tnsVector3d Pos)
{
  tnsVector3d vec1, vec2;

  tMatVectorMinus3d(vec1, v2, v1);
  tMatVectorMinus3d(vec2, v3, v1);
  tmat_vector_cross_3d(n, vec1, vec2);
  tmat_normalize_self_3d(n);
  if (Pos && (tmat_dot_3d(n, Pos, 1) < 0)) {
    tMatVectorMultiSelf3d(n, -1.0f);
    return 1;
  }
  return 0;
}

int lanpr_bound_box_crosses(tnsVector4d xxyy1, tnsVector4d xxyy2)
{
  real XMax1 = MAX2(xxyy1[0], xxyy1[1]);
  real XMin1 = MIN2(xxyy1[0], xxyy1[1]);
  real YMax1 = MAX2(xxyy1[2], xxyy1[3]);
  real YMin1 = MIN2(xxyy1[2], xxyy1[3]);
  real XMax2 = MAX2(xxyy2[0], xxyy2[1]);
  real XMin2 = MIN2(xxyy2[0], xxyy2[1]);
  real YMax2 = MAX2(xxyy2[2], xxyy2[3]);
  real YMin2 = MIN2(xxyy2[2], xxyy2[3]);

  if (XMax1 < XMin2 || XMin1 > XMax2)
    return 0;
  if (YMax1 < YMin2 || YMin1 > YMax2)
    return 0;

  return 1;
}
int lanpr_point_inside_triangled(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2)
{
  double cl, c;

  cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
  c = cl;

  cl = (v1[0] - v[0]) * (v2[1] - v[1]) - (v1[1] - v[1]) * (v2[0] - v[0]);
  if (c * cl <= 0)
    return 0;
  else
    c = cl;

  cl = (v2[0] - v[0]) * (v0[1] - v[1]) - (v2[1] - v[1]) * (v0[0] - v[0]);
  if (c * cl <= 0)
    return 0;
  else
    c = cl;

  cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
  if (c * cl <= 0)
    return 0;

  return 1;
}
int lanpr_point_on_lined(tnsVector2d v, tnsVector2d v0, tnsVector2d v1)
{
  real c1, c2;

  c1 = tMatGetLinearRatio(v0[0], v1[0], v[0]);
  c2 = tMatGetLinearRatio(v0[1], v1[1], v[1]);

  if (TNS_DOUBLE_CLOSE_ENOUGH(c1, c2) && c1 >= 0 && c1 <= 1)
    return 1;

  return 0;
}
int lanpr_point_triangle_relation(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2)
{
  double cl, c;
  real r;
  if (lanpr_point_on_lined(v, v0, v1) || lanpr_point_on_lined(v, v1, v2) ||
      lanpr_point_on_lined(v, v2, v0))
    return 1;

  cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
  c = cl;

  cl = (v1[0] - v[0]) * (v2[1] - v[1]) - (v1[1] - v[1]) * (v2[0] - v[0]);
  if ((r = c * cl) < 0)
    return 0;
  // else if(r == 0) return 1; // removed, point could still be on the extention line of some edge
  else
    c = cl;

  cl = (v2[0] - v[0]) * (v0[1] - v[1]) - (v2[1] - v[1]) * (v0[0] - v[0]);
  if ((r = c * cl) < 0)
    return 0;
  // else if(r == 0) return 1;
  else
    c = cl;

  cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
  if ((r = c * cl) < 0)
    return 0;
  else if (r == 0)
    return 1;

  return 2;
}
int lanpr_point_inside_triangle3d(tnsVector3d v, tnsVector3d v0, tnsVector3d v1, tnsVector3d v2)
{
  tnsVector3d l, r;
  tnsVector3d N1, N2;

  tMatVectorMinus3d(l, v1, v0);
  tMatVectorMinus3d(r, v, v1);
  tmat_vector_cross_3d(N1, l, r);

  tMatVectorMinus3d(l, v2, v1);
  tMatVectorMinus3d(r, v, v2);
  tmat_vector_cross_3d(N2, l, r);

  if (tmat_dot_3d(N1, N2, 0) < 0)
    return 0;

  tMatVectorMinus3d(l, v0, v2);
  tMatVectorMinus3d(r, v, v0);
  tmat_vector_cross_3d(N1, l, r);

  if (tmat_dot_3d(N1, N2, 0) < 0)
    return 0;

  tMatVectorMinus3d(l, v1, v0);
  tMatVectorMinus3d(r, v, v1);
  tmat_vector_cross_3d(N2, l, r);

  if (tmat_dot_3d(N1, N2, 0) < 0)
    return 0;

  return 1;
}
int lanpr_point_inside_triangle3de(tnsVector3d v, tnsVector3d v0, tnsVector3d v1, tnsVector3d v2)
{
  tnsVector3d l, r;
  tnsVector3d N1, N2;
  real d;

  tMatVectorMinus3d(l, v1, v0);
  tMatVectorMinus3d(r, v, v1);
  // tmat_normalize_self_3d(l);
  // tmat_normalize_self_3d(r);
  tmat_vector_cross_3d(N1, l, r);

  tMatVectorMinus3d(l, v2, v1);
  tMatVectorMinus3d(r, v, v2);
  // tmat_normalize_self_3d(l);
  // tmat_normalize_self_3d(r);
  tmat_vector_cross_3d(N2, l, r);

  if ((d = tmat_dot_3d(N1, N2, 0)) < 0)
    return 0;
  // if (d<DBL_EPSILON) return -1;

  tMatVectorMinus3d(l, v0, v2);
  tMatVectorMinus3d(r, v, v0);
  // tmat_normalize_self_3d(l);
  // tmat_normalize_self_3d(r);
  tmat_vector_cross_3d(N1, l, r);

  if ((d = tmat_dot_3d(N1, N2, 0)) < 0)
    return 0;
  // if (d<DBL_EPSILON) return -1;

  tMatVectorMinus3d(l, v1, v0);
  tMatVectorMinus3d(r, v, v1);
  // tmat_normalize_self_3d(l);
  // tmat_normalize_self_3d(r);
  tmat_vector_cross_3d(N2, l, r);

  if ((d = tmat_dot_3d(N1, N2, 0)) < 0)
    return 0;
  // if (d<DBL_EPSILON) return -1;

  return 1;
}

LANPR_RenderElementLinkNode *lanpr_new_cull_triangle_space64(LANPR_RenderBuffer *rb)
{
  LANPR_RenderElementLinkNode *reln;

  LANPR_RenderTriangle *RenderTriangles = MEM_callocN(
      64 * rb->triangle_size,
      "render triangle space");  // CreateNewBuffer(LANPR_RenderTriangle, 64);

  reln = list_append_pointer_static_sized(&rb->triangle_buffer_pointers,
                                          &rb->render_data_pool,
                                          RenderTriangles,
                                          sizeof(LANPR_RenderElementLinkNode));
  reln->element_count = 64;
  reln->additional = 1;

  return reln;
}
LANPR_RenderElementLinkNode *lanpr_new_cull_point_space64(LANPR_RenderBuffer *rb)
{
  LANPR_RenderElementLinkNode *reln;

  LANPR_RenderVert *Rendervertices = MEM_callocN(
      sizeof(LANPR_RenderVert) * 64,
      "render vert space");  // CreateNewBuffer(LANPR_RenderVert, 64);

  reln = list_append_pointer_static_sized(&rb->vertex_buffer_pointers,
                                          &rb->render_data_pool,
                                          Rendervertices,
                                          sizeof(LANPR_RenderElementLinkNode));
  reln->element_count = 64;
  reln->additional = 1;

  return reln;
}
void lanpr_calculate_render_triangle_normal(LANPR_RenderTriangle *rt);
void lanpr_assign_render_line_with_triangle(LANPR_RenderTriangle *rt)
{
  if (!rt->rl[0]->tl)
    rt->rl[0]->tl = rt;
  else if (!rt->rl[0]->tr)
    rt->rl[0]->tr = rt;

  if (!rt->rl[1]->tl)
    rt->rl[1]->tl = rt;
  else if (!rt->rl[1]->tr)
    rt->rl[1]->tr = rt;

  if (!rt->rl[2]->tl)
    rt->rl[2]->tl = rt;
  else if (!rt->rl[2]->tr)
    rt->rl[2]->tr = rt;
}
void lanpr_post_triangle(LANPR_RenderTriangle *rt, LANPR_RenderTriangle *orig)
{
  if (rt->v[0])
    tMatVectorAccum3d(rt->gc, rt->v[0]->fbcoord);
  if (rt->v[1])
    tMatVectorAccum3d(rt->gc, rt->v[1]->fbcoord);
  if (rt->v[2])
    tMatVectorAccum3d(rt->gc, rt->v[2]->fbcoord);
  tMatVectorMultiSelf3d(rt->gc, 1.0f / 3.0f);

  tMatVectorCopy3d(orig->gn, rt->gn);
}

#define RT_AT(head, rb, offset) ((BYTE *)head + offset * rb->triangle_size)

void lanpr_cull_triangles(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLine *rl;
  LANPR_RenderTriangle *rt, *rt1, *rt2;
  LANPR_RenderVert *rv;
  LANPR_RenderElementLinkNode *reln, *veln, *teln;
  LANPR_RenderLineSegment *rls;
  real *mv_inverse = rb->vp_inverse;
  real *vp = rb->view_projection;
  int i;
  real a;
  int v_count = 0, t_count = 0;
  Object *o;

  real cam_pos[3];
  Object *cam = ((Object *)rb->scene->camera);
  cam_pos[0] = cam->obmat[3][0];
  cam_pos[1] = cam->obmat[3][1];
  cam_pos[2] = cam->obmat[3][2];

  real view_dir[3], clip_advance[3];
  tMatVectorCopy3d(rb->view_vector, view_dir);
  tMatVectorCopy3d(rb->view_vector, clip_advance);
  tMatVectorMultiSelf3d(clip_advance, -((Camera *)cam->data)->clip_start);
  tMatVectorAccum3d(cam_pos, clip_advance);

  veln = lanpr_new_cull_point_space64(rb);
  teln = lanpr_new_cull_triangle_space64(rb);
  rv = &((LANPR_RenderVert *)veln->pointer)[v_count];
  rt1 = (void *)(((BYTE *)teln->pointer) + rb->triangle_size * t_count);

  for (reln = rb->triangle_buffer_pointers.first; reln; reln = reln->item.next) {
    i = 0;
    if (reln->additional)
      continue;
    o = reln->object_ref;
    for (i; i < reln->element_count; i++) {
      int In1 = 0, In2 = 0, In3 = 0;
      rt = (void *)(((BYTE *)reln->pointer) + rb->triangle_size * i);
      if (rt->v[0]->fbcoord[3] < 0)
        In1 = 1;
      if (rt->v[1]->fbcoord[3] < 0)
        In2 = 1;
      if (rt->v[2]->fbcoord[3] < 0)
        In3 = 1;

      rt->rl[0]->object_ref = o;
      rt->rl[1]->object_ref = o;
      rt->rl[2]->object_ref = o;

      if (v_count > 60) {
        veln->element_count = v_count;
        veln = lanpr_new_cull_point_space64(rb);
        v_count = 0;
      }

      if (t_count > 60) {
        teln->element_count = t_count;
        teln = lanpr_new_cull_triangle_space64(rb);
        t_count = 0;
      }

      // if ((!rt->rl[0]->item.next && !rt->rl[0]->item.prev) ||
      //    (!rt->rl[1]->item.next && !rt->rl[1]->item.prev) ||
      //    (!rt->rl[2]->item.next && !rt->rl[2]->item.prev)) {
      //	printf("'"); // means this triangle is lonely????
      //}

      rv = &((LANPR_RenderVert *)veln->pointer)[v_count];
      rt1 = (void *)(((BYTE *)teln->pointer) + rb->triangle_size * t_count);
      rt2 = (void *)(((BYTE *)teln->pointer) + rb->triangle_size * (t_count + 1));

      real vv1[3], vv2[3], dot1, dot2;

      switch (In1 + In2 + In3) {
        case 0:
          continue;
        case 3:
          rt->cull_status = TNS_CULL_DISCARD;
          BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
          rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
          BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
          rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;
          BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
          rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;
          continue;
        case 2:
          rt->cull_status = TNS_CULL_USED;
          if (!In1) {
            tMatVectorMinus3d(vv1, rt->v[0]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[2]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[0]->gloc[0] + a * rt->v[2]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[0]->gloc[1] + a * rt->v[2]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[0]->gloc[2] + a * rt->v[2]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[0]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[1]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[0]->gloc[0] + a * rt->v[1]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[0]->gloc[1] + a * rt->v[1]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[0]->gloc[2] + a * rt->v[1]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
            rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
            rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
            rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = &rv[0];
            rl->tl = rt1;
            rt1->rl[1] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = rt->v[0];
            rl->tl = rt->rl[0]->tl == rt ? rt1 : rt->rl[0]->tl;
            rl->tr = rt->rl[0]->tr == rt ? rt1 : rt->rl[0]->tr;
            rt1->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[0];
            rl->r = &rv[0];
            rl->tl = rt->rl[2]->tl == rt ? rt1 : rt->rl[2]->tl;
            rl->tr = rt->rl[2]->tr == rt ? rt1 : rt->rl[2]->tr;
            rt1->rl[2] = rl;

            rt1->v[0] = rt->v[0];
            rt1->v[1] = &rv[1];
            rt1->v[2] = &rv[0];

            lanpr_post_triangle(rt1, rt);

            v_count += 2;
            t_count += 1;
            continue;
          }
          else if (!In3) {
            tMatVectorMinus3d(vv1, rt->v[2]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[0]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[2]->gloc[0] + a * rt->v[0]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[2]->gloc[1] + a * rt->v[0]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[2]->gloc[2] + a * rt->v[0]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[2]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[1]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[2]->gloc[0] + a * rt->v[1]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[2]->gloc[1] + a * rt->v[1]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[2]->gloc[2] + a * rt->v[1]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
            rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
            rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
            rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[0];
            rl->r = &rv[1];
            rl->tl = rt1;
            rt1->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = rt->v[2];
            rl->tl = rt->rl[1]->tl == rt ? rt1 : rt->rl[1]->tl;
            rl->tr = rt->rl[1]->tr == rt ? rt1 : rt->rl[1]->tr;
            rt1->rl[1] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[2];
            rl->r = &rv[0];
            rl->tl = rt->rl[2]->tl == rt ? rt1 : rt->rl[2]->tl;
            rl->tr = rt->rl[2]->tr == rt ? rt1 : rt->rl[2]->tr;
            rt1->rl[2] = rl;

            rt1->v[0] = &rv[1];
            rt1->v[1] = rt->v[2];
            rt1->v[2] = &rv[0];

            lanpr_post_triangle(rt1, rt);

            v_count += 2;
            t_count += 1;
            continue;
          }
          else if (!In2) {
            tMatVectorMinus3d(vv1, rt->v[1]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[2]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[1]->gloc[0] + a * rt->v[2]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[1]->gloc[1] + a * rt->v[2]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[1]->gloc[2] + a * rt->v[2]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[1]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[0]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[1]->gloc[0] + a * rt->v[0]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[1]->gloc[1] + a * rt->v[0]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[1]->gloc[2] + a * rt->v[0]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
            rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
            rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
            rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = &rv[0];
            rl->tl = rt1;
            rt1->rl[2] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[0];
            rl->r = rt->v[1];
            rl->tl = rt->rl[0]->tl == rt ? rt1 : rt->rl[0]->tl;
            rl->tr = rt->rl[0]->tr == rt ? rt1 : rt->rl[0]->tr;
            rt1->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[1];
            rl->r = &rv[1];
            rl->tl = rt->rl[1]->tl == rt ? rt1 : rt->rl[1]->tl;
            rl->tr = rt->rl[1]->tr == rt ? rt1 : rt->rl[1]->tr;
            rt1->rl[1] = rl;

            rt1->v[0] = rt->v[1];
            rt1->v[1] = &rv[1];
            rt1->v[2] = &rv[0];

            lanpr_post_triangle(rt1, rt);

            v_count += 2;
            t_count += 1;
            continue;
          }
          break;
        case 1:
          rt->cull_status = TNS_CULL_USED;
          if (In1) {
            tMatVectorMinus3d(vv1, rt->v[0]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[2]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[0]->gloc[0] + a * rt->v[2]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[0]->gloc[1] + a * rt->v[2]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[0]->gloc[2] + a * rt->v[2]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[0]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[1]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[0]->gloc[0] + a * rt->v[1]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[0]->gloc[1] + a * rt->v[1]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[0]->gloc[2] + a * rt->v[1]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
            rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
            rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = &rv[0];
            rl->tl = rt1;
            rt1->rl[1] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[0];
            rl->r = rt->v[1];
            rl->tl = rt1;
            rl->tr = rt2;
            rt1->rl[2] = rl;
            rt2->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[1];
            rl->r = &rv[1];
            rl->tl = rt->rl[0]->tl == rt ? rt1 : rt->rl[0]->tl;
            rl->tr = rt->rl[0]->tr == rt ? rt1 : rt->rl[0]->tr;
            rt1->rl[0] = rl;

            rt1->v[0] = rt->v[1];
            rt1->v[1] = &rv[1];
            rt1->v[2] = &rv[0];

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[2];
            rl->r = &rv[0];
            rl->tl = rt->rl[2]->tl == rt ? rt1 : rt->rl[2]->tl;
            rl->tr = rt->rl[2]->tr == rt ? rt1 : rt->rl[2]->tr;
            rt2->rl[2] = rl;
            rt2->rl[1] = rt->rl[1];

            rt2->v[0] = &rv[0];
            rt2->v[1] = rt->v[1];
            rt2->v[2] = rt->v[2];

            lanpr_post_triangle(rt1, rt);
            lanpr_post_triangle(rt2, rt);

            v_count += 2;
            t_count += 2;
            continue;
          }
          else if (In2) {

            tMatVectorMinus3d(vv1, rt->v[1]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[2]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[1]->gloc[0] + a * rt->v[2]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[1]->gloc[1] + a * rt->v[2]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[1]->gloc[2] + a * rt->v[2]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[1]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[0]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[1]->gloc[0] + a * rt->v[0]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[1]->gloc[1] + a * rt->v[0]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[1]->gloc[2] + a * rt->v[0]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[0]);
            rt->rl[0]->item.next = rt->rl[0]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
            rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = &rv[0];
            rl->tl = rt1;
            rt1->rl[1] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[0];
            rl->r = rt->v[2];
            rl->tl = rt1;
            rl->tr = rt2;
            rt1->rl[2] = rl;
            rt2->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[2];
            rl->r = &rv[1];
            rl->tl = rt->rl[1]->tl == rt ? rt1 : rt->rl[1]->tl;
            rl->tr = rt->rl[1]->tr == rt ? rt1 : rt->rl[1]->tr;
            rt1->rl[0] = rl;

            rt1->v[0] = rt->v[2];
            rt1->v[1] = &rv[1];
            rt1->v[2] = &rv[0];

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[0];
            rl->r = &rv[0];
            rl->tl = rt->rl[0]->tl == rt ? rt1 : rt->rl[0]->tl;
            rl->tr = rt->rl[0]->tr == rt ? rt1 : rt->rl[0]->tr;
            rt2->rl[2] = rl;
            rt2->rl[1] = rt->rl[2];

            rt2->v[0] = &rv[0];
            rt2->v[1] = rt->v[2];
            rt2->v[2] = rt->v[0];

            lanpr_post_triangle(rt1, rt);
            lanpr_post_triangle(rt2, rt);

            v_count += 2;
            t_count += 2;
            continue;
          }
          else if (In3) {

            tMatVectorMinus3d(vv1, rt->v[2]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[0]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[0].gloc[0] = (1 - a) * rt->v[2]->gloc[0] + a * rt->v[0]->gloc[0];
            rv[0].gloc[1] = (1 - a) * rt->v[2]->gloc[1] + a * rt->v[0]->gloc[1];
            rv[0].gloc[2] = (1 - a) * rt->v[2]->gloc[2] + a * rt->v[0]->gloc[2];
            tmat_apply_transform_44d(rv[0].fbcoord, vp, rv[0].gloc);

            tMatVectorMinus3d(vv1, rt->v[2]->gloc, cam_pos);
            tMatVectorMinus3d(vv2, cam_pos, rt->v[1]->gloc);
            dot1 = tmat_dot_3d(vv1, view_dir, 0);
            dot2 = tmat_dot_3d(vv2, view_dir, 0);
            a = dot1 / (dot1 + dot2);
            rv[1].gloc[0] = (1 - a) * rt->v[2]->gloc[0] + a * rt->v[1]->gloc[0];
            rv[1].gloc[1] = (1 - a) * rt->v[2]->gloc[1] + a * rt->v[1]->gloc[1];
            rv[1].gloc[2] = (1 - a) * rt->v[2]->gloc[2] + a * rt->v[1]->gloc[2];
            tmat_apply_transform_44d(rv[1].fbcoord, vp, rv[1].gloc);

            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[1]);
            rt->rl[1]->item.next = rt->rl[1]->item.prev = 0;
            BLI_remlink(&rb->all_render_lines, (void *)rt->rl[2]);
            rt->rl[2]->item.next = rt->rl[2]->item.prev = 0;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[1];
            rl->r = &rv[0];
            rl->tl = rt1;
            rt1->rl[1] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = &rv[0];
            rl->r = rt->v[0];
            rl->tl = rt1;
            rl->tr = rt2;
            rt1->rl[2] = rl;
            rt2->rl[0] = rl;

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[0];
            rl->r = &rv[1];
            rl->tl = rt->rl[0]->tl == rt ? rt1 : rt->rl[0]->tl;
            rl->tr = rt->rl[0]->tr == rt ? rt1 : rt->rl[0]->tr;
            rt1->rl[0] = rl;

            rt1->v[0] = rt->v[0];
            rt1->v[1] = &rv[1];
            rt1->v[2] = &rv[0];

            rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
            rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
            BLI_addtail(&rl->segments, rls);
            BLI_addtail(&rb->all_render_lines, rl);
            rl->l = rt->v[1];
            rl->r = &rv[0];
            rl->tl = rt->rl[1]->tl == rt ? rt1 : rt->rl[1]->tl;
            rl->tr = rt->rl[1]->tr == rt ? rt1 : rt->rl[1]->tr;
            rt2->rl[2] = rl;
            rt2->rl[1] = rt->rl[0];

            rt2->v[0] = &rv[0];
            rt2->v[1] = rt->v[0];
            rt2->v[2] = rt->v[1];

            lanpr_post_triangle(rt1, rt);
            lanpr_post_triangle(rt2, rt);

            v_count += 2;
            t_count += 2;
            continue;
          }
          break;
      }
    }
    teln->element_count = t_count;
    veln->element_count = v_count;
  }
}
void lanpr_perspective_division(LANPR_RenderBuffer *rb)
{
  LANPR_RenderVert *rv;
  LANPR_RenderElementLinkNode *reln;
  Camera *cam = rb->scene->camera->data;
  int i;

  if (cam->type != CAM_PERSP)
    return;

  for (reln = rb->vertex_buffer_pointers.first; reln; reln = reln->item.next) {
    rv = reln->pointer;
    for (i = 0; i < reln->element_count; i++) {
      // if (rv->fbcoord[2] < -DBL_EPSILON) continue;
      tMatVectorMultiSelf3d(rv[i].fbcoord, 1 / rv[i].fbcoord[3]);
      // rv[i].fbcoord[2] = cam->clipsta * cam->clipend / (cam->clipend -
      // fabs(rv[i].fbcoord[2]) * (cam->clipend - cam->clipsta));
    }
  }
}

void lanpr_transform_render_vert(
    BMVert *v, int index, LANPR_RenderVert *RvBuf, real *MvMat, real *MvPMat, Camera *Camera)
{  // real HeightMultiply, real clipsta, real clipend) {
  LANPR_RenderVert *rv = &RvBuf[index];
  // rv->v = v;
  // v->Rv = rv;
  tmat_apply_transform_43df(rv->gloc, MvMat, v->co);
  tmat_apply_transform_43dfND(rv->fbcoord, MvPMat, v->co);

  // if(rv->fbcoord[2]>0)tMatVectorMultiSelf3d(rv->fbcoord, (1 /
  // rv->fbcoord[3])); else tMatVectorMultiSelf3d(rv->fbcoord,
  // -rv->fbcoord[3]);
  //   rv->fbcoord[2] = Camera->clipsta* Camera->clipend / (Camera->clipend -
  //   fabs(rv->fbcoord[2]) * (Camera->clipend - Camera->clipsta));
}
void lanpr_calculate_render_triangle_normal(LANPR_RenderTriangle *rt)
{
  tnsVector3d l, r;
  tMatVectorMinus3d(l, rt->v[1]->gloc, rt->v[0]->gloc);
  tMatVectorMinus3d(r, rt->v[2]->gloc, rt->v[0]->gloc);
  tmat_vector_cross_3d(rt->gn, l, r);
  tmat_normalize_self_3d(rt->gn);
}
void lanpr_make_render_geometry_buffers_object(Object *o,
                                               real *MvMat,
                                               real *MvPMat,
                                               LANPR_RenderBuffer *rb)
{
  Object *oc;
  Mesh *mo;
  BMesh *bm;
  BMVert *v;
  BMFace *f;
  BMEdge *e;
  BMLoop *loop;
  LANPR_RenderLine *rl;
  LANPR_RenderTriangle *rt;
  tnsMatrix44d new_mvp;
  tnsMatrix44d new_mv;
  tnsMatrix44d self_transform;
  tnsMatrix44d Normal;
  LANPR_RenderElementLinkNode *reln;
  Object *cam_object = rb->scene->camera;
  Camera *c = cam_object->data;
  Material *m;
  LANPR_RenderVert *orv;
  LANPR_RenderLine *orl;
  LANPR_RenderTriangle *ort;
  FreestyleEdge *fe;
  int CanFindFreestyle = 0;
  int i;

  if (o->type == OB_MESH) {

    tmat_obmat_to_16d(o->obmat, self_transform);

    tmat_multiply_44d(new_mvp, MvPMat, self_transform);
    tmat_multiply_44d(new_mv, MvMat, self_transform);

    invert_m4_m4(o->imat, o->obmat);
    transpose_m4(o->imat);
    tmat_obmat_to_16d(o->imat, Normal);

    const BMAllocTemplate allocsize = BMALLOC_TEMPLATE_FROM_ME(((Mesh *)(o->data)));
    bm = BM_mesh_create(&allocsize,
                        &((struct BMeshCreateParams){
                            .use_toolflags = true,
                        }));
    BM_mesh_bm_from_me(bm,
                       o->data,
                       &((struct BMeshFromMeshParams){
                           .calc_face_normal = true,
                       }));
    BM_mesh_elem_hflag_disable_all(bm, BM_FACE | BM_EDGE, BM_ELEM_TAG, false);
    BM_mesh_triangulate(
        bm, MOD_TRIANGULATE_QUAD_BEAUTY, MOD_TRIANGULATE_NGON_BEAUTY, 4, false, NULL, NULL, NULL);
    BM_mesh_normals_update(bm);
    BM_mesh_elem_table_ensure(bm, BM_VERT | BM_EDGE | BM_FACE);
    BM_mesh_elem_index_ensure(bm, BM_VERT | BM_EDGE | BM_FACE);

    if (CustomData_has_layer(&bm->edata, CD_FREESTYLE_EDGE)) {
      CanFindFreestyle = 1;
    }

    orv = MEM_callocN(sizeof(LANPR_RenderVert) * bm->totvert, "object render verts");
    ort = MEM_callocN(
        bm->totface * rb->triangle_size,
        "object render triangles");  // CreateNewBuffer(LANPR_RenderTriangle, mo->triangle_count);
    orl = MEM_callocN(sizeof(LANPR_RenderLine) * bm->totedge, "object render edge");

    reln = list_append_pointer_static_sized(&rb->vertex_buffer_pointers,
                                            &rb->render_data_pool,
                                            orv,
                                            sizeof(LANPR_RenderElementLinkNode));
    reln->element_count = bm->totvert;
    reln->object_ref = o;

    reln = list_append_pointer_static_sized(&rb->line_buffer_pointers,
                                            &rb->render_data_pool,
                                            orl,
                                            sizeof(LANPR_RenderElementLinkNode));
    reln->element_count = bm->totedge;
    reln->object_ref = o;

    reln = list_append_pointer_static_sized(&rb->triangle_buffer_pointers,
                                            &rb->render_data_pool,
                                            ort,
                                            sizeof(LANPR_RenderElementLinkNode));
    reln->element_count = bm->totface;
    reln->object_ref = o;

    for (i = 0; i < bm->totvert; i++) {
      v = BM_vert_at_index(bm, i);
      lanpr_transform_render_vert(v, i, orv, new_mv, new_mvp, c);
    }

    rl = orl;
    for (i = 0; i < bm->totedge; i++) {
      e = BM_edge_at_index(bm, i);
      if (CanFindFreestyle) {
        fe = CustomData_bmesh_get(&bm->edata, e->head.data, CD_FREESTYLE_EDGE);
        if (fe->flag & FREESTYLE_EDGE_MARK)
          rl->flags |= LANPR_EDGE_FLAG_EDGE_MARK;
      }
      if (use_smooth_contour_modifier_contour) {
        rl->edge_idx = i;
        if (BM_elem_flag_test(e->v1, BM_ELEM_SELECT) && BM_elem_flag_test(e->v2, BM_ELEM_SELECT))
          rl->flags |= LANPR_EDGE_FLAG_CONTOUR;
      }

      rl->l = &orv[BM_elem_index_get(e->v1)];
      rl->r = &orv[BM_elem_index_get(e->v2)];
      LANPR_RenderLineSegment *rls = mem_static_aquire(&rb->render_data_pool,
                                                       sizeof(LANPR_RenderLineSegment));
      BLI_addtail(&rl->segments, rls);
      BLI_addtail(&rb->all_render_lines, rl);
      rl++;
    }

    rt = ort;
    for (i = 0; i < bm->totface; i++) {
      f = BM_face_at_index(bm, i);

      loop = f->l_first;
      rt->v[0] = &orv[BM_elem_index_get(loop->v)];
      rt->rl[0] = &orl[BM_elem_index_get(loop->e)];
      loop = loop->next;
      rt->v[1] = &orv[BM_elem_index_get(loop->v)];
      rt->rl[1] = &orl[BM_elem_index_get(loop->e)];
      loop = loop->next;
      rt->v[2] = &orv[BM_elem_index_get(loop->v)];
      rt->rl[2] = &orl[BM_elem_index_get(loop->e)];

      rt->material_id = f->mat_nr;
      rt->gn[0] = f->no[0];
      rt->gn[1] = f->no[1];
      rt->gn[2] = f->no[2];

      tMatVectorAccum3d(rt->gc, rt->v[0]->fbcoord);
      tMatVectorAccum3d(rt->gc, rt->v[1]->fbcoord);
      tMatVectorAccum3d(rt->gc, rt->v[2]->fbcoord);
      tMatVectorMultiSelf3d(rt->gc, 1.0f / 3.0f);
      tmat_apply_normal_transform_43df(rt->gn, Normal, f->no);
      tmat_normalize_self_3d(rt->gn);
      lanpr_assign_render_line_with_triangle(rt);
      // m = tnsGetIndexedMaterial(rb->scene, f->material_id);
      // if(m) m->Previewv_count += (f->triangle_count*3);

      if (BM_elem_flag_test(f, BM_ELEM_SELECT))
        rt->material_id = 1;

      rt = (LANPR_RenderTriangle *)(((unsigned char *)rt) + rb->triangle_size);
    }

    BM_mesh_free(bm);
  }
}
void lanpr_make_render_geometry_buffers(Depsgraph *depsgraph,
                                        Scene *s,
                                        Object *c /*camera*/,
                                        LANPR_RenderBuffer *rb)
{
  Object *o;
  Collection *collection;
  CollectionObject *co;
  tnsMatrix44d obmat16;
  tnsMatrix44d proj, view, result, inv;
  Camera *cam = c->data;

  float sensor = BKE_camera_sensor_size(cam->sensor_fit, cam->sensor_x, cam->sensor_y);
  real fov = focallength_to_fov(cam->lens, sensor);

  memset(rb->material_pointers, 0, sizeof(void *) * 2048);

  real asp = ((real)rb->w / (real)rb->h);

  if (cam->type == CAM_PERSP) {
    tmat_make_perspective_matrix_44d(proj, fov, asp, cam->clip_start, cam->clip_end);
  }
  else if (cam->type == CAM_ORTHO) {
    real w = cam->ortho_scale / 2;
    tmat_make_ortho_matrix_44d(proj, -w, w, -w / asp, w / asp, cam->clip_start, cam->clip_end);
  }

  tmat_load_identity_44d(view);

  // tObjApplyself_transformMatrix(c, 0);
  // tObjApplyGlobalTransformMatrixReverted(c);
  tmat_obmat_to_16d(c->obmat, obmat16);
  tmat_inverse_44d(inv, obmat16);
  tmat_multiply_44d(result, proj, inv);
  memcpy(proj, result, sizeof(tnsMatrix44d));
  memcpy(rb->view_projection, proj, sizeof(tnsMatrix44d));

  tmat_inverse_44d(rb->vp_inverse, rb->view_projection);

  void *a;
  while (a = BLI_pophead(&rb->triangle_buffer_pointers))
    MEM_freeN(a);
  while (a = BLI_pophead(&rb->vertex_buffer_pointers))
    MEM_freeN(a);

  DEG_OBJECT_ITER_BEGIN (depsgraph,
                         o,
                         DEG_ITER_OBJECT_FLAG_LINKED_DIRECTLY | DEG_ITER_OBJECT_FLAG_VISIBLE |
                             DEG_ITER_OBJECT_FLAG_DUPLI | DEG_ITER_OBJECT_FLAG_LINKED_VIA_SET) {
    lanpr_make_render_geometry_buffers_object(o, view, proj, rb);
  }
  DEG_OBJECT_ITER_END;

  // for (collection = s->master_collection.first; collection; collection = collection->ID.next) {
  //	for (co = collection->gobject.first; co; co = co->next) {
  //		//tObjApplyGlobalTransformMatrixRecursive(o);
  //		lanpr_make_render_geometry_buffers_object(o, view, proj, rb);
  //	}
  //}
}

#define INTERSECT_SORT_MIN_TO_MAX_3(ia, ib, ic, lst) \
  { \
    lst[0] = TNS_MIN3_INDEX(ia, ib, ic); \
    lst[1] = (((ia <= ib && ib <= ic) || (ic <= ib && ib <= ia)) ? \
                  1 : \
                  (((ic <= ia && ia <= ib) || (ib < ia && ia <= ic)) ? 0 : 2)); \
    lst[2] = TNS_MAX3_INDEX(ia, ib, ic); \
  }

// ia ib ic are ordered
#define INTERSECT_JUST_GREATER(is, order, num, index) \
  { \
    index = (num < is[order[0]] ? \
                 order[0] : \
                 (num < is[order[1]] ? order[1] : (num < is[order[2]] ? order[2] : order[2]))); \
  }

// ia ib ic are ordered
#define INTERSECT_JUST_SMALLER(is, order, num, index) \
  { \
    index = (num > is[order[2]] ? \
                 order[2] : \
                 (num > is[order[1]] ? order[1] : (num > is[order[0]] ? order[0] : order[0]))); \
  }

void lanpr_get_interpolate_point2d(tnsVector2d l, tnsVector2d r, real Ratio, tnsVector2d Result)
{
  Result[0] = tnsLinearItp(l[0], r[0], Ratio);
  Result[1] = tnsLinearItp(l[1], r[1], Ratio);
}
void lanpr_get_interpolate_point3d(tnsVector2d l, tnsVector2d r, real Ratio, tnsVector2d Result)
{
  Result[0] = tnsLinearItp(l[0], r[0], Ratio);
  Result[1] = tnsLinearItp(l[1], r[1], Ratio);
  Result[2] = tnsLinearItp(l[2], r[2], Ratio);
}

int lanpr_get_z_inersect_point(
    tnsVector3d tl, tnsVector3d tr, tnsVector3d LL, tnsVector3d LR, tnsVector3d IntersectResult)
{
  // real lzl = 1 / LL[2], lzr = 1 / LR[2], tzl = 1 / tl[2], tzr = 1 / tr[2];
  real lzl = LL[2], lzr = LR[2], tzl = tl[2], tzr = tr[2];
  real l = tzl - lzl, r = tzr - lzr;
  real ratio;
  int rev = l < 0 ? -1 : 1;  //-1:occlude left 1:occlude right

  if (l * r >= 0) {
    if (l == 0)
      IntersectResult[2] = r > 0 ? -1 : 1;
    else if (r == 0)
      IntersectResult[2] = l > 0 ? -1 : 1;
    else
      IntersectResult[2] = rev;
    return 0;
  }
  l = fabsf(l);
  r = fabsf(r);
  ratio = l / (l + r);

  IntersectResult[2] = lanpr_LinearInterpolate(lzl, lzr, ratio);
  lanpr_get_interpolate_point2d(LL, LR, ratio, IntersectResult);

  return rev;
}
LANPR_RenderVert *lanpr_find_shaerd_vertex(LANPR_RenderLine *rl, LANPR_RenderTriangle *rt)
{
  if (rl->l == rt->v[0] || rl->l == rt->v[1] || rl->l == rt->v[2])
    return rl->l;
  else if (rl->r == rt->v[0] || rl->r == rt->v[1] || rl->r == rt->v[2])
    return rl->r;
  else
    return 0;
}
void lanpr_find_edge_no_vertex(LANPR_RenderTriangle *rt,
                               LANPR_RenderVert *rv,
                               tnsVector3d l,
                               tnsVector3d r)
{
  if (rt->v[0] == rv) {
    tMatVectorCopy3d(rt->v[1]->fbcoord, l);
    tMatVectorCopy3d(rt->v[2]->fbcoord, r);
  }
  else if (rt->v[1] == rv) {
    tMatVectorCopy3d(rt->v[2]->fbcoord, l);
    tMatVectorCopy3d(rt->v[0]->fbcoord, r);
  }
  else if (rt->v[2] == rv) {
    tMatVectorCopy3d(rt->v[0]->fbcoord, l);
    tMatVectorCopy3d(rt->v[1]->fbcoord, r);
  }
}
LANPR_RenderLine *lanpr_another_edge(LANPR_RenderTriangle *rt, LANPR_RenderVert *rv)
{
  if (rt->v[0] == rv) {
    return rt->rl[1];
  }
  else if (rt->v[1] == rv) {
    return rt->rl[2];
  }
  else if (rt->v[2] == rv) {
    return rt->rl[0];
  }
  return 0;
}

int lanpr_share_edge(LANPR_RenderTriangle *rt, LANPR_RenderVert *rv, LANPR_RenderLine *rl)
{
  LANPR_RenderVert *another = rv == rl->l ? rl->r : rl->l;

  if (rt->v[0] == rv) {
    if (another == rt->v[1] || another == rt->v[2])
      return 1;
    return 0;
  }
  else if (rt->v[1] == rv) {
    if (another == rt->v[0] || another == rt->v[2])
      return 1;
    return 0;
  }
  else if (rt->v[2] == rv) {
    if (another == rt->v[0] || another == rt->v[1])
      return 1;
    return 0;
  }
  return 0;
}
int lanpr_share_edge_direct(LANPR_RenderTriangle *rt, LANPR_RenderLine *rl)
{
  if (rt->rl[0] == rl || rt->rl[1] == rl || rt->rl[2] == rl)
    return 1;
  return 0;
}
int lanpr_TriangleLineImageSpaceIntersectTestOnly(LANPR_RenderTriangle *rt,
                                                  LANPR_RenderLine *rl,
                                                  double *From,
                                                  double *To)
{
  double dl, dr;
  double ratio;
  double is[3];
  int order[3];
  int LCross, RCross;
  int a, b, c;
  int ret;
  int noCross = 0;
  tnsVector3d tl, tr, LL, LR;
  tnsVector3d IntersectResult;
  LANPR_RenderVert *Share;
  int StL = 0, StR = 0;
  int OccludeSide;

  double *LFBC = rl->l->fbcoord, *RFBC = rl->r->fbcoord, *FBC0 = rt->v[0]->fbcoord,
         *FBC1 = rt->v[1]->fbcoord, *FBC2 = rt->v[2]->fbcoord;

  // bound box.
  if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX2(LFBC[2], RFBC[2]))
    return 0;
  if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN2(LFBC[0], RFBC[0]))
    return 0;
  if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX2(LFBC[0], RFBC[0]))
    return 0;
  if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN2(LFBC[1], RFBC[1]))
    return 0;
  if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX2(LFBC[1], RFBC[1]))
    return 0;

  if (Share = lanpr_find_shaerd_vertex(rl, rt)) {
    tnsVector3d CL, CR;
    double r;
    int status;
    double r2;

    // if (rl->IgnoreConnectedFace/* && lanpr_share_edge(rt, Share, rl)*/)
    // return 0;

    lanpr_find_edge_no_vertex(rt, Share, CL, CR);
    status = lanpr_LineIntersectTest2d(LFBC, RFBC, CL, CR, &r);

    // LL[2] = 1 / tnsLinearItp(1 / LFBC[2], 1 / RFBC[2], r);
    LL[0] = tnsLinearItp(LFBC[0], RFBC[0], r);
    LL[1] = tnsLinearItp(LFBC[1], RFBC[1], r);
    LL[2] = tnsLinearItp(LFBC[2], RFBC[2], r);

    r2 = lanpr_GetLinearRatio(CL, CR, LL);
    // LR[2] = 1 / tnsLinearItp(1 / CL[2], 1 / CR[2], r2);
    LR[0] = tnsLinearItp(CL[0], CR[0], r2);
    LR[1] = tnsLinearItp(CL[1], CR[1], r2);
    LR[2] = tnsLinearItp(CL[2], CR[2], r2);

    if (LL[2] <= (LR[2] + 0.000000001))
      return 0;

    StL = lanpr_point_inside_triangled(LFBC, FBC0, FBC1, FBC2);
    StR = lanpr_point_inside_triangled(RFBC, FBC0, FBC1, FBC2);

    if ((StL && Share == rl->r) || (StR && Share == rl->l)) {
      *From = 0;
      *To = 1;
      return 1;
    }

    if (!status)
      return 0;

    if (rl->l == Share) {
      *From = 0;
      *To = r;
    }
    else {
      *From = r;
      *To = 1;
    }
    return 1;
  }

  a = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC0, FBC1, &is[0]);
  b = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC1, FBC2, &is[1]);
  c = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC2, FBC0, &is[2]);

  if (!a && !b && !c) {
    if (!(StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2)) &&
        !(StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2))) {
      return 0;  // not occluding
    }
  }

  StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2);
  StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2);

  INTERSECT_SORT_MIN_TO_MAX_3(is[0], is[1], is[2], order);

  if (StL) {
    INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
    INTERSECT_JUST_GREATER(is, order, -DBL_TRIANGLE_LIM, RCross);
    // if (is[LCross]>=0 || is[RCross] >= 1) return 0;
  }
  else if (StR) {
    INTERSECT_JUST_SMALLER(is, order, 1.0f + DBL_TRIANGLE_LIM, LCross);
    INTERSECT_JUST_GREATER(is, order, 1.0f - DBL_TRIANGLE_LIM, RCross);
    // if (is[LCross] <= 0 || is[RCross] <= 1) return 0;
  }
  else {
    if (a) {
      if (b) {
        LCross = is[0] < is[1] ? 0 : 1;
        RCross = is[0] < is[1] ? 1 : 0;
      }
      else {
        LCross = is[0] < is[2] ? 0 : 2;
        RCross = is[0] < is[2] ? 2 : 0;
      }
    }
    else if (c) {
      LCross = is[1] < is[2] ? 1 : 2;
      RCross = is[1] < is[2] ? 2 : 1;
    }
    else {
      return 0;
    }
    // if (rl->IgnoreConnectedFace/* && lanpr_share_edge(rt, Share, rl)*/)
    //	return 0;
    if (MAX3(FBC0[2], FBC1[2], FBC2[2]) < (MIN2(LFBC[2], RFBC[2]) - 0.000001)) {
      *From = is[LCross];
      *To = is[RCross];
      TNS_CLAMP((*From), 0, 1);
      TNS_CLAMP((*To), 0, 1);
      return 1;
    }
  }

  LL[2] = lanpr_GetLineZ(LFBC, RFBC, is[LCross]);
  LR[2] = lanpr_GetLineZ(LFBC, RFBC, is[RCross]);
  lanpr_get_interpolate_point2d(LFBC, RFBC, is[LCross], LL);
  lanpr_get_interpolate_point2d(LFBC, RFBC, is[RCross], LR);

  tl[2] = lanpr_GetLineZPoint(
      rt->v[LCross]->fbcoord, rt->v[(LCross > 1 ? 0 : (LCross + 1))]->fbcoord, LL);
  tr[2] = lanpr_GetLineZPoint(
      rt->v[RCross]->fbcoord, rt->v[(RCross > 1 ? 0 : (RCross + 1))]->fbcoord, LR);
  tMatVectorCopy2d(LL, tl);
  tMatVectorCopy2d(LR, tr);

  if (OccludeSide = lanpr_get_z_inersect_point(tl, tr, LL, LR, IntersectResult)) {
    real r = lanpr_GetLinearRatio(LFBC, RFBC, IntersectResult);
    if (OccludeSide > 0) {
      if (r > 1 /*|| r < 0*/)
        return 0;
      *From = MAX2(r, 0);
      *To = MIN2(is[RCross], 1);
    }
    else {
      if (r < 0 /*|| r > 1*/)
        return 0;
      *From = MAX2(is[LCross], 0);
      *To = MIN2(r, 1);
    }
    //*From = TNS_MAX2(TNS_MAX2(r, is[LCross]), 0);
    //*To = TNS_MIN2(r, TNS_MIN2(is[RCross], 1));
  }
  else if (IntersectResult[2] < 0) {
    if ((!StL) && (!StR) && (a + b + c < 2) || is[LCross] > is[RCross])
      return 0;
    *From = is[LCross];
    *To = is[RCross];
  }
  else
    return 0;

  TNS_CLAMP((*From), 0, 1);
  TNS_CLAMP((*To), 0, 1);

  // if ((TNS_FLOAT_CLOSE_ENOUGH(*From, 0) && TNS_FLOAT_CLOSE_ENOUGH(*To, 1)) ||
  //	(TNS_FLOAT_CLOSE_ENOUGH(*To, 0) && TNS_FLOAT_CLOSE_ENOUGH(*From, 1)) ||
  //	TNS_FLOAT_CLOSE_ENOUGH(*From, *To)) return 0;

  return 1;
}
int lanpr_triangle_line_imagespace_intersection_v2(SpinLock *spl,
                                                   LANPR_RenderTriangle *rt,
                                                   LANPR_RenderLine *rl,
                                                   Object *cam,
                                                   tnsMatrix44d vp,
                                                   real *CameraDir,
                                                   double *From,
                                                   double *To)
{
  double dl, dr;
  double ratio;
  double is[3] = {0};
  int order[3];
  int LCross = -1, RCross = -1;
  int a, b, c;
  int ret;
  int noCross = 0;
  tnsVector3d IntersectResult;
  LANPR_RenderVert *Share;
  int StL = 0, StR = 0, In;
  int OccludeSide;

  tnsVector3d Lv;
  tnsVector3d Rv;
  tnsVector4d vd4;
  real Cv[3];
  real DotL, DotR, DotLA, DotRA;
  real DotF;
  LANPR_RenderVert *Result, *rv;
  tnsVector4d gloc, Trans;
  real Cut = -1;
  int NextCut, NextCut1;
  int status;

  double *LFBC = rl->l->fbcoord, *RFBC = rl->r->fbcoord, *FBC0 = rt->v[0]->fbcoord,
         *FBC1 = rt->v[1]->fbcoord, *FBC2 = rt->v[2]->fbcoord;

  // printf("(%f %f)(%f %f)(%f %f)   (%f %f)(%f %f)\n", FBC0[0], FBC0[1], FBC1[0], FBC1[1],
  // FBC2[0], FBC2[1], LFBC[0], LFBC[1], RFBC[0], RFBC[1]);

  // bound box.
  // if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX2(LFBC[2], RFBC[2]))
  //	return 0;
  if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN2(LFBC[0], RFBC[0]))
    return 0;
  if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX2(LFBC[0], RFBC[0]))
    return 0;
  if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN2(LFBC[1], RFBC[1]))
    return 0;
  if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX2(LFBC[1], RFBC[1]))
    return 0;

  if (lanpr_share_edge_direct(rt, rl))
    return 0;

  a = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC0, FBC1, &is[0]);
  b = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC1, FBC2, &is[1]);
  c = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC2, FBC0, &is[2]);

  // printf("abc: %d %d %d\n", a,b,c);

  INTERSECT_SORT_MIN_TO_MAX_3(is[0], is[1], is[2], order);

  tMatVectorMinus3d(Lv, rl->l->gloc, rt->v[0]->gloc);
  tMatVectorMinus3d(Rv, rl->r->gloc, rt->v[0]->gloc);

  tMatVectorCopy3d(CameraDir, Cv);

  tMatVectorConvert4fd(cam->obmat[3], vd4);
  if (((Camera *)cam->data)->type == CAM_PERSP)
    tMatVectorMinus3d(Cv, vd4, rt->v[0]->gloc);

  DotL = tmat_dot_3d(Lv, rt->gn, 0);
  DotR = tmat_dot_3d(Rv, rt->gn, 0);
  DotF = tmat_dot_3d(Cv, rt->gn, 0);

  if (!DotF)
    return 0;

  if (!a && !b && !c) {
    if (!(StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2)) &&
        !(StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2))) {
      return 0;  // not occluding
    }
  }

  StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2);
  StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2);

  // for (rv = rt->intersecting_verts.first; rv; rv = rv->item.next) {
  //	if (rv->intersecting_with == rt && rv->intersecting_line == rl) {
  //		Cut = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0],
  // rv->fbcoord[0]); 		break;
  //	}
  //}

  DotLA = fabs(DotL);
  if (DotLA < DBL_EPSILON) {
    DotLA = 0;
    DotL = 0;
  }
  DotRA = fabs(DotR);
  if (DotRA < DBL_EPSILON) {
    DotRA = 0;
    DotR = 0;
  }
  if (DotL - DotR == 0)
    Cut = 100000;
  else if (DotL * DotR <= 0) {
    Cut = DotLA / fabs(DotL - DotR);
  }
  else {
    Cut = fabs(DotR + DotL) / fabs(DotL - DotR);
    Cut = DotRA > DotLA ? 1 - Cut : Cut;
  }

  if (((Camera *)cam->data)->type == CAM_PERSP) {
    lanpr_LinearInterpolate3dv(rl->l->gloc, rl->r->gloc, Cut, gloc);
    tmat_apply_transform_44d(Trans, vp, gloc);
    tMatVectorMultiSelf3d(Trans, (1 / Trans[3]) /**HeightMultiply/2*/);
  }
  else {
    lanpr_LinearInterpolate3dv(rl->l->fbcoord, rl->r->fbcoord, Cut, Trans);
    // tmat_apply_transform_44d(Trans, vp, gloc);
  }

  // Trans[2] = tmat_dist_3dv(gloc, cam->Base.gloc);
  // Trans[2] = cam->clipsta*cam->clipend / (cam->clipend - fabs(Trans[2]) * (cam->clipend -
  // cam->clipsta));

  // prevent vertical problem ?
  if (rl->l->fbcoord[0] != rl->r->fbcoord[0])
    Cut = tMatGetLinearRatio(rl->l->fbcoord[0], rl->r->fbcoord[0], Trans[0]);
  else
    Cut = tMatGetLinearRatio(rl->l->fbcoord[1], rl->r->fbcoord[1], Trans[1]);

  In = lanpr_point_inside_triangled(
      Trans, rt->v[0]->fbcoord, rt->v[1]->fbcoord, rt->v[2]->fbcoord);

  if (StL == 2) {
    if (StR == 2) {
      INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
    }
    else if (StR == 1) {
      INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
    }
    else if (StR == 0) {
      INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 0, RCross);
    }
  }
  else if (StL == 1) {
    if (StR == 2) {
      INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
    }
    else if (StR == 1) {
      INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
    }
    else if (StR == 0) {
      INTERSECT_JUST_GREATER(is, order, DBL_TRIANGLE_LIM, RCross);
      if (TNS_ABC(RCross) && is[RCross] > (DBL_TRIANGLE_LIM)) {
        INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
      }
      else {
        INTERSECT_JUST_SMALLER(is, order, -DBL_TRIANGLE_LIM, LCross);
        INTERSECT_JUST_GREATER(is, order, -DBL_TRIANGLE_LIM, RCross);
      }
    }
  }
  else if (StL == 0) {
    if (StR == 2) {
      INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
      INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
    }
    else if (StR == 1) {
      INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
      if (TNS_ABC(LCross) && is[LCross] < (1 - DBL_TRIANGLE_LIM)) {
        INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
      }
      else {
        INTERSECT_JUST_SMALLER(is, order, 1 + DBL_TRIANGLE_LIM, LCross);
        INTERSECT_JUST_GREATER(is, order, 1 + DBL_TRIANGLE_LIM, RCross);
      }
    }
    else if (StR == 0) {
      INTERSECT_JUST_GREATER(is, order, 0, LCross);
      if (TNS_ABC(LCross) && is[LCross] > 0) {
        INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
      }
      else {
        INTERSECT_JUST_GREATER(is, order, is[LCross], LCross);
        INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
      }
    }
  }

  real LF = DotL * DotF, RF = DotR * DotF;
  // int CrossCount = a + b + c;
  // if (CrossCount == 2) {
  //	INTERSECT_JUST_GREATER(is, order, 0, LCross);
  //	if (!TNS_ABC(LCross)) INTERSECT_JUST_GREATER(is, order, is[LCross], LCross);
  //	INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
  //}else if(CrossCount == 1 || StL+StR==1) {
  //	if (StL) {
  //		INTERSECT_JUST_GREATER(is, order, DBL_TRIANGLE_LIM, RCross);
  //		INTERSECT_JUST_SMALLER(is, order, is[RCross], LCross);
  //	}else if(StR) {
  //		INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
  //		INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
  //	}
  //}else if(CrossCount == 0) {
  //	INTERSECT_JUST_SMALLER(is, order, 0, LCross);
  //	INTERSECT_JUST_GREATER(is, order, 1, RCross);
  //}

  if (LF <= 0 && RF <= 0 && (DotL || DotR)) {

    *From = MAX2(0, is[LCross]);
    *To = MIN2(1, is[RCross]);
    if (*From >= *To)
      return 0;
    // printf("1 From %f to %f\n",*From, *To);
    return 1;
  }
  else if (LF >= 0 && RF <= 0 && (DotL || DotR)) {
    *From = MAX2(Cut, is[LCross]);
    *To = MIN2(1, is[RCross]);
    if (*From >= *To)
      return 0;
    // printf("2 From %f to %f\n",*From, *To);
    return 1;
  }
  else if (LF <= 0 && RF >= 0 && (DotL || DotR)) {
    *From = MAX2(0, is[LCross]);
    *To = MIN2(Cut, is[RCross]);
    if (*From >= *To)
      return 0;
    // printf("3 From %f to %f\n",*From, *To);
    return 1;
  }
  else
    return 0;
  return 1;
}

LANPR_RenderLine *lanpr_triangle_share_edge(LANPR_RenderTriangle *l, LANPR_RenderTriangle *r)
{
  if (l->rl[0] == r->rl[0])
    return r->rl[0];
  if (l->rl[0] == r->rl[1])
    return r->rl[1];
  if (l->rl[0] == r->rl[2])
    return r->rl[2];
  if (l->rl[1] == r->rl[0])
    return r->rl[0];
  if (l->rl[1] == r->rl[1])
    return r->rl[1];
  if (l->rl[1] == r->rl[2])
    return r->rl[2];
  if (l->rl[2] == r->rl[0])
    return r->rl[0];
  if (l->rl[2] == r->rl[1])
    return r->rl[1];
  if (l->rl[2] == r->rl[2])
    return r->rl[2];
  return 0;
}
LANPR_RenderVert *lanpr_triangle_share_point(LANPR_RenderTriangle *l, LANPR_RenderTriangle *r)
{
  if (l->v[0] == r->v[0])
    return r->v[0];
  if (l->v[0] == r->v[1])
    return r->v[1];
  if (l->v[0] == r->v[2])
    return r->v[2];
  if (l->v[1] == r->v[0])
    return r->v[0];
  if (l->v[1] == r->v[1])
    return r->v[1];
  if (l->v[1] == r->v[2])
    return r->v[2];
  if (l->v[2] == r->v[0])
    return r->v[0];
  if (l->v[2] == r->v[1])
    return r->v[1];
  if (l->v[2] == r->v[2])
    return r->v[2];
  return 0;
}

LANPR_RenderVert *lanpr_triangle_line_intersection_test(LANPR_RenderBuffer *rb,
                                                        LANPR_RenderLine *rl,
                                                        LANPR_RenderTriangle *rt,
                                                        LANPR_RenderTriangle *testing,
                                                        LANPR_RenderVert *Last)
{
  tnsVector3d Lv;
  tnsVector3d Rv;
  real DotL, DotR;
  LANPR_RenderVert *Result, *rv;
  tnsVector3d gloc;
  LANPR_RenderVert *l = rl->l, *r = rl->r;
  int result;

  int i;

  for (rv = testing->intersecting_verts.first; rv; rv = rv->item.next) {
    if (rv->intersecting_with == rt && rv->intersecting_line == rl)
      return rv;
  }

  tMatVectorMinus3d(Lv, l->gloc, testing->v[0]->gloc);
  tMatVectorMinus3d(Rv, r->gloc, testing->v[0]->gloc);

  DotL = tmat_dot_3d(Lv, testing->gn, 0);
  DotR = tmat_dot_3d(Rv, testing->gn, 0);

  if (DotL * DotR > 0 || (!DotL && !DotR))
    return 0;

  DotL = fabs(DotL);
  DotR = fabs(DotR);

  lanpr_LinearInterpolate3dv(l->gloc, r->gloc, DotL / (DotL + DotR), gloc);

  if (Last && TNS_DOUBLE_CLOSE_ENOUGH(Last->gloc[0], gloc[0]) &&
      TNS_DOUBLE_CLOSE_ENOUGH(Last->gloc[1], gloc[1]) &&
      TNS_DOUBLE_CLOSE_ENOUGH(Last->gloc[2], gloc[2])) {

    Last->intersecting_line2 = rl;
    return 0;
  }

  if (!(result = lanpr_point_inside_triangle3de(
            gloc, testing->v[0]->gloc, testing->v[1]->gloc, testing->v[2]->gloc)))
    return 0;
  /*else if(result < 0) {
     return 0;
     }*/

  Result = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderVert));

  if (DotL > 0 || DotR < 0)
    Result->positive = 1;
  else
    Result->positive = 0;

  // Result->IntersectingOnFace = testing;
  Result->edge_used = 1;
  // Result->IntersectL = l;
  Result->v = (void *)r;  // Caution!
                          // Result->intersecting_with = rt;
  tMatVectorCopy3d(gloc, Result->gloc);

  BLI_addtail(&testing->intersecting_verts, Result);

  return Result;
}
LANPR_RenderLine *lanpr_triangle_generate_intersection_line_only(LANPR_RenderBuffer *rb,
                                                                 LANPR_RenderTriangle *rt,
                                                                 LANPR_RenderTriangle *testing)
{
  LANPR_RenderVert *l = 0, *r = 0;
  LANPR_RenderVert **Next = &l;
  LANPR_RenderLine *Result;
  LANPR_RenderVert *E0T = 0;
  LANPR_RenderVert *E1T = 0;
  LANPR_RenderVert *E2T = 0;
  LANPR_RenderVert *TE0 = 0;
  LANPR_RenderVert *TE1 = 0;
  LANPR_RenderVert *TE2 = 0;
  tnsVector3d cl;  // rb->scene->ActiveCamera->gloc;
  real ZMax = ((Camera *)rb->scene->camera->data)->clip_end;
  real ZMin = ((Camera *)rb->scene->camera->data)->clip_start;
  LANPR_RenderVert *Share = lanpr_triangle_share_point(testing, rt);

  tMatVectorConvert3fd(rb->scene->camera->obmat[3], cl);

  if (Share) {
    LANPR_RenderVert *NewShare;
    LANPR_RenderLine *rl = lanpr_another_edge(rt, Share);

    l = NewShare = mem_static_aquire(&rb->render_data_pool, (sizeof(LANPR_RenderVert)));

    NewShare->positive = 1;
    NewShare->edge_used = 1;
    // NewShare->IntersectL = l;
    NewShare->v = (void *)r;  // Caution!
    // Result->intersecting_with = rt;
    tMatVectorCopy3d(Share->gloc, NewShare->gloc);

    r = lanpr_triangle_line_intersection_test(rb, rl, rt, testing, 0);

    if (!r) {
      rl = lanpr_another_edge(testing, Share);
      r = lanpr_triangle_line_intersection_test(rb, rl, testing, rt, 0);
      if (!r)
        return 0;
      BLI_addtail(&testing->intersecting_verts, NewShare);
    }
    else {
      BLI_addtail(&rt->intersecting_verts, NewShare);
    }
  }
  else {
    if (!rt->rl[0] || !rt->rl[1] || !rt->rl[2])
      return 0;  // shouldn't need this, there must be problems in culling.
    E0T = lanpr_triangle_line_intersection_test(rb, rt->rl[0], rt, testing, 0);
    if (E0T && (!(*Next))) {
      (*Next) = E0T;
      (*Next)->intersecting_line = rt->rl[0];
      Next = &r;
    }
    E1T = lanpr_triangle_line_intersection_test(rb, rt->rl[1], rt, testing, l);
    if (E1T && (!(*Next))) {
      (*Next) = E1T;
      (*Next)->intersecting_line = rt->rl[1];
      Next = &r;
    }
    if (!(*Next))
      E2T = lanpr_triangle_line_intersection_test(rb, rt->rl[2], rt, testing, l);
    if (E2T && (!(*Next))) {
      (*Next) = E2T;
      (*Next)->intersecting_line = rt->rl[2];
      Next = &r;
    }

    if (!(*Next))
      TE0 = lanpr_triangle_line_intersection_test(rb, testing->rl[0], testing, rt, l);
    if (TE0 && (!(*Next))) {
      (*Next) = TE0;
      (*Next)->intersecting_line = testing->rl[0];
      Next = &r;
    }
    if (!(*Next))
      TE1 = lanpr_triangle_line_intersection_test(rb, testing->rl[1], testing, rt, l);
    if (TE1 && (!(*Next))) {
      (*Next) = TE1;
      (*Next)->intersecting_line = testing->rl[1];
      Next = &r;
    }
    if (!(*Next))
      TE2 = lanpr_triangle_line_intersection_test(rb, testing->rl[2], testing, rt, l);
    if (TE2 && (!(*Next))) {
      (*Next) = TE2;
      (*Next)->intersecting_line = testing->rl[2];
      Next = &r;
    }

    if (!(*Next))
      return 0;
  }
  tmat_apply_transform_44d(l->fbcoord, rb->view_projection, l->gloc);
  tmat_apply_transform_44d(r->fbcoord, rb->view_projection, r->gloc);
  tMatVectorMultiSelf3d(l->fbcoord, (1 / l->fbcoord[3]) /**HeightMultiply/2*/);
  tMatVectorMultiSelf3d(r->fbcoord, (1 / r->fbcoord[3]) /**HeightMultiply/2*/);

  l->fbcoord[2] = ZMin * ZMax / (ZMax - fabs(l->fbcoord[2]) * (ZMax - ZMin));
  r->fbcoord[2] = ZMin * ZMax / (ZMax - fabs(r->fbcoord[2]) * (ZMax - ZMin));

  l->intersecting_with = rt;
  r->intersecting_with = testing;

  ///*((1 / rl->l->fbcoord[3])*rb->FrameBuffer->H / 2)

  Result = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
  Result->l = l;
  Result->r = r;
  Result->tl = rt;
  Result->tr = testing;
  LANPR_RenderLineSegment *rls = mem_static_aquire(&rb->render_data_pool,
                                                   sizeof(LANPR_RenderLineSegment));
  BLI_addtail(&Result->segments, rls);
  BLI_addtail(&rb->all_render_lines, Result);
  Result->flags |= LANPR_EDGE_FLAG_INTERSECTION;
  list_append_pointer_static(&rb->intersection_lines, &rb->render_data_pool, Result);
  int r1, r2, c1, c2, row, col;
  if (lanpr_get_line_bounding_areas(rb, Result, &r1, &r2, &c1, &c2)) {
    for (row = r1; row != r2 + 1; row++) {
      for (col = c1; col != c2 + 1; col++) {
        lanpr_link_line_with_bounding_area(rb, &rb->initial_bounding_areas[row * 4 + col], Result);
      }
    }
  }

  // printf("intersection (%f %f)-(%f %f)\n",Result->l->fbcoord[0],
  // Result->l->fbcoord[1],Result->r->fbcoord[0], Result->r->fbcoord[1]);

  // tnsglobal_TriangleIntersectionCount++;

  rb->intersection_count++;

  return Result;
}
void lanpr_triangle_enable_intersections_in_bounding_area(LANPR_RenderBuffer *rb,
                                                          LANPR_RenderTriangle *rt,
                                                          LANPR_BoundingArea *ba)
{
  tnsVector3d n, c = {0};
  tnsVector3d tl, tr;
  LANPR_RenderTriangle *TestingTriangle;
  LANPR_RenderLine *TestingLine;
  LANPR_RenderLine *Result = 0;
  LANPR_RenderVert *rv;
  LinkData *lip, *next_lip;
  real l, r;
  int a = 0;

  real *FBC0 = rt->v[0]->fbcoord, *FBC1 = rt->v[1]->fbcoord, *FBC2 = rt->v[2]->fbcoord;

  if (ba->child) {
    lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, &ba->child[0]);
    lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, &ba->child[1]);
    lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, &ba->child[2]);
    lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, &ba->child[3]);
    return;
  }

  for (lip = ba->linked_triangles.first; lip; lip = next_lip) {
    next_lip = lip->next;
    TestingTriangle = lip->data;
    if (TestingTriangle == rt || TestingTriangle->testing == rt ||
        lanpr_triangle_share_edge(rt, TestingTriangle)) {

      continue;
    }

    TestingTriangle->testing = rt;
    real *RFBC0 = TestingTriangle->v[0]->fbcoord, *RFBC1 = TestingTriangle->v[1]->fbcoord,
         *RFBC2 = TestingTriangle->v[2]->fbcoord;

    if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX3(RFBC0[2], RFBC1[2], RFBC2[2]))
      continue;
    if (MAX3(FBC0[2], FBC1[2], FBC2[2]) < MIN3(RFBC0[2], RFBC1[2], RFBC2[2]))
      continue;
    if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX3(RFBC0[0], RFBC1[0], RFBC2[0]))
      continue;
    if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN3(RFBC0[0], RFBC1[0], RFBC2[0]))
      continue;
    if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX3(RFBC0[1], RFBC1[1], RFBC2[1]))
      continue;
    if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN3(RFBC0[1], RFBC1[1], RFBC2[1]))
      continue;

    Result = lanpr_triangle_generate_intersection_line_only(rb, rt, TestingTriangle);
  }
}

int lanpr_line_crosses_frame(tnsVector2d l, tnsVector2d r)
{
  real vx, vy;
  tnsVector4d converted;
  real c1, c;

  if (-1 > MAX2(l[0], r[0]))
    return 0;
  if (1 < MIN2(l[0], r[0]))
    return 0;
  if (-1 > MAX2(l[1], r[1]))
    return 0;
  if (1 < MIN2(l[1], r[1]))
    return 0;

  vx = l[0] - r[0];
  vy = l[1] - r[1];

  c1 = vx * (-1 - l[1]) - vy * (-1 - l[0]);
  c = c1;

  c1 = vx * (-1 - l[1]) - vy * (1 - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  c1 = vx * (1 - l[1]) - vy * (-1 - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  c1 = vx * (1 - l[1]) - vy * (1 - l[0]);
  if (c1 * c <= 0)
    return 1;
  else
    c = c1;

  return 0;
}

void lanpr_compute_view_Vector(LANPR_RenderBuffer *rb)
{
  tnsVector3d Direction = {0, 0, 1};
  tnsVector3d Trans;
  tnsMatrix44d inv;
  tnsMatrix44d obmat;

  tmat_obmat_to_16d(rb->scene->camera->obmat, obmat);
  tmat_inverse_44d(inv, obmat);
  tmat_apply_rotation_43d(Trans, inv, Direction);
  tMatVectorCopy3d(Trans, rb->view_vector);
  // tMatVectorMultiSelf3d(Trans, -1);
  // tMatVectorCopy3d(Trans, ((Camera*)rb->scene->camera)->RenderviewDir);
}

void lanpr_compute_scene_contours(LANPR_RenderBuffer *rb, float threshold)
{
  real *view_vector = rb->view_vector;
  BMEdge *e;
  real Dot1 = 0, Dot2 = 0;
  real Result;
  tnsVector4d GNormal;
  tnsVector3d cam_location;
  int Add = 0;
  Object *cam_obj = rb->scene->camera;
  Camera *c = cam_obj->data;
  LANPR_RenderLine *rl;
  int contour_count = 0;
  int crease_count = 0;
  int MaterialCount = 0;

  rb->overall_progress = 20;
  rb->calculation_status = TNS_CALCULATION_CONTOUR;
  // nulThreadNotifyUsers("tns.render_buffer_list.calculation_status");

  if (c->type == CAM_ORTHO) {
    lanpr_compute_view_Vector(rb);
  }

  for (rl = rb->all_render_lines.first; rl; rl = rl->item.next) {
    // if(rl->testing)
    // if (!lanpr_line_crosses_frame(rl->l->fbcoord, rl->r->fbcoord))
    //	continue;

    Add = 0;
    Dot1 = 0;
    Dot2 = 0;

    if (c->type == CAM_PERSP) {
      tMatVectorConvert3fd(cam_obj->obmat[3], cam_location);
      tMatVectorMinus3d(view_vector, rl->l->gloc, cam_location);
    }

    if (use_smooth_contour_modifier_contour) {
      if (rl->flags & LANPR_EDGE_FLAG_CONTOUR)
        Add = 1;
    }
    else {
      if (rl->tl)
        Dot1 = tmat_dot_3d(view_vector, rl->tl->gn, 0);
      else
        Add = 1;
      if (rl->tr)
        Dot2 = tmat_dot_3d(view_vector, rl->tr->gn, 0);
      else
        Add = 1;
    }

    if (!Add) {
      if ((Result = Dot1 * Dot2) <= 0 && (Dot1 + Dot2))
        Add = 1;
      else if (tmat_dot_3d(rl->tl->gn, rl->tr->gn, 0) < threshold)
        Add = 2;
      else if (rl->tl && rl->tr && rl->tl->material_id != rl->tr->material_id)
        Add = 3;
    }

    if (Add == 1) {
      rl->flags |= LANPR_EDGE_FLAG_CONTOUR;
      list_append_pointer_static(&rb->contours, &rb->render_data_pool, rl);
      contour_count++;
    }
    else if (Add == 2) {
      rl->flags |= LANPR_EDGE_FLAG_CREASE;
      list_append_pointer_static(&rb->crease_lines, &rb->render_data_pool, rl);
      crease_count++;
    }
    else if (Add == 3) {
      rl->flags |= LANPR_EDGE_FLAG_MATERIAL;
      list_append_pointer_static(&rb->material_lines, &rb->render_data_pool, rl);
      MaterialCount++;
    }
    if (rl->flags & LANPR_EDGE_FLAG_EDGE_MARK) {
      // no need to mark again
      Add = 4;
      list_append_pointer_static(&rb->edge_marks, &rb->render_data_pool, rl);
      // continue;
    }
    if (Add) {
      int r1, r2, c1, c2, row, col;
      if (lanpr_get_line_bounding_areas(rb, rl, &r1, &r2, &c1, &c2)) {
        for (row = r1; row != r2 + 1; row++) {
          for (col = c1; col != c2 + 1; col++) {
            lanpr_link_line_with_bounding_area(rb, &rb->initial_bounding_areas[row * 4 + col], rl);
          }
        }
      }
    }

    // line count reserved for feature such as progress feedback
  }
}

void lanpr_clear_render_state(LANPR_RenderBuffer *rb)
{
  rb->contour_count = 0;
  rb->contour_managed = 0;
  rb->intersection_count = 0;
  rb->intersection_managed = 0;
  rb->material_line_count = 0;
  rb->material_managed = 0;
  rb->crease_count = 0;
  rb->crease_managed = 0;
  rb->calculation_status = TNS_CALCULATION_IDLE;
}

/* ====================================== render control ======================================= */

extern LANPR_SharedResource lanpr_share;

void lanpr_destroy_render_data(LANPR_RenderBuffer *rb)
{
  LANPR_RenderElementLinkNode *reln;

  rb->contour_count = 0;
  rb->contour_managed = 0;
  rb->intersection_count = 0;
  rb->intersection_managed = 0;
  rb->material_line_count = 0;
  rb->material_managed = 0;
  rb->crease_count = 0;
  rb->crease_managed = 0;
  rb->edge_mark_count = 0;
  rb->edge_mark_managed = 0;
  rb->calculation_status = TNS_CALCULATION_IDLE;

  list_handle_empty(&rb->contours);
  list_handle_empty(&rb->intersection_lines);
  list_handle_empty(&rb->crease_lines);
  list_handle_empty(&rb->material_lines);
  list_handle_empty(&rb->edge_marks);
  list_handle_empty(&rb->all_render_lines);
  list_handle_empty(&rb->chains);

  while (reln = BLI_pophead(&rb->vertex_buffer_pointers)) {
    MEM_freeN(reln->pointer);
  }

  while (reln = BLI_pophead(&rb->line_buffer_pointers)) {
    MEM_freeN(reln->pointer);
  }

  while (reln = BLI_pophead(&rb->triangle_buffer_pointers)) {
    MEM_freeN(reln->pointer);
  }

  BLI_spin_end(&rb->cs_data);
  BLI_spin_end(&rb->cs_info);
  BLI_spin_end(&rb->cs_management);
  BLI_spin_end(&rb->render_data_pool.cs_mem);

  mem_static_destroy(&rb->render_data_pool);
}

LANPR_RenderBuffer *lanpr_create_render_buffer(SceneLANPR *lanpr)
{
  if (lanpr_share.render_buffer_shared) {
    lanpr->render_buffer = lanpr_share.render_buffer_shared;
    lanpr_destroy_render_data(lanpr->render_buffer);
    return lanpr->render_buffer;
    // lanpr_destroy_render_data(lanpr->render_buffer);
    // MEM_freeN(lanpr->render_buffer);
  }

  LANPR_RenderBuffer *rb = MEM_callocN(sizeof(LANPR_RenderBuffer), "creating LANPR render buffer");

  lanpr->render_buffer = rb;
  lanpr_share.render_buffer_shared = rb;

  rb->cached_for_frame = -1;

  BLI_spin_init(&rb->cs_data);
  BLI_spin_init(&rb->cs_info);
  BLI_spin_init(&rb->cs_management);
  BLI_spin_init(&rb->render_data_pool.cs_mem);

  return rb;
}

void lanpr_rebuild_render_draw_command(LANPR_RenderBuffer *rb, LANPR_LineLayer *ll);

int lanpr_get_render_triangle_size(LANPR_RenderBuffer *rb)
{
  if (rb->thread_count == 0)
    rb->thread_count = BKE_render_num_threads(&rb->scene->r);
  return sizeof(LANPR_RenderTriangle) + (sizeof(LANPR_RenderLine *) * rb->thread_count);
}

static char Message[] = "Please fill in these fields before exporting image:";
static char MessageFolder[] = "    - Output folder";
static char MessagePrefix[] = "    - File name prefix";
static char MessageConnector[] = "    - File name connector";
static char MessageLayerName[] = "    - One or more layers have empty/illegal names.";
static char MessageSuccess[] = "Sucessfully Saved Image(s).";
static char MessageHalfSuccess[] = "Some image(s) failed to save.";
static char MessageFailed[] = "No saving action performed.";

// int ACTINv_SaveRenderBufferPreview(nActuatorIntern* a, nEvent* e) {
//	LANPR_RenderBuffer* rb = a->This->EndInstance;
//	LANPR_LineStyle* ll;
//	char FullPath[1024] = "";
//
//	if (!rb) return;
//
//	tnsFrameBuffer *fb = rb->FrameBuffer;
//
//	if (fb->OutputMode == TNS_OUTPUT_MODE_COMBINED) {
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix ||
//! fb->ImagePrefix->Ptr)) { 			nPanelMessageList List = {0}; 			nulAddPanelMessage(&List,
//! Message); 			if
//((!fb->OutputFolder || !fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder); if
//((!fb->ImagePrefix || !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//			nulAddPanelMessage(&List, MessageFailed);
//			nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//			return NUL_FINISHED;
//		}
//		strcat(FullPath, fb->OutputFolder->Ptr);
//		strcat(FullPath, fb->ImagePrefix->Ptr);
//		lanpr_SaveRenderBufferPreviewAsImage(rb, FullPath, 0, 0);
//	}else if(fb->OutputMode == TNS_OUTPUT_MODE_PER_LAYER) {
//		nPanelMessageList List = { 0 };
//		int unnamed = 0;
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix ||
//! fb->ImagePrefix->Ptr)|| (!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) {
//			nulAddPanelMessage(&List, Message);
//			if ((!fb->OutputFolder||!fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder);
//			if ((!fb->ImagePrefix|| !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//			if ((!fb->ImageNameConnector|| !fb->ImageNameConnector->Ptr)) nulAddPanelMessage(&List,
// MessageConnector); 			nulAddPanelMessage(&List, MessageFailed);
// nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e); 			return
// NUL_FINISHED;
//		}
//		for (ll = lanpr->line_layers.first; ll; ll = ll->item.next) {
//			FullPath[0] = 0;
//			if ((!ll->Name || !ll->Name->Ptr) && !unnamed) {
//				nulAddPanelMessage(&List, MessageHalfSuccess);
//				nulAddPanelMessage(&List, MessageLayerName);
//				unnamed = 1;
//				continue;
//			}
//			strcat(FullPath, fb->OutputFolder->Ptr);
//			strcat(FullPath, fb->ImagePrefix->Ptr);
//			strcat(FullPath, fb->ImageNameConnector->Ptr);
//			strcat(FullPath, ll->Name->Ptr);
//			lanpr_SaveRenderBufferPreviewAsImage(rb, FullPath, ll, 0);
//		}
//		if(unnamed)nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//	}
//
//	return NUL_FINISHED;
//}
// int ACTINv_SaveSingleLayer(nActuator* a, nEvent* e) {
//	LANPR_LineStyle* ll = a->This->EndInstance;
//	char FullPath[1024] = "";
//	int fail = 0;
//
//	if (!ll)return;
//
//	tnsFrameBuffer* fb = ll->ParentRB->FrameBuffer;
//
//	if (!fb) return;
//
//	nPanelMessageList List = { 0 };
//
//	if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix ||
//! fb->ImagePrefix->Ptr) || (!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) {
//		nulAddPanelMessage(&List, Message);
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder);
//		if ((!fb->ImagePrefix || !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//		if ((!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) nulAddPanelMessage(&List,
// MessageConnector); 		fail = 1;
//	}
//	if (!ll->Name || !ll->Name->Ptr) {
//		nulAddPanelMessage(&List, MessageHalfSuccess);
//		nulAddPanelMessage(&List, MessageLayerName);
//		fail = 1;
//	}
//	if (fail) {
//		nulAddPanelMessage(&List, MessageFailed);
//		nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//		return NUL_FINISHED;
//	}
//
//
//	FullPath[0] = 0;
//	strcat(FullPath, fb->OutputFolder->Ptr);
//	strcat(FullPath, fb->ImagePrefix->Ptr);
//	strcat(FullPath, fb->ImageNameConnector->Ptr);
//	strcat(FullPath, ll->Name->Ptr);
//	lanpr_SaveRenderBufferPreviewAsImage(ll->ParentRB, FullPath, ll, 0);
//
//
//	return NUL_FINISHED;
//}

int lanpr_count_this_line(LANPR_RenderLine *rl, LANPR_LineLayer *ll)
{
  LANPR_LineLayerComponent *llc = ll->components.first;
  int AndResult = 1, OrResult = 0;
  if (!llc)
    return 1;
  for (llc; llc; llc = llc->next) {
    if (llc->component_mode == LANPR_COMPONENT_MODE_ALL) {
      OrResult = 1;
    }
    else if (llc->component_mode == LANPR_COMPONENT_MODE_OBJECT && llc->object_select) {
      if (rl->object_ref && rl->object_ref->id.orig_id == &llc->object_select->id) {
        OrResult = 1;
      }
      else {
        AndResult = 0;
      }
    }
    else if (llc->component_mode == LANPR_COMPONENT_MODE_MATERIAL && llc->material_select) {
      if ((rl->tl && rl->tl->material_id == llc->material_select->index) ||
          (rl->tr && rl->tr->material_id == llc->material_select->index) ||
          (!(rl->flags & LANPR_EDGE_FLAG_INTERSECTION))) {
        OrResult = 1;
      }
      else {
        AndResult = 0;
      }
    }
    else if (llc->component_mode == LANPR_COMPONENT_MODE_COLLECTION && llc->collection_select) {
      if (BKE_collection_has_object(llc->collection_select,
                                    (Object *)rl->object_ref->id.orig_id)) {
        OrResult = 1;
      }
      else {
        AndResult = 0;
      }
    }
  }
  if (ll->logic_mode == LANPR_COMPONENT_LOGIG_OR)
    return OrResult;
  else
    return AndResult;
}
long lanpr_count_leveled_edge_segment_count(ListBase *LineList, LANPR_LineLayer *ll)
{
  LinkData *lip;
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls;
  Object *o;
  int not = 0;
  long Count = 0;
  for (lip = LineList->first; lip; lip = lip->next) {
    rl = lip->data;
    if (!lanpr_count_this_line(rl, ll))
      continue;

    for (rls = rl->segments.first; rls; rls = rls->item.next) {

      if (rls->occlusion >= ll->qi_begin && rls->occlusion <= ll->qi_end)
        Count++;
    }
  }
  return Count;
}
long lanpr_count_intersection_segment_count(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls;
  long Count = 0;
  for (rl = rb->intersection_lines.first; rl; rl = rl->item.next) {
    Count++;
  }
  return Count;
}
void *lanpr_make_leveled_edge_vertex_array(LANPR_RenderBuffer *rb,
                                           ListBase *LineList,
                                           float *vertexArray,
                                           float *NormalArray,
                                           float **NextNormal,
                                           LANPR_LineLayer *ll,
                                           float componet_id)
{
  LinkData *lip;
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls, *irls;
  Object *o;
  real W = rb->w / 2, H = rb->h / 2;
  long i = 0;
  float *v = vertexArray;
  float *N = NormalArray;
  for (lip = LineList->first; lip; lip = lip->next) {
    rl = lip->data;
    if (!lanpr_count_this_line(rl, ll))
      continue;

    for (rls = rl->segments.first; rls; rls = rls->item.next) {
      if (rls->occlusion >= ll->qi_begin && rls->occlusion <= ll->qi_end) {

        if (rl->tl) {
          N[0] += rl->tl->gn[0];
          N[1] += rl->tl->gn[1];
          N[2] += rl->tl->gn[2];
        }
        if (rl->tr) {
          N[0] += rl->tr->gn[0];
          N[1] += rl->tr->gn[1];
          N[2] += rl->tr->gn[2];
        }
        if (rl->tl || rl->tr) {
          normalize_v3(N);
          copy_v3_v3(&N[3], N);
        }
        N += 6;

        CLAMP(rls->at, 0, 1);
        if (irls = rls->item.next)
          CLAMP(irls->at, 0, 1);

        *v = tnsLinearItp(rl->l->fbcoord[0], rl->r->fbcoord[0], rls->at);
        v++;
        *v = tnsLinearItp(rl->l->fbcoord[1], rl->r->fbcoord[1], rls->at);
        v++;
        *v = componet_id;
        v++;
        *v = tnsLinearItp(rl->l->fbcoord[0], rl->r->fbcoord[0], irls ? irls->at : 1);
        v++;
        *v = tnsLinearItp(rl->l->fbcoord[1], rl->r->fbcoord[1], irls ? irls->at : 1);
        v++;
        *v = componet_id;
        v++;
      }
    }
  }
  *NextNormal = N;
  return v;
}

void lanpr_chain_generate_draw_command(LANPR_RenderBuffer *rb);

/* ============================================ viewport display
 * ================================================= */

void lanpr_rebuild_render_draw_command(LANPR_RenderBuffer *rb, LANPR_LineLayer *ll)
{
  int Count = 0;
  int level;
  float *v, *tv, *N, *tn;
  int i;
  int vertCount;

  if (ll->type == TNS_COMMAND_LINE) {
    static GPUVertFormat format = {0};
    static struct {
      uint pos, normal;
    } attr_id;
    if (format.attr_len == 0) {
      attr_id.pos = GPU_vertformat_attr_add(&format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
      attr_id.normal = GPU_vertformat_attr_add(
          &format, "normal", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
    }

    GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);

    if (ll->enable_contour)
      Count += lanpr_count_leveled_edge_segment_count(&rb->contours, ll);
    if (ll->enable_crease)
      Count += lanpr_count_leveled_edge_segment_count(&rb->crease_lines, ll);
    if (ll->enable_intersection)
      Count += lanpr_count_leveled_edge_segment_count(&rb->intersection_lines, ll);
    if (ll->enable_edge_mark)
      Count += lanpr_count_leveled_edge_segment_count(&rb->edge_marks, ll);
    if (ll->enable_material_seperate)
      Count += lanpr_count_leveled_edge_segment_count(&rb->material_lines, ll);

    vertCount = Count * 2;

    if (!vertCount)
      return;

    GPU_vertbuf_data_alloc(vbo, vertCount);

    tv = v = CreateNewBuffer(float, 6 * Count);
    tn = N = CreateNewBuffer(float, 6 * Count);

    if (ll->enable_contour)
      tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->contours, tv, tn, &tn, ll, 1.0f);
    if (ll->enable_crease)
      tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->crease_lines, tv, tn, &tn, ll, 2.0f);
    if (ll->enable_material_seperate)
      tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->material_lines, tv, tn, &tn, ll, 3.0f);
    if (ll->enable_edge_mark)
      tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->edge_marks, tv, tn, &tn, ll, 4.0f);
    if (ll->enable_intersection)
      tv = lanpr_make_leveled_edge_vertex_array(
          rb, &rb->intersection_lines, tv, tn, &tn, ll, 5.0f);

    for (i = 0; i < vertCount; i++) {
      GPU_vertbuf_attr_set(vbo, attr_id.pos, i, &v[i * 3]);
      GPU_vertbuf_attr_set(vbo, attr_id.normal, i, &N[i * 3]);
    }

    MEM_freeN(v);
    MEM_freeN(N);

    ll->batch = GPU_batch_create_ex(
        GPU_PRIM_LINES, vbo, 0, GPU_USAGE_DYNAMIC | GPU_BATCH_OWNS_VBO);

    return;
  }

  // if (ll->type == TNS_COMMAND_MATERIAL || ll->type == TNS_COMMAND_EDGE) {
  // later implement ....
  //}
}
void lanpr_rebuild_all_command(SceneLANPR *lanpr)
{
  LANPR_LineLayer *ll;
  if (!lanpr || !lanpr->render_buffer)
    return;

  if (lanpr->enable_chaining) {
    lanpr_chain_generate_draw_command(lanpr->render_buffer);
  }
  else {
    for (ll = lanpr->line_layers.first; ll; ll = ll->next) {
      if (ll->batch)
        GPU_batch_discard(ll->batch);
      lanpr_rebuild_render_draw_command(lanpr->render_buffer, ll);
    }
  }
}

void lanpr_viewport_draw_offline_result(LANPR_TextureList *txl,
                                        LANPR_FramebufferList *fbl,
                                        LANPR_PassList *psl,
                                        LANPR_PrivateData *pd,
                                        SceneLANPR *lanpr)
{
  float clear_col[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float clear_depth = 1.0f;
  uint clear_stencil = 0xFF;

  DefaultTextureList *dtxl = DRW_viewport_texture_list_get();
  DefaultFramebufferList *dfbl = DRW_viewport_framebuffer_list_get();

  int texw = GPU_texture_width(dtxl->color), texh = GPU_texture_height(dtxl->color);
  int tsize = texw * texh;

  const DRWContextState *draw_ctx = DRW_context_state_get();
  Scene *scene = DEG_get_evaluated_scene(draw_ctx->depsgraph);
  View3D *v3d = draw_ctx->v3d;
  Object *camera;
  if (v3d) {
    RegionView3D *rv3d = draw_ctx->rv3d;
    camera = (rv3d->persp == RV3D_CAMOB) ? v3d->camera : NULL;
  }
  else {
    camera = scene->camera;
  }

  GPU_framebuffer_bind(fbl->dpix_transform);
  DRW_draw_pass(psl->dpix_transform_pass);

  GPU_framebuffer_bind(fbl->dpix_preview);
  eGPUFrameBufferBits clear_bits = GPU_COLOR_BIT;
  GPU_framebuffer_clear(
      fbl->dpix_preview, clear_bits, lanpr->background_color, clear_depth, clear_stencil);
  DRW_draw_pass(psl->dpix_preview_pass);

  GPU_framebuffer_bind(dfbl->default_fb);
  GPU_framebuffer_clear(
      dfbl->default_fb, clear_bits, lanpr->background_color, clear_depth, clear_stencil);
  DRW_multisamples_resolve(txl->depth, txl->color, 1);
}

void lanpr_NO_THREAD_chain_feature_lines(LANPR_RenderBuffer *rb, float dist_threshold);

void lanpr_calculate_normal_object_vector(LANPR_LineLayer *ll, float *normal_object_direction)
{
  Object *ob;
  switch (ll->normal_mode) {
    case LANPR_NORMAL_DONT_CARE:
      return;
    case LANPR_NORMAL_DIRECTIONAL:
      if (!(ob = ll->normal_control_object)) {
        normal_object_direction[0] = 0;
        normal_object_direction[1] = 0;
        normal_object_direction[2] = 1;  // default z up direction
      }
      else {
        float dir[3] = {0, 0, 1};
        float mat[3][3];
        copy_m3_m4(mat, ob->obmat);
        mul_v3_m3v3(normal_object_direction, mat, dir);
        normalize_v3(normal_object_direction);
      }
      return;
    case LANPR_NORMAL_POINT:
      if (!(ob = ll->normal_control_object)) {
        normal_object_direction[0] = 0;
        normal_object_direction[1] = 0;
        normal_object_direction[2] = 0;  // default origin position
      }
      else {
        normal_object_direction[0] = ob->obmat[3][0];
        normal_object_direction[1] = ob->obmat[3][1];
        normal_object_direction[2] = ob->obmat[3][2];
      }
      return;
    case LANPR_NORMAL_2D:
      return;
  }
}

void lanpr_software_draw_scene(void *vedata, GPUFrameBuffer *dfb, int is_render)
{
  LANPR_LineLayer *ll;
  LANPR_PassList *psl = ((LANPR_Data *)vedata)->psl;
  LANPR_TextureList *txl = ((LANPR_Data *)vedata)->txl;
  LANPR_StorageList *stl = ((LANPR_Data *)vedata)->stl;
  LANPR_FramebufferList *fbl = ((LANPR_Data *)vedata)->fbl;
  LANPR_PrivateData *pd = stl->g_data;
  const DRWContextState *draw_ctx = DRW_context_state_get();
  Scene *scene = DEG_get_evaluated_scene(draw_ctx->depsgraph);
  SceneLANPR *lanpr = &scene->lanpr;
  View3D *v3d = draw_ctx->v3d;
  float indentity_mat[4][4];
  static float normal_object_direction[3] = {0, 0, 1};

  if (is_render) {
    lanpr_rebuild_all_command(lanpr);
  }
  else {
    if (lanpr_during_render()) {
      printf(
          "LANPR Warning: To avoid resource duplication, viewport will not display when rendering "
          "is in progress\n");
      return;  // don't draw viewport during render
    }
  }

  float clear_col[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float clear_depth = 1.0f;
  uint clear_stencil = 0xFF;
  eGPUFrameBufferBits clear_bits = GPU_DEPTH_BIT | GPU_COLOR_BIT;

  // Object *camera;
  // if (v3d) {
  //	RegionView3D *rv3d = draw_ctx->rv3d;
  //	camera = (rv3d->persp == RV3D_CAMOB) ? v3d->camera : NULL;
  //}
  // else {
  //	camera = scene->camera;
  //}

  GPU_framebuffer_bind(fbl->software_ms);
  GPU_framebuffer_clear(
      fbl->software_ms, clear_bits, lanpr->background_color, clear_depth, clear_stencil);

  if (lanpr->render_buffer) {

    int texw = GPU_texture_width(txl->ms_resolve_color),
        texh = GPU_texture_height(txl->ms_resolve_color);
    ;
    pd->output_viewport[2] = scene->r.xsch;
    pd->output_viewport[3] = scene->r.ysch;
    pd->dpix_viewport[2] = texw;
    pd->dpix_viewport[3] = texh;

    unit_m4(indentity_mat);

    DRWView *view = DRW_view_create(indentity_mat, indentity_mat, NULL, NULL, NULL);
    DRW_view_default_set(view);
    DRW_view_set_active(view);

    if (lanpr->enable_chaining && lanpr->render_buffer->chain_draw_batch) {
      for (ll = lanpr->line_layers.last; ll; ll = ll->prev) {
        LANPR_RenderBuffer *rb;
        psl->software_pass = DRW_pass_create("Software Render Preview",
                                             DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH |
                                                 DRW_STATE_DEPTH_LESS_EQUAL);
        rb = lanpr->render_buffer;
        rb->ChainShgrp = DRW_shgroup_create(lanpr_share.software_chaining_shader,
                                            psl->software_pass);

        lanpr_calculate_normal_object_vector(ll, normal_object_direction);

        DRW_shgroup_uniform_vec4(rb->ChainShgrp, "color", ll->color, 1);
        DRW_shgroup_uniform_vec4(rb->ChainShgrp, "crease_color", ll->crease_color, 1);
        DRW_shgroup_uniform_vec4(rb->ChainShgrp, "material_color", ll->material_color, 1);
        DRW_shgroup_uniform_vec4(rb->ChainShgrp, "edge_mark_color", ll->edge_mark_color, 1);
        DRW_shgroup_uniform_vec4(rb->ChainShgrp, "intersection_color", ll->intersection_color, 1);
        DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness", &ll->thickness, 1);
        DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness_crease", &ll->thickness_crease, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "thickness_material", &ll->thickness_material, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "thickness_edge_mark", &ll->thickness_edge_mark, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "thickness_intersection", &ll->thickness_intersection, 1);
        DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_contour", &ll->enable_contour, 1);
        DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_crease", &ll->enable_crease, 1);
        DRW_shgroup_uniform_int(
            rb->ChainShgrp, "enable_material", &ll->enable_material_seperate, 1);
        DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_edge_mark", &ll->enable_edge_mark, 1);
        DRW_shgroup_uniform_int(
            rb->ChainShgrp, "enable_intersection", &ll->enable_intersection, 1);

        DRW_shgroup_uniform_int(rb->ChainShgrp, "normal_mode", &ll->normal_mode, 1);
        DRW_shgroup_uniform_int(
            rb->ChainShgrp, "normal_effect_inverse", &ll->normal_effect_inverse, 1);
        DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_ramp_begin", &ll->normal_ramp_begin, 1);
        DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_ramp_end", &ll->normal_ramp_end, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "normal_thickness_begin", &ll->normal_thickness_begin, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "normal_thickness_end", &ll->normal_thickness_end, 1);
        DRW_shgroup_uniform_vec3(rb->ChainShgrp, "normal_direction", normal_object_direction, 1);

        DRW_shgroup_uniform_int(rb->ChainShgrp, "occlusion_level_begin", &ll->qi_begin, 1);
        DRW_shgroup_uniform_int(rb->ChainShgrp, "occlusion_level_end", &ll->qi_end, 1);

        DRW_shgroup_uniform_vec4(
            rb->ChainShgrp, "preview_viewport", stl->g_data->dpix_viewport, 1);
        DRW_shgroup_uniform_vec4(
            rb->ChainShgrp, "output_viewport", stl->g_data->output_viewport, 1);

        float *tld = &lanpr->taper_left_distance, *tls = &lanpr->taper_left_strength,
              *trd = &lanpr->taper_right_distance, *trs = &lanpr->taper_right_strength;

        DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_l_dist", tld, 1);
        DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_l_strength", tls, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "taper_r_dist", lanpr->use_same_taper ? tld : trd, 1);
        DRW_shgroup_uniform_float(
            rb->ChainShgrp, "taper_r_strength", lanpr->use_same_taper ? tls : trs, 1);

        // need to add component enable/disable option.
        DRW_shgroup_call(rb->ChainShgrp, lanpr->render_buffer->chain_draw_batch, NULL);
        // debug purpose
        // DRW_draw_pass(psl->color_pass);
        // DRW_draw_pass(psl->color_pass);
        DRW_draw_pass(psl->software_pass);
      }
    }
    else if (!lanpr->enable_chaining) {
      for (ll = lanpr->line_layers.last; ll; ll = ll->prev) {
        if (ll->batch) {
          psl->software_pass = DRW_pass_create("Software Render Preview",
                                               DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH |
                                                   DRW_STATE_DEPTH_LESS_EQUAL);
          ll->shgrp = DRW_shgroup_create(lanpr_share.software_shader, psl->software_pass);

          lanpr_calculate_normal_object_vector(ll, normal_object_direction);

          DRW_shgroup_uniform_vec4(ll->shgrp, "color", ll->color, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "crease_color", ll->crease_color, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "material_color", ll->material_color, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "edge_mark_color", ll->edge_mark_color, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "intersection_color", ll->intersection_color, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "thickness", &ll->thickness, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "thickness_crease", &ll->thickness_crease, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "thickness_material", &ll->thickness_material, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "thickness_edge_mark", &ll->thickness_edge_mark, 1);
          DRW_shgroup_uniform_float(
              ll->shgrp, "thickness_intersection", &ll->thickness_intersection, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "preview_viewport", stl->g_data->dpix_viewport, 1);
          DRW_shgroup_uniform_vec4(ll->shgrp, "output_viewport", stl->g_data->output_viewport, 1);

          DRW_shgroup_uniform_int(ll->shgrp, "normal_mode", &ll->normal_mode, 1);
          DRW_shgroup_uniform_int(
              ll->shgrp, "normal_effect_inverse", &ll->normal_effect_inverse, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "normal_ramp_begin", &ll->normal_ramp_begin, 1);
          DRW_shgroup_uniform_float(ll->shgrp, "normal_ramp_end", &ll->normal_ramp_end, 1);
          DRW_shgroup_uniform_float(
              ll->shgrp, "normal_thickness_begin", &ll->normal_thickness_begin, 1);
          DRW_shgroup_uniform_float(
              ll->shgrp, "normal_thickness_end", &ll->normal_thickness_end, 1);
          DRW_shgroup_uniform_vec3(ll->shgrp, "normal_direction", normal_object_direction, 1);

          DRW_shgroup_call(ll->shgrp, ll->batch, NULL);
          DRW_draw_pass(psl->software_pass);
        }
      }
    }
  }

  GPU_framebuffer_blit(fbl->software_ms, 0, dfb, 0, GPU_COLOR_BIT);
}

/* ============================================ operators =========================================
 */

int lanpr_compute_feature_lines_internal(Depsgraph *depsgraph, SceneLANPR *lanpr, Scene *scene)
{
  LANPR_RenderBuffer *rb;

  rb = lanpr_create_render_buffer(lanpr);
  rb->scene = scene;
  rb->w = scene->r.xsch;
  rb->h = scene->r.ysch;
  rb->enable_intersections = lanpr->enable_intersections;

  rb->triangle_size = lanpr_get_render_triangle_size(rb);

  lanpr_make_render_geometry_buffers(depsgraph, scene, scene->camera, rb);

  lanpr_compute_view_Vector(rb);
  lanpr_cull_triangles(rb);

  lanpr_perspective_division(rb);

  lanpr_make_initial_bounding_areas(rb);

  lanpr_compute_scene_contours(rb, lanpr->crease_threshold);

  lanpr_add_triangles(rb);

  lanpr_THREAD_calculate_line_occlusion_begin(rb);

  if (lanpr->enable_chaining) {
    lanpr_NO_THREAD_chain_feature_lines(rb, 0.00001);  // should use user_adjustable value
  }

  rb->cached_for_frame = scene->r.cfra;

  return OPERATOR_FINISHED;
}

// seems we don't quite need this operator...
static int lanpr_clear_render_buffer_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;
  LANPR_RenderBuffer *rb;
  Depsgraph *depsgraph = CTX_data_depsgraph(C);

  lanpr_destroy_render_data(lanpr->render_buffer);

  return OPERATOR_FINISHED;
}
static int lanpr_compute_feature_lines_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  if (!scene->camera) {
    BKE_report(op->reports, RPT_ERROR, "There is no active camera in this scene!");
    printf("LANPR Warning: There is no active camera in this scene!\n");
    return OPERATOR_FINISHED;
  }

  return lanpr_compute_feature_lines_internal(CTX_data_depsgraph(C), lanpr, scene);
}
static void lanpr_compute_feature_lines_cancel(struct bContext *C, struct wmOperator *op)
{

  return;
}

void SCENE_OT_lanpr_calculate_feature_lines(struct wmOperatorType *ot)
{

  /* identifiers */
  ot->name = "Calculate Feature Lines";
  ot->description = "LANPR calculates feature line in current scene";
  ot->idname = "SCENE_OT_lanpr_calculate";

  /* api callbacks */
  // ot->invoke = screen_render_invoke; /* why we need both invoke and exec? */
  // ot->modal = screen_render_modal;
  ot->cancel = lanpr_compute_feature_lines_cancel;
  ot->exec = lanpr_compute_feature_lines_exec;
}

LANPR_LineLayer *lanpr_new_line_layer(SceneLANPR *lanpr)
{
  LANPR_LineLayer *ll = MEM_callocN(sizeof(LANPR_LineLayer), "Line Layer");
  BLI_addtail(&lanpr->line_layers, ll);
  lanpr->active_layer = ll;
  return ll;
}
LANPR_LineLayerComponent *lanpr_new_line_component(SceneLANPR *lanpr)
{
  if (!lanpr->active_layer)
    return 0;
  LANPR_LineLayer *ll = lanpr->active_layer;

  LANPR_LineLayerComponent *llc = MEM_callocN(sizeof(LANPR_LineLayerComponent), "Line Component");
  BLI_addtail(&ll->components, llc);

  return llc;
}

int lanpr_add_line_layer_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  lanpr_new_line_layer(lanpr);

  return OPERATOR_FINISHED;
}
int lanpr_delete_line_layer_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  LANPR_LineLayer *ll = lanpr->active_layer;

  if (!ll)
    return OPERATOR_FINISHED;

  if (ll->prev)
    lanpr->active_layer = ll->prev;
  else if (ll->next)
    lanpr->active_layer = ll->next;
  else
    lanpr->active_layer = 0;

  BLI_remlink(&scene->lanpr.line_layers, ll);

  // if (ll->batch) GPU_batch_discard(ll->batch);

  MEM_freeN(ll);

  return OPERATOR_FINISHED;
}
int lanpr_move_line_layer_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  LANPR_LineLayer *ll = lanpr->active_layer;

  if (!ll)
    return OPERATOR_FINISHED;

  int dir = RNA_enum_get(op->ptr, "direction");

  if (dir == 1 && ll->prev) {
    BLI_remlink(&lanpr->line_layers, ll);
    BLI_insertlinkbefore(&lanpr->line_layers, ll->prev, ll);
  }
  else if (dir == -1 && ll->next) {
    BLI_remlink(&lanpr->line_layers, ll);
    BLI_insertlinkafter(&lanpr->line_layers, ll->next, ll);
  }

  return OPERATOR_FINISHED;
}
int lanpr_add_line_component_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  lanpr_new_line_component(lanpr);

  return OPERATOR_FINISHED;
}
int lanpr_delete_line_component_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;
  LANPR_LineLayer *ll = lanpr->active_layer;
  LANPR_LineLayerComponent *llc;
  int i = 0;

  if (!ll)
    return OPERATOR_FINISHED;

  int index = RNA_int_get(op->ptr, "index");

  for (llc = ll->components.first; llc; llc = llc->next) {
    if (index == i)
      break;
    i++;
  }

  if (llc) {
    BLI_remlink(&ll->components, llc);
    MEM_freeN(llc);
  }

  return OPERATOR_FINISHED;
}
int lanpr_rebuild_all_commands_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  lanpr_rebuild_all_command(lanpr);
  return OPERATOR_FINISHED;
}
int lanpr_enable_all_line_types_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;
  LANPR_LineLayer *ll;

  if (!(ll = lanpr->active_layer))
    return OPERATOR_FINISHED;

  ll->enable_contour = 1;
  ll->enable_crease = 1;
  ll->enable_edge_mark = 1;
  ll->enable_material_seperate = 1;
  ll->enable_intersection = 1;

  copy_v3_v3(ll->crease_color, ll->color);
  copy_v3_v3(ll->edge_mark_color, ll->color);
  copy_v3_v3(ll->material_color, ll->color);
  copy_v3_v3(ll->intersection_color, ll->color);

  ll->thickness_crease = 1;
  ll->thickness_material = 1;
  ll->thickness_edge_mark = 1;
  ll->thickness_intersection = 1;

  return OPERATOR_FINISHED;
}
int lanpr_auto_create_line_layer_exec(struct bContext *C, struct wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  SceneLANPR *lanpr = &scene->lanpr;

  LANPR_LineLayer *ll;

  ll = lanpr_new_line_layer(lanpr);
  ll->thickness = 1.7;
  ll->color[0] = 1;
  ll->color[1] = 1;
  ll->color[2] = 1;

  lanpr_enable_all_line_types_exec(C, op);

  ll = lanpr_new_line_layer(lanpr);
  ll->thickness = 0.9;
  ll->qi_begin = 1;
  ll->qi_end = 1;
  ll->color[0] = 0.314;
  ll->color[1] = 0.596;
  ll->color[2] = 1;

  lanpr_enable_all_line_types_exec(C, op);

  ll = lanpr_new_line_layer(lanpr);
  ll->thickness = 0.7;
  ll->qi_begin = 2;
  ll->qi_end = 2;
  ll->color[0] = 0.135;
  ll->color[1] = 0.304;
  ll->color[2] = 0.508;

  lanpr_enable_all_line_types_exec(C, op);

  lanpr_rebuild_all_command(lanpr);

  return OPERATOR_FINISHED;
}

void SCENE_OT_lanpr_add_line_layer(struct wmOperatorType *ot)
{

  ot->name = "Add Line Layer";
  ot->description = "Add a new line layer";
  ot->idname = "SCENE_OT_lanpr_add_line_layer";

  ot->exec = lanpr_add_line_layer_exec;
}
void SCENE_OT_lanpr_delete_line_layer(struct wmOperatorType *ot)
{

  ot->name = "Delete Line Layer";
  ot->description = "Delete selected line layer";
  ot->idname = "SCENE_OT_lanpr_delete_line_layer";

  ot->exec = lanpr_delete_line_layer_exec;
}
void SCENE_OT_lanpr_rebuild_all_commands(struct wmOperatorType *ot)
{

  ot->name = "Refresh Drawing Commands";
  ot->description = "Refresh LANPR line layer drawing commands";
  ot->idname = "SCENE_OT_lanpr_rebuild_all_commands";

  ot->exec = lanpr_rebuild_all_commands_exec;
}
void SCENE_OT_lanpr_auto_create_line_layer(struct wmOperatorType *ot)
{

  ot->name = "Auto Create Line Layer";
  ot->description = "Automatically create defalt line layer config";
  ot->idname = "SCENE_OT_lanpr_auto_create_line_layer";

  ot->exec = lanpr_auto_create_line_layer_exec;
}
void SCENE_OT_lanpr_move_line_layer(struct wmOperatorType *ot)
{
  static const EnumPropertyItem line_layer_move[] = {
      {1, "up", 0, "Up", ""}, {-1, "DOWN", 0, "Down", ""}, {0, NULL, 0, NULL, NULL}};

  ot->name = "Move Line Layer";
  ot->description = "Move LANPR line layer up and down";
  ot->idname = "SCENE_OT_lanpr_move_line_layer";

  // this need property to assign up/down direction

  ot->exec = lanpr_move_line_layer_exec;

  RNA_def_enum(ot->srna,
               "direction",
               line_layer_move,
               0,
               "Direction",
               "Direction to move the active line layer towards");
}
void SCENE_OT_lanpr_enable_all_line_types(struct wmOperatorType *ot)
{
  ot->name = "Enable All Line Types";
  ot->description = "Enable All Line Types In This Line Layer.";
  ot->idname = "SCENE_OT_lanpr_enable_all_line_types";

  ot->exec = lanpr_enable_all_line_types_exec;
}

void SCENE_OT_lanpr_add_line_component(struct wmOperatorType *ot)
{

  ot->name = "Add Line Component";
  ot->description = "Add a new line Component";
  ot->idname = "SCENE_OT_lanpr_add_line_component";

  ot->exec = lanpr_add_line_component_exec;
}
void SCENE_OT_lanpr_delete_line_component(struct wmOperatorType *ot)
{

  ot->name = "Delete Line Component";
  ot->description = "Delete selected line component";
  ot->idname = "SCENE_OT_lanpr_delete_line_component";

  ot->exec = lanpr_delete_line_component_exec;

  RNA_def_int(ot->srna, "index", 0, 0, 10000, "index", "index of this line component", 0, 10000);
}

#ifdef USE_LANPR_HINT

// how to access LANPR's occlusion info after LANPR software mode calculation

// You can access descrete occlusion data from every edge,
// but you can also access occlusion using LANPR's chain data.
// Two examples are given.

// [1.descrete occlusion data for
// edges]======================================================================================
//
// LANPR occlusion related data storage :
//
// LANPR_RenderBuffer :: all_render_lines   ====>  All LANPR_RenderLine nodes. Each node for a
// singe edge on the mesh.
//                                               Only features lines are in this list.
//                                               LANPR_RenderLine stores a list of occlusion info
//                                               in LANPR_RenderLineSegment.
//
// LANPR_RenderBuffer :: contours (and crease/material_lines/Intersections/edge_marks)
//                                        ====>  ListItemPointers to LANPR_RenderLine nodes.
//                                               Use these lists to access individual line types
//                                               for convenience. For how to access this list,
//                                               refer to this file line 728-730 as an example.
//
// LANPR_RenderLine :: segments           ====>  List of LANPR_RenderLineSegment to represent
// occlusion info.
//                                               See below for how occlusion is reoresented in
//                                               Renderline and RenderLineSegment.
//
//  RenderLine Diagram:
//    +[RenderLine]
//       [segments]
//         [Segment]  at=0      occlusion=0
//         [Segment]  at=0.5    occlusion=1
//         [Segment]  at=0.7    occlusion=0
//
//  Then you get a line with such occlusion:
//  [l]|-------------------------|=========|-----------[r]
//
//  the beginning to 50% of the line :   Not occluded
//  50% to 70% of the line :             Occluded 1 time
//  70% to the end of the line :         Not occluded
//
//  cut positions are linear interpolated in image space from line->l->fbcoord to
//  line->r->fbcoord (always l to r)
//                    ~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~
//  to see an example of iterating occlusion data for all lines for drawing, see below or refer to
//  this file line 2930.
//
//  [Iterating occlusion data]
void lanpr_iterate_renderline_and_occlusion(LANPR_RenderBuffer *rb, double *v_OUT, double Occ_OUT)
{
  LinkData *lip;
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls, *irls;
  double *v = v_Out;
  double *O = Occ_OUT;

  for (rl = rb->all_render_lines->first; rl; rl = lip->next) {
    for (rls = rl->segments.first; rls; rls = rls->item.next) {

      irls = rls->item.next;

      // safety reasons
      CLAMP(rls->at, 0, 1);
      if (irls)
        CLAMP(irls->at, 0, 1);

      // segment begin at some X and Y
      // tnsLinearItp() is a linear interpolate function.
      *v = tnsLinearItp(rl->l->fbcoord[0], rl->r->fbcoord[0], rls->at);
      v++;
      *v = tnsLinearItp(rl->l->fbcoord[1], rl->r->fbcoord[1], rls->at);
      v++;
      *O = rls->occlusion;
      *O++;

      // segment end at some X and Y
      *v = tnsLinearItp(rl->l->fbcoord[0], rl->r->fbcoord[0], irls ? irls->at : 1);
      v++;
      *v = tnsLinearItp(rl->l->fbcoord[1], rl->r->fbcoord[1], irls ? irls->at : 1);
      v++;
      *O = rls->occlusion;
      *O++;

      // please note that LANPR_RenderVert::fbcoord is in NDC coorninates
      // to transform it into screen pixel space use rb->W/2 and rb->H/2
    }
  }
}

// [2. LANPR's chain
// data]======================================================================================
//
// Chain data storage: (Also see lanpr_all.h line 485 for diagram)
//
// LANPR_RenderBuffer :: chains          ====> For storing LANPR_RenderLineChain nodes.
//                                             Only available when chaining enabled and calculation
//                                             is done.
//
// LANPR_RenderLineChain :: Chain        ====> LANPR_RenderLineChainItem node list.
//
// LANPR_RenderLineChainItem :: pos      ====> pos[0] and pos[1] for x y NDC coordinates, pos[2]
// reserved, do not use.
//
// LANPR_RenderLineChainItem :: LineType and occlusion   ====> See lanpr_all.h line 485 for
// diagram.
//                                                                  These 2 fields in the last
//                                                                  ChainItem of a Chain is not
//                                                                  used.
//
// to see an example of accessing occlusion data as a whole chain, see below or refer to
// lanpr_chain.c line 336.
//
// [Iteration chains and occlusion data for each chain segments]
int lanpr_iterate_chains_and_occlusion(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChainItem *rlci;
  LANPR_RenderLineChain *rlc;
  for (rlc = rb->chains.first; rlc; rlc = rlc->item.next) {
    for (rlci = rlc->Chain.first; rlci; rlci = rlci->item.next) {
      // vL = rlci->pos;
      // vR = ((LANPR_RenderLineChainItem*)rlci->item.next)->pos;
      // line_type_of_this_segment = rlci->LineType;          // ====> values are defined in
      // lanpr_all.h line 456. occlusion_of_this_segment = rlci->occlusion;
    }
  }
}

#endif
