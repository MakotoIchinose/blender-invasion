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
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup bke
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>

#include "MEM_guardedalloc.h"

#include "DNA_color_types.h"
#include "DNA_curve_types.h"

#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "BLI_task.h"
#include "BLI_threads.h"

#include "BKE_profile_path.h"
#include "BKE_curve.h"
#include "BKE_fcurve.h"

#include "IMB_colormanagement.h"
#include "IMB_imbuf_types.h"

void profilepath_set_defaults(
    CurveMapping *cumap, int tot, float minx, float miny, float maxx, float maxy)
{
  int a;
  float clipminx, clipminy, clipmaxx, clipmaxy;

  cumap->flag = CUMA_DO_CLIP;
  if (tot == 4) {
    cumap->cur = 3; /* rhms, hack for 'col' curve? */
  }

  clipminx = min_ff(minx, maxx);
  clipminy = min_ff(miny, maxy);
  clipmaxx = max_ff(minx, maxx);
  clipmaxy = max_ff(miny, maxy);

  BLI_rctf_init(&cumap->curr, clipminx, clipmaxx, clipminy, clipmaxy);
  cumap->clipr = cumap->curr;

  cumap->white[0] = cumap->white[1] = cumap->white[2] = 1.0f;
  cumap->bwmul[0] = cumap->bwmul[1] = cumap->bwmul[2] = 1.0f;

  for (a = 0; a < tot; a++) {
    cumap->cm[a].flag = CUMA_EXTEND_EXTRAPOLATE;
    cumap->cm[a].totpoint = 2;
    cumap->cm[a].curve = MEM_callocN(2 * sizeof(CurveMapPoint), "curve points");

    cumap->cm[a].curve[0].x = minx;
    cumap->cm[a].curve[0].y = miny;
    cumap->cm[a].curve[1].x = maxx;
    cumap->cm[a].curve[1].y = maxy;
  }

  cumap->changed_timestamp = 0;
}

CurveMapping *profilepath_add(int tot, float minx, float miny, float maxx, float maxy)
{
  CurveMapping *cumap;

  cumap = MEM_callocN(sizeof(CurveMapping), "new curvemap");

  curvemapping_set_defaults(cumap, tot, minx, miny, maxx, maxy);

  return cumap;
}

void profilepath_free_data(CurveMapping *cumap)
{
  if (cumap->cm->curve) {
    MEM_freeN(cumap->cm->curve);
    cumap->cm->curve = NULL;
  }
  if (cumap->cm->table) {
    MEM_freeN(cumap->cm->table);
    cumap->cm->table = NULL;
  }
  if (cumap->cm->premultable) {
    MEM_freeN(cumap->cm->premultable);
    cumap->cm->premultable = NULL;
  }
  if (cumap->cm->x_segment_vals) {
    MEM_freeN(cumap->cm->x_segment_vals);
    cumap->cm->x_segment_vals = NULL;
  }
  if (cumap->cm->y_segment_vals) {
    MEM_freeN(cumap->cm->y_segment_vals);
    cumap->cm->y_segment_vals = NULL;
  }
}

void profilepath_free(CurveMapping *cumap)
{
  if (cumap) {
    curvemapping_free_data(cumap);
    MEM_freeN(cumap);
  }
}

void profilepath_copy_data(CurveMapping *target, const CurveMapping *cumap)
{
  *target = *cumap;

    if (cumap->cm->curve) {
      target->cm->curve = MEM_dupallocN(cumap->cm->curve);
    }
    if (cumap->cm->table) {
      target->cm->table = MEM_dupallocN(cumap->cm->table);
    }
    if (cumap->cm->premultable) {
      target->cm->premultable = MEM_dupallocN(cumap->cm->premultable);
    }
    if (cumap->cm->x_segment_vals) {
      target->cm->x_segment_vals = MEM_dupallocN(cumap->cm->x_segment_vals);
    }
    if (cumap->cm->y_segment_vals) {
      target->cm->y_segment_vals = MEM_dupallocN(cumap->cm->y_segment_vals);
    }
}

CurveMapping *profilepath_copy(const CurveMapping *cumap)
{
  if (cumap) {
    CurveMapping *cumapn = MEM_dupallocN(cumap);
    curvemapping_copy_data(cumapn, cumap);
    return cumapn;
  }
  return NULL;
}

/* HANS_TODO: Delete */
void profilepath_black_white_ex(const float black[3], const float white[3], float r_bwmul[3])
{
  int a;

  for (a = 0; a < 3; a++) {
    const float delta = max_ff(white[a] - black[a], 1e-5f);
    r_bwmul[a] = 1.0f / delta;
  }
}

void profilepath_set_black_white(CurveMapping *cumap, const float black[3], const float white[3])
{
  if (white) {
    copy_v3_v3(cumap->white, white);
  }
  if (black) {
    copy_v3_v3(cumap->black, black);
  }

  curvemapping_set_black_white_ex(cumap->black, cumap->white, cumap->bwmul);
  cumap->changed_timestamp++;
}

/* ********** NOTE: requires curvemapping_changed() call after ******** */

