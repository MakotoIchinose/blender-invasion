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
 * The Original Code is Copyright (C) 2015, Blender Foundation
 * This is a new part of Blender
 * Brush based operators for editing Grease Pencil strokes
 */

/** \file
 * \ingroup edgpencil
 */

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_math.h"

#include "BLT_translation.h"

#include "DNA_brush_types.h"
#include "DNA_gpencil_types.h"

#include "BKE_colortools.h"
#include "BKE_context.h"
#include "BKE_gpencil.h"
#include "BKE_gpencil_modifier.h"
#include "BKE_material.h"
#include "BKE_report.h"

#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "UI_view2d.h"

#include "ED_gpencil.h"
#include "ED_screen.h"
#include "ED_view3d.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "gpencil_intern.h"

/* ************************************************ */
/* General Brush Editing Context */

/* Context for brush operators */
typedef struct tGP_BrushTintData {
  /* Current editor/region/etc. */
  /* NOTE: This stuff is mainly needed to handle 3D view projection stuff... */
  struct Main *bmain;
  Scene *scene;
  Object *object;

  ScrArea *sa;
  ARegion *ar;

  /* Current GPencil datablock */
  bGPdata *gpd;

  Brush *brush;
  eGP_Sculpt_Flag flag;

  /* Space Conversion Data */
  GP_SpaceConversion gsc;

  /* Is the brush currently painting? */
  bool is_painting;
  bool is_transformed;

  /* Start of new tint */
  bool first;

  /* Is multiframe editing enabled, and are we using falloff for that? */
  bool is_multiframe;
  bool use_multiframe_falloff;

  /* Brush Runtime Data: */
  /* - position and pressure
   * - the *_prev variants are the previous values
   */
  float mval[2], mval_prev[2];
  float pressure, pressure_prev;

  /* - effect vector */
  float dvec[3];

  /* rotation for evaluated data */
  float rot_eval;

  /* - multiframe falloff factor */
  float mf_falloff;

  /* brush geometry (bounding box) */
  rcti brush_rect;

  /* Object invert matrix */
  float inv_mat[4][4];

} tGP_BrushTintData;

/* Callback for performing some brush operation on a single point */
typedef bool (*GP_TintApplyCb)(tGP_BrushTintData *gso,
                               bGPDstroke *gps,
                               float rotation,
                               int pt_index,
                               const int radius,
                               const int co[2]);

/* Brush Operations ------------------------------- */

/* Invert behavior of brush? */
static bool brush_invert_check(tGP_BrushTintData *gso)
{
  /* The basic setting is no inverted */
  bool invert = false;

  /* During runtime, the user can hold down the Ctrl key to invert the basic behavior */
  if (gso->flag & GP_SCULPT_FLAG_INVERT) {
    invert ^= true;
  }

  return invert;
}

/* Compute strength of effect. */
static float brush_influence_calc(tGP_BrushTintData *gso, const int radius, const int co[2])
{
  Brush *brush = gso->brush;
  float influence = brush->size;

  /* use pressure? */
  if (brush->gpencil_settings->flag & GP_BRUSH_USE_PRESSURE) {
    influence *= gso->pressure;
  }

  /* distance fading */
  int mval_i[2];
  round_v2i_v2fl(mval_i, gso->mval);
  float distance = (float)len_v2v2_int(mval_i, co);
  float fac;

  CLAMP(distance, 0.0f, (float)radius);
  fac = 1.0f - (distance / (float)radius);

  influence *= fac;

  /* apply multiframe falloff */
  influence *= gso->mf_falloff;

  /* return influence */
  return influence;
}

