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

const char OP_TOSPHERE[] = "TRANSFORM_OT_tosphere";

/* -------------------------------------------------------------------- */
/* Transform (ToSphere) */

/** \name Transform ToSphere
 * \{ */

static void applyToSphere(TransInfo *t, const int UNUSED(mval[2]))
{
  float vec[3];
  float ratio, radius;
  int i;
  char str[UI_MAX_DRAW_STR];

  ratio = t->values[0];

  snapGridIncrement(t, &ratio);

  applyNumInput(&t->num, &ratio);

  CLAMP(ratio, 0.0f, 1.0f);

  t->values_final[0] = ratio;

  /* header print for NumInput */
  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN];

    outputNumInput(&(t->num), c, &t->scene->unit);

    BLI_snprintf(str, sizeof(str), TIP_("To Sphere: %s %s"), c, t->proptext);
  }
  else {
    /* default header print */
    BLI_snprintf(str, sizeof(str), TIP_("To Sphere: %.4f %s"), ratio, t->proptext);
  }

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      float tratio;
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      sub_v3_v3v3(vec, td->iloc, tc->center_local);

      radius = normalize_v3(vec);

      tratio = ratio * td->factor;

      mul_v3_fl(vec, radius * (1.0f - tratio) + t->val * tratio);

      add_v3_v3v3(td->loc, tc->center_local, vec);
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initToSphere(TransInfo *t)
{
  int i;

  t->mode = TFM_TOSPHERE;
  t->transform = applyToSphere;

  initMouseInputMode(t, &t->mouse, INPUT_HORIZONTAL_RATIO);

  t->idx_max = 0;
  t->num.idx_max = 0;
  t->snap[0] = 0.0f;
  t->snap[1] = 0.1f;
  t->snap[2] = t->snap[1] * 0.1f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_type[0] = B_UNIT_NONE;

  t->num.val_flag[0] |= NUM_NULL_ONE | NUM_NO_NEGATIVE;
  t->flag |= T_NO_CONSTRAINT;

  // Calculate average radius
  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      t->val += len_v3v3(tc->center_local, td->iloc);
    }
  }

  t->val /= (float)t->data_len_all;
}
/** \} */

void TRANSFORM_OT_tosphere(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "To Sphere";
  // added "around mesh center" to differentiate between "MESH_OT_vertices_to_sphere()"
  ot->description = "Move selected vertices outward in a spherical shape around mesh center";
  ot->idname = OP_TOSPHERE;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_screenactive;
  ot->poll_property = transform_poll_property;

  RNA_def_float_factor(ot->srna, "value", 0, 0, 1, "Factor", "", 0, 1);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot, P_PROPORTIONAL | P_MIRROR | P_SNAP | P_GPENCIL_EDIT | P_CENTER);
}
