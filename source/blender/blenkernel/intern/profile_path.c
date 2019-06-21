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
 * Copyright (C) 2019 Blender Foundation.
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

#include "DNA_profilepath_types.h"
#include "DNA_curve_types.h"

#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "BLI_task.h"
#include "BLI_threads.h"

#include "BKE_profile_path.h"
#include "BKE_curve.h"
#include "BKE_fcurve.h"

#define DEBUG_PRWDGT 0

void profilewidget_set_defaults(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET SET DEFAULTS\n");
#endif
  prwdgt->flag = PROF_DO_CLIP;

  BLI_rctf_init(&prwdgt->curr, 0.0f, 1.0f, 0.0f, 1.0f);
  prwdgt->clipr = prwdgt->curr;

  prwdgt->profile->totpoint = 2;
  prwdgt->profile->path = MEM_callocN(2 * sizeof(ProfilePoint), "path points");

  prwdgt->profile->path[0].x = 1.0f;
  prwdgt->profile->path[0].y = 0.0f;
  prwdgt->profile->path[1].x = 1.0f;
  prwdgt->profile->path[1].y = 1.0f;

  prwdgt->changed_timestamp = 0;
}

struct ProfileWidget *profilewidget_add(int preset)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET ADD\n");
#endif

  ProfileWidget *prwdgt = MEM_callocN(sizeof(ProfileWidget), "new profile widget");
  prwdgt->profile = MEM_callocN(sizeof(ProfilePath), "new profile path");

  profilewidget_set_defaults(prwdgt);
  profilepath_reset(prwdgt->profile, preset);

  return prwdgt;
}

void profilewidget_free_data(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET FREE DATA\n");
#endif

  if (!prwdgt->profile) {
    /* Why are you even here in the first place? */
    printf("ProfileWidget has no profile\n"); /* HANS-TODO: Remove */
    return;
  }
  if (prwdgt->profile->path) {
    MEM_freeN(prwdgt->profile->path);
    prwdgt->profile->path = NULL;
  }
  if (prwdgt->profile->table) {
    MEM_freeN(prwdgt->profile->table);
    prwdgt->profile->table = NULL;
  }
  if (prwdgt->profile->x_segment_vals) {
    MEM_freeN(prwdgt->profile->x_segment_vals);
    prwdgt->profile->x_segment_vals = NULL;
  }
  if (prwdgt->profile->y_segment_vals) {
    MEM_freeN(prwdgt->profile->y_segment_vals);
    prwdgt->profile->y_segment_vals = NULL;
  }
  MEM_freeN(prwdgt->profile);
}

void profilewidget_free(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET FREE\n");
#endif

  if (prwdgt) {
    profilewidget_free_data(prwdgt);
    MEM_freeN(prwdgt);
  }
}

void profilewidget_copy_data(ProfileWidget *target, const ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET COPY DATA\n");
#endif
  *target = *prwdgt;

  if (prwdgt->profile->path) {
    target->profile->path = MEM_dupallocN(prwdgt->profile->path);
  }
  if (prwdgt->profile->table) {
    target->profile->table = MEM_dupallocN(prwdgt->profile->table);
  }
  if (prwdgt->profile->x_segment_vals) {
    target->profile->x_segment_vals = MEM_dupallocN(prwdgt->profile->x_segment_vals);
  }
  if (prwdgt->profile->y_segment_vals) {
    target->profile  ->y_segment_vals = MEM_dupallocN(prwdgt->profile->y_segment_vals);
  }
}

ProfileWidget *profilewidget_copy(const ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET COPY\n");
#endif

  if (prwdgt) {
    ProfileWidget *prwdgtn = MEM_dupallocN(prwdgt);
    profilewidget_copy_data(prwdgtn, prwdgt);
    return prwdgtn;
  }
  return NULL;
}


/* ********** requires profilewidget_changed() call after ******** */

