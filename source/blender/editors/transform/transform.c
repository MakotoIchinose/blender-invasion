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

#include "DNA_movieclip_types.h"
#include "DNA_scene_types.h" /* PET modes */

#include "BLI_utildefines.h"
#include "BLI_math.h"
#include "BLI_rect.h"

#include "BKE_context.h"
#include "BKE_mask.h"

#include "ED_image.h"
#include "ED_screen.h"
#include "ED_view3d.h"
#include "ED_clip.h"

#include "UI_view2d.h"

#include "transform.h"
#include "transform_convert.h"

/* Disabling, since when you type you know what you are doing,
 * and being able to set it to zero is handy. */
// #define USE_NUM_NO_ZERO

bool transdata_check_local_center(TransInfo *t, short around)
{
  return ((around == V3D_AROUND_LOCAL_ORIGINS) &&
          ((t->flag & (T_OBJECT | T_POSE)) ||
           /* implicit: (t->flag & T_EDIT) */
           (ELEM(t->obedit_type, OB_MESH, OB_CURVE, OB_MBALL, OB_ARMATURE, OB_GPENCIL)) ||
           (t->spacetype == SPACE_GRAPH) ||
           (t->options & (CTX_MOVIECLIP | CTX_MASK | CTX_PAINT_CURVE))));
}

bool transdata_check_local_islands(TransInfo *t, short around)
{
  return ((around == V3D_AROUND_LOCAL_ORIGINS) && ((ELEM(t->obedit_type, OB_MESH))));
}

/* ************************** SPACE DEPENDENT CODE **************************** */

void setTransformViewMatrices(TransInfo *t)
{
  if (t->spacetype == SPACE_VIEW3D && t->ar && t->ar->regiontype == RGN_TYPE_WINDOW) {
    RegionView3D *rv3d = t->ar->regiondata;

    copy_m4_m4(t->viewmat, rv3d->viewmat);
    copy_m4_m4(t->viewinv, rv3d->viewinv);
    copy_m4_m4(t->persmat, rv3d->persmat);
    copy_m4_m4(t->persinv, rv3d->persinv);
    t->persp = rv3d->persp;
  }
  else {
    unit_m4(t->viewmat);
    unit_m4(t->viewinv);
    unit_m4(t->persmat);
    unit_m4(t->persinv);
    t->persp = RV3D_ORTHO;
  }

  calculateCenter2D(t);
  calculateCenterLocal(t, t->center_global);
}

void setTransformViewAspect(TransInfo *t, float r_aspect[3])
{
  copy_v3_fl(r_aspect, 1.0f);

  if (t->spacetype == SPACE_IMAGE) {
    SpaceImage *sima = t->sa->spacedata.first;

    if (t->options & CTX_MASK) {
      ED_space_image_get_aspect(sima, &r_aspect[0], &r_aspect[1]);
    }
    else if (t->options & CTX_PAINT_CURVE) {
      /* pass */
    }
    else {
      ED_space_image_get_uv_aspect(sima, &r_aspect[0], &r_aspect[1]);
    }
  }
  else if (t->spacetype == SPACE_CLIP) {
    SpaceClip *sclip = t->sa->spacedata.first;

    if (t->options & CTX_MOVIECLIP) {
      ED_space_clip_get_aspect_dimension_aware(sclip, &r_aspect[0], &r_aspect[1]);
    }
    else {
      ED_space_clip_get_aspect(sclip, &r_aspect[0], &r_aspect[1]);
    }
  }
  else if (t->spacetype == SPACE_GRAPH) {
    /* depemds on context of usage */
  }
}

static void convertViewVec2D(View2D *v2d, float r_vec[3], int dx, int dy)
{
  float divx = BLI_rcti_size_x(&v2d->mask);
  float divy = BLI_rcti_size_y(&v2d->mask);

  r_vec[0] = BLI_rctf_size_x(&v2d->cur) * dx / divx;
  r_vec[1] = BLI_rctf_size_y(&v2d->cur) * dy / divy;
  r_vec[2] = 0.0f;
}

static void convertViewVec2D_mask(View2D *v2d, float r_vec[3], int dx, int dy)
{
  float divx = BLI_rcti_size_x(&v2d->mask);
  float divy = BLI_rcti_size_y(&v2d->mask);

  float mulx = BLI_rctf_size_x(&v2d->cur);
  float muly = BLI_rctf_size_y(&v2d->cur);

  /* difference with convertViewVec2D */
  /* clamp w/h, mask only */
  if (mulx / divx < muly / divy) {
    divy = divx;
    muly = mulx;
  }
  else {
    divx = divy;
    mulx = muly;
  }
  /* end difference */

  r_vec[0] = mulx * dx / divx;
  r_vec[1] = muly * dy / divy;
  r_vec[2] = 0.0f;
}