/* remove specified point */
bool profile_remove_point(CurveMap *cuma, CurveMapPoint *point)
{
  CurveMapPoint *cmp;
  int a, b, removed = 0;

  /* must have 2 points minimum */
  if (cuma->totpoint <= 2) {
    return false;
  }

  cmp = MEM_mallocN((cuma->totpoint) * sizeof(CurveMapPoint), "curve points");

  /* well, lets keep the two outer points! */
  for (a = 0, b = 0; a < cuma->totpoint; a++) {
    if (&cuma->curve[a] != point) {
      cmp[b] = cuma->curve[a];
      b++;
    }
    else {
      removed++;
    }
  }

  MEM_freeN(cuma->curve);
  cuma->curve = cmp;
  cuma->totpoint -= removed;
  return (removed != 0);
}

/* removes with flag set */
void profile_remove(CurveMap *cuma, const short flag)
{
  CurveMapPoint *cmp = MEM_mallocN((cuma->totpoint) * sizeof(CurveMapPoint), "curve points");
  int a, b, removed = 0;

  /* well, lets keep the two outer points! */
  cmp[0] = cuma->curve[0];
  for (a = 1, b = 1; a < cuma->totpoint - 1; a++) {
    if (!(cuma->curve[a].flag & flag)) {
      cmp[b] = cuma->curve[a];
      b++;
    }
    else {
      removed++;
    }
  }
  cmp[b] = cuma->curve[a];

  MEM_freeN(cuma->curve);
  cuma->curve = cmp;
  cuma->totpoint -= removed;
}

CurveMapPoint *profile_insert(CurveMap *cuma, float x, float y)
{
  CurveMapPoint *cmp = MEM_callocN((cuma->totpoint + 1) * sizeof(CurveMapPoint), "curve points");
  CurveMapPoint *newcmp = NULL;
  int a, b;
  bool foundloc = false;

  /* insert fragments of the old one and the new point to the new curve */
  cuma->totpoint++;
  for (a = 0, b = 0; a < cuma->totpoint; a++) {
    if ((foundloc == false) && ((a + 1 == cuma->totpoint) || (x < cuma->curve[a].x))) {
      cmp[a].x = x;
      cmp[a].y = y;
      cmp[a].flag = CUMA_SELECT;
      foundloc = true;
      newcmp = &cmp[a];
    }
    else {
      cmp[a].x = cuma->curve[b].x;
      cmp[a].y = cuma->curve[b].y;
      /* make sure old points don't remain selected */
      cmp[a].flag = cuma->curve[b].flag & ~CUMA_SELECT;
      cmp[a].shorty = cuma->curve[b].shorty;
      b++;
    }
  }

  /* free old curve and replace it with new one */
  MEM_freeN(cuma->curve);
  cuma->curve = cmp;

  return newcmp;
}

