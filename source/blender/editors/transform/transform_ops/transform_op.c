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
 */

/** \file
 * \ingroup edtransform
 */

#include "MEM_guardedalloc.h"

#include "DNA_anim_types.h"
#include "DNA_armature_types.h"
#include "DNA_constraint_types.h"
#include "DNA_mask_types.h"
#include "DNA_mesh_types.h"
#include "DNA_movieclip_types.h"
#include "DNA_scene_types.h" /* PET modes */
#include "DNA_workspace_types.h"
#include "DNA_gpencil_types.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_report.h"
#include "BKE_editmesh.h"
#include "BKE_layer.h"
#include "BKE_scene.h"

#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "ED_image.h"
#include "ED_keyframing.h"
#include "ED_screen.h"
#include "ED_space_api.h"
#include "ED_markers.h"
#include "ED_view3d.h"
#include "ED_mesh.h"
#include "ED_clip.h"
#include "ED_node.h"
#include "ED_gpencil.h"
#include "ED_sculpt.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "BLF_api.h"
#include "BLT_translation.h"

#include "WM_api.h"
#include "WM_message.h"
#include "WM_types.h"
#include "WM_toolsystem.h"

#include "UI_view2d.h"
#include "UI_interface.h"
#include "UI_interface_icons.h"
#include "UI_resources.h"

#include "ED_screen.h"
/* for USE_LOOPSLIDE_HACK only */
#include "ED_mesh.h"

#include "transform.h"
#include "transform_convert.h"
#include "transform_op.h"

const float VecOne[3] = {1, 1, 1};

typedef struct TransformModeItem {
  const char *idname;
  int mode;
  void (*opfunc)(wmOperatorType *);
} TransformModeItem;

static TransformModeItem transform_modes[] = {
    {OP_TRANSLATION, TFM_TRANSLATION, TRANSFORM_OT_translate},
    {OP_ROTATION, TFM_ROTATION, TRANSFORM_OT_rotate},
    {OP_TOSPHERE, TFM_TOSPHERE, TRANSFORM_OT_tosphere},
    {OP_RESIZE, TFM_RESIZE, TRANSFORM_OT_resize},
    {OP_SKIN_RESIZE, TFM_SKIN_RESIZE, TRANSFORM_OT_skin_resize},
    {OP_SHEAR, TFM_SHEAR, TRANSFORM_OT_shear},
    {OP_BEND, TFM_BEND, TRANSFORM_OT_bend},
    {OP_SHRINK_FATTEN, TFM_SHRINKFATTEN, TRANSFORM_OT_shrink_fatten},
    {OP_PUSH_PULL, TFM_PUSHPULL, TRANSFORM_OT_push_pull},
    {OP_TILT, TFM_TILT, TRANSFORM_OT_tilt},
    {OP_TRACKBALL, TFM_TRACKBALL, TRANSFORM_OT_trackball},
    {OP_MIRROR, TFM_MIRROR, TRANSFORM_OT_mirror},
    {OP_EDGE_SLIDE, TFM_EDGE_SLIDE, TRANSFORM_OT_edge_slide},
    {OP_VERT_SLIDE, TFM_VERT_SLIDE, TRANSFORM_OT_vert_slide},
    {OP_EDGE_CREASE, TFM_CREASE, TRANSFORM_OT_edge_crease},
    {OP_EDGE_BWEIGHT, TFM_BWEIGHT, TRANSFORM_OT_edge_bevelweight},
    {OP_SEQ_SLIDE, TFM_SEQ_SLIDE, TRANSFORM_OT_seq_slide},
    {OP_NORMAL_ROTATION, TFM_NORMAL_ROTATION, TRANSFORM_OT_rotate_normal},
    {NULL, 0},
};

const EnumPropertyItem rna_enum_transform_mode_types[] = {
    {TFM_INIT, "INIT", 0, "Init", ""},
    {TFM_DUMMY, "DUMMY", 0, "Dummy", ""},
    {TFM_TRANSLATION, "TRANSLATION", 0, "Translation", ""},
    {TFM_ROTATION, "ROTATION", 0, "Rotation", ""},
    {TFM_RESIZE, "RESIZE", 0, "Resize", ""},
    {TFM_SKIN_RESIZE, "SKIN_RESIZE", 0, "Skin Resize", ""},
    {TFM_TOSPHERE, "TOSPHERE", 0, "Tosphere", ""},
    {TFM_SHEAR, "SHEAR", 0, "Shear", ""},
    {TFM_BEND, "BEND", 0, "Bend", ""},
    {TFM_SHRINKFATTEN, "SHRINKFATTEN", 0, "Shrinkfatten", ""},
    {TFM_TILT, "TILT", 0, "Tilt", ""},
    {TFM_TRACKBALL, "TRACKBALL", 0, "Trackball", ""},
    {TFM_PUSHPULL, "PUSHPULL", 0, "Pushpull", ""},
    {TFM_CREASE, "CREASE", 0, "Crease", ""},
    {TFM_MIRROR, "MIRROR", 0, "Mirror", ""},
    {TFM_BONESIZE, "BONE_SIZE", 0, "Bonesize", ""},
    {TFM_BONE_ENVELOPE, "BONE_ENVELOPE", 0, "Bone Envelope", ""},
    {TFM_BONE_ENVELOPE_DIST, "BONE_ENVELOPE_DIST", 0, "Bone Envelope Distance", ""},
    {TFM_CURVE_SHRINKFATTEN, "CURVE_SHRINKFATTEN", 0, "Curve Shrinkfatten", ""},
    {TFM_MASK_SHRINKFATTEN, "MASK_SHRINKFATTEN", 0, "Mask Shrinkfatten", ""},
    {TFM_GPENCIL_SHRINKFATTEN, "GPENCIL_SHRINKFATTEN", 0, "GPencil Shrinkfatten", ""},
    {TFM_BONE_ROLL, "BONE_ROLL", 0, "Bone Roll", ""},
    {TFM_TIME_TRANSLATE, "TIME_TRANSLATE", 0, "Time Translate", ""},
    {TFM_TIME_SLIDE, "TIME_SLIDE", 0, "Time Slide", ""},
    {TFM_TIME_SCALE, "TIME_SCALE", 0, "Time Scale", ""},
    {TFM_TIME_EXTEND, "TIME_EXTEND", 0, "Time Extend", ""},
    {TFM_BAKE_TIME, "BAKE_TIME", 0, "Bake Time", ""},
    {TFM_BWEIGHT, "BWEIGHT", 0, "Bweight", ""},
    {TFM_ALIGN, "ALIGN", 0, "Align", ""},
    {TFM_EDGE_SLIDE, "EDGESLIDE", 0, "Edge Slide", ""},
    {TFM_SEQ_SLIDE, "SEQSLIDE", 0, "Sequence Slide", ""},
    {TFM_GPENCIL_OPACITY, "GPENCIL_OPACITY", 0, "GPencil Opacity", ""},
    {0, NULL, 0, NULL, NULL},
};

static void viewRedrawForce(const bContext *C, TransInfo *t)
{
  if (t->options & CTX_GPENCIL_STROKES) {
    bGPdata *gpd = ED_gpencil_data_get_active(C);
    if (gpd) {
      DEG_id_tag_update(&gpd->id, ID_RECALC_GEOMETRY);
    }
    WM_event_add_notifier(C, NC_GPENCIL | NA_EDITED, NULL);
  }
  else if (t->spacetype == SPACE_VIEW3D) {
    if (t->options & CTX_PAINT_CURVE) {
      wmWindow *window = CTX_wm_window(C);
      WM_paint_cursor_tag_redraw(window, t->ar);
    }
    else {
      /* Do we need more refined tags? */
      if (t->flag & T_POSE) {
        WM_event_add_notifier(C, NC_OBJECT | ND_POSE, NULL);
      }
      else {
        WM_event_add_notifier(C, NC_OBJECT | ND_TRANSFORM, NULL);
      }

      /* For real-time animation record - send notifiers recognized by animation editors */
      // XXX: is this notifier a lame duck?
      if ((t->animtimer) && IS_AUTOKEY_ON(t->scene)) {
        WM_event_add_notifier(C, NC_OBJECT | ND_KEYS, NULL);
      }
    }
  }
  else if (t->spacetype == SPACE_ACTION) {
    // SpaceAction *saction = (SpaceAction *)t->sa->spacedata.first;
    WM_event_add_notifier(C, NC_ANIMATION | ND_KEYFRAME | NA_EDITED, NULL);
  }
  else if (t->spacetype == SPACE_GRAPH) {
    // SpaceGraph *sipo = (SpaceGraph *)t->sa->spacedata.first;
    WM_event_add_notifier(C, NC_ANIMATION | ND_KEYFRAME | NA_EDITED, NULL);
  }
  else if (t->spacetype == SPACE_NLA) {
    WM_event_add_notifier(C, NC_ANIMATION | ND_NLA | NA_EDITED, NULL);
  }
  else if (t->spacetype == SPACE_NODE) {
    // ED_area_tag_redraw(t->sa);
    WM_event_add_notifier(C, NC_SPACE | ND_SPACE_NODE_VIEW, NULL);
  }
  else if (t->spacetype == SPACE_SEQ) {
    WM_event_add_notifier(C, NC_SCENE | ND_SEQUENCER, NULL);
    /* Keyframes on strips has been moved, so make sure related editos are informed. */
    WM_event_add_notifier(C, NC_ANIMATION, NULL);
  }
  else if (t->spacetype == SPACE_IMAGE) {
    if (t->options & CTX_MASK) {
      Mask *mask = CTX_data_edit_mask(C);

      WM_event_add_notifier(C, NC_MASK | NA_EDITED, mask);
    }
    else if (t->options & CTX_PAINT_CURVE) {
      wmWindow *window = CTX_wm_window(C);
      WM_paint_cursor_tag_redraw(window, t->ar);
    }
    else if (t->flag & T_CURSOR) {
      ED_area_tag_redraw(t->sa);
    }
    else {
      // XXX how to deal with lock?
      SpaceImage *sima = (SpaceImage *)t->sa->spacedata.first;
      if (sima->lock) {
        WM_event_add_notifier(C, NC_GEOM | ND_DATA, OBEDIT_FROM_VIEW_LAYER(t->view_layer)->data);
      }
      else {
        ED_area_tag_redraw(t->sa);
      }
    }
  }
  else if (t->spacetype == SPACE_CLIP) {
    SpaceClip *sc = (SpaceClip *)t->sa->spacedata.first;

    if (ED_space_clip_check_show_trackedit(sc)) {
      MovieClip *clip = ED_space_clip_get_clip(sc);

      /* objects could be parented to tracking data, so send this for viewport refresh */
      WM_event_add_notifier(C, NC_OBJECT | ND_TRANSFORM, NULL);

      WM_event_add_notifier(C, NC_MOVIECLIP | NA_EDITED, clip);
    }
    else if (ED_space_clip_check_show_maskedit(sc)) {
      Mask *mask = CTX_data_edit_mask(C);

      WM_event_add_notifier(C, NC_MASK | NA_EDITED, mask);
    }
  }
}

static void viewRedrawPost(bContext *C, TransInfo *t)
{
  ED_area_status_text(t->sa, NULL);

  if (t->spacetype == SPACE_VIEW3D) {
    /* if autokeying is enabled, send notifiers that keyframes were added */
    if (IS_AUTOKEY_ON(t->scene)) {
      WM_main_add_notifier(NC_ANIMATION | ND_KEYFRAME | NA_EDITED, NULL);
    }

    /* redraw UV editor */
    if (ELEM(t->mode, TFM_VERT_SLIDE, TFM_EDGE_SLIDE) &&
        (t->settings->uvcalc_flag & UVCALC_TRANSFORM_CORRECT)) {
      WM_event_add_notifier(C, NC_GEOM | ND_DATA, NULL);
    }

    /* XXX temp, first hack to get auto-render in compositor work (ton) */
    WM_event_add_notifier(C, NC_SCENE | ND_TRANSFORM_DONE, CTX_data_scene(C));
  }

#if 0  // TRANSFORM_FIX_ME
  if (t->spacetype == SPACE_VIEW3D) {
    allqueue(REDRAWBUTSOBJECT, 0);
    allqueue(REDRAWVIEW3D, 0);
  }
  else if (t->spacetype == SPACE_IMAGE) {
    allqueue(REDRAWIMAGE, 0);
    allqueue(REDRAWVIEW3D, 0);
  }
  else if (ELEM(t->spacetype, SPACE_ACTION, SPACE_NLA, SPACE_GRAPH)) {
    allqueue(REDRAWVIEW3D, 0);
    allqueue(REDRAWACTION, 0);
    allqueue(REDRAWNLA, 0);
    allqueue(REDRAWIPO, 0);
    allqueue(REDRAWTIME, 0);
    allqueue(REDRAWBUTSOBJECT, 0);
  }

  scrarea_queue_headredraw(curarea);
#endif
}

