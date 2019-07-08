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
 * The Original Code is Copyright (C) 2019 by Blender Foundation
 * All rights reserved.
 */

#include "MEM_guardedalloc.h"

#include "BLI_math.h"
#include "BLI_task.h"
#include "BLI_utildefines.h"
#include "BLI_ghash.h"
#include "BLI_listbase.h"
#include "BLI_polyfill_2d.h"
#include "BLI_math_color_blend.h"

#include "DNA_customdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_node_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_camera_types.h"

#include "BKE_context.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_mesh.h"
#include "BKE_editmesh.h"
#include "BKE_modifier.h"
#include "BKE_object.h"
#include "BKE_report.h"
#include "BKE_scene.h"
#include "BKE_screen.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "WM_api.h"
#include "WM_types.h"
#include "WM_message.h"
#include "WM_toolsystem.h"

#include "ED_screen.h"
#include "ED_object.h"
#include "ED_view3d.h"
#include "ED_space_api.h"
#include "ED_transform_snap_object_context.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "GPU_draw.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef enum BlueprintContext {
  BLUEPRINT_CTX_GEOMETRY,
  BLUEPRINT_CTX_SELECTION,
  BLUEPRINT_CTX_VOLUME,
} BlueprintContext;

typedef enum BlueprintState {
  BLUEPRINT_STATE_PLANE,
  BLUEPRINT_STATE_DRAW,
  BLUEPRINT_STATE_EXTRUDE,
  BLUEPRINT_STATE_BOOLEAN,
} BlueprintState;

typedef enum BlueprintDrawTool {
  BLUEPRINT_DT_RECT,
  BLUEPRINT_DT_ELLIPSE,
  BLUEPRINT_DT_POLYLINE,
  BLUEPRINT_DT_FREEHAND,
} BlueprintDrawTool;

typedef struct BlueprintConfig {
  BlueprintContext context;

} BlueprintConfig;

typedef enum BlueprintFlags {
  BLUEPRINT_SYMMETRICAL_EXTRUDE = 1 << 0,
  BLUEPRINT_AUTO_ENTER_DRAW_STATE = 1 << 1,
  BLUEPRINT_AUTO_START_DRAW_STATE = 1 << 2,
  BLUEPRINT_SKIP_EXTRUDE_STATE = 1 << 3,
  BLUEPRINT_ENABLE_GRID = 1 << 4,
  BLUEPRINT_ENABLE_GRID_BACKGROUD = 1 << 5,
  BLUEPRINT_ENABLE_GEOMETRY_FILL = 1 << 6,
  BLUEPRINT_EXTRUDE_ON_RELEASE = 1 << 7,
} BlueprintFlags;

typedef struct BlueprintGeometryData {
  float n[3];
  float t[3];
  float co[3];

  Object *sf_object;
} BlueprintGeometryData;

EnumPropertyItem prop_blueprint_draw_tool_types[] = {
    {BLUEPRINT_DT_RECT, "RECT", 0, "Rect", "Rectangle drawing"},
    {BLUEPRINT_DT_ELLIPSE, "ELLIPSE", 0, "Ellipse", "Ellipse drawing"},
    {BLUEPRINT_DT_POLYLINE, "POLYLINE", 0, "Polyline", "Polyline drawing"},
    {BLUEPRINT_DT_FREEHAND, "FREEHAND", 0, "Freehand", "Freehand drawing"},
    {0, NULL, 0, NULL, NULL},
};


typedef struct BlueprintPolyPoint {
  struct BlueprintPolyPoint *next, *prev;

  float co[2];
  int status;
  char _pad[4];
} BlueprintPolyPoint;

typedef struct BlueprintColor {
  uchar grid[4];
  uchar geom_stroke[4];
  uchar geom_fill[4];
  uchar cursor[4];
  uchar bg[4];
} BlueprintColor;

typedef struct BlueprintCache {
  BlueprintContext context;
  BlueprintState state;
  BlueprintDrawTool drawtool;
  BlueprintColor color;

  ARegion *ar;
  View3D *v3d;

  /* geometry creation context */
  Object *ob;



  float plane_offset;

  int grid_snap;
  float gridx_size;
  float gridy_size;
  float gridz_size;

  int grid_padding;
  float grid_extends_min[2];
  float grid_extends_max[2];

  float n[3];
  float bn[3];
  float co[3];

  float plane[4];
  float origin[3];
  float mat[4][4];
  float mat_inv[4][4];

  float cursor[3];
  float cursor_global[3];
  float cursor_mat[4][4];

  float extrudedist;
  float extrude_co[3];

  int extrude_both_sides;

  int point_count;
  ListBase poly;
  float (*coords)[2];
  uint (*r_tris)[3];

  /* for Draw rect tool */
  BlueprintPolyPoint *ps[4];

  /* for Ellipse tool */
  float cursor_start[3];
  int ellipse_nsegments;

  /* for Freehand tool */
  float freehand_step_threshold;

  int flags;
  bool draw_tool_init;
  char _pad[7];

  Object *sf_ob;
  Object *new_ob;
} BlueprintCache;

typedef struct BlueprintCD {
  BlueprintCache cache;

  void *draw_handle;

} BlueprintCD;

void BKE_blueprint_color_init(BlueprintCache *cache)
{
  cache->color.cursor[0] = 235;
  cache->color.cursor[1] = 59;
  cache->color.cursor[2] = 59;
  cache->color.cursor[3] = 255;

  cache->color.grid[0] = 220;
  cache->color.grid[1] = 220;
  cache->color.grid[2] = 220;
  cache->color.grid[3] = 200;

  cache->color.bg[0] = 111;
  cache->color.bg[1] = 158;
  cache->color.bg[2] = 232;
  cache->color.bg[3] = 180;

  cache->color.geom_fill[0] = 190;
  cache->color.geom_fill[1] = 190;
  cache->color.geom_fill[2] = 190;
  cache->color.geom_fill[3] = 150;

  cache->color.geom_stroke[0] = 230;
  cache->color.geom_stroke[1] = 230;
  cache->color.geom_stroke[2] = 230;
  cache->color.geom_stroke[3] = 255;
}