void profile_reset(CurveMap *cuma, const rctf *clipr, int preset, int slope)
{
  if (cuma->curve) {
    MEM_freeN(cuma->curve);
  }

  switch (preset) {
    case CURVE_PRESET_LINE:
      cuma->totpoint = 2;
      break;
    case CURVE_PRESET_SHARP:
      cuma->totpoint = 4;
      break;
    case CURVE_PRESET_SMOOTH:
      cuma->totpoint = 4;
      break;
    case CURVE_PRESET_MAX:
      cuma->totpoint = 2;
      break;
    case CURVE_PRESET_MID9:
      cuma->totpoint = 9;
      break;
    case CURVE_PRESET_ROUND:
      cuma->totpoint = 4;
      break;
    case CURVE_PRESET_ROOT:
      cuma->totpoint = 4;
      break;
    case CURVE_PRESET_GAUSS:
      cuma->totpoint = 7;
      break;
    case CURVE_PRESET_BELL:
      cuma->totpoint = 3;
      break;
  }

  cuma->curve = MEM_callocN(cuma->totpoint * sizeof(CurveMapPoint), "curve points");

  switch (preset) {
    case CURVE_PRESET_LINE:
      cuma->curve[0].x = clipr->xmin;
      cuma->curve[0].y = clipr->ymax;
      cuma->curve[1].x = clipr->xmax;
      cuma->curve[1].y = clipr->ymin;
      if (slope == CURVEMAP_SLOPE_POS_NEG) {
        cuma->curve[0].flag |= CUMA_HANDLE_VECTOR;
        cuma->curve[1].flag |= CUMA_HANDLE_VECTOR;
      }
      break;
    case CURVE_PRESET_SHARP:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 1;
      cuma->curve[1].x = 0.25f;
      cuma->curve[1].y = 0.50f;
      cuma->curve[2].x = 0.75f;
      cuma->curve[2].y = 0.04f;
      cuma->curve[3].x = 1;
      cuma->curve[3].y = 0;
      break;
    case CURVE_PRESET_SMOOTH:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 1;
      cuma->curve[1].x = 0.25f;
      cuma->curve[1].y = 0.94f;
      cuma->curve[2].x = 0.75f;
      cuma->curve[2].y = 0.06f;
      cuma->curve[3].x = 1;
      cuma->curve[3].y = 0;
      break;
    case CURVE_PRESET_MAX:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 1;
      cuma->curve[1].x = 1;
      cuma->curve[1].y = 1;
      break;
    case CURVE_PRESET_MID9: {
      int i;
      for (i = 0; i < cuma->totpoint; i++) {
        cuma->curve[i].x = i / ((float)cuma->totpoint - 1);
        cuma->curve[i].y = 0.5;
      }
      break;
    }
    case CURVE_PRESET_ROUND:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 1;
      cuma->curve[1].x = 0.5;
      cuma->curve[1].y = 0.90f;
      cuma->curve[2].x = 0.86f;
      cuma->curve[2].y = 0.5;
      cuma->curve[3].x = 1;
      cuma->curve[3].y = 0;
      break;
    case CURVE_PRESET_ROOT:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 1;
      cuma->curve[1].x = 0.25f;
      cuma->curve[1].y = 0.95f;
      cuma->curve[2].x = 0.75f;
      cuma->curve[2].y = 0.44f;
      cuma->curve[3].x = 1;
      cuma->curve[3].y = 0;
      break;
    case CURVE_PRESET_GAUSS:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 0.025f;
      cuma->curve[1].x = 0.16f;
      cuma->curve[1].y = 0.135f;
      cuma->curve[2].x = 0.298f;
      cuma->curve[2].y = 0.36f;

      cuma->curve[3].x = 0.50f;
      cuma->curve[3].y = 1.0f;

      cuma->curve[4].x = 0.70f;
      cuma->curve[4].y = 0.36f;
      cuma->curve[5].x = 0.84f;
      cuma->curve[5].y = 0.135f;
      cuma->curve[6].x = 1.0f;
      cuma->curve[6].y = 0.025f;
      break;
    case CURVE_PRESET_BELL:
      cuma->curve[0].x = 0;
      cuma->curve[0].y = 0.025f;

      cuma->curve[1].x = 0.50f;
      cuma->curve[1].y = 1.0f;

      cuma->curve[2].x = 1.0f;
      cuma->curve[2].y = 0.025f;
      break;
  }

  /* mirror curve in x direction to have positive slope
   * rather than default negative slope */
  if (slope == CURVEMAP_SLOPE_POSITIVE) {
    int i, last = cuma->totpoint - 1;
    CurveMapPoint *newpoints = MEM_dupallocN(cuma->curve);

    for (i = 0; i < cuma->totpoint; i++) {
      newpoints[i].y = cuma->curve[last - i].y;
    }

    MEM_freeN(cuma->curve);
    cuma->curve = newpoints;
  }
  else if (slope == CURVEMAP_SLOPE_POS_NEG) {
    const int num_points = cuma->totpoint * 2 - 1;
    CurveMapPoint *new_points = MEM_mallocN(num_points * sizeof(CurveMapPoint),
                                            "curve symmetric points");
    int i;
    for (i = 0; i < cuma->totpoint; i++) {
      const int src_last_point = cuma->totpoint - i - 1;
      const int dst_last_point = num_points - i - 1;
      new_points[i] = cuma->curve[src_last_point];
      new_points[i].x = (1.0f - cuma->curve[src_last_point].x) * 0.5f;
      new_points[dst_last_point] = new_points[i];
      new_points[dst_last_point].x = 0.5f + cuma->curve[src_last_point].x * 0.5f;
    }
    cuma->totpoint = num_points;
    MEM_freeN(cuma->curve);
    cuma->curve = new_points;
  }

  if (cuma->table) {
    MEM_freeN(cuma->table);
    cuma->table = NULL;
  }
}

/**
 * \param type: eBezTriple_Handle
 */
void profile_handle_set(CurveMap *cuma, int type)
{
  int a;

  for (a = 0; a < cuma->totpoint; a++) {
    if (cuma->curve[a].flag & CUMA_SELECT) {
      cuma->curve[a].flag &= ~(CUMA_HANDLE_VECTOR | CUMA_HANDLE_AUTO_ANIM);
      if (type == HD_VECT) {
        cuma->curve[a].flag |= CUMA_HANDLE_VECTOR;
      }
      else if (type == HD_AUTO_ANIM) {
        cuma->curve[a].flag |= CUMA_HANDLE_AUTO_ANIM;
      }
      else {
        /* pass */
      }
    }
  }
}

/* *********************** Making the tables and display ************** */

/**
 * reduced copy of #calchandleNurb_intern code in curve.c
 */