static void initSnapSpatial(TransInfo *t, float r_snap[3])
{
  if (t->spacetype == SPACE_VIEW3D) {
    RegionView3D *rv3d = t->ar->regiondata;

    if (rv3d) {
      View3D *v3d = t->sa->spacedata.first;
      r_snap[0] = 0.0f;
      r_snap[1] = ED_view3d_grid_view_scale(t->scene, v3d, rv3d, NULL) * 1.0f;
      r_snap[2] = r_snap[1] * 0.1f;
    }
  }
  else if (t->spacetype == SPACE_IMAGE) {
    r_snap[0] = 0.0f;
    r_snap[1] = 0.0625f;
    r_snap[2] = 0.03125f;
  }
  else if (t->spacetype == SPACE_CLIP) {
    r_snap[0] = 0.0f;
    r_snap[1] = 0.125f;
    r_snap[2] = 0.0625f;
  }
  else if (t->spacetype == SPACE_NODE) {
    r_snap[0] = 0.0f;
    r_snap[1] = r_snap[2] = ED_node_grid_size();
  }
  else if (t->spacetype == SPACE_GRAPH) {
    r_snap[0] = 0.0f;
    r_snap[1] = 1.0;
    r_snap[2] = 0.1f;
  }
  else {
    r_snap[0] = 0.0f;
    r_snap[1] = r_snap[2] = 1.0f;
  }
}

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT,
} ArrowDirection;

#define POS_INDEX 0
/* NOTE: this --^ is a bit hackish, but simplifies GPUVertFormat usage among functions
 * private to this file  - merwin
 */

static void drawArrow(ArrowDirection d, short offset, short length, short size)
{
  immBegin(GPU_PRIM_LINES, 6);

  switch (d) {
    case LEFT:
      offset = -offset;
      length = -length;
      size = -size;
      ATTR_FALLTHROUGH;
    case RIGHT:
      immVertex2f(POS_INDEX, offset, 0);
      immVertex2f(POS_INDEX, offset + length, 0);
      immVertex2f(POS_INDEX, offset + length, 0);
      immVertex2f(POS_INDEX, offset + length - size, -size);
      immVertex2f(POS_INDEX, offset + length, 0);
      immVertex2f(POS_INDEX, offset + length - size, size);
      break;

    case DOWN:
      offset = -offset;
      length = -length;
      size = -size;
      ATTR_FALLTHROUGH;
    case UP:
      immVertex2f(POS_INDEX, 0, offset);
      immVertex2f(POS_INDEX, 0, offset + length);
      immVertex2f(POS_INDEX, 0, offset + length);
      immVertex2f(POS_INDEX, -size, offset + length - size);
      immVertex2f(POS_INDEX, 0, offset + length);
      immVertex2f(POS_INDEX, size, offset + length - size);
      break;
  }

  immEnd();
}

static void drawArrowHead(ArrowDirection d, short size)
{
  immBegin(GPU_PRIM_LINES, 4);

  switch (d) {
    case LEFT:
      size = -size;
      ATTR_FALLTHROUGH;
    case RIGHT:
      immVertex2f(POS_INDEX, 0, 0);
      immVertex2f(POS_INDEX, -size, -size);
      immVertex2f(POS_INDEX, 0, 0);
      immVertex2f(POS_INDEX, -size, size);
      break;

    case DOWN:
      size = -size;
      ATTR_FALLTHROUGH;
    case UP:
      immVertex2f(POS_INDEX, 0, 0);
      immVertex2f(POS_INDEX, -size, -size);
      immVertex2f(POS_INDEX, 0, 0);
      immVertex2f(POS_INDEX, size, -size);
      break;
  }

  immEnd();
}

static void drawArc(float size, float angle_start, float angle_end, int segments)
{
  float delta = (angle_end - angle_start) / segments;
  float angle;
  int a;

  immBegin(GPU_PRIM_LINE_STRIP, segments + 1);

  for (angle = angle_start, a = 0; a < segments; angle += delta, a++) {
    immVertex2f(POS_INDEX, cosf(angle) * size, sinf(angle) * size);
  }
  immVertex2f(POS_INDEX, cosf(angle_end) * size, sinf(angle_end) * size);

  immEnd();
}

static bool helpline_poll(bContext *C)
{
  ARegion *ar = CTX_wm_region(C);

  if (ar && ar->regiontype == RGN_TYPE_WINDOW) {
    return 1;
  }
  return 0;
}