/* remove specified point */
bool profilepath_remove_point(ProfilePath *prpath, ProfilePoint *point)
{
  ProfilePoint *pts;
  int a, b, removed = 0;

#if DEBUG_PRWDGT
  printf("PROFILEPATH REMOVE POINT\n");
#endif

  /* must have 2 points minimum */
  if (prpath->totpoint <= 2) {
    return false;
  }

  pts = MEM_mallocN((size_t)prpath->totpoint * sizeof(ProfilePoint), "path points");

  /* well, lets keep the two outer points! */
  for (a = 0, b = 0; a < prpath->totpoint; a++) {
    if (&prpath->path[a] != point) {
      pts[b] = prpath->path[a];
      b++;
    }
    else {
      removed++;
    }
  }

  MEM_freeN(prpath->path);
  prpath->path = pts;
  prpath->totpoint -= removed;
  return (removed != 0);
}

/* removes with flag set */
void profilepath_remove(ProfilePath *prpath, const short flag)
{
  ProfilePoint *pts = MEM_mallocN(((size_t)prpath->totpoint) * sizeof(ProfilePoint),
                                  "path points");
  int a, b, removed = 0;

#if DEBUG_PRWDGT
  printf("PROFILEPATH REMOVE\n");
#endif

  /* well, lets keep the two outer points! */
  pts[0] = prpath->path[0];
  for (a = 1, b = 1; a < prpath->totpoint - 1; a++) {
    if (!(prpath->path[a].flag & flag)) {
      pts[b] = prpath->path[a];
      b++;
    }
    else {
      removed++;
    }
  }
  pts[b] = prpath->path[a];

  MEM_freeN(prpath->path);
  prpath->path = pts;
  prpath->totpoint -= removed;
}

ProfilePoint *profilepath_insert(ProfilePath *prpath, float x, float y)
{
  ProfilePoint *pts = MEM_callocN(((size_t)prpath->totpoint + 1) * sizeof(ProfilePoint),
                                  "path points");
  ProfilePoint *newpt = NULL;
  int a, b;
  bool foundloc = false;

#if DEBUG_PRWDGT
  printf("PROFILEPATH INSERT");
  printf("(begin total points = %d)", prpath->total_points);
#endif

  /* HANS-TODO: New insertion algorithm. Find closest points in 2D and then insert them in the
   * middle of those. Maybe just lengthen the size of the array instead of allocating a new one
   * too, but that probbaly doesn't matter so much.
   *
   * New algorithm would probably be: Sort the points by their proximity to the new location. Then
   * find the two points closest to the new position that are ordered one after the next in the
   * original array of points (this will probably be the two closest points, but for more
   * complicated profiles it could be points on opposite sides of the profile). Then insert the new
   * point between the two we just found. */
  /* insert fragments of the old one and the new point to the new curve */
  prpath->totpoint++;
  for (a = 0, b = 0; a < prpath->totpoint; a++) {
    /* Insert the new point at the correct X location */
    if ((foundloc == false) && ((a + 1 == prpath->totpoint) || (x < prpath->path[a].x))) {
      pts[a].x = x;
      pts[a].y = y;
      pts[a].flag = PROF_SELECT;
      foundloc = true;
      newpt = &pts[a];
    }
    else {
      /* Copy old point over to the new array */
      pts[a].x = prpath->path[b].x;
      pts[a].y = prpath->path[b].y;
      /* make sure old points don't remain selected */
      pts[a].flag = prpath->path[b].flag & ~PROF_SELECT;
      pts[a].shorty = prpath->path[b].shorty;
      b++;
    }
  }
#if DEBUG_PRWDGT
  printf("PROFILEPATH INSERT");
  printf("(end total points = %d)\n", prpath->total_points);
#endif

  /* free old path and replace it with the new one */
  MEM_freeN(prpath->path);
  prpath->path = pts;

  return newpt;
}

void profilepath_reset(ProfilePath *prpath, int preset)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH RESET\n");
#endif

  if (prpath->path) {
    MEM_freeN(prpath->path);
  }

  switch (preset) {
    case PROF_PRESET_LINE:
      prpath->totpoint = 2;
      break;
  }

  prpath->path = MEM_callocN((size_t)prpath->totpoint * sizeof(ProfilePoint), "path points");

  switch (preset) {
    case PROF_PRESET_LINE:
      prpath->path[0].x = 0.0;
      prpath->path[0].y = 0.0;
      prpath->path[1].x = 1.0;
      prpath->path[1].y = 1.0;
      break;
  }

  if (prpath->table) {
    MEM_freeN(prpath->table);
    prpath->table = NULL;
  }
}