/* Compute effect vector for directional brushes. */
static void brush_grab_calc_dvec(tGP_BrushTintData *gso)
{
  /* Convert mouse-movements to movement vector */
  RegionView3D *rv3d = gso->ar->regiondata;
  float *rvec = gso->object->loc;
  float zfac = ED_view3d_calc_zfac(rv3d, rvec, NULL);

  float mval_f[2];

  /* convert from 2D screenspace to 3D... */
  mval_f[0] = (float)(gso->mval[0] - gso->mval_prev[0]);
  mval_f[1] = (float)(gso->mval[1] - gso->mval_prev[1]);

  /* apply evaluated data transformation */
  if (gso->rot_eval != 0.0f) {
    const float cval = cos(gso->rot_eval);
    const float sval = sin(gso->rot_eval);
    float r[2];
    r[0] = (mval_f[0] * cval) - (mval_f[1] * sval);
    r[1] = (mval_f[0] * sval) + (mval_f[1] * cval);
    copy_v2_v2(mval_f, r);
  }

  ED_view3d_win_to_delta(gso->ar, mval_f, gso->dvec, zfac);
}

/* ************************************************ */
/* Brush Callbacks
/* This section defines the callbacks used by each brush to perform their magic.
 * These are called on each point within the brush's radius. */

static bool brush_fill_asspply(tGP_BrushTintData *gso,
                               bGPDstroke *gps,
                               const int radius,
                               const int co[2])
{
  ToolSettings *ts = gso->scene->toolsettings;
  Brush *brush = gso->brush;
  float inf = gso->pressure;

  float alpha_fill = gps->mix_color_fill[3];
  if (brush_invert_check(gso)) {
    alpha_fill -= inf;
  }
  else {
    alpha_fill += inf;
    /* Limit max strength target. */
    CLAMP_MAX(alpha_fill, brush->gpencil_settings->draw_strength);
  }

  /* Apply color to Fill area. */
  if (GPENCIL_TINT_VERTEX_COLOR_FILL(ts)) {
    CLAMP(alpha_fill, 0.0f, 1.0f);
    copy_v3_v3(gps->mix_color_fill, brush->rgb);
    gps->mix_color_fill[3] = alpha_fill;
  }

  return true;
}

/* Tint Brush */
static bool brush_tint_apply(tGP_BrushTintData *gso,
                             bGPDstroke *gps,
                             float UNUSED(rot_eval),
                             int pt_index,
                             const int radius,
                             const int co[2])
{
  ToolSettings *ts = gso->scene->toolsettings;
  Brush *brush = gso->brush;

  /* Attenuate factor to get a smoother tinting. */
  float inf = (brush_influence_calc(gso, radius, co) * brush->gpencil_settings->draw_strength) /
              100.0f;
  float inf_fill = (gso->pressure * brush->gpencil_settings->draw_strength) / 1000.0f;

  bGPDspoint *pt = &gps->points[pt_index];

  float alpha = pt->mix_color[3];
  float alpha_fill = gps->mix_color_fill[3];

  if (brush_invert_check(gso)) {
    alpha -= inf;
    alpha_fill -= inf_fill;
  }
  else {
    alpha += inf;
    alpha_fill += inf_fill;
  }

  /* Apply color to Stroke point. */
  if (GPENCIL_TINT_VERTEX_COLOR_STROKE(ts)) {
    CLAMP(alpha, 0.0f, 1.0f);
    interp_v3_v3v3(pt->mix_color, pt->mix_color, brush->rgb, inf);
    pt->mix_color[3] = alpha;
  }

  /* Apply color to Fill area (all with same color and factor). */
  if (GPENCIL_TINT_VERTEX_COLOR_FILL(ts)) {
    CLAMP(alpha_fill, 0.0f, 1.0f);
    copy_v3_v3(gps->mix_color_fill, brush->rgb);
    gps->mix_color_fill[3] = alpha_fill;
  }

  return true;
}

/* ************************************************ */
/* Header Info */
static void gptint_brush_header_set(bContext *C, tGP_BrushTintData *UNUSED(gso))
{
  ED_workspace_status_text(C,
                           TIP_("GPencil Tint: LMB to paint | RMB/Escape to Exit"
                                " | Ctrl to Invert Action"));
}

/* ************************************************ */
/* Grease Pencil Tinting Operator */

/* Init/Exit ----------------------------------------------- */