/* HANS-TODO: Update */
static void calchandle_profile(BezTriple *bezt, const BezTriple *prev, const BezTriple *next)
{
  /* defines to avoid confusion */
#define p2_h1 ((p2)-3)
#define p2_h2 ((p2) + 3)

  const float *p1, *p3;
  float *p2;
  float pt[3];
  float len, len_a, len_b;
  float dvec_a[2], dvec_b[2];

  if (bezt->h1 == 0 && bezt->h2 == 0) {
    return;
  }

  p2 = bezt->vec[1];

  if (prev == NULL) {
    p3 = next->vec[1];
    pt[0] = 2.0f * p2[0] - p3[0];
    pt[1] = 2.0f * p2[1] - p3[1];
    p1 = pt;
  }
  else {
    p1 = prev->vec[1];
  }

  if (next == NULL) {
    p1 = prev->vec[1];
    pt[0] = 2.0f * p2[0] - p1[0];
    pt[1] = 2.0f * p2[1] - p1[1];
    p3 = pt;
  }
  else {
    p3 = next->vec[1];
  }

  sub_v2_v2v2(dvec_a, p2, p1);
  sub_v2_v2v2(dvec_b, p3, p2);

  len_a = len_v2(dvec_a);
  len_b = len_v2(dvec_b);

  if (len_a == 0.0f) {
    len_a = 1.0f;
  }
  if (len_b == 0.0f) {
    len_b = 1.0f;
  }

  if (ELEM(bezt->h1, HD_AUTO, HD_AUTO_ANIM) || ELEM(bezt->h2, HD_AUTO, HD_AUTO_ANIM)) { /* auto */
    float tvec[2];
    tvec[0] = dvec_b[0] / len_b + dvec_a[0] / len_a;
    tvec[1] = dvec_b[1] / len_b + dvec_a[1] / len_a;

    len = len_v2(tvec) * 2.5614f;
    if (len != 0.0f) {

      if (ELEM(bezt->h1, HD_AUTO, HD_AUTO_ANIM)) {
        len_a /= len;
        madd_v2_v2v2fl(p2_h1, p2, tvec, -len_a);

        if ((bezt->h1 == HD_AUTO_ANIM) && next && prev) { /* keep horizontal if extrema */
          const float ydiff1 = prev->vec[1][1] - bezt->vec[1][1];
          const float ydiff2 = next->vec[1][1] - bezt->vec[1][1];
          if ((ydiff1 <= 0.0f && ydiff2 <= 0.0f) || (ydiff1 >= 0.0f && ydiff2 >= 0.0f)) {
            bezt->vec[0][1] = bezt->vec[1][1];
          }
          else { /* handles should not be beyond y coord of two others */
            if (ydiff1 <= 0.0f) {
              if (prev->vec[1][1] > bezt->vec[0][1]) {
                bezt->vec[0][1] = prev->vec[1][1];
              }
            }
            else {
              if (prev->vec[1][1] < bezt->vec[0][1]) {
                bezt->vec[0][1] = prev->vec[1][1];
              }
            }
          }
        }
      }
      if (ELEM(bezt->h2, HD_AUTO, HD_AUTO_ANIM)) {
        len_b /= len;
        madd_v2_v2v2fl(p2_h2, p2, tvec, len_b);

        if ((bezt->h2 == HD_AUTO_ANIM) && next && prev) { /* keep horizontal if extrema */
          const float ydiff1 = prev->vec[1][1] - bezt->vec[1][1];
          const float ydiff2 = next->vec[1][1] - bezt->vec[1][1];
          if ((ydiff1 <= 0.0f && ydiff2 <= 0.0f) || (ydiff1 >= 0.0f && ydiff2 >= 0.0f)) {
            bezt->vec[2][1] = bezt->vec[1][1];
          }
          else { /* handles should not be beyond y coord of two others */
            if (ydiff1 <= 0.0f) {
              if (next->vec[1][1] < bezt->vec[2][1]) {
                bezt->vec[2][1] = next->vec[1][1];
              }
            }
            else {
              if (next->vec[1][1] > bezt->vec[2][1]) {
                bezt->vec[2][1] = next->vec[1][1];
              }
            }
          }
        }
      }
    }
  }

  if (bezt->h1 == HD_VECT) { /* vector */
    madd_v2_v2v2fl(p2_h1, p2, dvec_a, -1.0f / 3.0f);
  }
  if (bezt->h2 == HD_VECT) {
    madd_v2_v2v2fl(p2_h2, p2, dvec_b, 1.0f / 3.0f);
  }

#undef p2_h1
#undef p2_h2
}

/* in X, out Y.
 * X is presumed to be outside first or last */
/* HANS-TODO: Probably delete */
static float profile_calc_extend(const CurveMap *cuma,
                                  float x,
                                  const float first[2],
                                  const float last[2])
{
  if (x <= first[0]) {
    if ((cuma->flag & CUMA_EXTEND_EXTRAPOLATE) == 0) {
      /* no extrapolate */
      return first[1];
    }
    else {
      if (cuma->ext_in[0] == 0.0f) {
        return first[1] + cuma->ext_in[1] * 10000.0f;
      }
      else {
        return first[1] + cuma->ext_in[1] * (x - first[0]) / cuma->ext_in[0];
      }
    }
  }
  else if (x >= last[0]) {
    if ((cuma->flag & CUMA_EXTEND_EXTRAPOLATE) == 0) {
      /* no extrapolate */
      return last[1];
    }
    else {
      if (cuma->ext_out[0] == 0.0f) {
        return last[1] - cuma->ext_out[1] * 10000.0f;
      }
      else {
        return last[1] + cuma->ext_out[1] * (x - last[0]) / cuma->ext_out[0];
      }
    }
  }
  return 0.0f;
}