/**
 * \param type: eBezTriple_Handle
 */
void profilepath_handle_set(ProfilePath *cuma, int type)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH HANDLE SET\n");
#endif

  if (cuma->path->flag & PROF_SELECT) {
    cuma->path->flag &= ~(PROF_HANDLE_VECTOR | PROF_HANDLE_AUTO_ANIM);
    if (type == HD_VECT) {
      cuma->path->flag |= PROF_HANDLE_VECTOR;
    }
    else if (type == HD_AUTO_ANIM) {
      cuma->path->flag |= PROF_HANDLE_AUTO_ANIM;
    }
    else {
      /* pass */
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
#if DEBUG_PRWDGT
  printf("CALCHANDLE PROFILE\n");
#endif
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

/* Creates the table of points with the sampled curves */
/* HANS-TODO: Update to add the ability to go back on itself */
static void profilepath_make_table(ProfilePath *prpath, const rctf *clipr)
{
  ProfilePoint *point = prpath->path;
  BezTriple *bezt;
  float *fp, *allpoints, *lastpoint, curf, range;
  int i, totpoint;

#if DEBUG_PRWDGT
  printf("PROFILEPATH MAKE TABLE\n");
#endif

  if (prpath->path == NULL) {
    printf("ProfilePath's path is NULL\n"); /* HANS-TODO: Remove */
    return;
  }

  /* HANS-TODO: Remove */
//  prpath->table = prpath->path;

  /* default rect also is table range */
  prpath->mintable = clipr->xmin;
  prpath->maxtable = clipr->xmax;

  /* hrmf... we now rely on blender ipo beziers, these are more advanced */
  bezt = MEM_callocN((size_t)prpath->totpoint * sizeof(BezTriple), "beztarr");

  for (i = 0; i < prpath->totpoint; i++) {
    prpath->mintable = min_ff(prpath->mintable, point[i].x);
    prpath->maxtable = max_ff(prpath->maxtable, point[i].x);
    bezt[i].vec[1][0] = point[i].x;
    bezt[i].vec[1][1] = point[i].y;
    if (point[i].flag & PROF_HANDLE_VECTOR) {
      bezt[i].h1 = bezt[i].h2 = HD_VECT;
    }
    else if (point[i].flag & PROF_HANDLE_AUTO_ANIM) {
      bezt[i].h1 = bezt[i].h2 = HD_AUTO_ANIM;
    }
    else {
      bezt[i].h1 = bezt[i].h2 = HD_AUTO;
    }
  }

  const BezTriple *bezt_prev = NULL;
  for (i = 0; i < prpath->totpoint; i++) {
    const BezTriple *bezt_next = (i != prpath->totpoint - 1) ? &bezt[i + 1] : NULL;
    calchandle_profile(&bezt[i], bezt_prev, bezt_next);
    bezt_prev = &bezt[i];
  }

  /* first and last handle need correction, instead of pointing to center of next/prev,
   * we let it point to the closest handle */
  /* HANS-TODO: Remove this correction  */
  if (prpath->totpoint > 2) {
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
    i = prpath->totpoint - 1;
    if (bezt[i].h2 == HD_AUTO) {

      hlen = len_v3v3(bezt[i].vec[1], bezt[i].vec[0]); /* original handle length */
      /* clip handle point */
      copy_v3_v3(vec, bezt[i - 1].vec[2]);
      if (vec[0] > bezt[i].vec[1][0]) {
        vec[0] = bezt[i].vec[1][0];
      }

      sub_v3_v3(vec, bezt[i].vec[1]);
      nlen = len_v3(vec);
      if (nlen > FLT_EPSILON) {
        mul_v3_fl(vec, hlen / nlen);
        add_v3_v3v3(bezt[i].vec[0], vec, bezt[i].vec[1]);
        sub_v3_v3v3(bezt[i].vec[2], bezt[i].vec[1], vec);
      }
    }
  }
  /* make the bezier curve */
  if (prpath->table) {
    MEM_freeN(prpath->table);
  }
  totpoint = (prpath->totpoint - 1) * PROF_RESOL;
  fp = allpoints = MEM_callocN((size_t)totpoint * 2 * sizeof(float), "table");

  for (i = 0; i < prpath->totpoint - 1; i++, fp += 2 * PROF_RESOL) {
    correct_bezpart(bezt[i].vec[1], bezt[i].vec[2], bezt[i + 1].vec[0], bezt[i + 1].vec[1]);
    BKE_curve_forward_diff_bezier(bezt[i].vec[1][0],
                                  bezt[i].vec[2][0],
                                  bezt[i + 1].vec[0][0],
                                  bezt[i + 1].vec[1][0],
                                  fp,
                                  PROF_RESOL - 1,
                                  2 * sizeof(float));
    BKE_curve_forward_diff_bezier(bezt[i].vec[1][1],
                                  bezt[i].vec[2][1],
                                  bezt[i + 1].vec[0][1],
                                  bezt[i + 1].vec[1][1],
                                  fp + 1,
                                  PROF_RESOL - 1,
                                  2 * sizeof(float));
  }

  /* cleanup */
  MEM_freeN(bezt);

  range = PROF_TABLEDIV * (prpath->maxtable - prpath->mintable);
  prpath->range = 1.0f / range;

  /* now make a table with CM_TABLE equal x distances */
  fp = allpoints;
  lastpoint = allpoints + 2 * (totpoint - 1);
  point = MEM_callocN((PROF_TABLE_SIZE + 1) * sizeof(ProfilePoint), "dist table");

  for (i = 0; i <= PROF_TABLE_SIZE; i++) {
    curf = prpath->mintable + range * (float)i;
    point[i].x = curf;

    /* get the first x coordinate larger than curf */
    while (curf >= fp[0] && fp != lastpoint) {
      fp += 2;
    }
    if (fp == allpoints || (curf >= fp[0] && fp == lastpoint)) {
      /* HANS-TODO: Remove this case */
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
      point[i].y = fac1 * fp[-1] + (1.0f - fac1) * fp[1];
    }
  }

  MEM_freeN(allpoints);
  prpath->table = point;
}

/* HANS-TODO: Update */
void profilewidget_changed(ProfileWidget *prwdgt, const bool rem_doubles)
{
  ProfilePath *prpath = prwdgt->profile;
  ProfilePoint *points = prpath->path;
  rctf *clipr = &prwdgt->clipr;
  float thresh = 0.01f * BLI_rctf_size_x(clipr);
  float dx = 0.0f, dy = 0.0f;
  int i;

  prwdgt->changed_timestamp++;

#if DEBUG_PRWDGT
  printf("PROFILEWIDGET CHANGED\n");
  if (prwdgt->profile->total_points < 0) {
    printf("Someone screwed up the totpoint\n");
  }
#endif

  /* Clamp with the clipping rect in case something got past */
  if (prwdgt->flag & PROF_DO_CLIP) {
    for (i = 0; i < prpath->totpoint; i++) {
      if (points[i].flag & PROF_SELECT) {
        if (points[i].x < clipr->xmin) {
          dx = min_ff(dx, points[i].x - clipr->xmin);
        }
        else if (points[i].x > clipr->xmax) {
          dx = max_ff(dx, points[i].x - clipr->xmax);
        }
        if (points[i].y < clipr->ymin) {
          dy = min_ff(dy, points[i].y - clipr->ymin);
        }
        else if (points[i].y > clipr->ymax) {
          dy = max_ff(dy, points[i].y - clipr->ymax);
        }
      }
    }
    for (i = 0; i < prpath->totpoint; i++) {
      if (points[i].flag & PROF_SELECT) {
        points[i].x -= dx;
        points[i].y -= dy;
      }
    }

    /* ensure zoom-level respects clipping */
    /* HANS-TODO: Isn't this done in the zooming functions? */
    if (BLI_rctf_size_x(&prwdgt->curr) > BLI_rctf_size_x(&prwdgt->clipr)) {
      prwdgt->curr.xmin = prwdgt->clipr.xmin;
      prwdgt->curr.xmax = prwdgt->clipr.xmax;
    }
    if (BLI_rctf_size_y(&prwdgt->curr) > BLI_rctf_size_y(&prwdgt->clipr)) {
      prwdgt->curr.ymin = prwdgt->clipr.ymin;
      prwdgt->curr.ymax = prwdgt->clipr.ymax;
    }
  }

  /* remove doubles, threshold set on 1% of default range */
  if (rem_doubles && prpath->totpoint > 2) {
    for (i = 0; i < prpath->totpoint - 1; i++) {
      dx = points[i].x - points[i + 1].x;
      dy = points[i].y - points[i + 1].y;
      if (sqrtf(dx * dx + dy * dy) < thresh) {
        if (i == 0) {
          points[i + 1].flag |= PROF_HANDLE_VECTOR;
          if (points[i + 1].flag & PROF_SELECT) {
            points[i].flag |= PROF_SELECT;
          }
        }
        else {
          points[i].flag |= PROF_HANDLE_VECTOR;
          if (points[i].flag & PROF_SELECT) {
            points[i + 1].flag |= PROF_SELECT;
          }
        }
        break; /* we assume 1 deletion per edit is ok */
      }
    }
    if (i != prpath->totpoint - 1) {
      profilepath_remove(prpath, 2);
    }
  }
  profilepath_make_table(prpath, clipr);
}



/* HANS-TODO: Maybe this should check index out of bound */
/* This might be a little less efficient because it has to fetch x and y */
/* rather than carrying them over from the last point while travelling */
float profilepath_linear_distance_to_next_point(const ProfilePath *prpath, int i)
{
  float x = prpath->path[i].x;
  float y = prpath->path[i].y;
  float x_next = prpath->path[i + 1].x;
  float y_next = prpath->path[i + 1].y;

  return sqrtf(powf(y_next - y, 2) + powf(x_next - x, 2));
}

/* Calculate the total length of the path (between all of the nodes and the ends at 0 and 1 */
float profilepath_total_length(const struct ProfilePath *prpath)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH TOTAL LENGTH\n");
#endif
  float total_length = 0;
  /*printf("Here are the locations of all of %d points in the list:\n", prpath->totpoint);
  for (int i = 0; i < prpath->totpoint; i++) {
    printf("(%f, %f)\n", prpath->path[i].x, prpath->path[i].y);
  }*/

  /* HANS-TODO: This is a bad way to do this */
  for (int i = 0; i < prpath->totpoint - 1; i++) {
    total_length += profilepath_linear_distance_to_next_point(prpath, i);
  }

  return total_length;
}

static inline float lerp(float a, float b, float f)
{
  return a + (b - a) * f;
}

/* Widget should have already been initialized */
void profilepath_evaluate(const struct ProfilePath *prpath,
                          float length_portion,
                          float *x_out,
                          float *y_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH EVALUATE\n");
  if (prpath->total_points < 0) {
    printf("Someone screwed up the totpoint\n");
  }
#endif
  /* HANS-TODO: For now I'm skipping the table and doing the evaluation here, */
  /*   but it should be moved later on so I don't have to travel down node list every call */
  float total_length = prpath->total_length;
  float requested_length = length_portion * total_length;

  /* Find the last point along the path with a lower length portion than the input */
  int i = 0;
  float length_travelled = 0.0f;
  while (length_travelled < requested_length) {
    /* Check if we reached the last point before the final one */
    if (i == prpath->totpoint - 2) {
      break;
    }
    float new_length = profilepath_linear_distance_to_next_point(prpath, i);
    if (length_travelled + new_length >= requested_length) {
      break;
    }
    length_travelled += new_length;
    i++;
  }

  /* Now travel the rest of the length portion down the path to the next point and find the
   * location there */
  float distance_to_next_point = profilepath_linear_distance_to_next_point(prpath, i);
  float lerp_factor = (requested_length - length_travelled) / distance_to_next_point;

#if DEBUG_PRWDGT
  printf("  length portion input: %f\n", (double)length_portion);
  printf("  requested path length: %f\n", (double)requested_length);
  printf("  distance to next point: %f\n", (double)distance_to_next_point);
  printf("  length travelled: %f\n", (double)length_travelled);
  printf("  lerp-factor: %f\n", (double)lerp_factor);
  printf("  ith point  (%f, %f)\n", (double)prpath->path[i].x, (double)prpath->path[i].y);
  printf("  next point (%f, %f)\n", (double)prpath->path[i + 1].x, (double)prpath->path[i + 1].y);
#endif

  *x_out = lerp(prpath->path[i].x, prpath->path[i + 1].x, lerp_factor);
  *y_out = lerp(prpath->path[i].y, prpath->path[i + 1].y, lerp_factor);
  /* HANS-TODO: Switch to interpf */
}

/* HANS-TODO: Test this! (And start using it) */
/* HANS-TODO: For the curves, switch this over to using the secondary higher resolution
 * set of points */
static void profilepath_fill_segment_table(struct ProfilePath *prpath)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH FILL SEGMENT TABLE\n");
#endif
  /* Fill table with values for the position of the graph at each of the segments */
  const float segment_length = prpath->total_length / prpath->nsegments;
  float length_travelled = 0.0f;
  float distance_to_next_point = profilepath_linear_distance_to_next_point(prpath, 0);
  float distance_to_previous_point = 0.0f;
  float travelled_since_last_point = 0.0f;
  float segment_left = segment_length;
  float f;
  int i_point = 0;

  prpath->x_segment_vals = MEM_callocN((size_t)prpath->nsegments * sizeof(float), "segmentvals x");
  prpath->y_segment_vals = MEM_callocN((size_t)prpath->nsegments * sizeof(float), "segmentvals y");

  /* Travel along the path, recording locations of segments as we pass where they should be */
  for (int i = 0; i < prpath->nsegments; i++) {
    /* Travel over all of the points that could be inside this segment */
    while (distance_to_next_point > segment_length * (i + 1) - length_travelled) {
      length_travelled += distance_to_next_point;
      segment_left -= distance_to_next_point;
      travelled_since_last_point += distance_to_next_point;
      i_point++;
      distance_to_next_point = profilepath_linear_distance_to_next_point(prpath, i_point);
      distance_to_previous_point = 0.0f;
    }
    /* We're now at the last point that fits inside the current segment */

    f = segment_left / (distance_to_previous_point + distance_to_next_point);
    prpath->x_segment_vals[i] = lerp(prpath->path[i_point].x, prpath->path[i_point+1].x, f);
    prpath->y_segment_vals[i] = lerp(prpath->path[i_point].x, prpath->path[i_point+1].x, f);
    distance_to_next_point -= segment_left;
    distance_to_previous_point += segment_left;

    length_travelled += segment_left;
  }
}

