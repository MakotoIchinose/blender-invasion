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

#include "DNA_profilewidget_types.h"
#include "DNA_curve_types.h"

#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "BLI_task.h"
#include "BLI_threads.h"

#include "BKE_profile_widget.h"
#include "BKE_curve.h"
#include "BKE_fcurve.h"

#define DEBUG_PRWDGT 1
#define DEBUG_PRWDGT_TABLE 1
#define DEBUG_PRWDGT_EVALUATE 0
#define DEBUG_PRWDGT_REVERSE 0

/* HANS-TODO: Organize functions, especially by which need initialization and which don't */

void profilewidget_set_defaults(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET SET DEFAULTS\n");
#endif
  prwdgt->flag = PROF_DO_CLIP;

  BLI_rctf_init(&prwdgt->view_rect, 0.0f, 1.0f, 0.0f, 1.0f);
  prwdgt->clip_rect = prwdgt->view_rect;

  prwdgt->totpoint = 2;
  prwdgt->path = MEM_callocN(2 * sizeof(ProfilePoint), "path points");

  prwdgt->path[0].x = 1.0f;
  prwdgt->path[0].y = 0.0f;
  prwdgt->path[1].x = 1.0f;
  prwdgt->path[1].y = 1.0f;

  prwdgt->changed_timestamp = 0;
}

struct ProfileWidget *profilewidget_add(int preset)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET ADD\n");
#endif

  ProfileWidget *prwdgt = MEM_callocN(sizeof(ProfileWidget), "new profile widget");

  profilewidget_set_defaults(prwdgt);
  prwdgt->preset = preset;
  profilewidget_reset(prwdgt);
  profilewidget_changed(prwdgt, false);

  return prwdgt;
}

void profilewidget_free_data(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET FREE DATA\n");
#endif

  if (prwdgt->path) {
    MEM_freeN(prwdgt->path);
    prwdgt->path = NULL;
  }
  else {
#if DEBUG_PRWDGT
  printf("The prwdgt had no path... probably a second redundant free call\n");
#endif
  }
  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
    prwdgt->table = NULL;
  }
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

  if (prwdgt->path) {
    target->path = MEM_dupallocN(prwdgt->path);
  }
  if (prwdgt->table) {
    target->table = MEM_dupallocN(prwdgt->table);
  }
}

ProfileWidget *profilewidget_copy(const ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET COPY\n");
#endif

  if (prwdgt) {
    ProfileWidget *new_prdgt = MEM_dupallocN(prwdgt);
    profilewidget_copy_data(new_prdgt, prwdgt);
    return new_prdgt;
  }
  return NULL;
}

/* ********** requires profilewidget_changed() call after ******** */

/* remove specified point */
bool profilewidget_remove_point(ProfileWidget *prwdgt, ProfilePoint *point)
{
  ProfilePoint *pts;
  int a, b, removed = 0;

#if DEBUG_PRWDGT
  printf("PROFILEPATH REMOVE POINT\n");
#endif

  /* must have 2 points minimum */
  if (prwdgt->totpoint <= 2) {
    return false;
  }

  pts = MEM_mallocN((size_t)prwdgt->totpoint * sizeof(ProfilePoint), "path points");

  /* well, lets keep the two outer points! */
  for (a = 0, b = 0; a < prwdgt->totpoint; a++) {
    if (&prwdgt->path[a] != point) {
      pts[b] = prwdgt->path[a];
      b++;
    }
    else {
      removed++;
    }
  }

  MEM_freeN(prwdgt->path);
  prwdgt->path = pts;
  prwdgt->totpoint -= removed;
  return (removed != 0);
}

/* removes with flag set */
void profilewidget_remove(ProfileWidget *prwdgt, const short flag)
{
  ProfilePoint *pts = MEM_mallocN(((size_t)prwdgt->totpoint) * sizeof(ProfilePoint),
                                  "path points");
  int a, b, removed = 0;

#if DEBUG_PRWDGT
  printf("PROFILEPATH REMOVE\n");
#endif

  /* well, lets keep the two outer points! */
  pts[0] = prwdgt->path[0];
  for (a = 1, b = 1; a < prwdgt->totpoint - 1; a++) {
    if (!(prwdgt->path[a].flag & flag)) {
      pts[b] = prwdgt->path[a];
      b++;
    }
    else {
      removed++;
    }
  }
  pts[b] = prwdgt->path[a];

  MEM_freeN(prwdgt->path);
  prwdgt->path = pts;
  prwdgt->totpoint -= removed;
}