/* only creates a table for a single channel in CurveMapping */
/* HANS-TODO: Update */
static void profile_make_table(CurveMap *cuma, const rctf *clipr)
{
  CurveMapPoint *cmp = cuma->curve;
  BezTriple *bezt;
  float *fp, *allpoints, *lastpoint, curf, range;
  int a, totpoint;

  if (cuma->curve == NULL) {
    return;
  }

  /* default rect also is table range */
  cuma->mintable = clipr->xmin;
  cuma->maxtable = clipr->xmax;

  /* hrmf... we now rely on blender ipo beziers, these are more advanced */
  bezt = MEM_callocN(cuma->totpoint * sizeof(BezTriple), "beztarr");

  for (a = 0; a < cuma->totpoint; a++) {
    cuma->mintable = min_ff(cuma->mintable, cmp[a].x);
    cuma->maxtable = max_ff(cuma->maxtable, cmp[a].x);
    bezt[a].vec[1][0] = cmp[a].x;
    bezt[a].vec[1][1] = cmp[a].y;
    if (cmp[a].flag & CUMA_HANDLE_VECTOR) {
      bezt[a].h1 = bezt[a].h2 = HD_VECT;
    }
    else if (cmp[a].flag & CUMA_HANDLE_AUTO_ANIM) {
      bezt[a].h1 = bezt[a].h2 = HD_AUTO_ANIM;
    }
    else {
      bezt[a].h1 = bezt[a].h2 = HD_AUTO;
    }
  }

  const BezTriple *bezt_prev = NULL;
  for (a = 0; a < cuma->totpoint; a++) {
    const BezTriple *bezt_next = (a != cuma->totpoint - 1) ? &bezt[a + 1] : NULL;
    calchandle_profile(&bezt[a], bezt_prev, bezt_next);
    bezt_prev = &bezt[a];
  }

  /* first and last handle need correction, instead of pointing to center of next/prev,
   * we let it point to the closest handle */
  if (cuma->totpoint > 2) {
    float hlen, nlen, vec[3];

    if (bezt[0].h2 == HD_AUTO) {

      hlen = len_v3v3(bezt[0].vec[1], bezt[0].vec[2]); /* original handle length */
      /* clip handle point */
      copy_v3_v3(vec, bezt[1].vec[0]);
      if (vec[0] < bezt[0].vec[1][0]) {
        vec[0] = bezt[0].vec[1][0];
      }

      sub_v3_v3(vec, bezt[0].vec[1]);
      nlen = len_v3(vec);
      if (nlen > FLT_EPSILON) {
        mul_v3_fl(vec, hlen / nlen);
        add_v3_v3v3(bezt[0].vec[2], vec, bezt[0].vec[1]);
        sub_v3_v3v3(bezt[0].vec[0], bezt[0].vec[1], vec);
      }
    }
    a = cuma->totpoint - 1;
    if (bezt[a].h2 == HD_AUTO) {

      hlen = len_v3v3(bezt[a].vec[1], bezt[a].vec[0]); /* original handle length */
      /* clip handle point */
      copy_v3_v3(vec, bezt[a - 1].vec[2]);
      if (vec[0] > bezt[a].vec[1][0]) {
        vec[0] = bezt[a].vec[1][0];
      }

      sub_v3_v3(vec, bezt[a].vec[1]);
      nlen = len_v3(vec);
      if (nlen > FLT_EPSILON) {
        mul_v3_fl(vec, hlen / nlen);
        add_v3_v3v3(bezt[a].vec[0], vec, bezt[a].vec[1]);
        sub_v3_v3v3(bezt[a].vec[2], bezt[a].vec[1], vec);
      }
    }
  }
  /* make the bezier curve */
  if (cuma->table) {
    MEM_freeN(cuma->table);
  }
  totpoint = (cuma->totpoint - 1) * CM_RESOL;
  fp = allpoints = MEM_callocN(totpoint * 2 * sizeof(float), "table");

  for (a = 0; a < cuma->totpoint - 1; a++, fp += 2 * CM_RESOL) {
    correct_bezpart(bezt[a].vec[1], bezt[a].vec[2], bezt[a + 1].vec[0], bezt[a + 1].vec[1]);
    BKE_curve_forward_diff_bezier(bezt[a].vec[1][0],
                                  bezt[a].vec[2][0],
                                  bezt[a + 1].vec[0][0],
                                  bezt[a + 1].vec[1][0],
                                  fp,
                                  CM_RESOL - 1,
                                  2 * sizeof(float));
    BKE_curve_forward_diff_bezier(bezt[a].vec[1][1],
                                  bezt[a].vec[2][1],
                                  bezt[a + 1].vec[0][1],
                                  bezt[a + 1].vec[1][1],
                                  fp + 1,
                                  CM_RESOL - 1,
                                  2 * sizeof(float));
  }

  /* store first and last handle for extrapolation, unit length */
  cuma->ext_in[0] = bezt[0].vec[0][0] - bezt[0].vec[1][0];
  cuma->ext_in[1] = bezt[0].vec[0][1] - bezt[0].vec[1][1];
  range = sqrtf(cuma->ext_in[0] * cuma->ext_in[0] + cuma->ext_in[1] * cuma->ext_in[1]);
  cuma->ext_in[0] /= range;
  cuma->ext_in[1] /= range;

  a = cuma->totpoint - 1;
  cuma->ext_out[0] = bezt[a].vec[1][0] - bezt[a].vec[2][0];
  cuma->ext_out[1] = bezt[a].vec[1][1] - bezt[a].vec[2][1];
  range = sqrtf(cuma->ext_out[0] * cuma->ext_out[0] + cuma->ext_out[1] * cuma->ext_out[1]);
  cuma->ext_out[0] /= range;
  cuma->ext_out[1] /= range;

  /* cleanup */
  MEM_freeN(bezt);

  range = CM_TABLEDIV * (cuma->maxtable - cuma->mintable);
  cuma->range = 1.0f / range;

  /* now make a table with CM_TABLE equal x distances */
  fp = allpoints;
  lastpoint = allpoints + 2 * (totpoint - 1);
  cmp = MEM_callocN((CM_TABLE + 1) * sizeof(CurveMapPoint), "dist table");

  for (a = 0; a <= CM_TABLE; a++) {
    curf = cuma->mintable + range * (float)a;
    cmp[a].x = curf;

    /* get the first x coordinate larger than curf */
    while (curf >= fp[0] && fp != lastpoint) {
      fp += 2;
    }
    if (fp == allpoints || (curf >= fp[0] && fp == lastpoint)) {
      cmp[a].y = profile_calc_extend(cuma, curf, allpoints, lastpoint);
    }
    else {
      float fac1 = fp[0] - fp[-2];
      float fac2 = fp[0] - curf;
      if (fac1 > FLT_EPSILON) {
        fac1 = fac2 / fac1;
      }
      else {
        fac1 = 0.0f;
      }
      cmp[a].y = fac1 * fp[-1] + (1.0f - fac1) * fp[1];
    }
  }

  MEM_freeN(allpoints);
  cuma->table = cmp;
}