static void drawHelpline(bContext *UNUSED(C), int x, int y, void *customdata)
{
  TransInfo *t = (TransInfo *)customdata;

  if (t->helpline != HLP_NONE) {
    float cent[2];
    const float mval[3] = {
        x,
        y,
        0.0f,
    };
    float tmval[2] = {
        (float)t->mval[0],
        (float)t->mval[1],
    };

    projectFloatViewEx(t, t->center_global, cent, V3D_PROJ_TEST_CLIP_ZERO);
    /* Offset the values for the area region. */
    const float offset[2] = {
        t->ar->winrct.xmin,
        t->ar->winrct.ymin,
    };

    for (int i = 0; i < 2; i++) {
      cent[i] += offset[i];
      tmval[i] += offset[i];
    }

    GPU_matrix_push();

    /* Dashed lines first. */
    if (ELEM(t->helpline, HLP_SPRING, HLP_ANGLE)) {
      const uint shdr_pos = GPU_vertformat_attr_add(
          immVertexFormat(), "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);

      UNUSED_VARS_NDEBUG(shdr_pos); /* silence warning */
      BLI_assert(shdr_pos == POS_INDEX);

      GPU_line_width(1.0f);

      immBindBuiltinProgram(GPU_SHADER_2D_LINE_DASHED_UNIFORM_COLOR);

      float viewport_size[4];
      GPU_viewport_size_get_f(viewport_size);
      immUniform2f("viewport_size", viewport_size[2], viewport_size[3]);

      immUniform1i("colors_len", 0); /* "simple" mode */
      immUniformThemeColor(TH_VIEW_OVERLAY);
      immUniform1f("dash_width", 6.0f);
      immUniform1f("dash_factor", 0.5f);

      immBegin(GPU_PRIM_LINES, 2);
      immVertex2fv(POS_INDEX, cent);
      immVertex2f(POS_INDEX, tmval[0], tmval[1]);
      immEnd();

      immUnbindProgram();
    }

    /* And now, solid lines. */
    uint pos = GPU_vertformat_attr_add(immVertexFormat(), "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
    UNUSED_VARS_NDEBUG(pos); /* silence warning */
    BLI_assert(pos == POS_INDEX);
    immBindBuiltinProgram(GPU_SHADER_2D_UNIFORM_COLOR);

    switch (t->helpline) {
      case HLP_SPRING:
        immUniformThemeColor(TH_VIEW_OVERLAY);

        GPU_matrix_translate_3fv(mval);
        GPU_matrix_rotate_axis(-RAD2DEGF(atan2f(cent[0] - tmval[0], cent[1] - tmval[1])), 'Z');

        GPU_line_width(3.0f);
        drawArrow(UP, 5, 10, 5);
        drawArrow(DOWN, 5, 10, 5);
        break;
      case HLP_HARROW:
        immUniformThemeColor(TH_VIEW_OVERLAY);
        GPU_matrix_translate_3fv(mval);

        GPU_line_width(3.0f);
        drawArrow(RIGHT, 5, 10, 5);
        drawArrow(LEFT, 5, 10, 5);
        break;
      case HLP_VARROW:
        immUniformThemeColor(TH_VIEW_OVERLAY);

        GPU_matrix_translate_3fv(mval);

        GPU_line_width(3.0f);
        drawArrow(UP, 5, 10, 5);
        drawArrow(DOWN, 5, 10, 5);
        break;
      case HLP_CARROW: {
        /* Draw arrow based on direction defined by custom-points. */
        immUniformThemeColor(TH_VIEW_OVERLAY);

        GPU_matrix_translate_3fv(mval);

        GPU_line_width(3.0f);

        const int *data = t->mouse.data;
        const float dx = data[2] - data[0], dy = data[3] - data[1];
        const float angle = -atan2f(dx, dy);

        GPU_matrix_push();

        GPU_matrix_rotate_axis(RAD2DEGF(angle), 'Z');

        drawArrow(UP, 5, 10, 5);
        drawArrow(DOWN, 5, 10, 5);

        GPU_matrix_pop();
        break;
      }
      case HLP_ANGLE: {
        float dx = tmval[0] - cent[0], dy = tmval[1] - cent[1];
        float angle = atan2f(dy, dx);
        float dist = hypotf(dx, dy);
        float delta_angle = min_ff(15.0f / dist, (float)M_PI / 4.0f);
        float spacing_angle = min_ff(5.0f / dist, (float)M_PI / 12.0f);

        immUniformThemeColor(TH_VIEW_OVERLAY);

        GPU_matrix_translate_3f(cent[0] - tmval[0] + mval[0], cent[1] - tmval[1] + mval[1], 0);

        GPU_line_width(3.0f);
        drawArc(dist, angle - delta_angle, angle - spacing_angle, 10);
        drawArc(dist, angle + spacing_angle, angle + delta_angle, 10);

        GPU_matrix_push();

        GPU_matrix_translate_3f(
            cosf(angle - delta_angle) * dist, sinf(angle - delta_angle) * dist, 0);
        GPU_matrix_rotate_axis(RAD2DEGF(angle - delta_angle), 'Z');

        drawArrowHead(DOWN, 5);

        GPU_matrix_pop();

        GPU_matrix_translate_3f(
            cosf(angle + delta_angle) * dist, sinf(angle + delta_angle) * dist, 0);
        GPU_matrix_rotate_axis(RAD2DEGF(angle + delta_angle), 'Z');

        drawArrowHead(UP, 5);
        break;
      }
      case HLP_TRACKBALL: {
        unsigned char col[3], col2[3];
        UI_GetThemeColor3ubv(TH_GRID, col);

        GPU_matrix_translate_3fv(mval);

        GPU_line_width(3.0f);

        UI_make_axis_color(col, col2, 'X');
        immUniformColor3ubv(col2);

        drawArrow(RIGHT, 5, 10, 5);
        drawArrow(LEFT, 5, 10, 5);

        UI_make_axis_color(col, col2, 'Y');
        immUniformColor3ubv(col2);

        drawArrow(UP, 5, 10, 5);
        drawArrow(DOWN, 5, 10, 5);
        break;
      }
    }

    immUnbindProgram();
    GPU_matrix_pop();
  }
}

static bool transinfo_show_overlay(const struct bContext *C, TransInfo *t, ARegion *ar)
{
  /* Don't show overlays when not the active view and when overlay is disabled: T57139 */
  bool ok = false;
  if (ar == t->ar) {
    ok = true;
  }
  else {
    ScrArea *sa = CTX_wm_area(C);
    if (sa->spacetype == SPACE_VIEW3D) {
      View3D *v3d = sa->spacedata.first;
      if ((v3d->flag2 & V3D_HIDE_OVERLAYS) == 0) {
        ok = true;
      }
    }
  }
  return ok;
}

static void drawTransformView(const struct bContext *C, ARegion *ar, void *arg)
{
  TransInfo *t = arg;

  if (!transinfo_show_overlay(C, t, ar)) {
    return;
  }

  GPU_line_width(1.0f);

  drawConstraint(t);
  drawPropCircle(C, t);
  drawSnapping(C, t);

  if (ar == t->ar) {
    /* edge slide, vert slide */
    drawEdgeSlide(t);
    drawVertSlide(t);

    /* Rotation */
    drawDial3d(t);
  }
}

/* just draw a little warning message in the top-right corner of the viewport
 * to warn that autokeying is enabled */
static void drawAutoKeyWarning(TransInfo *UNUSED(t), ARegion *ar)
{
  const char *printable = IFACE_("Auto Keying On");
  float printable_size[2];
  int xco, yco;

  const rcti *rect = ED_region_visible_rect(ar);

  const int font_id = BLF_default();
  BLF_width_and_height(
      font_id, printable, BLF_DRAW_STR_DUMMY_MAX, &printable_size[0], &printable_size[1]);

  xco = (rect->xmax - U.widget_unit) - (int)printable_size[0];
  yco = (rect->ymax - U.widget_unit);

  /* warning text (to clarify meaning of overlays)
   * - original color was red to match the icon, but that clashes badly with a less nasty border
   */
  unsigned char color[3];
  UI_GetThemeColorShade3ubv(TH_TEXT_HI, -50, color);
  BLF_color3ubv(font_id, color);
#ifdef WITH_INTERNATIONAL
  BLF_draw_default(xco, yco, 0.0f, printable, BLF_DRAW_STR_DUMMY_MAX);
#else
  BLF_draw_default_ascii(xco, yco, 0.0f, printable, BLF_DRAW_STR_DUMMY_MAX);
#endif

  /* autokey recording icon... */
  GPU_blend_set_func_separate(
      GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_ONE, GPU_ONE_MINUS_SRC_ALPHA);
  GPU_blend(true);

  xco -= U.widget_unit;
  yco -= (int)printable_size[1] / 2;

  UI_icon_draw(xco, yco, ICON_REC);

  GPU_blend(false);
}

static void drawTransformPixel(const struct bContext *C, ARegion *ar, void *arg)
{
  TransInfo *t = arg;

  if (!transinfo_show_overlay(C, t, ar)) {
    return;
  }

  if (ar == t->ar) {
    Scene *scene = t->scene;
    ViewLayer *view_layer = t->view_layer;
    Object *ob = OBACT(view_layer);

    /* draw auto-key-framing hint in the corner
     * - only draw if enabled (advanced users may be distracted/annoyed),
     *   for objects that will be autokeyframed (no point otherwise),
     *   AND only for the active region (as showing all is too overwhelming)
     */
    if ((U.autokey_flag & AUTOKEY_FLAG_NOWARNING) == 0) {
      if (ar == t->ar) {
        if (t->flag & (T_OBJECT | T_POSE)) {
          if (ob && autokeyframe_cfra_can_key(scene, &ob->id)) {
            drawAutoKeyWarning(t, ar);
          }
        }
      }
    }
  }
}

static void transformApply(bContext *C, TransInfo *t);
static void drawTransformApply(const bContext *C, ARegion *UNUSED(ar), void *arg)
{
  TransInfo *t = arg;

  if (t->redraw & TREDRAW_SOFT) {
    t->redraw |= TREDRAW_HARD;
    transformApply((bContext *)C, t);
  }
}

static void transformApply(bContext *C, TransInfo *t)
{
  t->context = C;

  if ((t->redraw & TREDRAW_HARD) || (t->draw_handle_apply == NULL && (t->redraw & TREDRAW_SOFT))) {
    selectConstraint(t);
    if (t->transform) {
      t->transform(t, t->mval);  // calls recalcData()
      viewRedrawForce(C, t);
    }
    t->redraw = TREDRAW_NOTHING;
  }
  else if (t->redraw & TREDRAW_SOFT) {
    viewRedrawForce(C, t);
  }

  /* If auto confirm is on, break after one pass */
  if (t->options & CTX_AUTOCONFIRM) {
    t->state = TRANS_CONFIRM;
  }

  t->context = NULL;
}

/**
 * \note  caller needs to free 't' on a 0 return
 * \warning  \a event might be NULL (when tweaking from redo panel)
 * \see #saveTransform which writes these values back.
 */
static bool initTransform(
    bContext *C, TransInfo *t, wmOperator *op, const wmEvent *event, int mode)
{
  int options = 0;
  PropertyRNA *prop;

  t->context = C;

  /* added initialize, for external calls to set stuff in TransInfo, like undo string */

  t->state = TRANS_STARTING;

  if ((prop = RNA_struct_find_property(op->ptr, "cursor_transform")) &&
      RNA_property_is_set(op->ptr, prop)) {
    if (RNA_property_boolean_get(op->ptr, prop)) {
      options |= CTX_CURSOR;
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "texture_space")) &&
      RNA_property_is_set(op->ptr, prop)) {
    if (RNA_property_boolean_get(op->ptr, prop)) {
      options |= CTX_TEXTURE;
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "gpencil_strokes")) &&
      RNA_property_is_set(op->ptr, prop)) {
    if (RNA_property_boolean_get(op->ptr, prop)) {
      options |= CTX_GPENCIL_STROKES;
    }
  }

  Object *ob = CTX_data_active_object(C);
  if (ob && ob->mode == OB_MODE_SCULPT && ob->sculpt) {
    options |= CTX_SCULPT;
  }

  t->options = options;

  t->mode = mode;

  /* Needed to translate tweak events to mouse buttons. */
  t->launch_event = event ? WM_userdef_event_type_from_keymap_type(event->type) : -1;

  /* XXX Remove this when wm_operator_call_internal doesn't use window->eventstate
   * (which can have type = 0) */
  /* For gizmo only, so assume LEFTMOUSE. */
  if (t->launch_event == 0) {
    t->launch_event = LEFTMOUSE;
  }

  unit_m3(t->spacemtx);

  initTransInfo(C, t, op, event);
  initTransformOrientation(C, t);

  if (t->spacetype == SPACE_VIEW3D) {
    t->draw_handle_apply = ED_region_draw_cb_activate(
        t->ar->type, drawTransformApply, t, REGION_DRAW_PRE_VIEW);
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_pixel = ED_region_draw_cb_activate(
        t->ar->type, drawTransformPixel, t, REGION_DRAW_POST_PIXEL);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }
  else if (t->spacetype == SPACE_IMAGE) {
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }
  else if (t->spacetype == SPACE_CLIP) {
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }
  else if (t->spacetype == SPACE_NODE) {
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }
  else if (t->spacetype == SPACE_GRAPH) {
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }
  else if (t->spacetype == SPACE_ACTION) {
    t->draw_handle_view = ED_region_draw_cb_activate(
        t->ar->type, drawTransformView, t, REGION_DRAW_POST_VIEW);
    t->draw_handle_cursor = WM_paint_cursor_activate(
        CTX_wm_manager(C), SPACE_TYPE_ANY, RGN_TYPE_ANY, helpline_poll, drawHelpline, t);
  }

  createTransData(C, t);  // make TransData structs from selection

  if (t->options & CTX_SCULPT) {
    ED_sculpt_init_transform(C);
  }

  if (t->data_len_all == 0) {
    postTrans(C, t);
    return 0;
  }

  if (event) {
    /* keymap for shortcut header prints */
    t->keymap = WM_keymap_active(CTX_wm_manager(C), op->type->modalkeymap);

    /* Stupid code to have Ctrl-Click on gizmo work ok.
     *
     * Do this only for translation/rotation/resize because only these
     * modes are available from gizmo and doing such check could
     * lead to keymap conflicts for other modes (see #31584)
     */
    if (ELEM(mode, TFM_TRANSLATION, TFM_ROTATION, TFM_RESIZE)) {
      wmKeyMapItem *kmi;

      for (kmi = t->keymap->items.first; kmi; kmi = kmi->next) {
        if (kmi->flag & KMI_INACTIVE) {
          continue;
        }

        if (kmi->propvalue == TFM_MODAL_SNAP_INV_ON && kmi->val == KM_PRESS) {
          if ((ELEM(kmi->type, LEFTCTRLKEY, RIGHTCTRLKEY) && event->ctrl) ||
              (ELEM(kmi->type, LEFTSHIFTKEY, RIGHTSHIFTKEY) && event->shift) ||
              (ELEM(kmi->type, LEFTALTKEY, RIGHTALTKEY) && event->alt) ||
              ((kmi->type == OSKEY) && event->oskey)) {
            t->modifiers |= MOD_SNAP_INVERT;
          }
          break;
        }
      }
    }
  }

  initSnapping(t, op);  // Initialize snapping data AFTER mode flags

  initSnapSpatial(t, t->snap_spatial);

  /* EVIL! posemode code can switch translation to rotate when 1 bone is selected.
   * will be removed (ton) */

  /* EVIL2: we gave as argument also texture space context bit... was cleared */

  /* EVIL3: extend mode for animation editors also switches modes...
   * but is best way to avoid duplicate code */
  mode = t->mode;

  calculatePropRatio(t);
  calculateCenter(t);

  /* Overwrite initial values if operator supplied a non-null vector.
   *
   * Run before init functions so 'values_modal_offset' can be applied on mouse input.
   */
  BLI_assert(is_zero_v4(t->values_modal_offset));
  if ((prop = RNA_struct_find_property(op->ptr, "value")) && RNA_property_is_set(op->ptr, prop)) {
    float values[4] = {0}; /* in case value isn't length 4, avoid uninitialized memory  */

    if (RNA_property_array_check(prop)) {
      RNA_float_get_array(op->ptr, "value", values);
    }
    else {
      values[0] = RNA_float_get(op->ptr, "value");
    }

    copy_v4_v4(t->values, values);

    if (t->flag & T_MODAL) {
      copy_v4_v4(t->values_modal_offset, values);
      t->redraw = TREDRAW_HARD;
    }
    else {
      copy_v4_v4(t->values, values);
      t->flag |= T_INPUT_IS_VALUES_FINAL;
    }
  }

  if (event) {
    /* Initialize accurate transform to settings requested by keymap. */
    bool use_accurate = false;
    if ((prop = RNA_struct_find_property(op->ptr, "use_accurate")) &&
        RNA_property_is_set(op->ptr, prop)) {
      if (RNA_property_boolean_get(op->ptr, prop)) {
        use_accurate = true;
      }
    }
    initMouseInput(t, &t->mouse, t->center2d, event->mval, use_accurate);
  }

  switch (mode) {
    case TFM_TRANSLATION:
      initTranslation(t);
      break;
    case TFM_ROTATION:
      initRotation(t);
      break;
    case TFM_RESIZE:
      initResize(t);
      break;
    case TFM_SKIN_RESIZE:
      initSkinResize(t);
      break;
    case TFM_TOSPHERE:
      initToSphere(t);
      break;
    case TFM_SHEAR:
      initShear(t);
      break;
    case TFM_BEND:
      initBend(t);
      break;
    case TFM_SHRINKFATTEN:
      initShrinkFatten(t);
      break;
    case TFM_TILT:
      initTilt(t);
      break;
    case TFM_CURVE_SHRINKFATTEN:
      initCurveShrinkFatten(t);
      break;
    case TFM_MASK_SHRINKFATTEN:
      initMaskShrinkFatten(t);
      break;
    case TFM_GPENCIL_SHRINKFATTEN:
      initGPShrinkFatten(t);
      break;
    case TFM_TRACKBALL:
      initTrackball(t);
      break;
    case TFM_PUSHPULL:
      initPushPull(t);
      break;
    case TFM_CREASE:
      initCrease(t);
      break;
    case TFM_BONESIZE: { /* used for both B-Bone width (bonesize) as for deform-dist (envelope) */
      /* Note: we have to pick one, use the active object. */
      TransDataContainer *tc = TRANS_DATA_CONTAINER_FIRST_OK(t);
      bArmature *arm = tc->poseobj->data;
      if (arm->drawtype == ARM_ENVELOPE) {
        initBoneEnvelope(t);
        t->mode = TFM_BONE_ENVELOPE_DIST;
      }
      else {
        initBoneSize(t);
      }
      break;
    }
    case TFM_BONE_ENVELOPE:
      initBoneEnvelope(t);
      break;
    case TFM_BONE_ENVELOPE_DIST:
      initBoneEnvelope(t);
      t->mode = TFM_BONE_ENVELOPE_DIST;
      break;
    case TFM_EDGE_SLIDE:
    case TFM_VERT_SLIDE: {
      const bool use_even = (op ? RNA_boolean_get(op->ptr, "use_even") : false);
      const bool flipped = (op ? RNA_boolean_get(op->ptr, "flipped") : false);
      const bool use_clamp = (op ? RNA_boolean_get(op->ptr, "use_clamp") : true);
      if (mode == TFM_EDGE_SLIDE) {
        const bool use_double_side = (op ? !RNA_boolean_get(op->ptr, "single_side") : true);
        initEdgeSlide_ex(t, use_double_side, use_even, flipped, use_clamp);
      }
      else {
        initVertSlide_ex(t, use_even, flipped, use_clamp);
      }
      break;
    }
    case TFM_BONE_ROLL:
      initBoneRoll(t);
      break;
    case TFM_TIME_TRANSLATE:
      initTimeTranslate(t);
      break;
    case TFM_TIME_SLIDE:
      initTimeSlide(t);
      break;
    case TFM_TIME_SCALE:
      initTimeScale(t);
      break;
    case TFM_TIME_DUPLICATE:
      /* same as TFM_TIME_EXTEND, but we need the mode info for later
       * so that duplicate-culling will work properly
       */
      if (ELEM(t->spacetype, SPACE_GRAPH, SPACE_NLA)) {
        initTranslation(t);
      }
      else {
        initTimeTranslate(t);
      }
      t->mode = mode;
      break;
    case TFM_TIME_EXTEND:
      /* now that transdata has been made, do like for TFM_TIME_TRANSLATE (for most Animation
       * Editors because they have only 1D transforms for time values) or TFM_TRANSLATION
       * (for Graph/NLA Editors only since they uses 'standard' transforms to get 2D movement)
       * depending on which editor this was called from
       */
      if (ELEM(t->spacetype, SPACE_GRAPH, SPACE_NLA)) {
        initTranslation(t);
      }
      else {
        initTimeTranslate(t);
      }
      break;
    case TFM_BAKE_TIME:
      initBakeTime(t);
      break;
    case TFM_MIRROR:
      initMirror(t);
      break;
    case TFM_BWEIGHT:
      initBevelWeight(t);
      break;
    case TFM_ALIGN:
      initAlign(t);
      break;
    case TFM_SEQ_SLIDE:
      initSeqSlide(t);
      break;
    case TFM_NORMAL_ROTATION:
      initNormalRotation(t);
      break;
    case TFM_GPENCIL_OPACITY:
      initGPOpacity(t);
      break;
  }

  if (t->state == TRANS_CANCEL) {
    postTrans(C, t);
    return 0;
  }

  /* Transformation axis from operator */
  if ((prop = RNA_struct_find_property(op->ptr, "orient_axis")) &&
      RNA_property_is_set(op->ptr, prop)) {
    t->orient_axis = RNA_property_enum_get(op->ptr, prop);
  }
  if ((prop = RNA_struct_find_property(op->ptr, "orient_axis_ortho")) &&
      RNA_property_is_set(op->ptr, prop)) {
    t->orient_axis_ortho = RNA_property_enum_get(op->ptr, prop);
  }

  /* Constraint init from operator */
  if ((t->flag & T_MODAL) ||
      /* For mirror operator the constraint axes are effectively the values. */
      (RNA_struct_find_property(op->ptr, "value") == NULL)) {
    if ((prop = RNA_struct_find_property(op->ptr, "constraint_axis")) &&
        RNA_property_is_set(op->ptr, prop)) {
      bool constraint_axis[3];

      RNA_property_boolean_get_array(op->ptr, prop, constraint_axis);

      if (constraint_axis[0] || constraint_axis[1] || constraint_axis[2]) {
        t->con.mode |= CON_APPLY;

        if (constraint_axis[0]) {
          t->con.mode |= CON_AXIS0;
        }
        if (constraint_axis[1]) {
          t->con.mode |= CON_AXIS1;
        }
        if (constraint_axis[2]) {
          t->con.mode |= CON_AXIS2;
        }

        setUserConstraint(t, t->orientation.user, t->con.mode, "%s");
      }
    }
  }
  else {
    /* So we can adjust in non global orientation. */
    if (t->orientation.user != V3D_ORIENT_GLOBAL) {
      t->con.mode |= CON_APPLY | CON_AXIS0 | CON_AXIS1 | CON_AXIS2;
      setUserConstraint(t, t->orientation.user, t->con.mode, "%s");
    }
  }

  /* Don't write into the values when non-modal because they are already set from operator redo
   * values. */
  if (t->flag & T_MODAL) {
    /* Setup the mouse input with initial values. */
    applyMouseInput(t, &t->mouse, t->mouse.imval, t->values);
  }

  if ((prop = RNA_struct_find_property(op->ptr, "preserve_clnor"))) {
    if ((t->flag & T_EDIT) && t->obedit_type == OB_MESH) {

      FOREACH_TRANS_DATA_CONTAINER (t, tc) {
        if ((((Mesh *)(tc->obedit->data))->flag & ME_AUTOSMOOTH)) {
          BMEditMesh *em = NULL;  // BKE_editmesh_from_object(t->obedit);
          bool do_skip = false;

          /* Currently only used for two of three most frequent transform ops,
           * can include more ops.
           * Note that scaling cannot be included here,
           * non-uniform scaling will affect normals. */
          if (ELEM(t->mode, TFM_TRANSLATION, TFM_ROTATION)) {
            if (em->bm->totvertsel == em->bm->totvert) {
              /* No need to invalidate if whole mesh is selected. */
              do_skip = true;
            }
          }

          if (t->flag & T_MODAL) {
            RNA_property_boolean_set(op->ptr, prop, false);
          }
          else if (!do_skip) {
            const bool preserve_clnor = RNA_property_boolean_get(op->ptr, prop);
            if (preserve_clnor) {
              BKE_editmesh_lnorspace_update(em);
              t->flag |= T_CLNOR_REBUILD;
            }
            BM_lnorspace_invalidate(em->bm, true);
          }
        }
      }
    }
  }

  t->context = NULL;

  return 1;
}