/* The choice for which points to place the new vertex between is more complex with a profile than
 * with a mapping function. We can't just find the new neighbors with X value comparisons. Instead
 * this function checks which line segment is closest to the new point with a handy pre-made
 * function.
*/
ProfilePoint *profilewidget_insert(ProfileWidget *prwdgt, float x, float y)
{
  ProfilePoint *new_pt;
  float new_loc[2] = {x, y};
#if DEBUG_PRWDGT
  printf("PROFILEPATH INSERT\n");
#endif

  /* Find the index at the line segment that's closest to the new position */
  float distance;
  float min_distance = FLT_MAX;
  int insert_i;
  for (int i = 0; i < prwdgt->totpoint - 1; i++) {
    float loc1[2] = {prwdgt->path[i].x, prwdgt->path[i].y};
    float loc2[2] = {prwdgt->path[i + 1].x, prwdgt->path[i + 1].y};

    distance = dist_squared_to_line_segment_v2(new_loc, loc1, loc2);
    if (distance < min_distance) {
      min_distance = distance;
      insert_i = i + 1;
    }
  }

  /* Insert the new point at the location we found and copy all of the old points in as well */
  prwdgt->totpoint++;
  ProfilePoint *new_pts = MEM_mallocN(((size_t)prwdgt->totpoint) * sizeof(ProfilePoint),
                                      "path points");
  for (int i_new = 0, i_old = 0; i_new < prwdgt->totpoint; i_new++) {
    if (i_new != insert_i) {
      /* Insert old point */
      new_pts[i_new].x = prwdgt->path[i_old].x;
      new_pts[i_new].y = prwdgt->path[i_old].y;
      new_pts[i_new].flag = prwdgt->path[i_old].flag & ~PROF_SELECT; /* Deselect old points */
      i_old++;
    }
    else {
      /* Insert new point */
      new_pts[i_new].x = x;
      new_pts[i_new].y = y;
      new_pts[i_new].flag = PROF_SELECT;
      new_pt = &new_pts[i_new];
    }
  }

  /* Free the old points and use the new ones */
  MEM_freeN(prwdgt->path);
  prwdgt->path = new_pts;
  return new_pt;
}

/* Requires ProfileWidget changed call afterwards */
/* HANS-TODO: Or we could just reverse the table here and not require that? */
void profilewidget_reverse(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH INSERT\n");
#endif
  /* Quick fix for when there are only two points and reversing shouldn't do anything */
  if (prwdgt->totpoint == 2) {
    return;
  }
  ProfilePoint *new_pts = MEM_mallocN(((size_t)prwdgt->totpoint) * sizeof(ProfilePoint),
                                      "path points");
  /* Mirror the new points across the y = 1 - x line */
  for (int i = 1; i < prwdgt->totpoint - 1; i++) {
    new_pts[prwdgt->totpoint - i - 1].x = 1.0f - prwdgt->path[i].y;
    new_pts[prwdgt->totpoint - i - 1].y = 1.0f - prwdgt->path[i].x;
    new_pts[prwdgt->totpoint - i - 1].flag = prwdgt->path[i].flag;
  }
  /* Set the location of the first and last points */
  new_pts[0].x = 0.0;
  new_pts[0].y = 0.0;
  new_pts[prwdgt->totpoint - 1].x = 1.0;
  new_pts[prwdgt->totpoint - 1].y = 1.0;

#if DEBUG_PRWDGT_REVERSE
  printf("Locations before:\n");
  for (int i = 0; i < prwdgt->totpoint; i++) {
    printf("(%.2f, %.2f)", (double)prwdgt->path[i].x, (double)prwdgt->path[i].y);
  }
  printf("\nLocations after:\n");
  for (int i = 0; i < prwdgt->totpoint; i++) {
    printf("(%.2f, %.2f)", (double)new_pts[i].x, (double)new_pts[i].y);
  }
  printf("\n");
#endif

  /* Free the old points and use the new ones */
  MEM_freeN(prwdgt->path);
  prwdgt->path = new_pts;
}

