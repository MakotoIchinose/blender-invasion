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

#include "BLI_math.h"

#include "BKE_context.h"
#include "BKE_unit.h"

#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_interface.h"

#include "ED_screen.h"

#include "transform.h"
#include "transform_op.h"

const char OP_SKIN_RESIZE[] = "TRANSFORM_OT_skin_resize";

/* -------------------------------------------------------------------- */
/* Transform (Skin) */

/** \name Transform Skin
 * \{ */

static void applySkinResize(TransInfo *t, const int UNUSED(mval[2]))
{
  float mat[3][3];
  int i;
  char str[UI_MAX_DRAW_STR];

  if (t->flag & T_INPUT_IS_VALUES_FINAL) {
    copy_v3_v3(t->values_final, t->values);
  }
  else {
    copy_v3_fl(t->values_final, t->values[0]);

    snapGridIncrement(t, t->values_final);

    if (applyNumInput(&t->num, t->values_final)) {
      constraintNumInput(t, t->values_final);
    }

    applySnapping(t, t->values_final);
  }

  size_to_mat3(mat, t->values_final);

  headerResize(t, t->values_final, str);

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      float tmat[3][3], smat[3][3];
      float fsize[3];

      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      if (t->flag & T_EDIT) {
        mul_m3_m3m3(smat, mat, td->mtx);
        mul_m3_m3m3(tmat, td->smtx, smat);
      }
      else {
        copy_m3_m3(tmat, mat);
      }

      if (t->con.applySize) {
        t->con.applySize(t, NULL, NULL, tmat);
      }

      mat3_to_size(fsize, tmat);
      td->val[0] = td->ext->isize[0] * (1 + (fsize[0] - 1) * td->factor);
      td->val[1] = td->ext->isize[1] * (1 + (fsize[1] - 1) * td->factor);
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initSkinResize(TransInfo *t)
{
  t->mode = TFM_SKIN_RESIZE;
  t->transform = applySkinResize;

  initMouseInputMode(t, &t->mouse, INPUT_SPRING_FLIP);

  t->flag |= T_NULL_ONE;
  t->num.val_flag[0] |= NUM_NULL_ONE;
  t->num.val_flag[1] |= NUM_NULL_ONE;
  t->num.val_flag[2] |= NUM_NULL_ONE;
  t->num.flag |= NUM_AFFECT_ALL;
  if ((t->flag & T_EDIT) == 0) {
    t->flag |= T_NO_ZERO;
#ifdef USE_NUM_NO_ZERO
    t->num.val_flag[0] |= NUM_NO_ZERO;
    t->num.val_flag[1] |= NUM_NO_ZERO;
    t->num.val_flag[2] |= NUM_NO_ZERO;
#endif
  }

  t->idx_max = 2;
  t->num.idx_max = 2;
  t->snap[0] = 0.0f;
  t->snap[1] = 0.1f;
  t->snap[2] = t->snap[1] * 0.1f;

  copy_v3_fl(t->num.val_inc, t->snap[1]);
  t->num.unit_sys = t->scene->unit.system;
  t->num.unit_type[0] = B_UNIT_NONE;
  t->num.unit_type[1] = B_UNIT_NONE;
  t->num.unit_type[2] = B_UNIT_NONE;
}
/** \} */

void TRANSFORM_OT_skin_resize(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Skin Resize";
  ot->description = "Scale selected vertices' skin radii";
  ot->idname = OP_SKIN_RESIZE;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_editmesh;
  ot->poll_property = transform_poll_property;

  RNA_def_float_vector(
      ot->srna, "value", 3, VecOne, -FLT_MAX, FLT_MAX, "Scale", "", -FLT_MAX, FLT_MAX);

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot,
                       P_ORIENT_MATRIX | P_CONSTRAINT | P_PROPORTIONAL | P_MIRROR | P_GEO_SNAP |
                           P_OPTIONS | P_NO_TEXSPACE);
}