void blueprint_draw_poly_volume(uint pos3d, BlueprintCache *cache, float z)
{
  int point_count = cache->point_count;
  GPU_depth_test(true);
  immUniformColor4ubv(cache->color.geom_fill);
    float z_base = 0.0f;
    if (cache->flags & BLUEPRINT_SYMMETRICAL_EXTRUDE) {
      z_base = -cache->extrudedist;
    }

   bool draw_fill = cache->flags & BLUEPRINT_ENABLE_GEOMETRY_FILL;
  if (point_count >= 3) {
    if (draw_fill) {
        immBegin(GPU_PRIM_TRIS, point_count * 6);
        LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
          BlueprintPolyPoint *np;
          if (p->next) {
            np = p->next;
          }
          else {
            np = cache->poly.first;
          }

          immVertex3f(pos3d, p->co[0], p->co[1], z_base);
          immVertex3f(pos3d, p->co[0], p->co[1], z);
          immVertex3f(pos3d, np->co[0], np->co[1], z);

          immVertex3f(pos3d, np->co[0], np->co[1], z_base);
          immVertex3f(pos3d, p->co[0], p->co[1], z_base);
          immVertex3f(pos3d, np->co[0], np->co[1], z);
        }

        immEnd();
    }

    immUniformColor4ubv(cache->color.geom_stroke);
    GPU_line_width(2.0f);
    immBegin(GPU_PRIM_LINES, point_count * 2);
    LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
      immVertex3f(pos3d, p->co[0], p->co[1], z_base);
      immVertex3f(pos3d, p->co[0], p->co[1], z);
    }

    immEnd();
  }
}
void blueprint_draw_poly_stroke(uint pos3d, BlueprintCache *cache, float z)
{

  if (BLI_listbase_is_empty(&cache->poly)) {
    return;
  }
  int point_count = cache->point_count;
  /* stroke */
  immUniformColor4ubv(cache->color.geom_stroke);
  GPU_depth_test(false);
  GPU_line_width(2.0f);
  if (point_count == 1) {
    BlueprintPolyPoint *p = cache->poly.first;
    GPU_point_size(5);
    immBegin(GPU_PRIM_POINTS, 1);
    immVertex3f(pos3d, p->co[0], p->co[1], 0.0f);
    immEnd();
  }
  if (point_count >= 2) {
    immBegin(GPU_PRIM_LINES, point_count * 2);
    BlueprintPolyPoint *p2;
    LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
      if (((Link *)p)->next) {
        float p3[3] = {p->co[0], p->co[1], z};
        immVertex3fv(pos3d, p3);
        p2 = p->next;
        float p3_2[3] = {p2->co[0], p2->co[1], z};
        immVertex3fv(pos3d, p3_2);
      }
    }

    BlueprintPolyPoint *pfirst = cache->poly.first;
    BlueprintPolyPoint *plast = cache->poly.last;
    float pf[3] = {pfirst->co[0], pfirst->co[1], z};
    float pl[3] = {plast->co[0], plast->co[1], z};
    immVertex3fv(pos3d, pf);
    immVertex3fv(pos3d, pl);

    immEnd();
  }
}

void blueprint_draw_poly_fill(uint pos3d, BlueprintCache *cache, float z)
{

  if (!(cache->flags & BLUEPRINT_ENABLE_GEOMETRY_FILL)) {
    return;
  }
  if (BLI_listbase_is_empty(&cache->poly)) {
    return;
  }
  int point_count = cache->point_count;

  /* fill */
  immUniformColor4ubv(cache->color.geom_fill);
  if (point_count >= 3) {

    immBegin(GPU_PRIM_TRIS, (point_count - 2) * 3);
    for (int i = 0; i < (point_count - 2); i++) {
      immVertex3f(
          pos3d, cache->coords[cache->r_tris[i][0]][0], cache->coords[cache->r_tris[i][0]][1], z);
      immVertex3f(
          pos3d, cache->coords[cache->r_tris[i][1]][0], cache->coords[cache->r_tris[i][1]][1], z);
      immVertex3f(
          pos3d, cache->coords[cache->r_tris[i][2]][0], cache->coords[cache->r_tris[i][2]][1], z);
    }
    immEnd();
  }
}

void blueprint_draw_grid(uint pos3d, BlueprintCache *cache)
{
  GPU_blend(true);
  GPU_line_smooth(true);

  /* backgroud */
  if (cache->flags & BLUEPRINT_ENABLE_GRID_BACKGROUD) {
      if (cache->state != BLUEPRINT_STATE_PLANE) {
        immUniformColor4ubv(cache->color.bg);
        immBegin(GPU_PRIM_TRIS, 6);
        immVertex3f(pos3d, cache->grid_extends_min[0], cache->grid_extends_min[1], 0.0f);
        immVertex3f(pos3d, cache->grid_extends_min[0], cache->grid_extends_max[1], 0.0f);
        immVertex3f(pos3d, cache->grid_extends_max[0], cache->grid_extends_max[1], 0.0f);

        immVertex3f(pos3d, cache->grid_extends_min[0], cache->grid_extends_min[1], 0.0f);
        immVertex3f(pos3d, cache->grid_extends_max[0], cache->grid_extends_min[1], 0.0f);
        immVertex3f(pos3d, cache->grid_extends_max[0], cache->grid_extends_max[1], 0.0f);
        immEnd();
      }
  }

  /* GRID */
  if (cache->flags & BLUEPRINT_ENABLE_GRID) {
      GPU_line_width(0.5f);
      immUniformColor4ubv(cache->color.grid);
      int x_it = (cache->grid_extends_max[0] - cache->grid_extends_min[0]) / cache->gridx_size;
      immBegin(GPU_PRIM_LINES, 2 * x_it + 2);
      for (int i = 0; i < x_it + 1; i++) {
        immVertex3f(pos3d,
                    i * cache->gridx_size + cache->grid_extends_min[0],
                    cache->grid_extends_min[1],
                    0.0f);
        immVertex3f(pos3d,
                    i * cache->gridx_size + cache->grid_extends_min[0],
                    cache->grid_extends_max[1],
                    0.0f);
      }
      immEnd();

      int y_it = (cache->grid_extends_max[1] - cache->grid_extends_min[1]) / cache->gridy_size;
      immBegin(GPU_PRIM_LINES, 2 * y_it + 2);
      for (int i = 0; i < y_it + 1; i++) {
        immVertex3f(pos3d,
                    cache->grid_extends_min[0],
                    i * cache->gridy_size + cache->grid_extends_min[1],
                    0.0f);
        immVertex3f(pos3d,
                    cache->grid_extends_max[0],
                    i * cache->gridy_size + cache->grid_extends_min[1],
                    0.0f);
      }
      immEnd();
  }

  GPU_point_size(4);
  immBegin(GPU_PRIM_POINTS, 1);
  immVertex3f(pos3d, 0.0f, 0.0f, 0.0f);
  immEnd();
}