void convertViewVec(TransInfo *t, float r_vec[3], double dx, double dy)
{
  if ((t->spacetype == SPACE_VIEW3D) && (t->ar->regiontype == RGN_TYPE_WINDOW)) {
    if (t->options & CTX_PAINT_CURVE) {
      r_vec[0] = dx;
      r_vec[1] = dy;
    }
    else {
      const float mval_f[2] = {(float)dx, (float)dy};
      ED_view3d_win_to_delta(t->ar, mval_f, r_vec, t->zfac);
    }
  }
  else if (t->spacetype == SPACE_IMAGE) {
    if (t->options & CTX_MASK) {
      convertViewVec2D_mask(t->view, r_vec, dx, dy);
    }
    else if (t->options & CTX_PAINT_CURVE) {
      r_vec[0] = dx;
      r_vec[1] = dy;
    }
    else {
      convertViewVec2D(t->view, r_vec, dx, dy);
    }

    r_vec[0] *= t->aspect[0];
    r_vec[1] *= t->aspect[1];
  }
  else if (ELEM(t->spacetype, SPACE_GRAPH, SPACE_NLA)) {
    convertViewVec2D(t->view, r_vec, dx, dy);
  }
  else if (ELEM(t->spacetype, SPACE_NODE, SPACE_SEQ)) {
    convertViewVec2D(&t->ar->v2d, r_vec, dx, dy);
  }
  else if (t->spacetype == SPACE_CLIP) {
    if (t->options & CTX_MASK) {
      convertViewVec2D_mask(t->view, r_vec, dx, dy);
    }
    else {
      convertViewVec2D(t->view, r_vec, dx, dy);
    }

    r_vec[0] *= t->aspect[0];
    r_vec[1] *= t->aspect[1];
  }
  else {
    printf("%s: called in an invalid context\n", __func__);
    zero_v3(r_vec);
  }
}

void projectIntViewEx(TransInfo *t, const float vec[3], int adr[2], const eV3DProjTest flag)
{
  if (t->spacetype == SPACE_VIEW3D) {
    if (t->ar->regiontype == RGN_TYPE_WINDOW) {
      if (ED_view3d_project_int_global(t->ar, vec, adr, flag) != V3D_PROJ_RET_OK) {
        /* this is what was done in 2.64, perhaps we can be smarter? */
        adr[0] = (int)2140000000.0f;
        adr[1] = (int)2140000000.0f;
      }
    }
  }
  else if (t->spacetype == SPACE_IMAGE) {
    SpaceImage *sima = t->sa->spacedata.first;

    if (t->options & CTX_MASK) {
      float v[2];

      v[0] = vec[0] / t->aspect[0];
      v[1] = vec[1] / t->aspect[1];

      BKE_mask_coord_to_image(sima->image, &sima->iuser, v, v);

      ED_image_point_pos__reverse(sima, t->ar, v, v);

      adr[0] = v[0];
      adr[1] = v[1];
    }
    else if (t->options & CTX_PAINT_CURVE) {
      adr[0] = vec[0];
      adr[1] = vec[1];
    }
    else {
      float v[2];

      v[0] = vec[0] / t->aspect[0];
      v[1] = vec[1] / t->aspect[1];

      UI_view2d_view_to_region(t->view, v[0], v[1], &adr[0], &adr[1]);
    }
  }
  else if (t->spacetype == SPACE_ACTION) {
    int out[2] = {0, 0};
#if 0
    SpaceAction *sact = t->sa->spacedata.first;

    if (sact->flag & SACTION_DRAWTIME) {
      //vec[0] = vec[0]/((t->scene->r.frs_sec / t->scene->r.frs_sec_base));
      /* same as below */
      UI_view2d_view_to_region((View2D *)t->view, vec[0], vec[1], &out[0], &out[1]);
    }
    else
#endif
    {
      UI_view2d_view_to_region((View2D *)t->view, vec[0], vec[1], &out[0], &out[1]);
    }

    adr[0] = out[0];
    adr[1] = out[1];
  }
  else if (ELEM(t->spacetype, SPACE_GRAPH, SPACE_NLA)) {
    int out[2] = {0, 0};

    UI_view2d_view_to_region((View2D *)t->view, vec[0], vec[1], &out[0], &out[1]);
    adr[0] = out[0];
    adr[1] = out[1];
  }
  else if (t->spacetype == SPACE_SEQ) { /* XXX not tested yet, but should work */
    int out[2] = {0, 0};

    UI_view2d_view_to_region((View2D *)t->view, vec[0], vec[1], &out[0], &out[1]);
    adr[0] = out[0];
    adr[1] = out[1];
  }
  else if (t->spacetype == SPACE_CLIP) {
    SpaceClip *sc = t->sa->spacedata.first;

    if (t->options & CTX_MASK) {
      MovieClip *clip = ED_space_clip_get_clip(sc);

      if (clip) {
        float v[2];

        v[0] = vec[0] / t->aspect[0];
        v[1] = vec[1] / t->aspect[1];

        BKE_mask_coord_to_movieclip(sc->clip, &sc->user, v, v);

        ED_clip_point_stable_pos__reverse(sc, t->ar, v, v);

        adr[0] = v[0];
        adr[1] = v[1];
      }
      else {
        adr[0] = 0;
        adr[1] = 0;
      }
    }
    else if (t->options & CTX_MOVIECLIP) {
      float v[2];

      v[0] = vec[0] / t->aspect[0];
      v[1] = vec[1] / t->aspect[1];

      UI_view2d_view_to_region(t->view, v[0], v[1], &adr[0], &adr[1]);
    }
    else {
      BLI_assert(0);
    }
  }
  else if (t->spacetype == SPACE_NODE) {
    UI_view2d_view_to_region((View2D *)t->view, vec[0], vec[1], &adr[0], &adr[1]);
  }
}
void projectIntView(TransInfo *t, const float vec[3], int adr[2])
{
  projectIntViewEx(t, vec, adr, V3D_PROJ_TEST_NOP);
}