/* Initialized with the number of segments to fill the table with */
/* HANS-TODO: Make sure isn't asked to make the segment table if
 * the "Sample only points" option is on */
/* HANS-TODO: Make the table equal the path for now */
void profilewidget_initialize(ProfileWidget *prwdgt, int nsegments)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH INITIALIZE\n");
#endif
  if (prwdgt == NULL) {
    return;
  }

  ProfilePath *prpath = prwdgt->profile;

  prpath->nsegments = nsegments;
  float total_length = profilepath_total_length(prwdgt->profile);
  prpath->total_length = total_length;

#if DEBUG_PRWDGT
  printf("Total length of the path is: %f\n", (double)total_length);
#endif

  /* Calculate the positions at nsegments steps along the total length of the path */
  profilepath_fill_segment_table(prwdgt->profile);
  profilepath_make_table(prwdgt->profile, &prwdgt->clipr);
}

/* Evaluates along the length of the path rather than with X coord */
/* Must initialize the table with the right amount of segments first! */
void profilewidget_evaluate(const struct ProfileWidget *prwdgt,
                           int segment,
                           float *x_out,
                           float *y_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH EVALUATE\n");
#endif
  /* HANS-TODO: Use profilepath_fill_segment_table */

  const ProfilePath *prpath = prwdgt->profile;
  profilepath_evaluate(prpath, ((float)segment / (float)prpath->nsegments), x_out, y_out);

  /* Clip down to 0 to 1 range for both coords */
  if (prwdgt->flag & PROF_DO_CLIP) {
    if (*x_out < prwdgt->curr.xmin) {
      *x_out = prwdgt->curr.xmin;
    }
    else if (*x_out > prwdgt->curr.xmax) {
      *x_out = prwdgt->curr.xmax;
    }
    if (*y_out < prwdgt->curr.ymin) {
      *y_out = prwdgt->curr.ymin;
    }
    else if (*y_out > prwdgt->curr.ymax) {
      *y_out = prwdgt->curr.ymax;
    }
  }
}
