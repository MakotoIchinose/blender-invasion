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
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup edtransform
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

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

#include "BLI_alloca.h"
#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_rect.h"
#include "BLI_listbase.h"
#include "BLI_string.h"
#include "BLI_ghash.h"
#include "BLI_utildefines_stack.h"
#include "BLI_memarena.h"

#include "BKE_nla.h"
#include "BKE_editmesh.h"
#include "BKE_editmesh_bvh.h"
#include "BKE_context.h"
#include "BKE_constraint.h"
#include "BKE_particle.h"
#include "BKE_unit.h"
#include "BKE_scene.h"
#include "BKE_mask.h"
#include "BKE_mesh.h"
#include "BKE_report.h"
#include "BKE_workspace.h"

#include "DEG_depsgraph.h"

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

#include "WM_types.h"
#include "WM_api.h"

#include "UI_view2d.h"
#include "UI_interface.h"
#include "UI_interface_icons.h"
#include "UI_resources.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "BLF_api.h"
#include "BLT_translation.h"

#include "transform.h"
#include "transform_op.h"

/* -------------------------------------------------------------------- */
/* Transform (EditBone (B-bone) width scaling) */

/** \name Transform B-bone width scaling
 * \{ */

static void headerBoneSize(TransInfo *t, const float vec[3], char str[UI_MAX_DRAW_STR])
{
  char tvec[NUM_STR_REP_LEN * 3];
  if (hasNumInput(&t->num)) {
    outputNumInput(&(t->num), tvec, &t->scene->unit);
  }
  else {
    BLI_snprintf(&tvec[0], NUM_STR_REP_LEN, "%.4f", vec[0]);
    BLI_snprintf(&tvec[NUM_STR_REP_LEN], NUM_STR_REP_LEN, "%.4f", vec[1]);
    BLI_snprintf(&tvec[NUM_STR_REP_LEN * 2], NUM_STR_REP_LEN, "%.4f", vec[2]);
  }

  /* hmm... perhaps the y-axis values don't need to be shown? */
  if (t->con.mode & CON_APPLY) {
    if (t->num.idx_max == 0) {
      BLI_snprintf(
          str, UI_MAX_DRAW_STR, TIP_("ScaleB: %s%s %s"), &tvec[0], t->con.text, t->proptext);
    }
    else {
      BLI_snprintf(str,
                   UI_MAX_DRAW_STR,
                   TIP_("ScaleB: %s : %s : %s%s %s"),
                   &tvec[0],
                   &tvec[NUM_STR_REP_LEN],
                   &tvec[NUM_STR_REP_LEN * 2],
                   t->con.text,
                   t->proptext);
    }
  }
  else {
    BLI_snprintf(str,
                 UI_MAX_DRAW_STR,
                 TIP_("ScaleB X: %s  Y: %s  Z: %s%s %s"),
                 &tvec[0],
                 &tvec[NUM_STR_REP_LEN],
                 &tvec[NUM_STR_REP_LEN * 2],
                 t->con.text,
                 t->proptext);
  }
}

static void ElementBoneSize(TransInfo *t, TransDataContainer *tc, TransData *td, float mat[3][3])
{
  float tmat[3][3], smat[3][3], oldy;
  float sizemat[3][3];

  mul_m3_m3m3(smat, mat, td->mtx);
  mul_m3_m3m3(tmat, td->smtx, smat);

  if (t->con.applySize) {
    t->con.applySize(t, tc, td, tmat);
  }

  /* we've tucked the scale in loc */
  oldy = td->iloc[1];
  size_to_mat3(sizemat, td->iloc);
  mul_m3_m3m3(tmat, tmat, sizemat);
  mat3_to_size(td->loc, tmat);
  td->loc[1] = oldy;
}

static void applyBoneSize(TransInfo *t, const int UNUSED(mval[2]))
{
  float size[3], mat[3][3];
  float ratio = t->values[0];
  int i;
  char str[UI_MAX_DRAW_STR];

  copy_v3_fl(size, ratio);

  snapGridIncrement(t, size);

  if (applyNumInput(&t->num, size)) {
    constraintNumInput(t, size);
  }

  copy_v3_v3(t->values_final, size);

  size_to_mat3(mat, size);

  if (t->con.applySize) {
    t->con.applySize(t, NULL, NULL, mat);
  }

  copy_m3_m3(t->mat, mat);  // used in gizmo

  headerBoneSize(t, size, str);

  FOREACH_TRANS_DATA_CONTAINER (t, tc) {
    TransData *td = tc->data;
    for (i = 0; i < tc->data_len; i++, td++) {
      if (td->flag & TD_NOACTION) {
        break;
      }

      if (td->flag & TD_SKIP) {
        continue;
      }

      ElementBoneSize(t, tc, td, mat);
    }
  }

  recalcData(t);

  ED_area_status_text(t->sa, str);
}

void initBoneSize(TransInfo *t)
{
  t->mode = TFM_BONESIZE;
  t->transform = applyBoneSize;

  initMouseInputMode(t, &t->mouse, INPUT_SPRING_FLIP);

  t->idx_max = 2;
  t->num.idx_max = 2;
  t->num.val_flag[0] |= NUM_NULL_ONE;
  t->num.val_flag[1] |= NUM_NULL_ONE;
  t->num.val_flag[2] |= NUM_NULL_ONE;
  t->num.flag |= NUM_AFFECT_ALL;
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