static bool gptint_brush_init(bContext *C, wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  ToolSettings *ts = CTX_data_tool_settings(C);
  Object *ob = CTX_data_active_object(C);
  Paint *paint = &ts->gp_paint->paint;

  /* set the brush using the tool */
  tGP_BrushTintData *gso;

  /* setup operator data */
  gso = MEM_callocN(sizeof(tGP_BrushTintData), "tGP_BrushTintData");
  op->customdata = gso;

  gso->bmain = CTX_data_main(C);
  gso->brush = paint->brush;

  gso->is_painting = false;
  gso->first = true;

  gso->gpd = ED_gpencil_data_get_active(C);
  gso->scene = scene;
  gso->object = ob;
  if (ob) {
    invert_m4_m4(gso->inv_mat, ob->obmat);
    /* Check if some modifier can transform the stroke. */
    gso->is_transformed = BKE_gpencil_has_transform_modifiers(ob);
  }
  else {
    unit_m4(gso->inv_mat);
    gso->is_transformed = false;
  }

  gso->sa = CTX_wm_area(C);
  gso->ar = CTX_wm_region(C);

  /* multiframe settings */
  gso->is_multiframe = (bool)GPENCIL_MULTIEDIT_SESSIONS_ON(gso->gpd);
  gso->use_multiframe_falloff = (ts->gp_sculpt.flag & GP_SCULPT_SETT_FLAG_FRAME_FALLOFF) != 0;

  /* Init multi-edit falloff curve data before doing anything,
   * so we won't have to do it again later. */
  if (gso->is_multiframe) {
    BKE_curvemapping_initialize(ts->gp_sculpt.cur_falloff);
  }

  /* setup space conversions */
  gp_point_conversion_init(C, &gso->gsc);

  /* update header */
  gptint_brush_header_set(C, gso);

  /* setup cursor drawing */
  ED_gpencil_toggle_brush_cursor(C, true, NULL);
  return true;
}

static void gptint_brush_exit(bContext *C, wmOperator *op)
{
  tGP_BrushTintData *gso = op->customdata;

  /* disable cursor and headerprints */
  ED_workspace_status_text(C, NULL);
  ED_gpencil_toggle_brush_cursor(C, false, NULL);

  /* disable temp invert flag */
  gso->brush->flag &= ~GP_SCULPT_FLAG_TMP_INVERT;

  /* free operator data */
  MEM_freeN(gso);
  op->customdata = NULL;
}

/* poll callback for stroke sculpting operator(s) */
static bool gptint_brush_poll(bContext *C)
{
  /* NOTE: this is a bit slower, but is the most accurate... */
  return CTX_DATA_COUNT(C, editable_gpencil_strokes) != 0;
}

/* Apply ----------------------------------------------- */

/* Get angle of the segment relative to the original segment before any transformation
 * For strokes with one point only this is impossible to calculate because there isn't a
 * valid reference point.
 */
