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
#include "RNA_enum_types.h"

#include "UI_view2d.h"

#include "ED_gpencil.h"
#include "ED_screen.h"
#include "ED_view3d.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "gpencil_intern.h"

enum {
  GP_PAINT_VERTEX_STROKE = 0,
  GP_PAINT_VERTEX_FILL = 1,
  GP_PAINT_VERTEX_BOTH = 2,
};

static const EnumPropertyItem gpencil_modesEnumPropertyItem_mode[] = {
    {GP_PAINT_VERTEX_STROKE, "STROKE", 0, "Stroke", ""},
    {GP_PAINT_VERTEX_FILL, "FILL", 0, "Fill", ""},
    {GP_PAINT_VERTEX_BOTH, "BOTH", 0, "Both", ""},
    {0, NULL, 0, NULL, NULL},
};

/* Poll callback for stroke vertex paint operator. */
static bool gp_vertexpaint_mode_poll(bContext *C)
{
  ToolSettings *ts = CTX_data_tool_settings(C);
  Object *ob = CTX_data_active_object(C);
  if ((ob == NULL) || (ob->type != OB_GPENCIL)) {
    return false;
  }

  bGPdata *gpd = (bGPdata *)ob->data;
  if (GPENCIL_VERTEX_MODE(gpd)) {
    if (!(GPENCIL_ANY_VERTEX_MASK(ts->gpencil_selectmode_vertex))) {
      return false;
    }

    /* Any data to use. */
    if (gpd->layers.first) {
      return true;
    }
  }

  return false;
}

static int gp_vertexpaint_brightness_contrast_exec(bContext *C, wmOperator *op)
{
  Object *ob = CTX_data_active_object(C);
  bGPdata *gpd = (bGPdata *)ob->data;
  bool changed = false;
  int i;
  bGPDspoint *pt;
  const int mode = RNA_enum_get(op->ptr, "mode");

  float gain, offset;
  {
    float brightness = RNA_float_get(op->ptr, "brightness");
    float contrast = RNA_float_get(op->ptr, "contrast");
    brightness /= 100.0f;
    float delta = contrast / 200.0f;
    /*
     * The algorithm is by Werner D. Streidt
     * (http://visca.com/ffactory/archives/5-99/msg00021.html)
     * Extracted of OpenCV demhist.c
     */
    if (contrast > 0) {
      gain = 1.0f - delta * 2.0f;
      gain = 1.0f / max_ff(gain, FLT_EPSILON);
      offset = gain * (brightness - delta);
    }
    else {
      delta *= -1;
      gain = max_ff(1.0f - delta * 2.0f, 0.0f);
      offset = gain * brightness + delta;
    }
  }

  /* Loop all selected strokes. */
  GP_EDITABLE_STROKES_BEGIN (gpstroke_iter, C, gpl, gps) {
    if (gps->flag & GP_STROKE_SELECT) {
      changed = true;
      /* Fill color. */
      if (gps->flag & GP_STROKE_SELECT) {
        changed = true;
        if (mode != GP_PAINT_VERTEX_STROKE) {
          if (gps->mix_color_fill[3] > 0.0f) {
            for (int i = 0; i < 3; i++) {
              gps->mix_color_fill[i] = gain * gps->mix_color_fill[i] + offset;
            }
          }
        }
      }

      /* Stroke points. */
      if (mode != GP_PAINT_VERTEX_FILL) {
        for (i = 0, pt = gps->points; i < gps->totpoints; i++, pt++) {
          if ((pt->flag & GP_SPOINT_SELECT) && (pt->mix_color[3] > 0.0f)) {
            for (int i = 0; i < 3; i++) {
              pt->mix_color[i] = gain * pt->mix_color[i] + offset;
            }
          }
        }
      }
    }
  }
  GP_EDITABLE_STROKES_END(gpstroke_iter);

  /* notifiers */
  if (changed) {
    DEG_id_tag_update(&gpd->id, ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_GPENCIL | ND_DATA | NA_EDITED, NULL);
  }

  return OPERATOR_FINISHED;
}