void blueprint_draw_cursor(uint pos3d, BlueprintCache *cache)
{
  if (cache->state == BLUEPRINT_STATE_DRAW) {
    immUniformColor4ubv(cache->color.cursor);
    GPU_point_size(5);
    immBegin(GPU_PRIM_POINTS, 1);
    immVertex3fv(pos3d, cache->cursor);
    immEnd();
    if (cache->drawtool == BLUEPRINT_DT_POLYLINE) {
      if (cache->point_count > 0) {
        immUniformColor4ubv(cache->color.geom_stroke);
        immBegin(GPU_PRIM_LINES, 2);
        BlueprintPolyPoint *p = (BlueprintPolyPoint *)cache->poly.last;
        immVertex3f(pos3d, p->co[0], p->co[1], 0.0f);
        immVertex3fv(pos3d, cache->cursor);
        immEnd();
      }
    }
  }
}

float blueprint_snap_float(float p, float increment)
{
  return floor((p + (increment * 0.5)) / increment) * increment;
}

void blueprint_add_bb_point(float pmin[2], float pmax[2], float co[2])
{
  if (co[0] < pmin[0]) {
    pmin[0] = co[0];
  }
  if (co[1] < pmin[1]) {
    pmin[1] = co[1];
  }

  if (co[0] > pmax[0]) {
    pmax[0] = co[0];
  }
  if (co[1] > pmax[1]) {
    pmax[1] = co[1];
  }
}

void blueprint_grid_extends_update(BlueprintCache *cache)
{
  float pmin[2], pmax[2];
  float origin[2] = {0.0f, 0.0f};
  pmin[0] = pmin[1] = FLT_MAX;
  pmax[0] = pmax[1] = -FLT_MAX;

  blueprint_add_bb_point(pmin, pmax, cache->cursor);
  blueprint_add_bb_point(pmin, pmax, origin);
  if (cache->point_count > 0) {
  LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
    blueprint_add_bb_point(pmin, pmax, p->co);
  }
  }
  cache->grid_extends_min[0] = pmin[0] - cache->grid_padding * cache->gridx_size;
  cache->grid_extends_min[1] = pmin[1] - cache->grid_padding * cache->gridy_size;
  cache->grid_extends_max[0] = pmax[0] + cache->grid_padding * cache->gridx_size;
  cache->grid_extends_max[1] = pmax[1] + cache->grid_padding * cache->gridy_size;

  cache->grid_extends_min[0] = blueprint_snap_float(cache->grid_extends_min[0],
                                                        cache->gridx_size);
  cache->grid_extends_min[1] = blueprint_snap_float(cache->grid_extends_min[1],
                                                        cache->gridx_size);
  cache->grid_extends_max[0] = blueprint_snap_float(cache->grid_extends_max[0],
                                                        cache->gridy_size);
  cache->grid_extends_max[1] = blueprint_snap_float(cache->grid_extends_max[1],
                                                        cache->gridy_size);
}


void blueprint_triangulation_free(BlueprintCache *cache)
{
  if (cache->coords) {
    MEM_freeN(cache->coords);
  }
  if (cache->r_tris) {
    MEM_freeN(cache->r_tris);
  }
}

void blueprint_triangulation_update(BlueprintCache *cache)
{
  int point_count = cache->point_count;
  if (cache->point_count < 3) {
    return;
  }
  cache->coords = MEM_mallocN(2 * sizeof(float) * point_count, "coords");
  cache->r_tris = MEM_mallocN(3 * sizeof(uint) * (point_count - 2), "tris");

  int i = 0;
  LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
    copy_v2_v2(cache->coords[i], p->co);
    i++;
  }
  BLI_polyfill_calc(cache->coords, cache->point_count, 0, cache->r_tris);
}

void blueprint_draw_geometry(uint pos3d, BlueprintCache *cache)
{


  if (cache->state == BLUEPRINT_STATE_DRAW) {
      blueprint_draw_poly_fill(pos3d, cache, 0.0f);
      blueprint_draw_poly_stroke(pos3d, cache, 0.0f);
  }

  if (cache->state == BLUEPRINT_STATE_EXTRUDE) {
    float z_base = 0.0f;
    if (cache->flags & BLUEPRINT_SYMMETRICAL_EXTRUDE) {
      z_base = -cache->extrudedist;
    }
      blueprint_draw_poly_fill(pos3d, cache, z_base);
      blueprint_draw_poly_stroke(pos3d, cache, z_base);
      blueprint_draw_poly_fill(pos3d, cache, cache->extrudedist);
    blueprint_draw_poly_volume(pos3d, cache, cache->extrudedist);
    blueprint_draw_poly_stroke(pos3d, cache, cache->extrudedist);
  }
  blueprint_triangulation_free(cache);
}