/* call when you do images etc, needs restore too. also verifies tables */
/* it uses a flag to prevent premul or free to happen twice */
/* HANS-TODO: Probably delete */
void profilepath_premultiply(CurveMapping *cumap, int restore)
{
  int a;

  if (restore) {
    if (cumap->flag & CUMA_PREMULLED) {
      for (a = 0; a < 3; a++) {
        MEM_freeN(cumap->cm[a].table);
        cumap->cm[a].table = cumap->cm[a].premultable;
        cumap->cm[a].premultable = NULL;

        copy_v2_v2(cumap->cm[a].ext_in, cumap->cm[a].premul_ext_in);
        copy_v2_v2(cumap->cm[a].ext_out, cumap->cm[a].premul_ext_out);
        zero_v2(cumap->cm[a].premul_ext_in);
        zero_v2(cumap->cm[a].premul_ext_out);
      }

      cumap->flag &= ~CUMA_PREMULLED;
    }
  }
  else {
    if ((cumap->flag & CUMA_PREMULLED) == 0) {
      /* verify and copy */
      for (a = 0; a < 3; a++) {
        if (cumap->cm[a].table == NULL) {
         profile_make_table(cumap->cm + a, &cumap->clipr);
        }
        cumap->cm[a].premultable = cumap->cm[a].table;
        cumap->cm[a].table = MEM_mallocN((CM_TABLE + 1) * sizeof(CurveMapPoint), "premul table");
        memcpy(
            cumap->cm[a].table, cumap->cm[a].premultable, (CM_TABLE + 1) * sizeof(CurveMapPoint));
      }

      if (cumap->cm[3].table == NULL) {
        profile_make_table(cumap->cm + 3, &cumap->clipr);
      }

      /* premul */
      for (a = 0; a < 3; a++) {
        int b;
        for (b = 0; b <= CM_TABLE; b++) {
          cumap->cm[a].table[b].y = curvemap_evaluateF(cumap->cm + 3, cumap->cm[a].table[b].y);
        }

        copy_v2_v2(cumap->cm[a].premul_ext_in, cumap->cm[a].ext_in);
        copy_v2_v2(cumap->cm[a].premul_ext_out, cumap->cm[a].ext_out);
        mul_v2_v2(cumap->cm[a].ext_in, cumap->cm[3].ext_in);
        mul_v2_v2(cumap->cm[a].ext_out, cumap->cm[3].ext_out);
      }

      cumap->flag |= CUMA_PREMULLED;
    }
  }
}