static float gptint_rotation_eval_get(tGP_BrushTintData *gso,
                                      bGPDstroke *gps_eval,
                                      bGPDspoint *pt_eval,
                                      int idx_eval)
{
  /* If multiframe or no modifiers, return 0. */
  if ((GPENCIL_MULTIEDIT_SESSIONS_ON(gso->gpd)) || (!gso->is_transformed)) {
    return 0.0f;
  }

  GP_SpaceConversion *gsc = &gso->gsc;
  bGPDstroke *gps_orig = gps_eval->runtime.gps_orig;
  bGPDspoint *pt_orig = &gps_orig->points[pt_eval->runtime.idx_orig];
  bGPDspoint *pt_prev_eval = NULL;
  bGPDspoint *pt_orig_prev = NULL;
  if (idx_eval != 0) {
    pt_prev_eval = &gps_eval->points[idx_eval - 1];
  }
  else {
    if (gps_eval->totpoints > 1) {
      pt_prev_eval = &gps_eval->points[idx_eval + 1];
    }
    else {
      return 0.0f;
    }
  }

  if (pt_eval->runtime.idx_orig != 0) {
    pt_orig_prev = &gps_orig->points[pt_eval->runtime.idx_orig - 1];
  }
  else {
    if (gps_orig->totpoints > 1) {
      pt_orig_prev = &gps_orig->points[pt_eval->runtime.idx_orig + 1];
    }
    else {
      return 0.0f;
    }
  }

  /* create 2D vectors of the stroke segments */
  float v_orig_a[2], v_orig_b[2], v_eval_a[2], v_eval_b[2];

  gp_point_3d_to_xy(gsc, GP_STROKE_3DSPACE, &pt_orig->x, v_orig_a);
  gp_point_3d_to_xy(gsc, GP_STROKE_3DSPACE, &pt_orig_prev->x, v_orig_b);
  sub_v2_v2(v_orig_a, v_orig_b);

  gp_point_3d_to_xy(gsc, GP_STROKE_3DSPACE, &pt_eval->x, v_eval_a);
  gp_point_3d_to_xy(gsc, GP_STROKE_3DSPACE, &pt_prev_eval->x, v_eval_b);
  sub_v2_v2(v_eval_a, v_eval_b);

  return angle_v2v2(v_orig_a, v_eval_a);
}