void blueprint_draw(const bContext *UNUSED(C), ARegion *UNUSED(ar), void *arg)
{
  BlueprintCD *cd = arg;
  BlueprintCache *cache = &cd->cache;

  if (is_zero_v3(cache->n)) {
    return;
  }

  GPU_blend(true);
  GPU_line_smooth(true);

  uint pos3d = GPU_vertformat_attr_add(immVertexFormat(), "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  immBindBuiltinProgram(GPU_SHADER_3D_UNIFORM_COLOR);
  GPU_matrix_push();
  GPU_matrix_mul(cache->mat);

  blueprint_grid_extends_update(cache);
  blueprint_draw_grid(pos3d, cache);

  if (cache->state != BLUEPRINT_STATE_PLANE) {
    blueprint_triangulation_update(cache);
    blueprint_draw_geometry(pos3d, cache);
  }

  blueprint_draw_cursor(pos3d, cache);

  GPU_matrix_pop();
  immUnbindProgram();

  GPU_blend(false);
  GPU_line_smooth(false);
}


int blueprint_init(bContext *C, wmOperator *op, BlueprintConfig *config)
{
  BlueprintCD *cd = MEM_callocN(sizeof(BlueprintCD), "CD");
  BlueprintCache *cache = &cd->cache;
  ViewContext vc;

  cache->context = config->context;
  cache->ar = CTX_wm_region(C);
  cache->v3d = CTX_wm_view3d(C);

  cache->state = BLUEPRINT_STATE_PLANE;

  cache->drawtool = RNA_enum_get(op->ptr, "draw_tool");

  cache->plane_offset = RNA_float_get(op->ptr, "plane_offset");

  float grid_size[3];
  RNA_float_get_array(op->ptr, "grid_size", grid_size);
  cache->gridx_size = grid_size[0];
  cache->gridy_size = grid_size[1];
  cache->gridz_size = grid_size[2];

  cache->grid_padding = RNA_int_get(op->ptr, "grid_padding");

  cache->ellipse_nsegments = RNA_int_get(op->ptr, "ellipse_nsegments");
  cache->point_count = 0;

  cache->draw_tool_init = false;

  cache->sf_ob = NULL;

  cache->flags = 0;
  if (RNA_boolean_get(op->ptr, "symmetrical_extrude")) {
    cache->flags |= BLUEPRINT_SYMMETRICAL_EXTRUDE;
  }
  if (RNA_boolean_get(op->ptr, "auto_start_draw_state")) {
    cache->flags |= BLUEPRINT_AUTO_START_DRAW_STATE;
  }
  if (RNA_boolean_get(op->ptr, "auto_enter_draw_state")) {
    cache->flags |= BLUEPRINT_AUTO_ENTER_DRAW_STATE;
  }
  if (RNA_boolean_get(op->ptr, "extrude_on_release")) {
    cache->flags |= BLUEPRINT_EXTRUDE_ON_RELEASE;
  }
  if (RNA_boolean_get(op->ptr, "enable_grid")) {
    cache->flags |= BLUEPRINT_ENABLE_GRID;
  }
  if (RNA_boolean_get(op->ptr, "enable_grid_background")) {
    cache->flags |= BLUEPRINT_ENABLE_GRID_BACKGROUD;
  }
  if (RNA_boolean_get(op->ptr, "enable_geometry_fill")) {
    cache->flags |= BLUEPRINT_ENABLE_GEOMETRY_FILL;
  }

  cache->freehand_step_threshold = 0.05f;


  zero_v3(cache->cursor);
  zero_v3(cache->cursor_global);

  BKE_blueprint_color_init(cache);
  blueprint_grid_extends_update(cache);

  ED_region_tag_redraw(cache->ar);
  ED_view3d_viewcontext_init(C, &vc);
  cd->draw_handle = ED_region_draw_cb_activate(
      cache->ar->type, blueprint_draw, cd, REGION_DRAW_POST_VIEW);

  op->customdata = cd;
  return 1;
}

void blueprint_op_properties(wmOperatorType *ot)
{
  ot->prop = RNA_def_boolean(ot->srna,
                             "extrude_both_sides",
                             false,
                             "Extrude both sides",
                             "Extrudes the sketch symetrically");
  ot->prop = RNA_def_boolean(
      ot->srna, "auto_enter_draw_state", false, "Auto enter draw state", "Enter draw state directly");
  ot->prop = RNA_def_boolean(
      ot->srna, "auto_start_draw_state", false, "Auto start draw state", "Start drawing on entering draw state");
  ot->prop = RNA_def_boolean(
      ot->srna, "extrude_on_release", false, "Extrude on release", "Start extrusion after releasing input");
  ot->prop = RNA_def_boolean(
      ot->srna, "enable_grid", true, "Enable grid", "Enable grid rendering");
  ot->prop = RNA_def_boolean(
      ot->srna, "enable_grid_background", true, "Enable grid background", "Enable grid background rendering");
  ot->prop = RNA_def_boolean(
      ot->srna, "enable_geometry_fill", true, "Enable geometry fill", "Enable geometry fill rendering");
  ot->prop = RNA_def_boolean(
      ot->srna, "grid_snap", false, "Snap to grid", "Snap sketch to the internal grid");
  ot->prop = RNA_def_boolean(
      ot->srna, "floor_grid_snap", false, "Snap to floor", "Snap blueprint to the scene floor grid");
  ot->prop = RNA_def_boolean(
      ot->srna, "vertex_snap", false, "Snap to vertices", "Snap blueprint to the scene geometry vertices");
  ot->prop = RNA_def_boolean(
      ot->srna, "symmetrical_extrude", false, "Symmetrical extrude", "Extrude both sides of the blueprint");
  ot->prop = RNA_def_boolean(
      ot->srna, "show_grid", true, "Show snapping grid", "Show snapping grid");
  ot->prop = RNA_def_int(
      ot->srna, "grid_padding", 5, 2, 100, "Padding", "Padiing of the blueprint content", 2, 100);
  RNA_def_enum(
      ot->srna, "draw_tool", prop_blueprint_draw_tool_types, BLUEPRINT_DT_RECT, "Draw tool", "Draw tool mode");
  ot->prop = RNA_def_int(
      ot->srna, "ellipse_nsegments", 32, 3, 256, "Ellipse segments", "Number of segmetns of the draw ellipse tool", 3, 256);
  ot->prop = RNA_def_float(ot->srna, "plane_offset", 0.0f, -1000.0f, 1000.0f, "Plane offset", "Blueprint plane offset to the surface",-1.0f, 1.0f);
  const float default_grid_size[3] = {0.5f};
  ot->prop = RNA_def_float_vector_xyz(ot->srna, "grid_size", 3, default_grid_size, -100.0f, 100.0f, "Grid size", "Blueprint internal grid size",-1.0f, 1.0f);

  ot->prop = RNA_def_string(
      ot->srna, "modifier", NULL, MAX_NAME, "Modifier", "Name of the modifier to edit");
  RNA_def_property_flag(ot->prop, PROP_HIDDEN);
}

void blueprint_end(bContext *C, wmOperator *op)
{
  BlueprintCD *cd = op->customdata;
  BlueprintCache *cache = &cd->cache;
  BLI_listbase_clear(&cache->poly);
  ED_region_tag_redraw(cache->ar);
  ED_region_draw_cb_exit(cache->ar->type, cd->draw_handle);
  if (op->customdata) {
    MEM_freeN(op->customdata);
  }
}

bool blueprint_is_start_drawing_event(BlueprintCache *cache, const wmEvent *event) {
  bool is_start_event = false;

  if (cache->draw_tool_init) {
    return false;
  }
     is_start_event = (event->type == LEFTMOUSE && event->val == KM_PRESS);
     is_start_event = is_start_event || cache->flags & BLUEPRINT_AUTO_START_DRAW_STATE;
  if (is_start_event) {
    cache->draw_tool_init = true;
  }
  return is_start_event;
}

bool blueprint_is_end_drawing_event(BlueprintCache *cache, const wmEvent *event) {
  if (!cache->draw_tool_init) {
    return false;
  }
  if (cache->drawtool == BLUEPRINT_DT_POLYLINE) {
      return event->type == RETKEY && event->val == KM_PRESS;
  }
  if (cache->drawtool == BLUEPRINT_DT_FREEHAND) {
      return event->type == LEFTMOUSE && event->val == KM_RELEASE;
  }

  /* rect and ellipse drawing modes */
  if (cache->flags & BLUEPRINT_EXTRUDE_ON_RELEASE) {
    return event->type == LEFTMOUSE && event->val == KM_RELEASE;
  } else {
    return event->type == LEFTMOUSE && event->val == KM_PRESS;
  }
}

bool blueprint_is_update_drawing_event(BlueprintCache *cache, const wmEvent *event) {
  if (cache->drawtool == BLUEPRINT_DT_POLYLINE) {
    return event->type == LEFTMOUSE && event->val == KM_PRESS;
  }
  return cache->draw_tool_init;
}

int blueprint_dt_rect_update_step(BlueprintCache *cache, const wmEvent *event) {
      if (blueprint_is_start_drawing_event(cache, event)) {
        for (int i = 0; i < 4; i++) {
          cache->ps[i] = MEM_mallocN(sizeof(BlueprintPolyPoint), "point");
          copy_v2_v2(cache->ps[i]->co, cache->cursor);
          BLI_addtail(&cache->poly, cache->ps[i]);
          cache->point_count++;
        }
          copy_v2_v2(cache->ps[2]->co, cache->cursor);
          cache->ps[1]->co[0] = cache->cursor[0];
          cache->ps[3]->co[1] = cache->cursor[1];
          return BLUEPRINT_STATE_DRAW;
      }

      if (blueprint_is_end_drawing_event(cache, event)) {
        copy_v3_v3(cache->extrude_co, cache->cursor_global);
        return BLUEPRINT_STATE_EXTRUDE;
      }

      if (blueprint_is_update_drawing_event(cache, event)) {
          copy_v2_v2(cache->ps[2]->co, cache->cursor);
          cache->ps[1]->co[0] = cache->cursor[0];
          cache->ps[3]->co[1] = cache->cursor[1];
          return BLUEPRINT_STATE_DRAW;
      }

      return BLUEPRINT_STATE_DRAW;

}
int blueprint_dt_ellipse_update_step(BlueprintCache *cache, const wmEvent *event) {
      if (blueprint_is_start_drawing_event(cache, event)) {
        copy_v2_v2(cache->cursor_start, cache->cursor);
        for (int i = 0; i < cache->ellipse_nsegments; i++) {
          BlueprintPolyPoint *p = MEM_mallocN(sizeof(BlueprintPolyPoint), "point");
          copy_v2_v2(p->co, cache->cursor);
          BLI_addtail(&cache->poly, p);
          cache->point_count++;
        }
      return BLUEPRINT_STATE_DRAW;
      }

      if (blueprint_is_end_drawing_event(cache, event)) {
        copy_v3_v3(cache->extrude_co, cache->cursor_global);
        return BLUEPRINT_STATE_EXTRUDE;
      }

      if (blueprint_is_update_drawing_event(cache, event)) {
          int i = 0; float rad = len_v2v2(cache->cursor_start, cache->cursor);
          LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
            float angle = (float)(2 * M_PI) * ((float)i / (float)cache->ellipse_nsegments);
            p->co[0] = cache->cursor_start[0] + rad * cosf(angle);
            p->co[1] = cache->cursor_start[1] + rad * sinf(angle);
            i++;
          }
      return BLUEPRINT_STATE_DRAW;
      }
      return BLUEPRINT_STATE_DRAW;

}