/* Requires ProfileWidget changed call afterwards */
/* HANS-TODO: Couldn't it just build the table at the end of this function here? */
void profilewidget_reset(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH RESET\n");
#endif

  if (prwdgt->path) {
    MEM_freeN(prwdgt->path);
  }

  int preset = prwdgt->preset;

  switch (preset) {
    case PROF_PRESET_LINE:
      prwdgt->totpoint = 2;
      break;
    case PROF_PRESET_SUPPORTS:
      prwdgt->totpoint = 12;
      break;
    case PROF_PRESET_EXAMPLE1:
      prwdgt->totpoint = 23;
      break;
  }

  prwdgt->path = MEM_callocN((size_t)prwdgt->totpoint * sizeof(ProfilePoint), "path points");

  switch (preset) {
    case PROF_PRESET_LINE:
      prwdgt->path[0].x = 0.0;
      prwdgt->path[0].y = 0.0;
      prwdgt->path[1].x = 1.0;
      prwdgt->path[1].y = 1.0;
      break;
    case PROF_PRESET_SUPPORTS:
      prwdgt->path[0].x = 0.0;
      prwdgt->path[0].y = 0.0;
      prwdgt->path[1].x = 0.0;
      prwdgt->path[1].y = 0.5;
      for (int i = 1; i < 10; i++) {
        prwdgt->path[i + 1].x = 0.5f * (1.0f - cosf((float)((i / 9.0) * M_PI_2)));
        prwdgt->path[i + 1].y = 0.5f + 0.5f * sinf((float)((i / 9.0) * M_PI_2));
      }
      prwdgt->path[10].x = 0.5;
      prwdgt->path[10].y = 1.0;
      prwdgt->path[11].x = 1.0;
      prwdgt->path[11].y = 1.0;
      break;
    case PROF_PRESET_EXAMPLE1: /* HANS-TODO: Don't worry, this is just temporary */
      prwdgt->path[0].x = 0.0f;
      prwdgt->path[0].y = 0.0f;
      prwdgt->path[1].x = 0.0f;
      prwdgt->path[1].y = 0.6f;
      prwdgt->path[2].x = 0.1f;
      prwdgt->path[2].y = 0.6f;
      prwdgt->path[3].x = 0.1f;
      prwdgt->path[3].y = 0.7f;
      prwdgt->path[4].x = 0.195024f;
      prwdgt->path[4].y = 0.709379f;
      prwdgt->path[5].x = 0.294767f;
      prwdgt->path[5].y = 0.735585f;
      prwdgt->path[6].x = 0.369792f;
      prwdgt->path[6].y = 0.775577f;
      prwdgt->path[7].x = 0.43429f;
      prwdgt->path[7].y = 0.831837f;
      prwdgt->path[8].x = 0.500148f;
      prwdgt->path[8].y = 0.884851f;
      prwdgt->path[9].x = 0.565882f;
      prwdgt->path[9].y = 0.91889f;
      prwdgt->path[10].x = 0.633279f;
      prwdgt->path[10].y = 0.935271f;
      prwdgt->path[11].x = 0.697628f;
      prwdgt->path[11].y = 0.937218f;
      prwdgt->path[12].x = 0.75148f;
      prwdgt->path[12].y = 0.924844f;
      prwdgt->path[13].x = 0.791918f;
      prwdgt->path[13].y = 0.904817f;
      prwdgt->path[14].x = 0.822379f;
      prwdgt->path[14].y = 0.873682f;
      prwdgt->path[15].x = 0.842155f;
      prwdgt->path[15].y = 0.83174f;
      prwdgt->path[16].x = 0.85f;
      prwdgt->path[16].y = 0.775f;
      prwdgt->path[17].x = 0.929009f;
      prwdgt->path[17].y = 0.775f;
      prwdgt->path[18].x = 0.953861f;
      prwdgt->path[18].y = 0.780265f;
      prwdgt->path[19].x = 0.967919f;
      prwdgt->path[19].y = 0.794104f;
      prwdgt->path[20].x = 0.978458f;
      prwdgt->path[20].y = 0.818784f;
      prwdgt->path[21].x = 0.988467f;
      prwdgt->path[21].y = 0.890742f;
      prwdgt->path[22].x = 1.0f;
      prwdgt->path[22].x = 1.0f;
      break;

  }

  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
    prwdgt->table = NULL;
  }
}

/**
 * \param type: eBezTriple_Handle
 */