/**
 * \see #initTransform which reads values from the operator.
 */
static void saveTransform(bContext *C, TransInfo *t, wmOperator *op)
{
  ToolSettings *ts = CTX_data_tool_settings(C);
  int proportional = 0;
  PropertyRNA *prop;

  // Save back mode in case we're in the generic operator
  if ((prop = RNA_struct_find_property(op->ptr, "mode"))) {
    RNA_property_enum_set(op->ptr, prop, t->mode);
  }

  if ((prop = RNA_struct_find_property(op->ptr, "value"))) {
    if (RNA_property_array_check(prop)) {
      RNA_property_float_set_array(op->ptr, prop, t->values_final);
    }
    else {
      RNA_property_float_set(op->ptr, prop, t->values_final[0]);
    }
  }

  if (t->flag & T_PROP_EDIT_ALL) {
    if (t->flag & T_PROP_EDIT) {
      proportional |= PROP_EDIT_USE;
    }
    if (t->flag & T_PROP_CONNECTED) {
      proportional |= PROP_EDIT_CONNECTED;
    }
    if (t->flag & T_PROP_PROJECTED) {
      proportional |= PROP_EDIT_PROJECTED;
    }
  }

  // If modal, save settings back in scene if not set as operator argument
  if ((t->flag & T_MODAL) || (op->flag & OP_IS_REPEAT)) {
    /* save settings if not set in operator */

    /* skip saving proportional edit if it was not actually used */
    if (!(t->options & CTX_NO_PET)) {
      if ((prop = RNA_struct_find_property(op->ptr, "use_proportional_edit")) &&
          !RNA_property_is_set(op->ptr, prop)) {
        if (t->spacetype == SPACE_GRAPH) {
          ts->proportional_fcurve = proportional;
        }
        else if (t->spacetype == SPACE_ACTION) {
          ts->proportional_action = proportional;
        }
        else if (t->obedit_type != -1) {
          ts->proportional_edit = proportional;
        }
        else if (t->options & CTX_MASK) {
          ts->proportional_mask = proportional != 0;
        }
        else {
          ts->proportional_objects = proportional != 0;
        }
      }

      if ((prop = RNA_struct_find_property(op->ptr, "proportional_size"))) {
        ts->proportional_size = RNA_property_is_set(op->ptr, prop) ?
                                    RNA_property_float_get(op->ptr, prop) :
                                    t->prop_size;
      }

      if ((prop = RNA_struct_find_property(op->ptr, "proportional_edit_falloff")) &&
          !RNA_property_is_set(op->ptr, prop)) {
        ts->prop_mode = t->prop_mode;
      }
    }

    if (t->spacetype == SPACE_VIEW3D) {
      if ((prop = RNA_struct_find_property(op->ptr, "orient_type")) &&
          !RNA_property_is_set(op->ptr, prop) &&
          (t->orientation.user != V3D_ORIENT_CUSTOM_MATRIX)) {
        TransformOrientationSlot *orient_slot = &t->scene->orientation_slots[SCE_ORIENT_DEFAULT];
        orient_slot->type = t->orientation.user;
        BLI_assert(((orient_slot->index_custom == -1) && (t->orientation.custom == NULL)) ||
                   (BKE_scene_transform_orientation_get_index(t->scene, t->orientation.custom) ==
                    orient_slot->index_custom));
      }
    }
  }

  if (t->flag & T_MODAL) {
    /* do we check for parameter? */
    if (transformModeUseSnap(t)) {
      if (t->modifiers & MOD_SNAP) {
        ts->snap_flag |= SCE_SNAP;
      }
      else {
        ts->snap_flag &= ~SCE_SNAP;
      }
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "use_proportional_edit"))) {
    RNA_property_boolean_set(op->ptr, prop, proportional & PROP_EDIT_USE);
    RNA_boolean_set(op->ptr, "use_proportional_connected", proportional & PROP_EDIT_CONNECTED);
    RNA_boolean_set(op->ptr, "use_proportional_projected", proportional & PROP_EDIT_PROJECTED);
    RNA_enum_set(op->ptr, "proportional_edit_falloff", t->prop_mode);
    RNA_float_set(op->ptr, "proportional_size", t->prop_size);
  }

  if ((prop = RNA_struct_find_property(op->ptr, "mirror"))) {
    RNA_property_boolean_set(op->ptr, prop, (t->flag & T_NO_MIRROR) == 0);
  }

  /* Orientation used for redo. */
  const bool use_orient_axis = (t->orient_matrix_is_set &&
                                (RNA_struct_find_property(op->ptr, "orient_axis") != NULL));
  short orientation;
  if (t->con.mode & CON_APPLY) {
    orientation = t->con.orientation;
    if (orientation == V3D_ORIENT_CUSTOM) {
      const int orientation_index_custom = BKE_scene_transform_orientation_get_index(
          t->scene, t->orientation.custom);
      /* Maybe we need a t->con.custom_orientation?
       * Seems like it would always match t->orientation.custom. */
      orientation = V3D_ORIENT_CUSTOM + orientation_index_custom;
      BLI_assert(orientation >= V3D_ORIENT_CUSTOM);
    }
  }
  else if ((t->orientation.user == V3D_ORIENT_CUSTOM_MATRIX) &&
           (prop = RNA_struct_find_property(op->ptr, "orient_matrix_type"))) {
    orientation = RNA_property_enum_get(op->ptr, prop);
  }
  else if (use_orient_axis) {
    /* We're not using an orientation, use the fallback. */
    orientation = t->orientation.unset;
  }
  else {
    orientation = V3D_ORIENT_GLOBAL;
  }

  if ((prop = RNA_struct_find_property(op->ptr, "orient_axis"))) {
    if (t->flag & T_MODAL) {
      if (t->con.mode & CON_APPLY) {
        int orient_axis = constraintModeToIndex(t);
        if (orient_axis != -1) {
          RNA_property_enum_set(op->ptr, prop, orient_axis);
        }
      }
      else {
        RNA_property_enum_set(op->ptr, prop, t->orient_axis);
      }
    }
  }
  if ((prop = RNA_struct_find_property(op->ptr, "orient_axis_ortho"))) {
    if (t->flag & T_MODAL) {
      RNA_property_enum_set(op->ptr, prop, t->orient_axis_ortho);
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "orient_matrix"))) {
    if (t->flag & T_MODAL) {
      if (orientation != V3D_ORIENT_CUSTOM_MATRIX) {
        if (t->flag & T_MODAL) {
          RNA_enum_set(op->ptr, "orient_matrix_type", orientation);
        }
      }
      if (t->con.mode & CON_APPLY) {
        RNA_float_set_array(op->ptr, "orient_matrix", &t->con.mtx[0][0]);
      }
      else if (use_orient_axis) {
        RNA_float_set_array(op->ptr, "orient_matrix", &t->orient_matrix[0][0]);
      }
      else {
        RNA_float_set_array(op->ptr, "orient_matrix", &t->spacemtx[0][0]);
      }
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "orient_type"))) {
    /* constraint orientation can be global, even if user selects something else
     * so use the orientation in the constraint if set */

    /* Use 'orient_matrix' instead. */
    if (t->flag & T_MODAL) {
      if (orientation != V3D_ORIENT_CUSTOM_MATRIX) {
        RNA_property_enum_set(op->ptr, prop, orientation);
      }
    }
  }

  if ((prop = RNA_struct_find_property(op->ptr, "constraint_axis"))) {
    bool constraint_axis[3] = {false, false, false};
    if (t->flag & T_MODAL) {
      /* Only set if needed, so we can hide in the UI when nothing is set.
       * See 'transform_poll_property'. */
      if (t->con.mode & CON_APPLY) {
        if (t->con.mode & CON_AXIS0) {
          constraint_axis[0] = true;
        }
        if (t->con.mode & CON_AXIS1) {
          constraint_axis[1] = true;
        }
        if (t->con.mode & CON_AXIS2) {
          constraint_axis[2] = true;
        }
      }
      if (ELEM(true, UNPACK3(constraint_axis))) {
        RNA_property_boolean_set_array(op->ptr, prop, constraint_axis);
      }
    }
  }

  {
    const char *prop_id = NULL;
    bool prop_state = true;
    if (t->mode == TFM_SHRINKFATTEN) {
      prop_id = "use_even_offset";
      prop_state = false;
    }

    if (prop_id && (prop = RNA_struct_find_property(op->ptr, prop_id))) {
      RNA_property_boolean_set(op->ptr, prop, ((t->flag & T_ALT_TRANSFORM) == 0) == prop_state);
    }
  }

  if (t->options & CTX_SCULPT) {
    ED_sculpt_end_transform(C);
  }

  if ((prop = RNA_struct_find_property(op->ptr, "correct_uv"))) {
    RNA_property_boolean_set(
        op->ptr, prop, (t->settings->uvcalc_flag & UVCALC_TRANSFORM_CORRECT) != 0);
  }
}