int blueprint_dt_freehand_update_step(BlueprintCache *cache, const wmEvent *event) {
     if (blueprint_is_start_drawing_event(cache, event)) {
        BlueprintPolyPoint *p = MEM_mallocN(sizeof(BlueprintPolyPoint), "point");
        copy_v2_v2(p->co, cache->cursor);
        BLI_addtail(&cache->poly, p);
        cache->point_count++;
        return BLUEPRINT_STATE_DRAW;
      }

      if (blueprint_is_end_drawing_event(cache, event)) {
        copy_v3_v3(cache->extrude_co, cache->cursor_global);
        return BLUEPRINT_STATE_EXTRUDE;
      }

      if (blueprint_is_update_drawing_event(cache, event)) {
        BlueprintPolyPoint *plast = cache->poly.last;
        if (len_v2v2(plast->co, cache->cursor) > cache->freehand_step_threshold) {
            BlueprintPolyPoint *p = MEM_mallocN(sizeof(BlueprintPolyPoint), "point");
            copy_v2_v2(p->co, cache->cursor);
            BLI_addtail(&cache->poly, p);
            cache->point_count++;
        }
        return BLUEPRINT_STATE_DRAW;
      }
      return BLUEPRINT_STATE_DRAW;
}

int blueprint_dt_polyline_update_step(BlueprintCache *cache, const wmEvent *event) {
      if (blueprint_is_start_drawing_event(cache, event) || blueprint_is_update_drawing_event(cache, event)) {
        BlueprintPolyPoint *p = MEM_mallocN(sizeof(BlueprintPolyPoint), "point");
        copy_v2_v2(p->co, cache->cursor);
        BLI_addtail(&cache->poly, p);
        cache->point_count++;
        return BLUEPRINT_STATE_DRAW;
      }

      if (blueprint_is_end_drawing_event(cache, event)) {
        copy_v3_v3(cache->extrude_co, cache->cursor_global);
        return BLUEPRINT_STATE_EXTRUDE;
      }

      return BLUEPRINT_STATE_DRAW;
}

ModifierData *edit_modifier_property_get_2(wmOperator *op, Object *ob, int type)
{
  char modifier_name[MAX_NAME];
  ModifierData *md;
  RNA_string_get(op->ptr, "modifier", modifier_name);

  md = modifiers_findByName(ob, modifier_name);

  if (md && type != 0 && md->type != type) {
    md = NULL;
  }

  return md;
}