/* HANS-TODO: Update */
void profilepath_changed(CurveMapping *cumap, const bool rem_doubles)
{
  CurveMap *cuma = cumap->cm + cumap->cur;
  CurveMapPoint *cmp = cuma->curve;
  rctf *clipr = &cumap->clipr;
  float thresh = 0.01f * BLI_rctf_size_x(clipr);
  float dx = 0.0f, dy = 0.0f;
  int a;

  cumap->changed_timestamp++;

  /* clamp with clip */
  if (cumap->flag & CUMA_DO_CLIP) {
    for (a = 0; a < cuma->totpoint; a++) {
      if (cmp[a].flag & CUMA_SELECT) {
        if (cmp[a].x < clipr->xmin) {
          dx = min_ff(dx, cmp[a].x - clipr->xmin);
        }
        else if (cmp[a].x > clipr->xmax) {
          dx = max_ff(dx, cmp[a].x - clipr->xmax);
        }
        if (cmp[a].y < clipr->ymin) {
          dy = min_ff(dy, cmp[a].y - clipr->ymin);
        }
        else if (cmp[a].y > clipr->ymax) {
          dy = max_ff(dy, cmp[a].y - clipr->ymax);
        }
      }
    }
    for (a = 0; a < cuma->totpoint; a++) {
      if (cmp[a].flag & CUMA_SELECT) {
        cmp[a].x -= dx;
        cmp[a].y -= dy;
      }
    }

    /* ensure zoom-level respects clipping */
    if (BLI_rctf_size_x(&cumap->curr) > BLI_rctf_size_x(&cumap->clipr)) {
      cumap->curr.xmin = cumap->clipr.xmin;
      cumap->curr.xmax = cumap->clipr.xmax;
    }
    if (BLI_rctf_size_y(&cumap->curr) > BLI_rctf_size_y(&cumap->clipr)) {
      cumap->curr.ymin = cumap->clipr.ymin;
      cumap->curr.ymax = cumap->clipr.ymax;
    }
  }

  /* qsort(cmp, cuma->totpoint, sizeof(CurveMapPoint), sort_curvepoints); */

  /* remove doubles, threshold set on 1% of default range */
  if (rem_doubles && cuma->totpoint > 2) {
    for (a = 0; a < cuma->totpoint - 1; a++) {
      dx = cmp[a].x - cmp[a + 1].x;
      dy = cmp[a].y - cmp[a + 1].y;
      if (sqrtf(dx * dx + dy * dy) < thresh) {
        if (a == 0) {
          cmp[a + 1].flag |= CUMA_HANDLE_VECTOR;
          if (cmp[a + 1].flag & CUMA_SELECT) {
            cmp[a].flag |= CUMA_SELECT;
          }
        }
        else {
          cmp[a].flag |= CUMA_HANDLE_VECTOR;
          if (cmp[a].flag & CUMA_SELECT) {
            cmp[a + 1].flag |= CUMA_SELECT;
          }
        }
        break; /* we assume 1 deletion per edit is ok */
      }
    }
    if (a != cuma->totpoint - 1) {
      curvemap_remove(cuma, 2);
    }
  }
  profile_make_table(cuma, clipr);
}

/* HANS-TODO: Update */
void profile_path_changed_all(CurveMapping *cumap)
{
  int a, cur = cumap->cur;

  for (a = 0; a < CM_TOT; a++) {
    if (cumap->cm[a].curve) {
      cumap->cur = a;
      curvemapping_changed(cumap, false);
    }
  }

  cumap->cur = cur;
}

/* table should be verified */
/* HANS-TODO: Delete */
float profile_evaluateF(const CurveMap *cuma, float value)
{
  float fi;
  int i;

  /* index in table */
  fi = (value - cuma->mintable) * cuma->range;
  i = (int)fi;

  /* fi is table float index and should check against table range i.e. [0.0 CM_TABLE] */
  if (fi < 0.0f || fi > CM_TABLE) {
    return profile_calc_extend(cuma, value, &cuma->table[0].x, &cuma->table[CM_TABLE].x);
  }
  else {
    if (i < 0) {
      return cuma->table[0].y;
    }
    if (i >= CM_TABLE) {
      return cuma->table[CM_TABLE].y;
    }

    fi = fi - (float)i;
    return (1.0f - fi) * cuma->table[i].y + (fi)*cuma->table[i + 1].y;
  }
}

/* works with curve 'cur' */
/* HANS-TODO: Delete */
float curvemapping_evaluateF(const CurveMapping *cumap, int cur, float value)
{
  const CurveMap *cuma = cumap->cm + cur;
  float val = curvemap_evaluateF(cuma, value);

  /* account for clipping */
  if (cumap->flag & CUMA_DO_CLIP) {
    if (val < cumap->curr.ymin) {
      val = cumap->curr.ymin;
    }
    else if (val > cumap->curr.ymax) {
      val = cumap->curr.ymax;
    }
  }

  return val;
}

void curvemapping_initialize(CurveMapping *cumap)
{
  if (cumap == NULL) {
    return;
  }

  if (cumap->cm->table == NULL) {
    profile_make_table(cumap->cm, &cumap->clipr);
  }
}

#define DEBUG_CUMAP 0

/* HANS-TODO: Maybe this should check index out of bound */
/* HANS-TODO: This falls back to linear interpolation for all points for now */
/* This might be a little less efficient because it has to get fetch x and y */
/*   rather than carrying them over from the last point while travelling */
float profile_linear_distance_to_next_point(const struct CurveMap *cuma, int i)
{
  float x = cuma->curve[i].x;
  float y = cuma->curve[i].y;
  float x_next = cuma->curve[i + 1].x;
  float y_next = cuma->curve[i + 1].y;

  return sqrtf(powf(y_next - y, 2) + powf(x_next - x, 2));
}

/* Calculate the total length of the path (between all of the nodes and the ends at 0 and 1 */
float profile_total_length(const struct CurveMap *cuma)
{
  float total_length = 0;
  /*printf("Here are the locations of all of %d points in the list:\n", cuma->totpoint);
  for (int i = 0; i < cuma->totpoint; i++) {
    printf("(%f, %f)\n", cuma->curve[i].x, cuma->curve[i].y);
  }*/

  for (int i = 0; i < cuma->totpoint - 1; i++) {
    total_length += curvemap_path_linear_distance_to_next_point(cuma, i);
  }

  return total_length;
}

static inline float lerp(float a, float b, float f)
{
  return a + (b - a) * f;
}