void projectFloatViewEx(TransInfo *t, const float vec[3], float adr[2], const eV3DProjTest flag)
{
  switch (t->spacetype) {
    case SPACE_VIEW3D: {
      if (t->options & CTX_PAINT_CURVE) {
        adr[0] = vec[0];
        adr[1] = vec[1];
      }
      else if (t->ar->regiontype == RGN_TYPE_WINDOW) {
        /* allow points behind the view [#33643] */
        if (ED_view3d_project_float_global(t->ar, vec, adr, flag) != V3D_PROJ_RET_OK) {
          /* XXX, 2.64 and prior did this, weak! */
          adr[0] = t->ar->winx / 2.0f;
          adr[1] = t->ar->winy / 2.0f;
        }
        return;
      }
      break;
    }
    default: {
      int a[2] = {0, 0};
      projectIntView(t, vec, a);
      adr[0] = a[0];
      adr[1] = a[1];
      break;
    }
  }
}
void projectFloatView(TransInfo *t, const float vec[3], float adr[2])
{
  projectFloatViewEx(t, vec, adr, V3D_PROJ_TEST_NOP);
}

void removeAspectRatio(TransInfo *t, float vec[2])
{
  if ((t->spacetype == SPACE_IMAGE) && (t->mode == TFM_TRANSLATION)) {
    SpaceImage *sima = t->sa->spacedata.first;

    if ((sima->flag & SI_COORDFLOATS) == 0) {
      int width, height;
      ED_space_image_get_size(sima, &width, &height);

      vec[0] /= width;
      vec[1] /= height;
    }

    vec[0] *= t->aspect[0];
    vec[1] *= t->aspect[1];
  }
  else if ((t->spacetype == SPACE_CLIP) && (t->mode == TFM_TRANSLATION)) {
    if (t->options & (CTX_MOVIECLIP | CTX_MASK)) {
      vec[0] *= t->aspect[0];
      vec[1] *= t->aspect[1];
    }
  }
}

/* ************************** TRANSFORMATIONS **************************** */

bool calculateTransformCenter(bContext *C, int centerMode, float cent3d[3], float cent2d[2])
{
  TransInfo *t = MEM_callocN(sizeof(TransInfo), "TransInfo data");
  bool success;

  t->context = C;

  t->state = TRANS_RUNNING;

  /* avoid calculating PET */
  t->options = CTX_NO_PET;

  t->mode = TFM_DUMMY;

  initTransInfo(C, t, NULL, NULL);

  /* avoid doing connectivity lookups (when V3D_AROUND_LOCAL_ORIGINS is set) */
  t->around = V3D_AROUND_CENTER_BOUNDS;

  createTransData(C, t);  // make TransData structs from selection

  t->around = centerMode;  // override userdefined mode

  if (t->data_len_all == 0) {
    success = false;
  }
  else {
    success = true;

    calculateCenter(t);

    if (cent2d) {
      copy_v2_v2(cent2d, t->center2d);
    }

    if (cent3d) {
      // Copy center from constraint center. Transform center can be local
      copy_v3_v3(cent3d, t->center_global);
    }
  }

  /* aftertrans does insert keyframes, and clears base flags; doesn't read transdata */
  special_aftertrans_update(C, t);

  postTrans(C, t);

  MEM_freeN(t);

  return success;
}

/* TODO, move to: transform_query.c */
bool checkUseAxisMatrix(TransInfo *t)
{
  /* currently only checks for editmode */
  if (t->flag & T_EDIT) {
    if ((t->around == V3D_AROUND_LOCAL_ORIGINS) &&
        (ELEM(t->obedit_type, OB_MESH, OB_CURVE, OB_MBALL, OB_ARMATURE))) {
      /* not all editmode supports axis-matrix */
      return true;
    }
  }

  return false;
}