void GPENCIL_OT_vertex_color_brightness_contrast(wmOperatorType *ot)
{
  PropertyRNA *prop;

  /* identifiers */
  ot->name = "Vertex Paint Bright/Contrast";
  ot->idname = "GPENCIL_OT_vertex_color_brightness_contrast";
  ot->description = "Adjust vertex color brightness/contrast";

  /* api callbacks */
  ot->exec = gp_vertexpaint_brightness_contrast_exec;
  ot->poll = gp_vertexpaint_mode_poll;

  /* flags */
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

  /* params */
  ot->prop = RNA_def_enum(ot->srna, "mode", gpencil_modesEnumPropertyItem_mode, 0, "Mode", "");
  const float min = -100, max = +100;
  prop = RNA_def_float(ot->srna, "brightness", 0.0f, min, max, "Brightness", "", min, max);
  prop = RNA_def_float(ot->srna, "contrast", 0.0f, min, max, "Contrast", "", min, max);
  RNA_def_property_ui_range(prop, min, max, 1, 1);
}

static int gp_vertexpaint_hsv_exec(bContext *C, wmOperator *op)
{
  Object *ob = CTX_data_active_object(C);
  bGPdata *gpd = (bGPdata *)ob->data;

  bool changed = false;
  int i;
  bGPDspoint *pt;
  float hsv[3];

  const int mode = RNA_enum_get(op->ptr, "mode");
  float hue = RNA_float_get(op->ptr, "h");
  float sat = RNA_float_get(op->ptr, "s");
  float val = RNA_float_get(op->ptr, "v");

  /* Loop all selected strokes. */
  GP_EDITABLE_STROKES_BEGIN (gpstroke_iter, C, gpl, gps) {
    if (gps->flag & GP_STROKE_SELECT) {
      changed = true;

      /* Fill color. */
      if (mode != GP_PAINT_VERTEX_STROKE) {
        if (gps->mix_color_fill[3] > 0.0f) {

          rgb_to_hsv_v(gps->mix_color_fill, hsv);

          hsv[0] += (hue - 0.5f);
          if (hsv[0] > 1.0f) {
            hsv[0] -= 1.0f;
          }
          else if (hsv[0] < 0.0f) {
            hsv[0] += 1.0f;
          }
          hsv[1] *= sat;
          hsv[2] *= val;

          hsv_to_rgb_v(hsv, gps->mix_color_fill);
        }
      }

      /* Stroke points. */
      if (mode != GP_PAINT_VERTEX_FILL) {
        for (i = 0, pt = gps->points; i < gps->totpoints; i++, pt++) {
          if ((pt->flag & GP_SPOINT_SELECT) && (pt->mix_color[3] > 0.0f)) {
            rgb_to_hsv_v(pt->mix_color, hsv);

            hsv[0] += (hue - 0.5f);
            if (hsv[0] > 1.0f) {
              hsv[0] -= 1.0f;
            }
            else if (hsv[0] < 0.0f) {
              hsv[0] += 1.0f;
            }
            hsv[1] *= sat;
            hsv[2] *= val;

            hsv_to_rgb_v(hsv, pt->mix_color);
          }
        }
      }
    }
  }
  GP_EDITABLE_STROKES_END(gpstroke_iter);

  /* notifiers */
  if (changed) {
    DEG_id_tag_update(&gpd->id, ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_GPENCIL | ND_DATA | NA_EDITED, NULL);
  }

  return OPERATOR_FINISHED;
}

void GPENCIL_OT_vertex_color_hsv(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Vertex Paint Hue Saturation Value";
  ot->idname = "GPENCIL_OT_vertex_color_hsv";
  ot->description = "Adjust vertex color HSV values";

  /* api callbacks */
  ot->exec = gp_vertexpaint_hsv_exec;
  ot->poll = gp_vertexpaint_mode_poll;

  /* flags */
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

  /* params */
  ot->prop = RNA_def_enum(ot->srna, "mode", gpencil_modesEnumPropertyItem_mode, 0, "Mode", "");
  RNA_def_float(ot->srna, "h", 0.5f, 0.0f, 1.0f, "Hue", "", 0.0f, 1.0f);
  RNA_def_float(ot->srna, "s", 1.0f, 0.0f, 2.0f, "Saturation", "", 0.0f, 2.0f);
  RNA_def_float(ot->srna, "v", 1.0f, 0.0f, 2.0f, "Value", "", 0.0f, 2.0f);
}

