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

#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BLI_math.h"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_report.h"
#include "BKE_editmesh.h"
#include "BKE_layer.h"
#include "BKE_scene.h"
#include "BKE_unit.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "WM_api.h"
#include "WM_message.h"
#include "WM_types.h"
#include "WM_toolsystem.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "ED_screen.h"
/* for USE_LOOPSLIDE_HACK only */
#include "ED_mesh.h"

#include "transform.h"
#include "transform_convert.h"
#include "transform_op.h"

const char OP_TRACKBALL[] = "TRANSFORM_OT_trackball";

/* -------------------------------------------------------------------- */
/* Transform (Rotation - Trackball) */

/** \name Transform Rotation - Trackball
 * \{ */

static void applyTrackballValue(TransInfo *t,
                                const float axis1[3],
                                const float axis2[3],
                                const float angles[2])
{
  float mat[3][3];
  float axis[3];
  float angle;
  int i;

  mul_v3_v3fl(axis, axis1, angles[0]);
  madd_v3_v3fl(axis, axis2, angles[1]);
  angle = normalize_v3(axis);
  axis_angle_normalized_to_mat3(mat, axis, angle);

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      if (t->flag & T_PROP_EDIT) {
        axis_angle_normalized_to_mat3(mat, axis, td->factor * angle);
      }

      ElementRotation(t, tc, td, mat, t->around);
    }
  }
}

static void applyTrackball(TransInfo *t, const int UNUSED(mval[2]))
{
  char str[UI_MAX_DRAW_STR];
  size_t ofs = 0;
  float axis1[3], axis2[3];
#if 0 /* UNUSED */
  float mat[3][3], totmat[3][3], smat[3][3];
#endif
  float phi[2];

  copy_v3_v3(axis1, t->persinv[0]);
  copy_v3_v3(axis2, t->persinv[1]);
  normalize_v3(axis1);
  normalize_v3(axis2);

  copy_v2_v2(phi, t->values);

  snapGridIncrement(t, phi);

  applyNumInput(&t->num, phi);

  copy_v2_v2(t->values_final, phi);

  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN * 2];

    outputNumInput(&(t->num), c, &t->scene->unit);

    ofs += BLI_snprintf(str + ofs,
                        sizeof(str) - ofs,
                        TIP_("Trackball: %s %s %s"),
                        &c[0],
                        &c[NUM_STR_REP_LEN],
                        t->proptext);
  }
  else {
    ofs += BLI_snprintf(str + ofs,
                        sizeof(str) - ofs,
                        TIP_("Trackball: %.2f %.2f %s"),
                        RAD2DEGF(phi[0]),
                        RAD2DEGF(phi[1]),
                        t->proptext);
  }

  if (t->flag & T_PROP_EDIT_ALL) {
    ofs += BLI_snprintf(
        str + ofs, sizeof(str) - ofs, TIP_(" Proportional size: %.2f"), t->prop_size);
  }

#if 0 /* UNUSED */
  axis_angle_normalized_to_mat3(smat, axis1, phi[0]);
  axis_angle_normalized_to_mat3(totmat, axis2, phi[1]);

  mul_m3_m3m3(mat, smat, totmat);

  // TRANSFORM_FIX_ME
  //copy_m3_m3(t->mat, mat);  // used in gizmo
#endif

  applyTrackballValue(t, axis1, axis2, phi);

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initTrackball(TransInfo *t)
{
  t->mode = TFM_TRACKBALL;
  t->transform = applyTrackball;

  initMouseInputMode(t, &t->mouse, INPUT_TRACKBALL);

  t->idx_max = 1;
  t->num.idx_max = 1;
  t->snap[0] = 0.0f;
  t->snap[1] = DEG2RAD(5.0);
  t->snap[2] = DEG2RAD(1.0);

  copy_v3_fl(t->num.val_inc, t->snap[2]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_use_radians = (t->scene->unit.system_rotation == USER_UNIT_ROT_RADIANS);
  t->num.unit_type[0] = B_UNIT_ROTATION;
  t->num.unit_type[1] = B_UNIT_ROTATION;

  t->flag |= T_NO_CONSTRAINT;
}
/** \} */

void TRANSFORM_OT_trackball(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Trackball";
  ot->description = "Trackball style rotation of selected items";
  ot->idname = OP_TRACKBALL;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_screenactive;
  ot->poll_property = transform_poll_property;

  /* Maybe we could use float_vector_xyz here too? */
  RNA_def_float_rotation(
      ot->srna, "value", 2, NULL, -FLT_MAX, FLT_MAX, "Angle", "", -FLT_MAX, FLT_MAX);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot, P_PROPORTIONAL | P_MIRROR | P_SNAP | P_GPENCIL_EDIT | P_CENTER);
}
