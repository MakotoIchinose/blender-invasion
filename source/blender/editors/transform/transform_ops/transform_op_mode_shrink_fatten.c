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
#include "BLI_string.h"

#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_unit.h"

#include "RNA_define.h"

#include "WM_api.h"
#include "WM_types.h"

#include "UI_interface.h"

#include "ED_screen.h"

#include "transform.h"
#include "transform_op.h"

const char OP_SHRINK_FATTEN[] = "TRANSFORM_OT_shrink_fatten";

/* -------------------------------------------------------------------- */
/* Transform (Shrink-Fatten) */

/** \name Transform Shrink-Fatten
 * \{ */

static void applyShrinkFatten(TransInfo *t, const int UNUSED(mval[2]))
{
  float distance;
  int i;
  char str[UI_MAX_DRAW_STR];
  size_t ofs = 0;

  distance = -t->values[0];

  snapGridIncrement(t, &distance);

  applyNumInput(&t->num, &distance);

  t->values_final[0] = -distance;

  /* header print for NumInput */
  ofs += BLI_strncpy_rlen(str + ofs, TIP_("Shrink/Fatten:"), sizeof(str) - ofs);
  if (hasNumInput(&t->num)) {
    char c[NUM_STR_REP_LEN];
    outputNumInput(&(t->num), c, &t->scene->unit);
    ofs += BLI_snprintf(str + ofs, sizeof(str) - ofs, " %s", c);
  }
  else {
    /* default header print */
    ofs += BLI_snprintf(str + ofs, sizeof(str) - ofs, " %.4f", distance);
  }

  if (t->proptext[0]) {
    ofs += BLI_snprintf(str + ofs, sizeof(str) - ofs, " %s", t->proptext);
  }
  ofs += BLI_strncpy_rlen(str + ofs, ", (", sizeof(str) - ofs);

  if (t->keymap) {
    wmKeyMapItem *kmi = WM_modalkeymap_find_propvalue(t->keymap, TFM_MODAL_RESIZE);
    if (kmi) {
      ofs += WM_keymap_item_to_string(kmi, false, str + ofs, sizeof(str) - ofs);
    }
  }
  BLI_snprintf(str + ofs,
               sizeof(str) - ofs,
               TIP_(" or Alt) Even Thickness %s"),
               WM_bool_as_string((t->flag & T_ALT_TRANSFORM) != 0));
  /* done with header string */

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      float tdistance; /* temp dist */
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      /* get the final offset */
      tdistance = distance * td->factor;
      if (td->ext && (t->flag & T_ALT_TRANSFORM) != 0) {
        tdistance *= td->ext->isize[0]; /* shell factor */
      }

      madd_v3_v3v3fl(td->loc, td->iloc, td->axismtx[2], tdistance);
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initShrinkFatten(TransInfo *t)
{
  // If not in mesh edit mode, fallback to Resize
  if ((t->flag & T_EDIT) == 0 || (t->obedit_type != OB_MESH)) {
    initResize(t);
  }
  else {
    t->mode = TFM_SHRINKFATTEN;
    t->transform = applyShrinkFatten;

    initMouseInputMode(t, &t->mouse, INPUT_VERTICAL_ABSOLUTE);

    t->idx_max = 0;
    t->num.idx_max = 0;
    t->snap[0] = 0.0f;
    t->snap[1] = 1.0f;
    t->snap[2] = t->snap[1] * 0.1f;

    copy_v3_fl(t->num.val_inc, t->snap[1]);
    t->num.unit_sys = t->scene->unit.system;
    t->num.unit_type[0] = B_UNIT_LENGTH;

    t->flag |= T_NO_CONSTRAINT;
  }
}
/** \} */

void TRANSFORM_OT_shrink_fatten(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Shrink/Fatten";
  ot->description = "Shrink/fatten selected vertices along normals";
  ot->idname = OP_SHRINK_FATTEN;
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO | OPTYPE_BLOCKING;

  /* api callbacks */
  ot->invoke = transform_invoke;
  ot->exec = transform_exec;
  ot->modal = transform_modal;
  ot->cancel = transform_cancel;
  ot->poll = ED_operator_editmesh;
  ot->poll_property = transform_poll_property;

  RNA_def_float_distance(ot->srna, "value", 0, -FLT_MAX, FLT_MAX, "Offset", "", -FLT_MAX, FLT_MAX);

  RNA_def_boolean(ot->srna,
                  "use_even_offset",
                  false,
                  "Offset Even",
                  "Scale the offset to give more even thickness");

  WM_operatortype_props_advanced_begin(ot);

  Transform_Properties(ot, P_PROPORTIONAL | P_MIRROR | P_SNAP);
}