bool EDBM_op_finish_(BMEditMesh *em, BMOperator *bmop, wmOperator *op, const bool do_report)
{
  const char *errmsg;

  BMO_op_finish(em->bm, bmop);

  if (BMO_error_get(em->bm, &errmsg, NULL)) {
    BMEditMesh *emcopy = em->emcopy;

    if (do_report) {
      BKE_report(op->reports, RPT_ERROR, errmsg);
    }

    //EDBM_mesh_free(em);
    *em = *emcopy;

    MEM_freeN(emcopy);
    em->emcopyusers = 0;
    em->emcopy = NULL;

    /* when copying, tessellation isn't to for faster copying,
     * but means we need to re-tessellate here */
    if (em->looptris == NULL) {
      BKE_editmesh_tessface_calc(em);
    }

    if (em->ob) {
      DEG_id_tag_update(&((Mesh *)em->ob->data)->id, ID_RECALC_COPY_ON_WRITE);
    }

    return false;
  }
  else {
    em->emcopyusers--;
    if (em->emcopyusers < 0) {
      printf("warning: em->emcopyusers was less than zero.\n");
    }

    if (em->emcopyusers <= 0) {
      BKE_editmesh_free(em->emcopy);
      MEM_freeN(em->emcopy);
      em->emcopy = NULL;
    }

    return true;
  }
}
bool EDBM_op_call_f(BMEditMesh *em, wmOperator *op, const char *fmt, ...)
{
  BMesh *bm = em->bm;
  BMOperator bmop;
  va_list list;

  va_start(list, fmt);

  if (!BMO_op_vinitf(bm, &bmop, BMO_FLAG_DEFAULTS, fmt, list)) {
    BKE_reportf(op->reports, RPT_ERROR, "Parse error in %s", __func__);
    va_end(list);
    return false;
  }

  if (!em->emcopy) {
    em->emcopy = BKE_editmesh_copy(em);
  }
  em->emcopyusers++;

  BMO_op_exec(bm, &bmop);

  va_end(list);
  return EDBM_op_finish_(em, &bmop, op, true);
}
int blueprint_update_step(bContext *C,
                              wmOperator *op,
                              const wmEvent *event,
                              BlueprintGeometryData *gdata)
{
  BlueprintCD *cd = op->customdata;
  BlueprintCache *cache = &cd->cache;

  Main *bmain = CTX_data_main(C);
Scene *scene = CTX_data_scene(C);
Depsgraph *depsgraph = CTX_data_depsgraph(C);

  if (event->type == ESCKEY && event->val == KM_PRESS) {
    blueprint_end(C, op);
    return OPERATOR_FINISHED;
  }

  if (event->type == RIGHTMOUSE && event->val == KM_PRESS) {
    blueprint_end(C, op);
    return OPERATOR_FINISHED;
  }

  if (cache->state == BLUEPRINT_STATE_PLANE) {
    zero_v3(cache->cursor);

    normalize_v3(gdata->n);
    copy_v3_v3(cache->n, gdata->n);
    copy_v3_v3(cache->co, gdata->co);

    /* plane offset */
    float plane_offset[3];
    copy_v3_v3(plane_offset, gdata->n);
    mul_v3_fl(plane_offset, cache->plane_offset);
    add_v3_v3(cache->co, plane_offset);


    /* blueprint matrix setup */
    float z_axis[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    float x_axis[4] = {1.0f, 0.0f, 0.0f, 0.0f};

    float bp_mat[4][4];
    float bp_z[3];
    float bp_y[3];
    float bp_x[3];

    plane_from_point_normal_v3(cache->plane, cache->co, cache->n);
    project_plane_normalized_v3_v3v3(bp_x, x_axis, cache->n);
    if (is_zero_v3(bp_x)) {
      copy_v3_v3(bp_x, z_axis);
    }
    copy_v3_v3(bp_z, cache->n);
    cross_v3_v3v3(bp_y, bp_z, bp_x);
    unit_m4(bp_mat);
    copy_v3_v3(bp_mat[0], bp_x);
    copy_v3_v3(bp_mat[1], bp_y);
    copy_v3_v3(bp_mat[2], bp_z);
    copy_v3_v3(bp_mat[3], cache->co);
    copy_m4_m4(cache->mat, bp_mat);
    invert_m4_m4(cache->mat_inv, cache->mat);
    copy_v3_v3(cache->origin, cache->co);


    if (gdata->sf_object) {
        cache->sf_ob = gdata->sf_object;
    }

    if (event->type == LEFTMOUSE && event->val == KM_PRESS) {
        if (false && cache->sf_ob != NULL) {
            ED_object_modifier_add(op->reports, bmain, scene, cache->sf_ob, "bool_op", eModifierType_Boolean);
                DEG_id_tag_update(&cache->sf_ob->id, ID_RECALC_GEOMETRY | ID_RECALC_COPY_ON_WRITE);
                WM_event_add_notifier(C, NC_OBJECT | ND_MODIFIER, cache->sf_ob);
        }
      cache->state = BLUEPRINT_STATE_DRAW;
      ED_region_tag_redraw(cache->ar);
      return OPERATOR_RUNNING_MODAL;
    }

    if (cache->flags & BLUEPRINT_AUTO_ENTER_DRAW_STATE) {
      cache->state = BLUEPRINT_STATE_DRAW;
      ED_region_tag_redraw(cache->ar);
      return OPERATOR_RUNNING_MODAL;
    }
  }

  if (cache->state == BLUEPRINT_STATE_DRAW) {

    zero_v3(cache->cursor);
    const float mval_fl[2] = {event->mval[0], event->mval[1]};
    ED_view3d_win_to_3d_on_plane(cache->ar, cache->plane, mval_fl, false, cache->cursor);
    float cursor_v4[4];
    copy_v3_v3(cursor_v4, cache->cursor);
    copy_v3_v3(cache->cursor_global, cache->cursor);
    cursor_v4[3] = 1.0f;
    mul_v4_m4v4(cursor_v4, cache->mat_inv, cursor_v4);
    copy_v3_v3(cache->cursor, cursor_v4);

    if (RNA_boolean_get(op->ptr, "grid_snap") && cache->drawtool != BLUEPRINT_DT_FREEHAND) {
      cache->cursor[0] = blueprint_snap_float(cache->cursor[0], cache->gridx_size);
      cache->cursor[1] = blueprint_snap_float(cache->cursor[1], cache->gridy_size);
    }

    copy_m4_m4(cache->cursor_mat, cache->mat);
    cache->cursor_mat[3][0] = cache->cursor[0];
    cache->cursor_mat[3][1] = cache->cursor[1];

    if (cache->drawtool == BLUEPRINT_DT_FREEHAND) {
      cache->state = blueprint_dt_freehand_update_step(cache, event);
    }
    if (cache->drawtool == BLUEPRINT_DT_POLYLINE) {
      cache->state = blueprint_dt_polyline_update_step(cache, event);
    }
    if (cache->drawtool == BLUEPRINT_DT_RECT) {
      cache->state = blueprint_dt_rect_update_step(cache, event);
    }
    if (cache->drawtool == BLUEPRINT_DT_ELLIPSE) {
      cache->state = blueprint_dt_ellipse_update_step(cache, event);
    }

      ED_region_tag_redraw(cache->ar);
      return OPERATOR_RUNNING_MODAL;

  }

  if (cache->state == BLUEPRINT_STATE_EXTRUDE) {
    float proj_mouse[3];
    const float mval_fl[2] = {event->mval[0], event->mval[1]};
    ED_view3d_win_to_3d(cache->v3d, cache->ar, cache->cursor_global, mval_fl, proj_mouse);
    cache->extrudedist = len_v3v3(cache->extrude_co, proj_mouse);
    if (plane_point_side_v3(cache->plane, proj_mouse) <= 0.0f) {
      cache->extrudedist = -cache->extrudedist;
    }

    if (RNA_boolean_get(op->ptr, "grid_snap")) {
      cache->extrudedist = blueprint_snap_float(cache->extrudedist, cache->gridz_size);
    }

    if (cache->context == BLUEPRINT_CTX_GEOMETRY) {
      if (event->type == 1 && event->val == 1) {

        blueprint_triangulation_update(cache);
        float ob_loc[3];
        float ob_rot[3];
        mat4_to_eul(ob_rot, cache->mat);
        copy_v3_v3(ob_loc, cache->origin);

        int point_count = BLI_listbase_count(&cache->poly);
        int totvert = point_count * 2;
        int tottri = 0;
        if (point_count == 4) {
           tottri = (point_count * 1) + 2;
        } else {
           tottri = (point_count * 1) + ((point_count - 2) * 2);
        }
        Mesh *newMesh = BKE_mesh_new_nomain(totvert, 0, tottri, 0, 0);

        int i = 0;
        float z_base = 0.0f;
        if (cache->flags & BLUEPRINT_SYMMETRICAL_EXTRUDE) {
          z_base = -cache->extrudedist;
        }
        LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
          float vco[3] = {p->co[0], p->co[1], z_base};
          copy_v3_v3(newMesh->mvert[i].co, vco);
          i++;
        }
        LISTBASE_FOREACH (BlueprintPolyPoint *, p, &cache->poly) {
          float vco[3] = {p->co[0], p->co[1], cache->extrudedist};
          copy_v3_v3(newMesh->mvert[i].co, vco);
          i++;
        }

        int face_it = 0;

        if (point_count == 4) {
              newMesh->mface[face_it].v1 = 0;
              newMesh->mface[face_it].v2 = 1;
              newMesh->mface[face_it].v3 = 2;
              newMesh->mface[face_it].v4 = 3;
              face_it++;
              newMesh->mface[face_it].v1 = 7;
              newMesh->mface[face_it].v2 = 6;
              newMesh->mface[face_it].v3 = 5;
              newMesh->mface[face_it].v4 = 4;
              face_it++;
        }
        else {
            for (i = 0; i < point_count - 2; i++) {
              uint *t_index = cache->r_tris[i];
              newMesh->mface[face_it].v3 = t_index[2];
              newMesh->mface[face_it].v2 = t_index[1];
              newMesh->mface[face_it].v1 = t_index[0];
              face_it++;
            }

            for (i = 0; i < point_count - 2; i++) {
              uint *t_index = cache->r_tris[i];
              newMesh->mface[face_it].v3 = t_index[0] + point_count;
              newMesh->mface[face_it].v2 = t_index[1] + point_count;
              newMesh->mface[face_it].v1 = t_index[2] + point_count;
              face_it++;
            }
        }

        for (i = 0; i < point_count; i++) {
          int current_index = i;
          int next_index;
          next_index = current_index + 1;
          if (next_index >= point_count) {
            next_index = 0;
          }
          newMesh->mface[face_it].v1 = next_index + point_count;
          newMesh->mface[face_it].v2 = next_index;
          newMesh->mface[face_it].v3 = current_index;
          newMesh->mface[face_it].v4 = current_index + point_count;
          face_it++;
        }

        blueprint_triangulation_free(cache);

        BKE_mesh_calc_edges_tessface(newMesh);
        BKE_mesh_convert_mfaces_to_mpolys(newMesh);
        BKE_mesh_calc_normals(newMesh);
        BKE_mesh_strip_loose_edges(newMesh);
        BKE_mesh_strip_loose_polysloops(newMesh);
        BKE_mesh_strip_loose_faces(newMesh);
        ushort local_view_bits = 0;
        View3D *v3d = cache->v3d;
        if (v3d && v3d->localvd) {
          local_view_bits = v3d->local_view_uuid;
        }
        Object *newob = ED_object_add_type(
            C, OB_MESH, NULL, ob_loc, ob_rot, false, local_view_bits);
        BKE_mesh_nomain_to_mesh(newMesh, newob->data, newob, &CD_MASK_EVERYTHING, true);
        BKE_mesh_calc_normals(newob->data);

        if (false) {
        const BMAllocTemplate allocsize = BMALLOC_TEMPLATE_FROM_ME(newMesh);
        BMesh *bm;
        bm = BM_mesh_create(&allocsize,
                            &((struct BMeshCreateParams){
                                .use_toolflags = false,
                            }));

        BM_mesh_bm_from_me(bm,
                           newMesh,
                           (&(struct BMeshFromMeshParams){
                               .calc_face_normal = true,
                           }));

            BMEditMesh *em = BKE_editmesh_create(bm, false);
            BMFace *f;
            BMIter iter;
            BM_ITER_MESH (f, &iter, bm, BM_FACES_OF_MESH) {
                BM_elem_flag_set(f, BM_ELEM_SELECT, true);
            }
            EDBM_op_call_f(em, op, "recalc_face_normals faces=%hf", BM_ELEM_SELECT);
            BKE_editmesh_free_derivedmesh(em);
        }

        bool bool_op_ok = false;


        if (false && cache->sf_ob != NULL) {
            //ED_object_modifier_add(op->reports, bmain, scene, cache->sf_ob, "bool_op", eModifierType_Boolean);
            BooleanModifierData *bmd = (BooleanModifierData *)modifiers_findByName(cache->sf_ob, "bool_op");
            if (bmd) {
                bmd->object = newob;
                bmd->operation = eBooleanModifierOp_Difference;
                DEG_id_tag_update(&cache->sf_ob->id, ID_RECALC_GEOMETRY | ID_RECALC_COPY_ON_WRITE);
                WM_event_add_notifier(C, NC_OBJECT | ND_MODIFIER, cache->sf_ob);
              bool_op_ok = true;
            }
            cache->new_ob = newob;
            WM_event_add_notifier(C, NC_OBJECT | ND_MODIFIER, newob);
            BKE_mesh_batch_cache_dirty_tag(newob->data, BKE_MESH_BATCH_DIRTY_ALL);
            DEG_relations_tag_update(bmain);
            DEG_id_tag_update(&newob->id, ID_RECALC_GEOMETRY);
            WM_event_add_notifier(C, NC_GEOM | ND_DATA, newob->data);
            cache->state = BLUEPRINT_STATE_BOOLEAN;
            return OPERATOR_RUNNING_MODAL;
        }

        if (!bool_op_ok) {
            WM_event_add_notifier(C, NC_OBJECT | ND_MODIFIER, newob);
            BKE_mesh_batch_cache_dirty_tag(newob->data, BKE_MESH_BATCH_DIRTY_ALL);
            DEG_relations_tag_update(bmain);
            DEG_id_tag_update(&newob->id, ID_RECALC_GEOMETRY);
            WM_event_add_notifier(C, NC_GEOM | ND_DATA, newob->data);
            blueprint_end(C, op);

            return OPERATOR_FINISHED;
        }

      }
    }
  }

  if (cache->state == BLUEPRINT_STATE_BOOLEAN) {
         BooleanModifierData *bmd = (BooleanModifierData *)modifiers_findByName(cache->sf_ob, "bool_op");
         ED_object_modifier_apply(bmain, op->reports, depsgraph, scene, cache->sf_ob, &bmd->modifier, MODIFIER_APPLY_DATA);
        DEG_id_tag_update(&cache->sf_ob->id, ID_RECALC_GEOMETRY | ID_RECALC_COPY_ON_WRITE);
        WM_event_add_notifier(C, NC_OBJECT | ND_MODIFIER, cache->sf_ob);
         ED_object_base_free_and_unlink(bmain, scene, cache->new_ob);
            DEG_relations_tag_update(bmain);

  }


  ED_region_tag_redraw(cache->ar);
  return OPERATOR_RUNNING_MODAL;
}