/* Apply brush operation to points in this stroke */
static bool gptint_brush_do_stroke(tGP_BrushTintData *gso,
                                   bGPDstroke *gps,
                                   const float diff_mat[4][4],
                                   GP_TintApplyCb apply)
{
  GP_SpaceConversion *gsc = &gso->gsc;
  rcti *rect = &gso->brush_rect;
  Brush *brush = gso->brush;
  const int radius = (brush->flag & GP_SCULPT_FLAG_PRESSURE_RADIUS) ?
                         gso->brush->size * gso->pressure :
                         gso->brush->size;
  const bool is_multiedit = (bool)GPENCIL_MULTIEDIT_SESSIONS_ON(gso->gpd);
  bGPDstroke *gps_active = (!is_multiedit) ? gps->runtime.gps_orig : gps;
  bGPDspoint *pt_active = NULL;

  bGPDspoint *pt1, *pt2;
  bGPDspoint *pt = NULL;
  int pc1[2] = {0};
  int pc2[2] = {0};
  int i;
  int index;
  bool include_last = false;
  bool changed = false;
  float rot_eval = 0.0f;
  if (gps->totpoints == 1) {
    bGPDspoint pt_temp;
    pt = &gps->points[0];
    gp_point_to_parent_space(gps->points, diff_mat, &pt_temp);
    gp_point_to_xy(gsc, gps, &pt_temp, &pc1[0], &pc1[1]);

    pt_active = (!is_multiedit) ? pt->runtime.pt_orig : pt;
    /* do boundbox check first */
    if ((!ELEM(V2D_IS_CLIPPED, pc1[0], pc1[1])) && BLI_rcti_isect_pt(rect, pc1[0], pc1[1])) {
      /* only check if point is inside */
      int mval_i[2];
      round_v2i_v2fl(mval_i, gso->mval);
      if (len_v2v2_int(mval_i, pc1) <= radius) {
        /* apply operation to this point */
        if (pt_active != NULL) {
          rot_eval = gptint_rotation_eval_get(gso, gps, pt, 0);
          changed = apply(gso, gps_active, rot_eval, 0, radius, pc1);
        }
      }
    }
  }
  else {
    /* Loop over the points in the stroke, checking for intersections
     * - an intersection means that we touched the stroke
     */
    for (i = 0; (i + 1) < gps->totpoints; i++) {
      /* Get points to work with */
      pt1 = gps->points + i;
      pt2 = gps->points + i + 1;

      bGPDspoint npt;
      gp_point_to_parent_space(pt1, diff_mat, &npt);
      gp_point_to_xy(gsc, gps, &npt, &pc1[0], &pc1[1]);

      gp_point_to_parent_space(pt2, diff_mat, &npt);
      gp_point_to_xy(gsc, gps, &npt, &pc2[0], &pc2[1]);

      /* Check that point segment of the boundbox of the selection stroke */
      if (((!ELEM(V2D_IS_CLIPPED, pc1[0], pc1[1])) && BLI_rcti_isect_pt(rect, pc1[0], pc1[1])) ||
          ((!ELEM(V2D_IS_CLIPPED, pc2[0], pc2[1])) && BLI_rcti_isect_pt(rect, pc2[0], pc2[1]))) {
        /* Check if point segment of stroke had anything to do with
         * brush region  (either within stroke painted, or on its lines)
         * - this assumes that linewidth is irrelevant
         */
        if (gp_stroke_inside_circle(
                gso->mval, gso->mval_prev, radius, pc1[0], pc1[1], pc2[0], pc2[1])) {

          /* Apply operation to these points */
          bool ok = false;

          /* To each point individually... */
          pt = &gps->points[i];
          pt_active = (!is_multiedit) ? pt->runtime.pt_orig : pt;
          index = (!is_multiedit) ? pt->runtime.idx_orig : i;
          if (pt_active != NULL) {
            rot_eval = gptint_rotation_eval_get(gso, gps, pt, i);
            ok = apply(gso, gps_active, rot_eval, index, radius, pc1);
          }

          /* Only do the second point if this is the last segment,
           * and it is unlikely that the point will get handled
           * otherwise.
           *
           * NOTE: There is a small risk here that the second point wasn't really
           *       actually in-range. In that case, it only got in because
           *       the line linking the points was!
           */
          if (i + 1 == gps->totpoints - 1) {
            pt = &gps->points[i + 1];
            pt_active = (!is_multiedit) ? pt->runtime.pt_orig : pt;
            index = (!is_multiedit) ? pt->runtime.idx_orig : i + 1;
            if (pt_active != NULL) {
              rot_eval = gptint_rotation_eval_get(gso, gps, pt, i + 1);
              ok |= apply(gso, gps_active, rot_eval, index, radius, pc2);
              include_last = false;
            }
          }
          else {
            include_last = true;
          }

          changed |= ok;
        }
        else if (include_last) {
          /* This case is for cases where for whatever reason the second vert (1st here)
           * doesn't get included because the whole edge isn't in bounds,
           * but it would've qualified since it did with the previous step
           * (but wasn't added then, to avoid double-ups).
           */
          pt = &gps->points[i];
          pt_active = (!is_multiedit) ? pt->runtime.pt_orig : pt;
          index = (!is_multiedit) ? pt->runtime.idx_orig : i;
          if (pt_active != NULL) {
            rot_eval = gptint_rotation_eval_get(gso, gps, pt, i);
            changed |= apply(gso, gps_active, rot_eval, index, radius, pc1);
            include_last = false;
          }
        }
      }
    }
  }

  return changed;
}

/* Apply sculpt brushes to strokes in the given frame */
static bool gptint_brush_do_frame(bContext *C,
                                  tGP_BrushTintData *gso,
                                  bGPDlayer *gpl,
                                  bGPDframe *gpf,
                                  const float diff_mat[4][4])
{
  bool changed = false;
  Object *ob = CTX_data_active_object(C);
  char tool = ob->mode == OB_MODE_VERTEX_GPENCIL ? gso->brush->gpencil_vertex_tool :
                                                   gso->brush->gpencil_tool;
  for (bGPDstroke *gps = gpf->strokes.first; gps; gps = gps->next) {
    /* skip strokes that are invalid for current view */
    if (ED_gpencil_stroke_can_use(C, gps) == false) {
      continue;
    }
    /* check if the color is editable */
    if (ED_gpencil_stroke_color_use(ob, gpl, gps) == false) {
      continue;
    }

    switch (tool) {
      case GPAINT_TOOL_TINT:
      case GPVERTEX_TOOL_DRAW: {
        changed |= gptint_brush_do_stroke(gso, gps, diff_mat, brush_tint_apply);
        break;
      }

      default:
        printf("ERROR: Unknown type of GPencil Tint brush\n");
        break;
    }
  }

  return changed;
}