#ifdef USE_LOOPSLIDE_HACK
/**
 * Special hack for MESH_OT_loopcut_slide so we get back to the selection mode
 */
static void transformops_loopsel_hack(bContext *C, wmOperator *op)
{
  if (op->type->idname == OP_EDGE_SLIDE) {
    if (op->opm && op->opm->opm && op->opm->opm->prev) {
      wmOperator *op_prev = op->opm->opm->prev;
      Scene *scene = CTX_data_scene(C);
      bool mesh_select_mode[3];
      PropertyRNA *prop = RNA_struct_find_property(op_prev->ptr, "mesh_select_mode_init");

      if (prop && RNA_property_is_set(op_prev->ptr, prop)) {
        ToolSettings *ts = scene->toolsettings;
        short selectmode_orig;

        RNA_property_boolean_get_array(op_prev->ptr, prop, mesh_select_mode);
        selectmode_orig = ((mesh_select_mode[0] ? SCE_SELECT_VERTEX : 0) |
                           (mesh_select_mode[1] ? SCE_SELECT_EDGE : 0) |
                           (mesh_select_mode[2] ? SCE_SELECT_FACE : 0));

        /* still switch if we were originally in face select mode */
        if ((ts->selectmode != selectmode_orig) && (selectmode_orig != SCE_SELECT_FACE)) {
          Object *obedit = CTX_data_edit_object(C);
          BMEditMesh *em = BKE_editmesh_from_object(obedit);
          em->selectmode = ts->selectmode = selectmode_orig;
          EDBM_selectmode_set(em);
        }
      }
    }
  }
}
#else
/* prevent removal by cleanup */
#  error "loopslide hack removed!"
#endif /* USE_LOOPSLIDE_HACK */

static void transformops_exit(bContext *C, wmOperator *op)
{
#ifdef USE_LOOPSLIDE_HACK
  transformops_loopsel_hack(C, op);
#endif

  saveTransform(C, op->customdata, op);
  MEM_freeN(op->customdata);
  op->customdata = NULL;
  G.moving = 0;
}

static int transformops_data(bContext *C, wmOperator *op, const wmEvent *event)
{
  int retval = 1;
  if (op->customdata == NULL) {
    TransInfo *t = MEM_callocN(sizeof(TransInfo), "TransInfo data2");
    TransformModeItem *tmode;
    int mode = -1;

    for (tmode = transform_modes; tmode->idname; tmode++) {
      if (op->type->idname == tmode->idname) {
        mode = tmode->mode;
        break;
      }
    }

    if (mode == -1) {
      mode = RNA_enum_get(op->ptr, "mode");
    }

    retval = initTransform(C, t, op, event, mode);

    /* store data */
    if (retval) {
      G.moving = special_transform_moving(t);
      op->customdata = t;
    }
    else {
      MEM_freeN(t);
    }
  }

  return retval; /* return 0 on error */
}

static int transformEnd(bContext *C, TransInfo *t)
{
  int exit_code = OPERATOR_RUNNING_MODAL;

  t->context = C;

  if (t->state != TRANS_STARTING && t->state != TRANS_RUNNING) {
    /* handle restoring objects */
    if (t->state == TRANS_CANCEL) {
      /* exception, edge slide transformed UVs too */
      if (t->mode == TFM_EDGE_SLIDE) {
        doEdgeSlide(t, 0.0f);
      }
      else if (t->mode == TFM_VERT_SLIDE) {
        doVertSlide(t, 0.0f);
      }

      exit_code = OPERATOR_CANCELLED;
      restoreTransObjects(t);  // calls recalcData()
    }
    else {
      if (t->flag & T_CLNOR_REBUILD) {
        FOREACH_TRANS_DATA_CONTAINER (t, tc) {
          BMEditMesh *em = BKE_editmesh_from_object(tc->obedit);
          BM_lnorspace_rebuild(em->bm, true);
        }
      }
      exit_code = OPERATOR_FINISHED;
    }

    /* aftertrans does insert keyframes, and clears base flags; doesn't read transdata */
    special_aftertrans_update(C, t);

    /* free data */
    postTrans(C, t);

    /* send events out for redraws */
    viewRedrawPost(C, t);

    viewRedrawForce(C, t);
  }

  t->context = NULL;

  return exit_code;
}

static void transform_event_xyz_constraint(TransInfo *t, short key_type, char cmode, bool is_plane)
{
  if (!(t->flag & T_NO_CONSTRAINT)) {
    int constraint_axis, constraint_plane;
    const bool edit_2d = (t->flag & T_2D_EDIT) != 0;
    const char *msg1 = "", *msg2 = "", *msg3 = "";
    char axis;

    /* Initialize */
    switch (key_type) {
      case XKEY:
        msg1 = TIP_("along X");
        msg2 = TIP_("along %s X");
        msg3 = TIP_("locking %s X");
        axis = 'X';
        constraint_axis = CON_AXIS0;
        break;
      case YKEY:
        msg1 = TIP_("along Y");
        msg2 = TIP_("along %s Y");
        msg3 = TIP_("locking %s Y");
        axis = 'Y';
        constraint_axis = CON_AXIS1;
        break;
      case ZKEY:
        msg1 = TIP_("along Z");
        msg2 = TIP_("along %s Z");
        msg3 = TIP_("locking %s Z");
        axis = 'Z';
        constraint_axis = CON_AXIS2;
        break;
      default:
        /* Invalid key */
        return;
    }
    constraint_plane = ((CON_AXIS0 | CON_AXIS1 | CON_AXIS2) & (~constraint_axis));

    if (edit_2d && (key_type != ZKEY)) {
      if (cmode == axis) {
        stopConstraint(t);
      }
      else {
        setUserConstraint(t, V3D_ORIENT_GLOBAL, constraint_axis, msg1);
      }
    }
    else if (!edit_2d) {
      if (cmode != axis) {
        /* First press, constraint to an axis. */
        t->orientation.index = 0;
        const short *orientation_ptr = t->orientation.types[t->orientation.index];
        const short orientation = orientation_ptr ? *orientation_ptr : V3D_ORIENT_GLOBAL;
        if (is_plane == false) {
          setUserConstraint(t, orientation, constraint_axis, msg2);
        }
        else {
          setUserConstraint(t, orientation, constraint_plane, msg3);
        }
      }
      else {
        /* Successive presses on existing axis, cycle orientation modes. */
        t->orientation.index = (t->orientation.index + 1) % ARRAY_SIZE(t->orientation.types);

        if (t->orientation.index == 0) {
          stopConstraint(t);
        }
        else {
          const short *orientation_ptr = t->orientation.types[t->orientation.index];
          const short orientation = orientation_ptr ? *orientation_ptr : V3D_ORIENT_GLOBAL;
          if (is_plane == false) {
            setUserConstraint(t, orientation, constraint_axis, msg2);
          }
          else {
            setUserConstraint(t, orientation, constraint_plane, msg3);
          }
        }
      }
    }
    t->redraw |= TREDRAW_HARD;
  }
}