static bool object_mode_blueprint_set_poll(bContext *C)
{
  return true;
}

static void scene_ray_cast(Scene *scene,
                           Main *bmain,
                           ViewLayer *view_layer,
                           float origin[3],
                           float direction[3],
                           float ray_dist,
                           bool *r_success,
                           float r_location[3],
                           float r_normal[3],
                           int *r_index,
                           Object **r_ob,
                           float r_obmat[16],
float mval[2],
bContext *C,
bool vertex_snap)
{
  normalize_v3(direction);

  Depsgraph *depsgraph = BKE_scene_get_depsgraph(scene, view_layer, true);
  ARegion *ar = CTX_wm_region(C);
  View3D *v3d = CTX_wm_view3d(C);
  SnapObjectContext *sctx = ED_transform_snap_object_context_create(bmain, scene, depsgraph, 0);

  float global_normal[3];
  float global_loc[3];
  float snap_normal[3];
  float snap_loc[3];

  bool ret = ED_transform_snap_object_project_ray_ex(sctx,
                                                     &(const struct SnapObjectParams){
                                                         .snap_select = SNAP_ALL,
                                                     },
                                                     origin,
                                                     direction,
                                                     &ray_dist,
                                                     global_loc,
                                                     global_normal,
                                                     r_index,
                                                     r_ob,
                                                     (float(*)[4])r_obmat);
  ED_transform_snap_object_context_destroy(sctx);
  sctx = ED_transform_snap_object_context_create_view3d(bmain, scene, depsgraph, 0, ar, v3d);
  bool ret_snnaped = ED_transform_snap_object_project_view3d_ex(sctx,
                                                        SCE_SNAP_MODE_VERTEX,
                                                     &(const struct SnapObjectParams){
                                                         .snap_select = SNAP_ALL,
                                                     },
                                                     mval,
                                                     &ray_dist,
                                                     snap_loc,
                                                     snap_normal,
                                                     r_index,
                                                     r_ob,
                                                     (float(*)[4])r_obmat);
  ED_transform_snap_object_context_destroy(sctx);

  if (ret_snnaped && vertex_snap) {
    copy_v3_v3(r_location, snap_loc);
    copy_v3_v3(r_normal, global_normal);
  } else {
    copy_v3_v3(r_location, global_loc);
    copy_v3_v3(r_normal, global_normal);
  }


  if (r_ob != NULL && *r_ob != NULL) {
    *r_ob = DEG_get_original_object(*r_ob);
  }

  if (ret || ret_snnaped) {
    *r_success = true;
  }
  else {
    *r_success = false;

    unit_m4((float(*)[4])r_obmat);
    zero_v3(r_location);
    zero_v3(r_normal);
  }
}