/* Perform two-pass brushes which modify the existing strokes */
static bool gptint_brush_apply_standard(bContext *C, tGP_BrushTintData *gso)
{
  ToolSettings *ts = CTX_data_tool_settings(C);
  Depsgraph *depsgraph = CTX_data_ensure_evaluated_depsgraph(C);
  Object *obact = gso->object;
  Object *ob_eval = DEG_get_evaluated_object(depsgraph, obact);
  bGPdata *gpd = gso->gpd;
  bool changed = false;

  /* Find visible strokes, and perform operations on those if hit */
  CTX_DATA_BEGIN (C, bGPDlayer *, gpl, editable_gpencil_layers) {
    /* If no active frame, don't do anything... */
    if (gpl->actframe == NULL) {
      continue;
    }
    /* Get evaluated frames array data */
    int idx_eval = BLI_findindex(&gpd->layers, gpl);
    bGPDframe *gpf_eval = &ob_eval->runtime.gpencil_evaluated_frames[idx_eval];
    if (gpf_eval == NULL) {
      continue;
    }

    /* calculate difference matrix */
    float diff_mat[4][4];
    ED_gpencil_parent_location(depsgraph, obact, gpd, gpl, diff_mat);

    /* Active Frame or MultiFrame? */
    if (gso->is_multiframe) {
      /* init multiframe falloff options */
      int f_init = 0;
      int f_end = 0;

      if (gso->use_multiframe_falloff) {
        BKE_gpencil_get_range_selected(gpl, &f_init, &f_end);
      }

      for (bGPDframe *gpf = gpl->frames.first; gpf; gpf = gpf->next) {
        /* Always do active frame; Otherwise, only include selected frames */
        if ((gpf == gpl->actframe) || (gpf->flag & GP_FRAME_SELECT)) {
          /* compute multiframe falloff factor */
          if (gso->use_multiframe_falloff) {
            /* Faloff depends on distance to active frame (relative to the overall frame range) */
            gso->mf_falloff = BKE_gpencil_multiframe_falloff_calc(
                gpf, gpl->actframe->framenum, f_init, f_end, ts->gp_sculpt.cur_falloff);
          }
          else {
            /* No falloff */
            gso->mf_falloff = 1.0f;
          }

          /* affect strokes in this frame */
          changed |= gptint_brush_do_frame(C, gso, gpl, gpf, diff_mat);
        }
      }
    }
    else {
      /* Apply to active frame's strokes */
      gso->mf_falloff = 1.0f;
      changed |= gptint_brush_do_frame(C, gso, gpl, gpf_eval, diff_mat);
    }
  }
  CTX_DATA_END;

  return changed;
}

/* Calculate settings for applying brush */
static void gptint_brush_apply(bContext *C, wmOperator *op, PointerRNA *itemptr)
{
  tGP_BrushTintData *gso = op->customdata;
  Brush *brush = gso->brush;
  const int radius = ((brush->flag & GP_SCULPT_FLAG_PRESSURE_RADIUS) ?
                          gso->brush->size * gso->pressure :
                          gso->brush->size);
  float mousef[2];
  int mouse[2];
  bool changed = false;

  /* Get latest mouse coordinates */
  RNA_float_get_array(itemptr, "mouse", mousef);
  gso->mval[0] = mouse[0] = (int)(mousef[0]);
  gso->mval[1] = mouse[1] = (int)(mousef[1]);

  gso->pressure = RNA_float_get(itemptr, "pressure");

  if (RNA_boolean_get(itemptr, "pen_flip")) {
    gso->flag |= GP_SCULPT_FLAG_INVERT;
  }
  else {
    gso->flag &= ~GP_SCULPT_FLAG_INVERT;
  }

  /* Store coordinates as reference, if operator just started running */
  if (gso->first) {
    gso->mval_prev[0] = gso->mval[0];
    gso->mval_prev[1] = gso->mval[1];
    gso->pressure_prev = gso->pressure;
  }

  /* Update brush_rect, so that it represents the bounding rectangle of brush */
  gso->brush_rect.xmin = mouse[0] - radius;
  gso->brush_rect.ymin = mouse[1] - radius;
  gso->brush_rect.xmax = mouse[0] + radius;
  gso->brush_rect.ymax = mouse[1] + radius;

  changed = gptint_brush_apply_standard(C, gso);

  /* Updates */
  if (changed) {
    DEG_id_tag_update(&gso->gpd->id, ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_GPENCIL | ND_DATA | NA_EDITED, NULL);
  }

  /* Store values for next step */
  gso->mval_prev[0] = gso->mval[0];
  gso->mval_prev[1] = gso->mval[1];
  gso->pressure_prev = gso->pressure;
  gso->first = false;
}