/* CurveMap should have already been initialized */
void profile_evaluate(const struct CurveMap *cuma,
                            float length_portion,
                            float *x_out,
                            float *y_out)
{
  /* HANS-TODO: For now I'm skipping the table and doing the evaluation here, */
  /*   but it should be moved later on so I don't have to travel down node list every call */
  float total_length = cuma->total_length;
  float requested_length = length_portion * total_length;

  /* Find the last point along the path with a lower length portion than the input */
  int i = 0;
  float length_travelled = 0.0f;
  while (length_travelled < requested_length) {
    /* Check if we reached the last point before the final one */
    if (i == cuma->totpoint - 2) {
      break;
    }
    float new_length = curvemap_path_linear_distance_to_next_point(cuma, i);
    if (length_travelled + new_length >= requested_length) {
      break;
    }
    length_travelled += new_length;
    i++;
  }

  /* Now travel the rest of the length portion down the path to the next point and find the
   * location there */
  float distance_to_next_point = curvemap_path_linear_distance_to_next_point(cuma, i);
  float lerp_factor = (requested_length - length_travelled) / distance_to_next_point;

#if DEBUG_CUMAP
  printf("  length portion input: %f\n", length_portion);
  printf("  requested path length: %f\n", requested_length);
  printf("  distance to next point: %f\n", distance_to_next_point);
  printf("  length travelled: %f\n", length_travelled);
  printf("  lerp-factor: %f\n", lerp_factor);
  printf("  ith point  (%f, %f)\n", cuma->curve[i].x, cuma->curve[i].y);
  printf("  next point (%f, %f)\n", cuma->curve[i + 1].x, cuma->curve[i + 1].y);
#endif

  *x_out = lerp(cuma->curve[i].x, cuma->curve[i + 1].x, lerp_factor);
  *y_out = lerp(cuma->curve[i].y, cuma->curve[i + 1].y, lerp_factor);
}

/* HANS-TODO: Test this! (And start using it) */
static void profile_make_table2(struct CurveMap *cuma)
{
  /* Fill table with values for the position of the graph at each of the segments */
    const float segment_length = cuma->total_length / cuma->nsegments;
    float length_travelled = 0.0f;
    float distance_to_next_point = curvemap_path_linear_distance_to_next_point(cuma, 0);
    float distance_to_previous_point = 0.0f;
    float travelled_since_last_point = 0.0f;
    float segment_left = segment_length;
    float f;
    int i_point = 0;

    cuma->x_segment_vals = MEM_callocN((size_t)cuma->nsegments * sizeof(float), "segment table x");
    cuma->y_segment_vals = MEM_callocN((size_t)cuma->nsegments * sizeof(float), "segment table y");

    /* Travel along the path, recording locations of segments as we pass where they should be */
    for (int i = 0; i < cuma->nsegments; i++) {
      /* Travel over all of the points that could be inside this segment */
      while (distance_to_next_point > segment_length * (i + 1) - length_travelled) {
        length_travelled += distance_to_next_point;
        segment_left -= distance_to_next_point;
        travelled_since_last_point += distance_to_next_point;
        i_point++;
        distance_to_next_point = curvemap_path_linear_distance_to_next_point(cuma, i_point);
        distance_to_previous_point = 0.0f;
      }
      /* We're now at the last point that fits inside the current segment */

      f = segment_left / (distance_to_previous_point + distance_to_next_point);
      cuma->x_segment_vals[i] = lerp(cuma->curve[i_point].x, cuma->curve[i_point+1].x, f);
      cuma->y_segment_vals[i] = lerp(cuma->curve[i_point].x, cuma->curve[i_point+1].x, f);
      distance_to_next_point -= segment_left;
      distance_to_previous_point += segment_left;

      length_travelled += segment_left;
    }
}

/* Used for a path where the curve isn't necessarily a function. */
/* Initialized with the number of segments to fill the table with */
void profile_path_initialize(struct CurveMapping *cumap, int nsegments)
{
  CurveMap *cuma = cumap->cm;

  cuma->nsegments = nsegments;
  float total_length = curvemap_path_total_length(cumap->cm);
  cuma->total_length = total_length;

#if DEBUG_CUMAP
  printf("Total length of the curve is: %f\n", (double)total_length);
#endif

  /* Fill a table with the position at nssegments steps along the total length of the path */
  profile_make_table2(cumap->cm);
}

/* Evaluates along the length of the path rather than with X coord */
/* Must initialize the table with the right amount of segments first! */
void profile_path_evaluate(const struct CurveMapping *cumap,
                                int segment,
                                float *x_out,
                                float *y_out)
{
  /* Return the location in the table of the input segment */

  const CurveMap *cuma = cumap->cm;
  curvemap_path_evaluate(cuma, ((float)segment / (float)cuma->nsegments), x_out, y_out);

  /* Clip down to 0 to 1 range for both coords */
  if (cumap->flag & CUMA_DO_CLIP) {
    if (*x_out < cumap->curr.xmin) {
      *x_out = cumap->curr.xmin;
    }
    else if (*x_out > cumap->curr.xmax) {
      *x_out = cumap->curr.xmax;
    }
    if (*y_out < cumap->curr.ymin) {
      *y_out = cumap->curr.ymin;
    }
    else if (*y_out > cumap->curr.ymax) {
      *y_out = cumap->curr.ymax;
    }
  }
}
