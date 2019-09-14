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

const char OP_EDGE_CREASE[] = "TRANSFORM_OT_edge_crease";

/* -------------------------------------------------------------------- */
/* Transform Init (Crease) */

/** \name Transform Crease
 * \{ */

static void applyCrease(TransInfo *t, const int UNUSED(mval[2]))
{
  float crease;
  int i;
  char str[UI_MAX_DRAW_STR];

  crease = t->values[0];

  CLAMP_MAX(crease, 1.0f);

  snapGridIncrement(t, &crease);

  applyNumInput(&t->num, &crease);

  t->values_final[0] = crease;

  /* header print for NumInput */
  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN];

    outputNumInput(&(t->num), c, &t->scene->unit);

    if (crease >= 0.0f) {
      BLI_snprintf(str, sizeof(str), TIP_("Crease: +%s %s"), c, t->proptext);
    }
    else {
      BLI_snprintf(str, sizeof(str), TIP_("Crease: %s %s"), c, t->proptext);
    }
  }
  else {
    /* default header print */
    if (crease >= 0.0f) {
      BLI_snprintf(str, sizeof(str), TIP_("Crease: +%.3f %s"), crease, t->proptext);
    }
    else {
      BLI_snprintf(str, sizeof(str), TIP_("Crease: %.3f %s"), crease, t->proptext);
    }
  }

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      if (td->val) {
        *td->val = td->ival + crease * td->factor;
        if (*td->val < 0.0f) {
          *td->val = 0.0f;
        }
        if (*td->val > 1.0f) {
          *td->val = 1.0f;
        }
      }
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initCrease(TransInfo *t)
{
  t->mode = TFM_CREASE;
  t->transform = applyCrease;

  initMouseInputMode(t, &t->mouse, INPUT_SPRING_DELTA);

  t->idx_max = 0;
  t->num.idx_max = 0;
  t->snap[0] = 0.0f;
  t->snap[1] = 0.1f;
  t->snap[2] = t->snap[1] * 0.1f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_type[0] = B_UNIT_NONE;

  t->flag |= T_NO_CONSTRAINT | T_NO_PROJECT;
}
/** \} */

void TRANSFORM_OT_edge_crease(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Edge Crease";
  ot->description = "Change the crease of edges";
  ot->idname = OP_EDGE_CREASE;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_editmesh;
  ot->poll_property = transform_poll_property;

  RNA_def_float_factor(ot->srna, "value", 0, -1.0f, 1.0f, "Factor", "", -1.0f, 1.0f);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot, P_SNAP);
}