/* Running --------------------------------------------- */

/* helper - a record stroke, and apply paint event */
static void gptint_brush_apply_event(bContext *C, wmOperator *op, const wmEvent *event)
{
  tGP_BrushTintData *gso = op->customdata;
  PointerRNA itemptr;
  float mouse[2];
  int tablet = 0;

  mouse[0] = event->mval[0] + 1;
  mouse[1] = event->mval[1] + 1;

  /* fill in stroke */
  RNA_collection_add(op->ptr, "stroke", &itemptr);

  RNA_float_set_array(&itemptr, "mouse", mouse);
  RNA_boolean_set(&itemptr, "pen_flip", event->ctrl != false);
  RNA_boolean_set(&itemptr, "is_start", gso->first);

  /* handle pressure sensitivity (which is supplied by tablets) */
  if (event->tablet_data) {
    const wmTabletData *wmtab = event->tablet_data;
    float pressure = wmtab->Pressure;

    tablet = (wmtab->Active != EVT_TABLET_NONE);

    /* special exception here for too high pressure values on first touch in
     * windows for some tablets: clamp the values to be sane
     */
    if (tablet && (pressure >= 0.99f)) {
      pressure = 1.0f;
    }
    RNA_float_set(&itemptr, "pressure", pressure);
  }
  else {
    RNA_float_set(&itemptr, "pressure", 1.0f);
  }

  /* apply */
  gptint_brush_apply(C, op, &itemptr);
}

/* reapply */
static int gptint_brush_exec(bContext *C, wmOperator *op)
{
  if (!gptint_brush_init(C, op)) {
    return OPERATOR_CANCELLED;
  }

  RNA_BEGIN (op->ptr, itemptr, "stroke") {
    gptint_brush_apply(C, op, &itemptr);
  }
  RNA_END;

  gptint_brush_exit(C, op);

  return OPERATOR_FINISHED;
}

/* start modal painting */
static int gptint_brush_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  tGP_BrushTintData *gso = NULL;
  const bool is_modal = RNA_boolean_get(op->ptr, "wait_for_input");
  const bool is_playing = ED_screen_animation_playing(CTX_wm_manager(C)) != NULL;

  /* the operator cannot work while play animation */
  if (is_playing) {
    BKE_report(op->reports, RPT_ERROR, "Cannot tint while play animation");

    return OPERATOR_CANCELLED;
  }

  /* init painting data */
  if (!gptint_brush_init(C, op)) {
    return OPERATOR_CANCELLED;
  }

  gso = op->customdata;

  /* register modal handler */
  WM_event_add_modal_handler(C, op);

  /* start drawing immediately? */
  if (is_modal == false) {
    ARegion *ar = CTX_wm_region(C);

    /* apply first dab... */
    gso->is_painting = true;
    gptint_brush_apply_event(C, op, event);

    /* redraw view with feedback */
    ED_region_tag_redraw(ar);
  }

  return OPERATOR_RUNNING_MODAL;
}