void profilewidget_handle_set(ProfileWidget *prwdgt, int type)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH HANDLE SET\n");
#endif
  /* HANS-TODO: This is only doing the first point? */

  if (prwdgt->path->flag & PROF_SELECT) {
    prwdgt->path->flag &= ~(PROF_HANDLE_VECTOR | PROF_HANDLE_AUTO_ANIM);
    if (type == HD_VECT) {
      prwdgt->path->flag |= PROF_HANDLE_VECTOR;
    }
    else if (type == HD_AUTO_ANIM) {
      prwdgt->path->flag |= PROF_HANDLE_AUTO_ANIM;
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
//  printf("CALCHANDLE PROFILE");
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
static void profilewidget_make_table(ProfileWidget *prwdgt)
{
  ProfilePoint *new_table = prwdgt->path;
  BezTriple *bezt;
  float *fp, *curvepoints, *lastpoint, curf, range;
  int i, n_curve_point;

#if DEBUG_PRWDGT
  printf("PROFILEPATH MAKE TABLE\n");
  if (prwdgt->path == NULL) {
    printf("(path is NULL)\n");
    return;
  }
#endif

  bezt = MEM_callocN((size_t)prwdgt->totpoint * sizeof(BezTriple), "beztarr");

  /* Create Bezier points for the path */
  for (i = 0; i < prwdgt->totpoint; i++) {
    bezt[i].vec[1][0] = new_table[i].x;
    bezt[i].vec[1][1] = new_table[i].y;
    if (new_table[i].flag & PROF_HANDLE_VECTOR) {
      bezt[i].h1 = bezt[i].h2 = HD_VECT;
    }
    else if (new_table[i].flag & PROF_HANDLE_AUTO_ANIM) {
      bezt[i].h1 = bezt[i].h2 = HD_AUTO_ANIM;
    }
    else {
      bezt[i].h1 = bezt[i].h2 = HD_AUTO;
    }
  }

  /* Find the automatic positions for the bezier handles */
  const BezTriple *bezt_prev = NULL;
  for (i = 0; i < prwdgt->totpoint; i++) {
    const BezTriple *bezt_next = (i != prwdgt->totpoint - 1) ? &bezt[i + 1] : NULL;
    calchandle_profile(&bezt[i], bezt_prev, bezt_next);
    bezt_prev = &bezt[i];
  }

  /* first and last handle need correction, instead of pointing to center of next/prev,
   * we let it point to the closest handle */
  /* HANS-TODO: Remove this correction?  */
  if (prwdgt->totpoint > 2) {
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
    i = prwdgt->totpoint - 1;
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

  /* Make the bezier curve */
  /* HANS-TODO: Only add new points between HD_AUTO_ANIM points */
  n_curve_point = (prwdgt->totpoint - 1) * PROF_RESOL;
  fp = curvepoints = MEM_callocN((size_t)n_curve_point * 2 * sizeof(float), "table");

  for (i = 0; i < prwdgt->totpoint - 1; i++, fp += 2 * PROF_RESOL) {
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

//  range = PROF_TABLEDIV * (prwdgt->maxtable - prwdgt->mintable);
  range = PROF_TABLEDIV * profilewidget_total_length(prwdgt);
  /* HANS-TODO: Change to length of bezier curve */
  prwdgt->range = 1.0f / range;

  /* Now make the table with PROF_TABLE_SIZE points */
  fp = curvepoints;
  lastpoint = curvepoints + 2 * (n_curve_point - 1);
  new_table = MEM_callocN((PROF_TABLE_SIZE + 1) * sizeof(ProfilePoint), "dist table");

  /* HANS-TODO: Use travel down path method */
  for (i = 0; i <= PROF_TABLE_SIZE; i++) {
    curf = range * (float)i;
    new_table[i].x = curf;

    /* get the first x coordinate larger than curf */
    while (curf >= fp[0] && fp != lastpoint) {
      fp += 2;
    }
    if (fp != curvepoints && fp != lastpoint) {
      /* HANS-QUESTION: I'm not sure of the idea behind this factor stuff. I'll have to look into
       * it closer, or possible use a different method. */
      float fac1 = fp[0] - fp[-2];
      float fac2 = fp[0] - curf;
      if (fac1 > FLT_EPSILON) {
        fac1 = fac2 / fac1;
      }
      else {
        fac1 = 0.0f;
      }
      new_table[i].y = fac1 * fp[-1] + (1.0f - fac1) * fp[1];
    }
  }

  MEM_freeN(curvepoints);
  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
  }
  prwdgt->table = new_table;
}

/* HANS-TODO: Update */
void profilewidget_changed(ProfileWidget *prwdgt, const bool rem_doubles)
{
  ProfilePoint *points = prwdgt->path;
  rctf *clipr = &prwdgt->clip_rect;
  float thresh = 0.01f * BLI_rctf_size_x(clipr);
  float dx = 0.0f, dy = 0.0f;
  int i;

  prwdgt->changed_timestamp++;

#if DEBUG_PRWDGT
  printf("PROFILEWIDGET CHANGED");
  if (prwdgt->totpoint < 0) {
    printf("(Someone screwed up the totpoint)");
  }
  printf("\n");
#endif

  /* Clamp with the clipping rect in case something got past */
  if (prwdgt->flag & PROF_DO_CLIP) {
    for (i = 0; i < prwdgt->totpoint; i++) {
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
    for (i = 0; i < prwdgt->totpoint; i++) {
      if (points[i].flag & PROF_SELECT) {
        points[i].x -= dx;
        points[i].y -= dy;
      }
    }

    /* Ensure zoom-level respects clipping */
    /* HANS-TODO: Isn't this done in the zooming functions? */
    if (BLI_rctf_size_x(&prwdgt->view_rect) > BLI_rctf_size_x(&prwdgt->clip_rect)) {
      prwdgt->view_rect.xmin = prwdgt->clip_rect.xmin;
      prwdgt->view_rect.xmax = prwdgt->clip_rect.xmax;
    }
    if (BLI_rctf_size_y(&prwdgt->view_rect) > BLI_rctf_size_y(&prwdgt->clip_rect)) {
      prwdgt->view_rect.ymin = prwdgt->clip_rect.ymin;
      prwdgt->view_rect.ymax = prwdgt->clip_rect.ymax;
    }
  }

  /* Remove doubles, threshold set on 1% of default range */
  if (rem_doubles && prwdgt->totpoint > 2) {
    for (i = 0; i < prwdgt->totpoint - 1; i++) {
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
        break; /* Assumes 1 deletion per edit is ok */
      }
    }
    if (i != prwdgt->totpoint - 1) {
      profilewidget_remove(prwdgt, 2);
    }
  }
  profilewidget_make_table(prwdgt);
}



/* This might be a little less efficient because it has to fetch x and y */
/* rather than carrying them over from the last point while travelling */
float profilewidget_linear_distance_to_next_point(const ProfileWidget *prwdgt, int i)
{
  /* HANS-TODO: Remove after sufficient testing */
//  BLI_assert(prwdgt != NULL);
//  BLI_assert(i >= 0);
//  BLI_assert(i < prwdgt->totpoint);

  float x = prwdgt->path[i].x;
  float y = prwdgt->path[i].y;
  float x_next = prwdgt->path[i + 1].x;
  float y_next = prwdgt->path[i + 1].y;

  return sqrtf(powf(y_next - y, 2) + powf(x_next - x, 2));
}

/* Calculate the total length of the path (between all of the nodes and the ends at 0 and 1 */
float profilewidget_total_length(const struct ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH TOTAL LENGTH\n");
#endif
  float total_length = 0;
  /*printf("Here are the locations of all of %d points in the list:\n", prwdgt->totpoint);
  for (int i = 0; i < prwdgt->totpoint; i++) {
    printf("(%f, %f)\n", prwdgt->path[i].x, prwdgt->path[i].y);
  }*/

  /* HANS-TODO: This is a bad way to do this */
  for (int i = 0; i < prwdgt->totpoint - 1; i++) {
    total_length += profilewidget_linear_distance_to_next_point(prwdgt, i);
  }

  return total_length;
}

static inline float lerp(float a, float b, float f)
{
  return a + (b - a) * f;
}

/** Gives a single evaluation along the profile's path
 * \param length_portion: The portion (0 to 1) of the path's full length to sample at
*/
void profilewidget_evaluate_portion(const struct ProfileWidget *prwdgt,
                          float length_portion,
                          float *x_out,
                          float *y_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH EVALUATE\n");
  if (prwdgt->totpoint < 0) {
    printf("Someone screwed up the totpoint\n");
  }
#endif
  /* HANS-TODO: For now I'm skipping the table and doing the evaluation here,
   * but it should be moved later on so I don't have to travel down node list every call */
  float total_length = prwdgt->total_length;
  float requested_length = length_portion * total_length;

  /* Find the last point along the path with a lower length portion than the input */
  int i = 0;
  float length_travelled = 0.0f;
  while (length_travelled < requested_length) {
    /* Check if we reached the last point before the final one */
    if (i == prwdgt->totpoint - 2) {
      break;
    }
    float new_length = profilewidget_linear_distance_to_next_point(prwdgt, i);
    if (length_travelled + new_length >= requested_length) {
      break;
    }
    length_travelled += new_length;
    i++;
  }

  /* Now travel the remaining distance of length portion down the path to the next point and
   * find the location where we stop */
  float distance_to_next_point = profilewidget_linear_distance_to_next_point(prwdgt, i);
  float lerp_factor = (requested_length - length_travelled) / distance_to_next_point;

#if DEBUG_PRWDGT_EVALUATE
  printf("  length portion input: %f\n", (double)length_portion);
  printf("  requested path length: %f\n", (double)requested_length);
  printf("  distance to next point: %f\n", (double)distance_to_next_point);
  printf("  length travelled: %f\n", (double)length_travelled);
  printf("  lerp-factor: %f\n", (double)lerp_factor);
  printf("  ith point  (%f, %f)\n", (double)prwdgt->path[i].x, (double)prwdgt->path[i].y);
  printf("  next point (%f, %f)\n", (double)prwdgt->path[i + 1].x, (double)prwdgt->path[i + 1].y);
#endif

  *x_out = lerp(prwdgt->path[i].x, prwdgt->path[i + 1].x, lerp_factor);
  *y_out = lerp(prwdgt->path[i].y, prwdgt->path[i + 1].y, lerp_factor);
}

/** Samples evenly spaced positions along the profile widget's path. Fills an entire table at once
  * for a speedup if all of the results are going to be used anyway. */
/* HANS-TODO: Enable this */
/* HANS-TODO: Switch this over to using the secondary higher resolution set of points which ideally
 * would be parametrically subdivided so that there are  only extra points between the curved
 * user-inputted points */
void profilewidget_fill_segment_table(const struct ProfileWidget *prwdgt,
                                    double *x_table_out,
                                    double *y_table_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH FILL SEGMENT TABLE\n");
#endif
  const float segment_length = prwdgt->total_length / prwdgt->nsegments;
  float length_travelled = 0.0f;
  float distance_to_next_point = profilewidget_linear_distance_to_next_point(prwdgt, 0);
  float distance_to_previous_point = 0.0f;
  float travelled_since_last_point = 0.0f;
  float segment_left = segment_length;
  float f;
  int i_point = 0;

  /* Travel along the path, recording locations of segments as we pass where they should be */
  for (int i = 0; i < prwdgt->nsegments; i++) {
    /* Travel over all of the points that could be inside this segment */
    while (distance_to_next_point > segment_length * (i + 1) - length_travelled) {
      length_travelled += distance_to_next_point;
      segment_left -= distance_to_next_point;
      travelled_since_last_point += distance_to_next_point;
      i_point++;
      distance_to_next_point = profilewidget_linear_distance_to_next_point(prwdgt, i_point);
      distance_to_previous_point = 0.0f;
    }
    /* We're now at the last point that fits inside the current segment */
    f = segment_left / (distance_to_previous_point + distance_to_next_point);
    x_table_out[i] = (double)lerp(prwdgt->path[i_point].x, prwdgt->path[i_point+1].x, f);
    y_table_out[i] = (double)lerp(prwdgt->path[i_point].x, prwdgt->path[i_point+1].x, f);
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

  prwdgt->nsegments = nsegments;
  float total_length = profilewidget_total_length(prwdgt);
  prwdgt->total_length = total_length;

#if DEBUG_PRWDGT
  printf("Total length of the path is: %f\n", (double)total_length);
#endif

  /* Calculate the high resolution table for display and evaluation */
  profilewidget_make_table(prwdgt);
}

/* Evaluates along the length of the path rather than with X coord */
void profilewidget_evaluate(const struct ProfileWidget *prwdgt,
                           int segment,
                           float *x_out,
                           float *y_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH EVALUATE\n");
#endif
  /* HANS-TODO: Use profilewidget_fill_segment_table */

  profilewidget_evaluate_portion(prwdgt, ((float)segment / (float)prwdgt->nsegments), x_out, y_out);

  /* Clip down to 0 to 1 range for both coords */
  if (prwdgt->flag & PROF_DO_CLIP) {
    if (*x_out < prwdgt->view_rect.xmin) {
      *x_out = prwdgt->view_rect.xmin;
    }
    else if (*x_out > prwdgt->view_rect.xmax) {
      *x_out = prwdgt->view_rect.xmax;
    }
    if (*y_out < prwdgt->view_rect.ymin) {
      *y_out = prwdgt->view_rect.ymin;
    }
    else if (*y_out > prwdgt->view_rect.ymax) {
      *y_out = prwdgt->view_rect.ymax;
    }
  }
}

#undef DEBUG_PRWDGT
#undef DEBUG_PRWDGT_TABLE