static int transformEvent(TransInfo *t, const wmEvent *event)
{
  char cmode = constraintModeToChar(t);
  bool handled = false;
  const int modifiers_prev = t->modifiers;
  const int mode_prev = t->mode;

  t->redraw |= handleMouseInput(t, &t->mouse, event);

  /* Handle modal numinput events first, if already activated. */
  if (((event->val == KM_PRESS) || (event->type == EVT_MODAL_MAP)) && hasNumInput(&t->num) &&
      handleNumInput(t->context, &(t->num), event)) {
    t->redraw |= TREDRAW_HARD;
    handled = true;
  }
  else if (event->type == MOUSEMOVE) {
    if (t->modifiers & MOD_CONSTRAINT_SELECT) {
      t->con.mode |= CON_SELECT;
    }

    copy_v2_v2_int(t->mval, event->mval);

    /* Use this for soft redraw. Might cause flicker in object mode */
    // t->redraw |= TREDRAW_SOFT;
    t->redraw |= TREDRAW_HARD;

    if (t->state == TRANS_STARTING) {
      t->state = TRANS_RUNNING;
    }

    applyMouseInput(t, &t->mouse, t->mval, t->values);

    // Snapping mouse move events
    t->redraw |= handleSnapping(t, event);
    handled = true;
  }
  /* handle modal keymap first */
  else if (event->type == EVT_MODAL_MAP) {
    switch (event->val) {
      case TFM_MODAL_CANCEL:
        t->state = TRANS_CANCEL;
        handled = true;
        break;
      case TFM_MODAL_CONFIRM:
        t->state = TRANS_CONFIRM;
        handled = true;
        break;
      case TFM_MODAL_TRANSLATE:
        /* only switch when... */
        if (ELEM(t->mode,
                 TFM_ROTATION,
                 TFM_RESIZE,
                 TFM_TRACKBALL,
                 TFM_EDGE_SLIDE,
                 TFM_VERT_SLIDE)) {
          restoreTransObjects(t);
          resetTransModal(t);
          resetTransRestrictions(t);
          initTranslation(t);
          initSnapping(t, NULL);  // need to reinit after mode change
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        else if (t->mode == TFM_SEQ_SLIDE) {
          t->flag ^= T_ALT_TRANSFORM;
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        else {
          if (t->obedit_type == OB_MESH) {
            if ((t->mode == TFM_TRANSLATION) && (t->spacetype == SPACE_VIEW3D)) {
              restoreTransObjects(t);
              resetTransModal(t);
              resetTransRestrictions(t);

              /* first try edge slide */
              initEdgeSlide(t);
              /* if that fails, do vertex slide */
              if (t->state == TRANS_CANCEL) {
                resetTransModal(t);
                t->state = TRANS_STARTING;
                initVertSlide(t);
              }
              /* vert slide can fail on unconnected vertices (rare but possible) */
              if (t->state == TRANS_CANCEL) {
                resetTransModal(t);
                t->mode = TFM_TRANSLATION;
                t->state = TRANS_STARTING;
                restoreTransObjects(t);
                resetTransRestrictions(t);
                initTranslation(t);
              }
              initSnapping(t, NULL);  // need to reinit after mode change
              t->redraw |= TREDRAW_HARD;
              handled = true;
            }
          }
          else if (t->options & (CTX_MOVIECLIP | CTX_MASK)) {
            if (t->mode == TFM_TRANSLATION) {
              restoreTransObjects(t);

              t->flag ^= T_ALT_TRANSFORM;
              t->redraw |= TREDRAW_HARD;
              handled = true;
            }
          }
        }
        break;
      case TFM_MODAL_ROTATE:
        /* only switch when... */
        if (!(t->options & CTX_TEXTURE) && !(t->options & (CTX_MOVIECLIP | CTX_MASK))) {
          if (ELEM(t->mode,
                   TFM_ROTATION,
                   TFM_RESIZE,
                   TFM_TRACKBALL,
                   TFM_TRANSLATION,
                   TFM_EDGE_SLIDE,
                   TFM_VERT_SLIDE)) {
            restoreTransObjects(t);
            resetTransModal(t);
            resetTransRestrictions(t);

            if (t->mode == TFM_ROTATION) {
              initTrackball(t);
            }
            else {
              initRotation(t);
            }
            initSnapping(t, NULL);  // need to reinit after mode change
            t->redraw |= TREDRAW_HARD;
            handled = true;
          }
        }
        break;
      case TFM_MODAL_RESIZE:
        /* only switch when... */
        if (ELEM(t->mode,
                 TFM_ROTATION,
                 TFM_TRANSLATION,
                 TFM_TRACKBALL,
                 TFM_EDGE_SLIDE,
                 TFM_VERT_SLIDE)) {

          /* Scale isn't normally very useful after extrude along normals, see T39756 */
          if ((t->con.mode & CON_APPLY) && (t->con.orientation == V3D_ORIENT_NORMAL)) {
            stopConstraint(t);
          }

          restoreTransObjects(t);
          resetTransModal(t);
          resetTransRestrictions(t);
          initResize(t);
          initSnapping(t, NULL);  // need to reinit after mode change
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        else if (t->mode == TFM_SHRINKFATTEN) {
          t->flag ^= T_ALT_TRANSFORM;
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        else if (t->mode == TFM_RESIZE) {
          if (t->options & CTX_MOVIECLIP) {
            restoreTransObjects(t);

            t->flag ^= T_ALT_TRANSFORM;
            t->redraw |= TREDRAW_HARD;
            handled = true;
          }
        }
        break;

      case TFM_MODAL_SNAP_INV_ON:
        t->modifiers |= MOD_SNAP_INVERT;
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;
      case TFM_MODAL_SNAP_INV_OFF:
        t->modifiers &= ~MOD_SNAP_INVERT;
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;
      case TFM_MODAL_SNAP_TOGGLE:
        t->modifiers ^= MOD_SNAP;
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;
      case TFM_MODAL_AXIS_X:
        if (!(t->flag & T_NO_CONSTRAINT)) {
          transform_event_xyz_constraint(t, XKEY, cmode, false);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_AXIS_Y:
        if ((t->flag & T_NO_CONSTRAINT) == 0) {
          transform_event_xyz_constraint(t, YKEY, cmode, false);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_AXIS_Z:
        if ((t->flag & (T_NO_CONSTRAINT)) == 0) {
          transform_event_xyz_constraint(t, ZKEY, cmode, false);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_PLANE_X:
        if ((t->flag & (T_NO_CONSTRAINT | T_2D_EDIT)) == 0) {
          transform_event_xyz_constraint(t, XKEY, cmode, true);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_PLANE_Y:
        if ((t->flag & (T_NO_CONSTRAINT | T_2D_EDIT)) == 0) {
          transform_event_xyz_constraint(t, YKEY, cmode, true);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_PLANE_Z:
        if ((t->flag & (T_NO_CONSTRAINT | T_2D_EDIT)) == 0) {
          transform_event_xyz_constraint(t, ZKEY, cmode, true);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_CONS_OFF:
        if ((t->flag & T_NO_CONSTRAINT) == 0) {
          stopConstraint(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_ADD_SNAP:
        addSnapPoint(t);
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;
      case TFM_MODAL_REMOVE_SNAP:
        removeSnapPoint(t);
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;
      case TFM_MODAL_PROPSIZE:
        /* MOUSEPAN usage... */
        if (t->flag & T_PROP_EDIT) {
          float fac = 1.0f + 0.005f * (event->y - event->prevy);
          t->prop_size *= fac;
          if (t->spacetype == SPACE_VIEW3D && t->persp != RV3D_ORTHO) {
            t->prop_size = max_ff(min_ff(t->prop_size, ((View3D *)t->view)->clip_end),
                                  T_PROP_SIZE_MIN);
          }
          else {
            t->prop_size = max_ff(min_ff(t->prop_size, T_PROP_SIZE_MAX), T_PROP_SIZE_MIN);
          }
          calculatePropRatio(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_PROPSIZE_UP:
        if (t->flag & T_PROP_EDIT) {
          t->prop_size *= (t->modifiers & MOD_PRECISION) ? 1.01f : 1.1f;
          if (t->spacetype == SPACE_VIEW3D && t->persp != RV3D_ORTHO) {
            t->prop_size = min_ff(t->prop_size, ((View3D *)t->view)->clip_end);
          }
          else {
            t->prop_size = min_ff(t->prop_size, T_PROP_SIZE_MAX);
          }
          calculatePropRatio(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_PROPSIZE_DOWN:
        if (t->flag & T_PROP_EDIT) {
          t->prop_size /= (t->modifiers & MOD_PRECISION) ? 1.01f : 1.1f;
          t->prop_size = max_ff(t->prop_size, T_PROP_SIZE_MIN);
          calculatePropRatio(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_AUTOIK_LEN_INC:
        if (t->flag & T_AUTOIK) {
          transform_autoik_update(t, 1);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_AUTOIK_LEN_DEC:
        if (t->flag & T_AUTOIK) {
          transform_autoik_update(t, -1);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case TFM_MODAL_INSERTOFS_TOGGLE_DIR:
        if (t->spacetype == SPACE_NODE) {
          SpaceNode *snode = (SpaceNode *)t->sa->spacedata.first;

          BLI_assert(t->sa->spacetype == t->spacetype);

          if (snode->insert_ofs_dir == SNODE_INSERTOFS_DIR_RIGHT) {
            snode->insert_ofs_dir = SNODE_INSERTOFS_DIR_LEFT;
          }
          else if (snode->insert_ofs_dir == SNODE_INSERTOFS_DIR_LEFT) {
            snode->insert_ofs_dir = SNODE_INSERTOFS_DIR_RIGHT;
          }
          else {
            BLI_assert(0);
          }

          t->redraw |= TREDRAW_SOFT;
        }
        break;
      /* Those two are only handled in transform's own handler, see T44634! */
      case TFM_MODAL_EDGESLIDE_UP:
      case TFM_MODAL_EDGESLIDE_DOWN:
      default:
        break;
    }
  }
  /* else do non-mapped events */
  else if (event->val == KM_PRESS) {
    switch (event->type) {
      case RIGHTMOUSE:
        t->state = TRANS_CANCEL;
        handled = true;
        break;
      /* enforce redraw of transform when modifiers are used */
      case LEFTSHIFTKEY:
      case RIGHTSHIFTKEY:
        t->modifiers |= MOD_CONSTRAINT_PLANE;
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;

      case SPACEKEY:
        t->state = TRANS_CONFIRM;
        handled = true;
        break;

      case MIDDLEMOUSE:
        if ((t->flag & T_NO_CONSTRAINT) == 0) {
          /* exception for switching to dolly, or trackball, in camera view */
          if (t->flag & T_CAMERA) {
            if (t->mode == TFM_TRANSLATION) {
              setLocalConstraint(t, (CON_AXIS2), TIP_("along local Z"));
            }
            else if (t->mode == TFM_ROTATION) {
              restoreTransObjects(t);
              initTrackball(t);
            }
          }
          else {
            t->modifiers |= MOD_CONSTRAINT_SELECT;
            if (t->con.mode & CON_APPLY) {
              stopConstraint(t);
            }
            else {
              if (event->shift) {
                /* bit hackish... but it prevents mmb select to print the
                 * orientation from menu */
                float mati[3][3];
                strcpy(t->spacename, "global");
                unit_m3(mati);
                initSelectConstraint(t, mati);
              }
              else {
                initSelectConstraint(t, t->spacemtx);
              }
              postSelectConstraint(t);
            }
          }
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case ESCKEY:
        t->state = TRANS_CANCEL;
        handled = true;
        break;
      case PADENTER:
      case RETKEY:
        t->state = TRANS_CONFIRM;
        handled = true;
        break;
      case GKEY:
        /* only switch when... */
        if (ELEM(t->mode, TFM_ROTATION, TFM_RESIZE, TFM_TRACKBALL)) {
          restoreTransObjects(t);
          resetTransModal(t);
          resetTransRestrictions(t);
          initTranslation(t);
          initSnapping(t, NULL);  // need to reinit after mode change
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case SKEY:
        /* only switch when... */
        if (ELEM(t->mode, TFM_ROTATION, TFM_TRANSLATION, TFM_TRACKBALL)) {
          restoreTransObjects(t);
          resetTransModal(t);
          resetTransRestrictions(t);
          initResize(t);
          initSnapping(t, NULL);  // need to reinit after mode change
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case RKEY:
        /* only switch when... */
        if (!(t->options & CTX_TEXTURE)) {
          if (ELEM(t->mode, TFM_ROTATION, TFM_RESIZE, TFM_TRACKBALL, TFM_TRANSLATION)) {
            restoreTransObjects(t);
            resetTransModal(t);
            resetTransRestrictions(t);

            if (t->mode == TFM_ROTATION) {
              initTrackball(t);
            }
            else {
              initRotation(t);
            }
            initSnapping(t, NULL);  // need to reinit after mode change
            t->redraw |= TREDRAW_HARD;
            handled = true;
          }
        }
        break;
      case CKEY:
        if (event->alt) {
          if (!(t->options & CTX_NO_PET)) {
            t->flag ^= T_PROP_CONNECTED;
            sort_trans_data_dist(t);
            calculatePropRatio(t);
            t->redraw = TREDRAW_HARD;
            handled = true;
          }
        }
        break;
      case OKEY:
        if (t->flag & T_PROP_EDIT && event->shift) {
          t->prop_mode = (t->prop_mode + 1) % PROP_MODE_MAX;
          calculatePropRatio(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case PADPLUSKEY:
        if (event->alt && t->flag & T_PROP_EDIT) {
          t->prop_size *= (t->modifiers & MOD_PRECISION) ? 1.01f : 1.1f;
          if (t->spacetype == SPACE_VIEW3D && t->persp != RV3D_ORTHO) {
            t->prop_size = min_ff(t->prop_size, ((View3D *)t->view)->clip_end);
          }
          calculatePropRatio(t);
          t->redraw = TREDRAW_HARD;
          handled = true;
        }
        break;
      case PAGEUPKEY:
      case WHEELDOWNMOUSE:
        if (t->flag & T_AUTOIK) {
          transform_autoik_update(t, 1);
        }
        t->redraw = TREDRAW_HARD;
        handled = true;
        break;
      case PADMINUS:
        if (event->alt && t->flag & T_PROP_EDIT) {
          t->prop_size /= (t->modifiers & MOD_PRECISION) ? 1.01f : 1.1f;
          calculatePropRatio(t);
          t->redraw = TREDRAW_HARD;
          handled = true;
        }
        break;
      case PAGEDOWNKEY:
      case WHEELUPMOUSE:
        if (t->flag & T_AUTOIK) {
          transform_autoik_update(t, -1);
        }
        t->redraw = TREDRAW_HARD;
        handled = true;
        break;
      case LEFTALTKEY:
      case RIGHTALTKEY:
        if (ELEM(t->spacetype, SPACE_SEQ, SPACE_VIEW3D)) {
          t->flag |= T_ALT_TRANSFORM;
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case NKEY:
        if (ELEM(t->mode, TFM_ROTATION)) {
          if ((t->flag & T_EDIT) && t->obedit_type == OB_MESH) {
            restoreTransObjects(t);
            resetTransModal(t);
            resetTransRestrictions(t);
            initNormalRotation(t);
            t->redraw = TREDRAW_HARD;
            handled = true;
          }
        }
        break;
      default:
        break;
    }

    /* Snapping key events */
    t->redraw |= handleSnapping(t, event);
  }
  else if (event->val == KM_RELEASE) {
    switch (event->type) {
      case LEFTSHIFTKEY:
      case RIGHTSHIFTKEY:
        t->modifiers &= ~MOD_CONSTRAINT_PLANE;
        t->redraw |= TREDRAW_HARD;
        handled = true;
        break;

      case MIDDLEMOUSE:
        if ((t->flag & T_NO_CONSTRAINT) == 0) {
          t->modifiers &= ~MOD_CONSTRAINT_SELECT;
          postSelectConstraint(t);
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      case LEFTALTKEY:
      case RIGHTALTKEY:
        if (ELEM(t->spacetype, SPACE_SEQ, SPACE_VIEW3D)) {
          t->flag &= ~T_ALT_TRANSFORM;
          t->redraw |= TREDRAW_HARD;
          handled = true;
        }
        break;
      default:
        break;
    }

    /* confirm transform if launch key is released after mouse move */
    if ((t->flag & T_RELEASE_CONFIRM) && event->type == t->launch_event) {
      t->state = TRANS_CONFIRM;
    }
  }

  /* if we change snap options, get the unsnapped values back */
  if ((mode_prev != t->mode) || ((t->modifiers & (MOD_SNAP | MOD_SNAP_INVERT)) !=
                                 (modifiers_prev & (MOD_SNAP | MOD_SNAP_INVERT)))) {
    applyMouseInput(t, &t->mouse, t->mval, t->values);
  }

  /* Per transform event, if present */
  if (t->handleEvent && (!handled ||
                         /* Needed for vertex slide, see [#38756] */
                         (event->type == MOUSEMOVE))) {
    t->redraw |= t->handleEvent(t, event);
  }

  /* Try to init modal numinput now, if possible. */
  if (!(handled || t->redraw) && ((event->val == KM_PRESS) || (event->type == EVT_MODAL_MAP)) &&
      handleNumInput(t->context, &(t->num), event)) {
    t->redraw |= TREDRAW_HARD;
    handled = true;
  }

  if (t->redraw && !ELEM(event->type, MOUSEMOVE, INBETWEEN_MOUSEMOVE)) {
    WM_window_status_area_tag_redraw(CTX_wm_window(t->context));
  }

  if (handled || t->redraw) {
    return 0;
  }
  else {
    return OPERATOR_PASS_THROUGH;
  }
}

int transform_modal(bContext *C, wmOperator *op, const wmEvent *event)
{
  int exit_code;

  TransInfo *t = op->customdata;
  const enum TfmMode mode_prev = t->mode;

#if defined(WITH_INPUT_NDOF) && 0
  // stable 2D mouse coords map to different 3D coords while the 3D mouse is active
  // in other words, 2D deltas are no longer good enough!
  // disable until individual 'transformers' behave better

  if (event->type == NDOF_MOTION) {
    return OPERATOR_PASS_THROUGH;
  }
#endif

  /* XXX insert keys are called here, and require context */
  t->context = C;
  exit_code = transformEvent(t, event);
  t->context = NULL;

  /* XXX, workaround: active needs to be calculated before transforming,
   * since we're not reading from 'td->center' in this case. see: T40241 */
  if (t->tsnap.target == SCE_SNAP_TARGET_ACTIVE) {
    /* In camera view, tsnap callback is not set
     * (see initSnappingMode() in transfrom_snap.c, and T40348). */
    if (t->tsnap.targetSnap && ((t->tsnap.status & TARGET_INIT) == 0)) {
      t->tsnap.targetSnap(t);
    }
  }

  transformApply(C, t);

  exit_code |= transformEnd(C, t);

  if ((exit_code & OPERATOR_RUNNING_MODAL) == 0) {
    transformops_exit(C, op);
    exit_code &= ~OPERATOR_PASS_THROUGH; /* preventively remove passthrough */
  }
  else {
    if (mode_prev != t->mode) {
      /* WARNING: this is not normal to switch operator types
       * normally it would not be supported but transform happens
       * to share callbacks between different operators. */
      wmOperatorType *ot_new = NULL;
      TransformModeItem *item = transform_modes;
      while (item->idname) {
        if (item->mode == t->mode) {
          ot_new = WM_operatortype_find(item->idname, false);
          break;
        }
        item++;
      }

      BLI_assert(ot_new != NULL);
      if (ot_new) {
        WM_operator_type_set(op, ot_new);
      }
      /* end suspicious code */
    }
  }

  return exit_code;
}

void transform_cancel(bContext *C, wmOperator *op)
{
  TransInfo *t = op->customdata;

  t->state = TRANS_CANCEL;
  transformEnd(C, t);
  transformops_exit(C, op);
}

int transform_exec(bContext *C, wmOperator *op)
{
  TransInfo *t;

  if (!transformops_data(C, op, NULL)) {
    G.moving = 0;
    return OPERATOR_CANCELLED;
  }

  t = op->customdata;

  t->options |= CTX_AUTOCONFIRM;

  transformApply(C, t);

  transformEnd(C, t);

  transformops_exit(C, op);

  WM_event_add_notifier(C, NC_OBJECT | ND_TRANSFORM, NULL);

  return OPERATOR_FINISHED;
}

int transform_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  if (!transformops_data(C, op, event)) {
    G.moving = 0;
    return OPERATOR_CANCELLED;
  }

  /* When modal, allow 'value' to set initial offset. */
  if ((event == NULL) && RNA_struct_property_is_set(op->ptr, "value")) {
    return transform_exec(C, op);
  }
  else {
    /* add temp handler */
    WM_event_add_modal_handler(C, op);

    op->flag |= OP_IS_MODAL_GRAB_CURSOR;  // XXX maybe we want this with the gizmo only?

    /* Use when modal input has some transformation to begin with. */
    {
      TransInfo *t = op->customdata;
      if (UNLIKELY(!is_zero_v4(t->values_modal_offset))) {
        transformApply(C, t);
      }
    }

    return OPERATOR_RUNNING_MODAL;
  }
}

bool transform_poll_property(const bContext *UNUSED(C), wmOperator *op, const PropertyRNA *prop)
{
  const char *prop_id = RNA_property_identifier(prop);

  /* Orientation/Constraints. */
  {
    /* Hide orientation axis if no constraints are set, since it wont be used. */
    PropertyRNA *prop_con = RNA_struct_find_property(op->ptr, "orient_type");
    if (prop_con != NULL && (prop_con != prop)) {
      if (STRPREFIX(prop_id, "constraint")) {

        /* Special case: show constraint axis if we don't have values,
         * needed for mirror operator. */
        if (STREQ(prop_id, "constraint_axis") &&
            (RNA_struct_find_property(op->ptr, "value") == NULL)) {
          return true;
        }

        return false;
      }
    }
  }

  /* Proportional Editing. */
  {
    PropertyRNA *prop_pet = RNA_struct_find_property(op->ptr, "use_proportional_edit");
    if (prop_pet && (prop_pet != prop) && (RNA_property_boolean_get(op->ptr, prop_pet) == false)) {
      if (STRPREFIX(prop_id, "proportional") || STRPREFIX(prop_id, "use_proportional")) {
        return false;
      }
    }
  }

  return true;
}

void Transform_Properties(struct wmOperatorType *ot, int flags)
{
  PropertyRNA *prop;

  if (flags & P_ORIENT_AXIS) {
    prop = RNA_def_property(ot->srna, "orient_axis", PROP_ENUM, PROP_NONE);
    RNA_def_property_ui_text(prop, "Axis", "");
    RNA_def_property_enum_default(prop, 2);
    RNA_def_property_enum_items(prop, rna_enum_axis_xyz_items);
    RNA_def_property_flag(prop, PROP_SKIP_SAVE);
  }
  if (flags & P_ORIENT_AXIS_ORTHO) {
    prop = RNA_def_property(ot->srna, "orient_axis_ortho", PROP_ENUM, PROP_NONE);
    RNA_def_property_ui_text(prop, "Axis Ortho", "");
    RNA_def_property_enum_default(prop, 0);
    RNA_def_property_enum_items(prop, rna_enum_axis_xyz_items);
    RNA_def_property_flag(prop, PROP_SKIP_SAVE);
  }

  if (flags & P_ORIENT_MATRIX) {
    prop = RNA_def_property(ot->srna, "orient_type", PROP_ENUM, PROP_NONE);
    RNA_def_property_ui_text(prop, "Orientation", "Transformation orientation");
    RNA_def_enum_funcs(prop, rna_TransformOrientation_itemf);

    /* Set by 'orient_type' or gizmo which acts on non-standard orientation. */
    prop = RNA_def_float_matrix(
        ot->srna, "orient_matrix", 3, 3, NULL, 0.0f, 0.0f, "Matrix", "", 0.0f, 0.0f);
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);

    /* Only use 'orient_matrix' when 'orient_matrix_type == orient_type',
     * this allows us to reuse the orientation set by a gizmo for eg, without disabling the ability
     * to switch over to other orientations. */
    prop = RNA_def_property(ot->srna, "orient_matrix_type", PROP_ENUM, PROP_NONE);
    RNA_def_property_ui_text(prop, "Matrix Orientation", "");
    RNA_def_enum_funcs(prop, rna_TransformOrientation_itemf);
    RNA_def_property_flag(prop, PROP_HIDDEN);
  }

  if (flags & P_CONSTRAINT) {
    RNA_def_boolean_vector(ot->srna, "constraint_axis", 3, NULL, "Constraint Axis", "");
  }

  if (flags & P_MIRROR) {
    prop = RNA_def_boolean(ot->srna, "mirror", 0, "Mirror Editing", "");
    if (flags & P_MIRROR_DUMMY) {
      /* only used so macros can disable this option */
      RNA_def_property_flag(prop, PROP_HIDDEN);
    }
  }

  if (flags & P_PROPORTIONAL) {
    RNA_def_boolean(ot->srna, "use_proportional_edit", 0, "Proportional Editing", "");
    prop = RNA_def_enum(ot->srna,
                        "proportional_edit_falloff",
                        rna_enum_proportional_falloff_items,
                        0,
                        "Proportional Falloff",
                        "Falloff type for proportional editing mode");
    /* Abusing id_curve :/ */
    RNA_def_property_translation_context(prop, BLT_I18NCONTEXT_ID_CURVE);
    RNA_def_float(ot->srna,
                  "proportional_size",
                  1,
                  T_PROP_SIZE_MIN,
                  T_PROP_SIZE_MAX,
                  "Proportional Size",
                  "",
                  0.001f,
                  100.0f);

    RNA_def_boolean(ot->srna, "use_proportional_connected", 0, "Connected", "");
    RNA_def_boolean(ot->srna, "use_proportional_projected", 0, "Projected (2D)", "");
  }

  if (flags & P_SNAP) {
    prop = RNA_def_boolean(ot->srna, "snap", 0, "Use Snapping Options", "");
    RNA_def_property_flag(prop, PROP_HIDDEN);

    if (flags & P_GEO_SNAP) {
      prop = RNA_def_enum(ot->srna, "snap_target", rna_enum_snap_target_items, 0, "Target", "");
      RNA_def_property_flag(prop, PROP_HIDDEN);
      prop = RNA_def_float_vector(
          ot->srna, "snap_point", 3, NULL, -FLT_MAX, FLT_MAX, "Point", "", -FLT_MAX, FLT_MAX);
      RNA_def_property_flag(prop, PROP_HIDDEN);

      if (flags & P_ALIGN_SNAP) {
        prop = RNA_def_boolean(ot->srna, "snap_align", 0, "Align with Point Normal", "");
        RNA_def_property_flag(prop, PROP_HIDDEN);
        prop = RNA_def_float_vector(
            ot->srna, "snap_normal", 3, NULL, -FLT_MAX, FLT_MAX, "Normal", "", -FLT_MAX, FLT_MAX);
        RNA_def_property_flag(prop, PROP_HIDDEN);
      }
    }
  }

  if (flags & P_GPENCIL_EDIT) {
    prop = RNA_def_boolean(ot->srna,
                           "gpencil_strokes",
                           0,
                           "Edit Grease Pencil",
                           "Edit selected Grease Pencil strokes");
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
  }

  if (flags & P_CURSOR_EDIT) {
    prop = RNA_def_boolean(ot->srna, "cursor_transform", 0, "Transform Cursor", "");
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
  }

  if ((flags & P_OPTIONS) && !(flags & P_NO_TEXSPACE)) {
    prop = RNA_def_boolean(
        ot->srna, "texture_space", 0, "Edit Texture Space", "Edit Object data texture space");
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
    prop = RNA_def_boolean(
        ot->srna, "remove_on_cancel", 0, "Remove on Cancel", "Remove elements on cancel");
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
  }

  if (flags & P_CORRECT_UV) {
    RNA_def_boolean(
        ot->srna, "correct_uv", true, "Correct UVs", "Correct UV coordinates when transforming");
  }

  if (flags & P_CENTER) {
    /* For gizmos that define their own center. */
    prop = RNA_def_property(ot->srna, "center_override", PROP_FLOAT, PROP_XYZ);
    RNA_def_property_array(prop, 3);
    RNA_def_property_flag(prop, PROP_HIDDEN | PROP_SKIP_SAVE);
    RNA_def_property_ui_text(prop, "Center Override", "Force using this center value (when set)");
  }

  if ((flags & P_NO_DEFAULTS) == 0) {
    prop = RNA_def_boolean(ot->srna,
                           "release_confirm",
                           0,
                           "Confirm on Release",
                           "Always confirm operation when releasing button");
    RNA_def_property_flag(prop, PROP_HIDDEN);

    prop = RNA_def_boolean(ot->srna, "use_accurate", 0, "Accurate", "Use accurate transformation");
    RNA_def_property_flag(prop, PROP_HIDDEN);
  }
}

static void TRANSFORM_OT_transform(struct wmOperatorType *ot)
{
  PropertyRNA *prop;

  /* identifiers */
  ot->name = "Transform";
  ot->description = "Transform selected items by mode type";
  ot->idname = "TRANSFORM_OT_transform";
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_screenactive;
  ot->poll_property = transform_poll_property;

  prop = RNA_def_enum(
      ot->srna, "mode", rna_enum_transform_mode_types, TFM_TRANSLATION, "Mode", "");
  RNA_def_property_flag(prop, PROP_HIDDEN);

  RNA_def_float_vector(
      ot->srna, "value", 4, NULL, -FLT_MAX, FLT_MAX, "Values", "", -FLT_MAX, FLT_MAX);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot,
                       P_ORIENT_AXIS | P_ORIENT_MATRIX | P_CONSTRAINT | P_PROPORTIONAL | P_MIRROR |
                           P_ALIGN_SNAP | P_GPENCIL_EDIT | P_CENTER);
}

static int transform_from_gizmo_invoke(bContext *C,
                                       wmOperator *UNUSED(op),
                                       const wmEvent *UNUSED(event))
{
  bToolRef *tref = WM_toolsystem_ref_from_context(C);
  if (tref) {
    ARegion *ar = CTX_wm_region(C);
    wmGizmoMap *gzmap = ar->gizmo_map;
    wmGizmoGroup *gzgroup = gzmap ? WM_gizmomap_group_find(gzmap, "VIEW3D_GGT_xform_gizmo") : NULL;
    if (gzgroup != NULL) {
      PointerRNA gzg_ptr;
      WM_toolsystem_ref_properties_ensure_from_gizmo_group(tref, gzgroup->type, &gzg_ptr);
      const int drag_action = RNA_enum_get(&gzg_ptr, "drag_action");
      const char *op_id = NULL;
      switch (drag_action) {
        case V3D_GIZMO_SHOW_OBJECT_TRANSLATE:
          op_id = "TRANSFORM_OT_translate";
          break;
        case V3D_GIZMO_SHOW_OBJECT_ROTATE:
          op_id = "TRANSFORM_OT_rotate";
          break;
        case V3D_GIZMO_SHOW_OBJECT_SCALE:
          op_id = "TRANSFORM_OT_resize";
          break;
        default:
          break;
      }
      if (op_id) {
        wmOperatorType *ot = WM_operatortype_find(op_id, true);
        PointerRNA op_ptr;
        WM_operator_properties_create_ptr(&op_ptr, ot);
        RNA_boolean_set(&op_ptr, "release_confirm", true);
        WM_operator_name_call_ptr(C, ot, WM_OP_INVOKE_DEFAULT, &op_ptr);
        WM_operator_properties_free(&op_ptr);
        return OPERATOR_FINISHED;
      }
    }
  }
  return OPERATOR_PASS_THROUGH;
}

/* Use with 'TRANSFORM_GGT_gizmo'. */
static void TRANSFORM_OT_from_gizmo(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Transform From Gizmo";
  ot->description = "Transform selected items by mode type";
  ot->idname = "TRANSFORM_OT_from_gizmo";
  ot->flag = 0;

  /* api callbacks */
  ot->invoke = transform_from_gizmo_invoke;
}

void transform_operatortypes(void)
{
  TransformModeItem *tmode;

  for (tmode = transform_modes; tmode->idname; tmode++) {
    WM_operatortype_append(tmode->opfunc);
  }

  WM_operatortype_append(TRANSFORM_OT_transform);

  WM_operatortype_append(TRANSFORM_OT_select_orientation);
  WM_operatortype_append(TRANSFORM_OT_create_orientation);
  WM_operatortype_append(TRANSFORM_OT_delete_orientation);

  WM_operatortype_append(TRANSFORM_OT_from_gizmo);
}

static bool transform_modal_item_poll(const wmOperator *op, int value)
{
  const TransInfo *t = op->customdata;
  switch (value) {
    case TFM_MODAL_CANCEL: {
      if ((t->flag & T_RELEASE_CONFIRM) && ISMOUSE(t->launch_event)) {
        return false;
      }
      break;
    }
    case TFM_MODAL_PROPSIZE:
    case TFM_MODAL_PROPSIZE_UP:
    case TFM_MODAL_PROPSIZE_DOWN: {
      if ((t->flag & T_PROP_EDIT) == 0) {
        return false;
      }
      break;
    }
    case TFM_MODAL_ADD_SNAP:
    case TFM_MODAL_REMOVE_SNAP: {
      if (t->spacetype != SPACE_VIEW3D) {
        return false;
      }
      else if (t->tsnap.mode & (SCE_SNAP_MODE_INCREMENT | SCE_SNAP_MODE_GRID)) {
        return false;
      }
      else if (!validSnap(t)) {
        return false;
      }
      break;
    }
    case TFM_MODAL_AXIS_X:
    case TFM_MODAL_AXIS_Y:
    case TFM_MODAL_AXIS_Z:
    case TFM_MODAL_PLANE_X:
    case TFM_MODAL_PLANE_Y:
    case TFM_MODAL_PLANE_Z: {
      if (t->flag & T_NO_CONSTRAINT) {
        return false;
      }
      if (!ELEM(value, TFM_MODAL_AXIS_X, TFM_MODAL_AXIS_Y)) {
        if (t->flag & T_2D_EDIT) {
          return false;
        }
      }
      break;
    }
    case TFM_MODAL_CONS_OFF: {
      if ((t->con.mode & CON_APPLY) == 0) {
        return false;
      }
      break;
    }
    case TFM_MODAL_EDGESLIDE_UP:
    case TFM_MODAL_EDGESLIDE_DOWN: {
      if (t->mode != TFM_EDGE_SLIDE) {
        return false;
      }
      break;
    }
    case TFM_MODAL_INSERTOFS_TOGGLE_DIR: {
      if (t->spacetype != SPACE_NODE) {
        return false;
      }
      break;
    }
    case TFM_MODAL_AUTOIK_LEN_INC:
    case TFM_MODAL_AUTOIK_LEN_DEC: {
      if ((t->flag & T_AUTOIK) == 0) {
        return false;
      }
      break;
    }
  }
  return true;
}

/* called in transform_ops.c, on each regeneration of keymaps */
static wmKeyMap *transform_modal_keymap(wmKeyConfig *keyconf)
{
  static const EnumPropertyItem modal_items[] = {
      {TFM_MODAL_CONFIRM, "CONFIRM", 0, "Confirm", ""},
      {TFM_MODAL_CANCEL, "CANCEL", 0, "Cancel", ""},
      {TFM_MODAL_AXIS_X, "AXIS_X", 0, "X axis", ""},
      {TFM_MODAL_AXIS_Y, "AXIS_Y", 0, "Y axis", ""},
      {TFM_MODAL_AXIS_Z, "AXIS_Z", 0, "Z axis", ""},
      {TFM_MODAL_PLANE_X, "PLANE_X", 0, "X plane", ""},
      {TFM_MODAL_PLANE_Y, "PLANE_Y", 0, "Y plane", ""},
      {TFM_MODAL_PLANE_Z, "PLANE_Z", 0, "Z plane", ""},
      {TFM_MODAL_CONS_OFF, "CONS_OFF", 0, "Clear Constraints", ""},
      {TFM_MODAL_SNAP_INV_ON, "SNAP_INV_ON", 0, "Snap Invert", ""},
      {TFM_MODAL_SNAP_INV_OFF, "SNAP_INV_OFF", 0, "Snap Invert (Off)", ""},
      {TFM_MODAL_SNAP_TOGGLE, "SNAP_TOGGLE", 0, "Snap Toggle", ""},
      {TFM_MODAL_ADD_SNAP, "ADD_SNAP", 0, "Add Snap Point", ""},
      {TFM_MODAL_REMOVE_SNAP, "REMOVE_SNAP", 0, "Remove Last Snap Point", ""},
      {NUM_MODAL_INCREMENT_UP, "INCREMENT_UP", 0, "Numinput Increment Up", ""},
      {NUM_MODAL_INCREMENT_DOWN, "INCREMENT_DOWN", 0, "Numinput Increment Down", ""},
      {TFM_MODAL_PROPSIZE_UP, "PROPORTIONAL_SIZE_UP", 0, "Increase Proportional Influence", ""},
      {TFM_MODAL_PROPSIZE_DOWN,
       "PROPORTIONAL_SIZE_DOWN",
       0,
       "Decrease Proportional Influence",
       ""},
      {TFM_MODAL_AUTOIK_LEN_INC, "AUTOIK_CHAIN_LEN_UP", 0, "Increase Max AutoIK Chain Length", ""},
      {TFM_MODAL_AUTOIK_LEN_DEC,
       "AUTOIK_CHAIN_LEN_DOWN",
       0,
       "Decrease Max AutoIK Chain Length",
       ""},
      {TFM_MODAL_EDGESLIDE_UP, "EDGESLIDE_EDGE_NEXT", 0, "Select next Edge Slide Edge", ""},
      {TFM_MODAL_EDGESLIDE_DOWN, "EDGESLIDE_PREV_NEXT", 0, "Select previous Edge Slide Edge", ""},
      {TFM_MODAL_PROPSIZE, "PROPORTIONAL_SIZE", 0, "Adjust Proportional Influence", ""},
      {TFM_MODAL_INSERTOFS_TOGGLE_DIR,
       "INSERTOFS_TOGGLE_DIR",
       0,
       "Toggle Direction for Node Auto-offset",
       ""},
      {TFM_MODAL_TRANSLATE, "TRANSLATE", 0, "Move", ""},
      {TFM_MODAL_ROTATE, "ROTATE", 0, "Rotate", ""},
      {TFM_MODAL_RESIZE, "RESIZE", 0, "Resize", ""},
      {0, NULL, 0, NULL, NULL},
  };

  wmKeyMap *keymap = WM_modalkeymap_get(keyconf, "Transform Modal Map");

  keymap = WM_modalkeymap_add(keyconf, "Transform Modal Map", modal_items);
  keymap->poll_modal_item = transform_modal_item_poll;

  return keymap;
}

void ED_keymap_transform(wmKeyConfig *keyconf)
{
  wmKeyMap *modalmap = transform_modal_keymap(keyconf);

  TransformModeItem *tmode;

  for (tmode = transform_modes; tmode->idname; tmode++) {
    WM_modalkeymap_assign(modalmap, tmode->idname);
  }
  WM_modalkeymap_assign(modalmap, "TRANSFORM_OT_transform");
}