static int gp_vertexpaint_invert_exec(bContext *C, wmOperator *op)
{
  Object *ob = CTX_data_active_object(C);
  bGPdata *gpd = (bGPdata *)ob->data;

  bool changed = false;
  int i;
  bGPDspoint *pt;

  const int mode = RNA_enum_get(op->ptr, "mode");

  /* Loop all selected strokes. */
  GP_EDITABLE_STROKES_BEGIN (gpstroke_iter, C, gpl, gps) {
    if (gps->flag & GP_STROKE_SELECT) {
      changed = true;
      /* Fill color. */
      if (gps->flag & GP_STROKE_SELECT) {
        changed = true;
        if (mode != GP_PAINT_VERTEX_STROKE) {
          if (gps->mix_color_fill[3] > 0.0f) {
            for (int i = 0; i < 3; i++) {
              gps->mix_color_fill[i] = 1.0f - gps->mix_color_fill[i];
            }
          }
        }
      }

      /* Stroke points. */
      if (mode != GP_PAINT_VERTEX_FILL) {
        for (i = 0, pt = gps->points; i < gps->totpoints; i++, pt++) {
          if ((pt->flag & GP_SPOINT_SELECT) && (pt->mix_color[3] > 0.0f)) {
            for (int i = 0; i < 3; i++) {
              pt->mix_color[i] = 1.0f - pt->mix_color[i];
            }
          }
        }
      }
    }
  }
  GP_EDITABLE_STROKES_END(gpstroke_iter);

  /* notifiers */
  if (changed) {
    DEG_id_tag_update(&gpd->id, ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_GPENCIL | ND_DATA | NA_EDITED, NULL);
  }

  return OPERATOR_FINISHED;
}

void GPENCIL_OT_vertex_color_invert(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Vertex Paint Invert";
  ot->idname = "GPENCIL_OT_vertex_color_invert";
  ot->description = "Invert RGB values";

  /* api callbacks */
  ot->exec = gp_vertexpaint_invert_exec;
  ot->poll = gp_vertexpaint_mode_poll;

  /* flags */
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

  /* params */
  ot->prop = RNA_def_enum(ot->srna, "mode", gpencil_modesEnumPropertyItem_mode, 0, "Mode", "");
}

static int gp_vertexpaint_levels_exec(bContext *C, wmOperator *op)
{
  Object *ob = CTX_data_active_object(C);
  bGPdata *gpd = (bGPdata *)ob->data;

  bool changed = false;
  int i;
  bGPDspoint *pt;

  const int mode = RNA_enum_get(op->ptr, "mode");
  float gain = RNA_float_get(op->ptr, "gain");
  float offset = RNA_float_get(op->ptr, "offset");

  /* Loop all selected strokes. */
  GP_EDITABLE_STROKES_BEGIN (gpstroke_iter, C, gpl, gps) {

    /* Fill color. */
    if (gps->flag & GP_STROKE_SELECT) {
      changed = true;
      if (mode != GP_PAINT_VERTEX_STROKE) {
        if (gps->mix_color_fill[3] > 0.0f) {
          for (int i = 0; i < 3; i++) {
            gps->mix_color_fill[i] = gain * (gps->mix_color_fill[i] + offset);
          }
        }
      }
    }

    /* Stroke points. */
    if (mode != GP_PAINT_VERTEX_FILL) {
      for (i = 0, pt = gps->points; i < gps->totpoints; i++, pt++) {
        if ((pt->flag & GP_SPOINT_SELECT) && (pt->mix_color[3] > 0.0f)) {
          for (int i = 0; i < 3; i++) {
            pt->mix_color[i] = gain * (pt->mix_color[i] + offset);
          }
        }
      }
    }
  }
  GP_EDITABLE_STROKES_END(gpstroke_iter);

  /* notifiers */
  if (changed) {
    DEG_id_tag_update(&gpd->id, ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY);
    WM_event_add_notifier(C, NC_GPENCIL | ND_DATA | NA_EDITED, NULL);
  }

  return OPERATOR_FINISHED;
}

void GPENCIL_OT_vertex_color_levels(wmOperatorType *ot)
{

  /* identifiers */
  ot->name = "Vertex Paint Levels";
  ot->idname = "GPENCIL_OT_vertex_color_levels";
  ot->description = "Adjust levels of vertex colors";

  /* api callbacks */
  ot->exec = gp_vertexpaint_levels_exec;
  ot->poll = gp_vertexpaint_mode_poll;

  /* flags */
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

  /* params */
  ot->prop = RNA_def_enum(ot->srna, "mode", gpencil_modesEnumPropertyItem_mode, 0, "Mode", "");

  RNA_def_float(
      ot->srna, "offset", 0.0f, -1.0f, 1.0f, "Offset", "Value to add to colors", -1.0f, 1.0f);
  RNA_def_float(
      ot->srna, "gain", 1.0f, 0.0f, FLT_MAX, "Gain", "Value to multiply colors by", 0.0f, 10.0f);
}