/* painting - handle events */
static int gptint_brush_modal(bContext *C, wmOperator *op, const wmEvent *event)
{
  tGP_BrushTintData *gso = op->customdata;
  const bool is_modal = RNA_boolean_get(op->ptr, "wait_for_input");
  bool redraw_region = false;
  bool redraw_toolsettings = false;

  /* The operator can be in 2 states: Painting and Idling */
  if (gso->is_painting) {
    /* Painting  */
    switch (event->type) {
      /* Mouse Move = Apply somewhere else */
      case MOUSEMOVE:
      case INBETWEEN_MOUSEMOVE:
        /* apply brush effect at new position */
        gptint_brush_apply_event(C, op, event);

        /* force redraw, so that the cursor will at least be valid */
        redraw_region = true;
        break;

      /* Painting mbut release = Stop painting (back to idle) */
      case LEFTMOUSE:
        if (is_modal) {
          /* go back to idling... */
          gso->is_painting = false;
        }
        else {
          /* end tint, since we're not modal */
          gso->is_painting = false;

          gptint_brush_exit(C, op);
          return OPERATOR_FINISHED;
        }
        break;

      /* Abort painting if any of the usual things are tried */
      case MIDDLEMOUSE:
      case RIGHTMOUSE:
      case ESCKEY:
        gptint_brush_exit(C, op);
        return OPERATOR_FINISHED;
    }
  }
  else {
    /* Idling */
    BLI_assert(is_modal == true);

    switch (event->type) {
      /* Painting mbut press = Start painting (switch to painting state) */
      case LEFTMOUSE:
        /* do initial "click" apply */
        gso->is_painting = true;
        gso->first = true;

        gptint_brush_apply_event(C, op, event);
        break;

      /* Exit modal operator, based on the "standard" ops */
      case RIGHTMOUSE:
      case ESCKEY:
        gptint_brush_exit(C, op);
        return OPERATOR_FINISHED;

      /* MMB is often used for view manipulations */
      case MIDDLEMOUSE:
        return OPERATOR_PASS_THROUGH;

      /* Mouse movements should update the brush cursor - Just redraw the active region */
      case MOUSEMOVE:
      case INBETWEEN_MOUSEMOVE:
        redraw_region = true;
        break;

      /* Change Frame - Allowed */
      case LEFTARROWKEY:
      case RIGHTARROWKEY:
      case UPARROWKEY:
      case DOWNARROWKEY:
        return OPERATOR_PASS_THROUGH;

      /* Camera/View Gizmo's - Allowed */
      /* (See rationale in gpencil_paint.c -> gpencil_draw_modal()) */
      case PAD0:
      case PAD1:
      case PAD2:
      case PAD3:
      case PAD4:
      case PAD5:
      case PAD6:
      case PAD7:
      case PAD8:
      case PAD9:
        return OPERATOR_PASS_THROUGH;

      /* Unhandled event */
      default:
        break;
    }
  }

  /* Redraw region? */
  if (redraw_region) {
    ARegion *ar = CTX_wm_region(C);
    ED_region_tag_redraw(ar);
  }

  /* Redraw toolsettings (brush settings)? */
  if (redraw_toolsettings) {
    DEG_id_tag_update(&gso->gpd->id, ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_SCENE | ND_TOOLSETTINGS, NULL);
  }

  return OPERATOR_RUNNING_MODAL;
}

void GPENCIL_OT_tint(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Stroke Tint";
  ot->idname = "GPENCIL_OT_tint";
  ot->description = "Tint strokes with a mix color";

  /* api callbacks */
  ot->exec = gptint_brush_exec;
  ot->invoke = gptint_brush_invoke;
  ot->modal = gptint_brush_modal;
  ot->cancel = gptint_brush_exit;
  ot->poll = gptint_brush_poll;

  /* flags */
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* properties */
  PropertyRNA *prop;
  prop = RNA_def_collection_runtime(ot->srna, "stroke", &RNA_OperatorStrokeElement, "Stroke", "");
  RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);

  prop = RNA_def_boolean(ot->srna, "wait_for_input", true, "Wait for Input", "");
  RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
}