static int object_blueprint_modal(bContext *C, wmOperator *op, const wmEvent *event)
{
  float mouse[2];
  mouse[0] = event->mval[0];
  mouse[1] = event->mval[1];

  float gs_normal[3] = {0.0f};
  float gs_co[3] = {0.0f};

  BlueprintGeometryData gdata;

  /* scene operator */
  ARegion *ar = CTX_wm_region(C);
  float ray_co[3], ray_no[3];
  ED_view3d_win_to_origin(ar, mouse, ray_co);
  ED_view3d_win_to_vector(ar, mouse, ray_no);
  bool success;
  int index;
  float obmat[4][4];
  Object *r_ob;
  bool vertex_snap = RNA_boolean_get(op->ptr, "vertex_snap");
  scene_ray_cast(CTX_data_scene(C),
                 CTX_data_main(C),
                 CTX_data_view_layer(C),
                 ray_co,
                 ray_no,
                 100,
                 &success,
                 gs_co,
                 gs_normal,
                 &index,
                 &gdata.sf_object,
                 obmat,
                 mouse,
                 C,
                 vertex_snap);




  if (!success) {
    float ground_plane[4];
    float ground_origin[3] = {0.0f, 0.0f, 0.0f};
    float ground_normal[3] = {0.0f, 0.0f, 1.0f};

    plane_from_point_normal_v3(ground_plane, ground_origin, ground_normal);
    ED_view3d_win_to_3d_on_plane(ar, ground_plane, mouse, false, gs_co);
    copy_v3_v3(gs_normal, ground_normal);
  }

    if (RNA_boolean_get(op->ptr,"floor_grid_snap")) {
      gs_co[0] = blueprint_snap_float(gs_co[0], 1.0f);
      gs_co[1] = blueprint_snap_float(gs_co[1], 1.0f);
      gs_co[2] = blueprint_snap_float(gs_co[2], 1.0f);
    }
  copy_v3_v3(gdata.n, gs_normal);
  copy_v3_v3(gdata.co, gs_co);
  int ret = blueprint_update_step(C, op, event, &gdata);
  return ret;
}

static int object_blueprint_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  BlueprintConfig config;

  config.context = BLUEPRINT_CTX_GEOMETRY;

  if (blueprint_init(C, op, &config)) {
    WM_event_add_modal_handler(C, op);
    return OPERATOR_RUNNING_MODAL;
  }
  else {
    return OPERATOR_CANCELLED;
  }
}

void OBJECT_OT_blueprint(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Blueprint";
  ot->idname = "OBJECT_OT_blueprint";
  ot->description = "Blueprint operator";

  /* api callbacks */
  ot->invoke =object_blueprint_invoke;
  ot->modal = object_blueprint_modal;
  ot->poll = object_mode_blueprint_set_poll;

  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;
  blueprint_op_properties(ot);
}
