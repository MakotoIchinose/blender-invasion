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

#define DEBUG_PRWDGT 0
#define DEBUG_PRWDGT_TABLE 0
#define DEBUG_PRWDGT_EVALUATE 0
#define DEBUG_PRWDGT_REVERSE 0
/* HANS-TODO: Remove debugging code */

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
  if (!prwdgt->path) {
    printf("The prwdgt had no path... probably a second redundant free call\n");
  }
#endif
  if (prwdgt->path) {
    MEM_freeN(prwdgt->path);
    prwdgt->path = NULL;
  }
  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
    prwdgt->table = NULL;
  }
  if (prwdgt->samples) {
    MEM_freeN(prwdgt->samples);
    prwdgt->samples = NULL;
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
  if (prwdgt->samples) {
    target->samples = MEM_dupallocN(prwdgt->samples);
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

/** Removes a specific point from the path of control points
 * \note: Requiress profilewidget_changed call after */
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

/** Removes every point in the widget with the supplied flag set, except for the first and last.
 * \param flag: ProfilePoint->flag
 * \note: Requiress profilewidget_changed call after */
void profilewidget_remove(ProfileWidget *prwdgt, const short flag)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH REMOVE\n");
#endif
  int i_old, i_new, n_removed = 0;

  /* Copy every point without the flag into the new path */
  ProfilePoint *new_pts = MEM_mallocN(((size_t)prwdgt->totpoint) * sizeof(ProfilePoint),
                                  "path points");

  /* Don't delete the starting and ending points */
  new_pts[0] = prwdgt->path[0];
  for (i_old = 1, i_new = 1; i_old < prwdgt->totpoint - 1; i_old++) {
    if (!(prwdgt->path[i_old].flag & flag)) {
      new_pts[i_new] = prwdgt->path[i_old];
      i_new++;
    }
    else {
      n_removed++;
    }
  }
  new_pts[i_new] = prwdgt->path[i_old];

  MEM_freeN(prwdgt->path);
  prwdgt->path = new_pts;
  prwdgt->totpoint -= n_removed;
}

/** Adds a new point at the specified location. The choice for which points to place the new vertex
 * between is more complex for a profile. We can't just find the new neighbors with X value
 * comparisons. Instead this function checks which line segment is closest to the new point.
 * \note: Requiress profilewidget_changed call after */
ProfilePoint *profilewidget_insert(ProfileWidget *prwdgt, float x, float y)
{
  ProfilePoint *new_pt = NULL;
  float new_loc[2] = {x, y};
#if DEBUG_PRWDGT
  printf("PROFILEPATH INSERT\n");
#endif

  if (prwdgt->totpoint == PROF_TABLE_SIZE - 1) {
    return NULL;
  }

  /* Find the index at the line segment that's closest to the new position */
  float distance;
  float min_distance = FLT_MAX;
  int insert_i = 0;
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
      /* Insert old points */
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

/** Sets the handle type of the selected control points.
 * \param type: Either HD_VECT or HD_AUTO_ANIM
 * \note: Requiress profilewidget_changed call after. */
void profilewidget_handle_set(ProfileWidget *prwdgt, int type)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH HANDLE SET\n");
#endif
  for (int i = 0; i < prwdgt->totpoint; i++) {
    if (prwdgt->path[i].flag & PROF_SELECT) {
      prwdgt->path[i].flag &= ~(PROF_HANDLE_VECTOR | PROF_HANDLE_AUTO_ANIM);
      if (type == HD_VECT) {
        prwdgt->path[i].flag |= PROF_HANDLE_VECTOR;
      }
      else if (type == HD_AUTO_ANIM) {
        prwdgt->path[i].flag |= PROF_HANDLE_AUTO_ANIM;
      }
    }
  }
}

/** Flips the profile across the diagonal so that its orientation is reversed.
 * \note: Requiress profilewidget_changed call after.  */
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
  /* Mirror the new points across the y = x line */
  for (int i = 1; i < prwdgt->totpoint - 1; i++) {
    new_pts[prwdgt->totpoint - i - 1].x = prwdgt->path[i].y;
    new_pts[prwdgt->totpoint - i - 1].y = prwdgt->path[i].x;
    new_pts[prwdgt->totpoint - i - 1].flag = prwdgt->path[i].flag;
  }
  /* Set the location of the first and last points */
  /* HANS-TODO: Bring this into the loop */
  new_pts[0].x = 1.0;
  new_pts[0].y = 0.0;
  new_pts[prwdgt->totpoint - 1].x = 0.0;
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

/** Resets the profile to the current preset.
 * \note: Requiress profilewidget_changed call after */
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
      prwdgt->path[0].x = 1.0;
      prwdgt->path[0].y = 0.0;
      prwdgt->path[1].x = 0.0;
      prwdgt->path[1].y = 1.0;
      break;
    case PROF_PRESET_SUPPORTS:
      prwdgt->path[0].x = 1.0;
      prwdgt->path[0].y = 0.0;
      prwdgt->path[1].x = 1.0;
      prwdgt->path[1].y = 0.5;
      for (int i = 1; i < 10; i++) {
        prwdgt->path[i + 1].x = 1.0f - (0.5f * (1.0f - cosf((float)((i / 9.0) * M_PI_2))));
        prwdgt->path[i + 1].y = 0.5f + 0.5f * sinf((float)((i / 9.0) * M_PI_2));
      }
      prwdgt->path[10].x = 0.5;
      prwdgt->path[10].y = 1.0;
      prwdgt->path[11].x = 0.0;
      prwdgt->path[11].y = 1.0;
      break;
    case PROF_PRESET_EXAMPLE1: /* HANS-TODO: Don't worry, this is just temporary */
      prwdgt->path[0].x = 1.0f;
      prwdgt->path[0].y = 0.0f;
      prwdgt->path[1].x = 1.0f;
      prwdgt->path[1].y = 0.6f;
      prwdgt->path[2].x = 0.9f;
      prwdgt->path[2].y = 0.6f;
      prwdgt->path[3].x = 0.9f;
      prwdgt->path[3].y = 0.7f;
      prwdgt->path[4].x = 1.0f - 0.195024f;
      prwdgt->path[4].y = 0.709379f;
      prwdgt->path[5].x = 1.0f - 0.294767f;
      prwdgt->path[5].y = 0.735585f;
      prwdgt->path[6].x = 1.0f - 0.369792f;
      prwdgt->path[6].y = 0.775577f;
      prwdgt->path[7].x = 1.0f - 0.43429f;
      prwdgt->path[7].y = 0.831837f;
      prwdgt->path[8].x = 1.0f - 0.500148f;
      prwdgt->path[8].y = 0.884851f;
      prwdgt->path[9].x = 1.0f - 0.565882f;
      prwdgt->path[9].y = 0.91889f;
      prwdgt->path[10].x = 1.0f - 0.633279f;
      prwdgt->path[10].y = 0.935271f;
      prwdgt->path[11].x = 1.0f - 0.697628f;
      prwdgt->path[11].y = 0.937218f;
      prwdgt->path[12].x = 1.0f - 0.75148f;
      prwdgt->path[12].y = 0.924844f;
      prwdgt->path[13].x = 1.0f - 0.791918f;
      prwdgt->path[13].y = 0.904817f;
      prwdgt->path[14].x = 1.0f - 0.822379f;
      prwdgt->path[14].y = 0.873682f;
      prwdgt->path[15].x = 1.0f - 0.842155f;
      prwdgt->path[15].y = 0.83174f;
      prwdgt->path[16].x = 0.15f;
      prwdgt->path[16].y = 0.775f;
      prwdgt->path[17].x = 1.0f - 0.929009f;
      prwdgt->path[17].y = 0.775f;
      prwdgt->path[18].x = 1.0f - 0.953861f;
      prwdgt->path[18].y = 0.780265f;
      prwdgt->path[19].x = 1.0f - 0.967919f;
      prwdgt->path[19].y = 0.794104f;
      prwdgt->path[20].x = 1.0f - 0.978458f;
      prwdgt->path[20].y = 0.818784f;
      prwdgt->path[21].x = 1.0f - 0.988467f;
      prwdgt->path[21].y = 0.890742f;
      prwdgt->path[22].x = 0.0f;
      prwdgt->path[22].y = 1.0f;
      break;
  }

  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
    prwdgt->table = NULL;
  }
}

/** Helper for profile_widget_create samples. Returns whether both handles that make up the edge
 * are vector handles */
static bool is_curved_edge(BezTriple * bezt, int i)
{
  return (bezt[i].h2 != HD_VECT || bezt[i + 1].h1 != HD_VECT);
}

/** Used in the sample creation process. Reduced copy of #calchandleNurb_intern code in curve.c */
static void calchandle_profile(BezTriple *bezt, const BezTriple *prev, const BezTriple *next)
{
#define p2_handle1 ((p2)-3)
#define p2_handle2 ((p2) + 3)

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
        madd_v2_v2v2fl(p2_handle1, p2, tvec, -len_a);

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
        madd_v2_v2v2fl(p2_handle2, p2, tvec, len_b);

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
    madd_v2_v2v2fl(p2_handle1, p2, dvec_a, -1.0f / 3.0f);
  }
  if (bezt->h2 == HD_VECT) {
    madd_v2_v2v2fl(p2_handle2, p2, dvec_b, 1.0f / 3.0f);
  }
#undef p2_handle1
#undef p2_handle2
}

/** Helper function for 'profilwidget_create_samples.' Compares BezTriple indices based on the
 * curvature of the edge after each of them in the table. Works by comparing the angle between the
 * handles that make up the edge: the secong handle of the first point and the first handle
 * of the second. */
static int compare_curvature_bezt_edge_i(const BezTriple *bezt, const int i_a, const int i_b)
{
  float handle_angle_a, handle_angle_b;
  float handle_loc[2], point_loc[2], handle_vec_1[2], handle_vec_2[2];

  handle_loc[0] = bezt[i_a].vec[0][0]; /* handle 2 x */
  handle_loc[1] = bezt[i_a].vec[0][1]; /* handle 2 y */
  point_loc[0] = bezt[i_a].vec[1][0]; /* point x */
  point_loc[1] = bezt[i_a].vec[1][1]; /* point y */
  sub_v2_v2v2(handle_vec_1, handle_loc, point_loc);
  handle_loc[0] = bezt[i_a].vec[0][0]; /* handle 1 x */
  handle_loc[1] = bezt[i_a].vec[0][1]; /* handle 1 y */
  point_loc[0] = bezt[i_a].vec[1][0];
  point_loc[1] = bezt[i_a].vec[1][1];
  sub_v2_v2v2(handle_vec_2, point_loc, handle_loc);
  handle_angle_a = acosf(dot_v2v2(handle_vec_1, handle_vec_2));

  handle_loc[0] = bezt[i_b].vec[0][0]; /* handle 2 x */
  handle_loc[1] = bezt[i_b].vec[0][1]; /* handle 2 y */
  point_loc[0] = bezt[i_b].vec[1][0]; /* point x */
  point_loc[1] = bezt[i_b].vec[1][1]; /* point y */
  sub_v2_v2v2(handle_vec_1, handle_loc, point_loc);
  handle_loc[0] = bezt[i_b].vec[0][0];
  handle_loc[1] = bezt[i_b].vec[0][1];
  point_loc[0] = bezt[i_b].vec[1][0];
  point_loc[1] = bezt[i_b].vec[1][1];
  sub_v2_v2v2(handle_vec_2, point_loc, handle_loc);
  handle_angle_b = acosf(dot_v2v2(handle_vec_1, handle_vec_2));

  /* Return 1 if edge a is more curved, 0 if edge b is more curved */
  if (handle_angle_a > handle_angle_b) {
    return 1; /* First edge more curved */
  }
  return 0; /* Second edge more curved */
}

/** Used for sampling curves along the profile's path. Any points more than the number of user-
 * defined points will be evenly distributed among the curved edges. Then the remainders will be
 * distributed to the most curved edges.
 * \param locations: An array of points to put the sampled positions. Must have length n_segments.
 * \param n_segments: The number of segments to sample along the path. It must be higher than the
 *        number of points used to define the profile (prwdgt->totpoint).
 * \param sample_straight_edges: Whether to sample points between vector handle control points. If
          this is true and there are only vector edges the straight edges will still be sampled. */
void profilewidget_create_samples(const ProfileWidget *prwdgt,
                                  float *locations,
                                  int n_segments,
                                  bool sample_straight_edges)
{
#if DEBUG_PRWDGT
  printf("PROFILEWIDGET CREATE SAMPLES\n");
  if (prwdgt->path == NULL) {
    printf("(path is NULL)\n");
    return;
  }
#endif
  BezTriple *bezt;
  int i, i_insert, n_left, n_common, i_segment, n_curved_edges;
  int *i_curve_sorted, *n_points;
  int totedges = prwdgt->totpoint - 1;
  int totpoints = prwdgt->totpoint;

  BLI_assert(n_segments >= totpoints);

  /* Create Bezier points for calculating the higher resolution path */
  bezt = MEM_callocN((size_t)totpoints * sizeof(BezTriple), "beztarr");
  for (i = 0; i < totpoints; i++) {
    bezt[i].vec[1][0] = prwdgt->path[i].x;
    bezt[i].vec[1][1] = prwdgt->path[i].y;
    bezt[i].h1 = bezt[i].h2 = (prwdgt->path[i].flag & PROF_HANDLE_VECTOR) ? HD_VECT : HD_AUTO_ANIM;
  }
  calchandle_profile(&bezt[0], NULL, &bezt[1]);
  for (i = 1; i < totpoints - 1; i++) {
    calchandle_profile(&bezt[i], &bezt[i - 1], &bezt[i + 1]);
  }
  calchandle_profile(&bezt[totpoints - 1], &bezt[totpoints - 2], NULL);

  /* First and last handle need correction, instead of pointing to center of next/prev,
   * we let it point to the closest handle */
  if (0 && prwdgt->totpoint > 2) {
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

  /* Sort a list of indexes by the curvature at each point*/
  i_curve_sorted = MEM_callocN((size_t)totedges * sizeof(int), "i curve sorted");
  /* (Insertion sort for now. It's probably fast enough though) */
  /* HANS-TODO: Test the sorting */
  for (i = 0; i < totedges; i++) {
    /* Place the new index (i) at the first spot where its edge has less curvature */
    for (i_insert = 0; i_insert < i; i_insert++) {
      if (compare_curvature_bezt_edge_i(bezt, i, i_curve_sorted[i_insert])) {
        break;
      }
    }
    i_curve_sorted[i_insert] = i;
  }

  /* Assign sampled points to each edge. */
  n_points = MEM_callocN((size_t)totedges * sizeof(int),  "create samples numbers");
  int n_added = 0;
  if (sample_straight_edges) {
    /* Assign an even number to each edge if it’s possible, then add the remainder of sampled
     * points starting with the most curved edges. */
    n_common = n_segments / totedges;
    n_left = n_segments % totedges;

    /* Assign the points that fill fit evenly to the edges */
    if (n_common > 0) {
      for (i = 0; i < totedges; i++) {
        n_points[i] = n_common;
        n_added += n_common;
      }
    }
  }
  else {
    /* Count the number of curved edges */
    n_curved_edges = 0;
    for (i = 0; i < totedges; i++) {
      if (is_curved_edge(bezt, i)) {
        n_curved_edges++;
      }
    }
    /* Just sample all of the edges if there are no curved edges */
    if (n_curved_edges == 0) {
      n_curved_edges = totedges;
    }

    /* Give all of the curved edges the same number of points and straight edges one point */
    n_left = n_segments - (totedges - n_curved_edges); /* Left after one for each straight edge */
    n_common = n_left / n_curved_edges; /* Number assigned to every curved edge */
    if (n_common > 0) {
      for (i = 0; i < totedges; i++) {
        if (is_curved_edge(bezt, i)) {
          n_points[i] += n_common;
          n_added += n_common;
        }
        else {
          n_points[i] = 1;
          n_added++;
        }
      }
    }
    n_left -= n_common * n_curved_edges;
  }
  /* Assign the remainder of the points that couldn't be spread out evenly */
  BLI_assert(n_left < totedges);
  for (i = 0; i < n_left; i++) {
    n_points[i_curve_sorted[i]]++;
    n_added++;
  }
  BLI_assert(n_added == n_segments);

  /* Sample the points and add them to the locations table */
  for (i_segment = 0, i = 0; i < totedges; i++) {
    if (n_points[i] > 0) {
      BKE_curve_forward_diff_bezier(bezt[i].vec[1][0],
                                    bezt[i].vec[2][0],
                                    bezt[i + 1].vec[0][0],
                                    bezt[i + 1].vec[1][0],
                                    &locations[2 * i_segment],
                                    n_points[i],
                                    2 * sizeof(float));
      BKE_curve_forward_diff_bezier(bezt[i].vec[1][1],
                                    bezt[i].vec[2][1],
                                    bezt[i + 1].vec[0][1],
                                    bezt[i + 1].vec[1][1],
                                    &locations[2 * i_segment + 1],
                                    n_points[i],
                                    2 * sizeof(float));
    }
    i_segment += n_points[i]; /* Add the next points after the ones we just added */
  }
#if DEBUG_PRWDGT_TABLE
  printf("n_segments: %d\n", n_segments);
  printf("totedges: %d\n", totedges);
  printf("n_common: %d\n", n_common);
  printf("n_left: %d\n", n_left);
  printf("n_points: ");
  for (i = 0; i < totedges; i++) {
    printf("%d, ", n_points[i]);
  }
  printf("\n");
  printf("i_curved_sorted: ");
  for (i = 0; i < totedges; i++) {
    printf("%d, ", i_curve_sorted[i]);
  }
  printf("\n");
#endif
  MEM_freeN(bezt);
  MEM_freeN(i_curve_sorted);
  MEM_freeN(n_points);
}

/** Creates a higher resolution table by sampling the curved points. This table is used for display
 * and evenly spaced evaluation. */
static void profilewidget_make_table(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH MAKE TABLE\n");
#endif
  ProfilePoint *new_table;
  float *locations;
  int i, n_samples;

  /* Get locations of samples from the sampling function */
  n_samples = PROF_TABLE_SIZE;
  locations = MEM_callocN((size_t)n_samples * 2 * sizeof(float), "temp loc storage");
  profilewidget_create_samples(prwdgt, locations, n_samples - 1, false);

  /* Put the locations in the new table */
  new_table = MEM_callocN((size_t)n_samples * sizeof(ProfilePoint), "high-res table");
  for (i = 0; i < n_samples; i++) {
    new_table[i].x = locations[2 * i];
    new_table[i].y = locations[2 * i + 1];
  }

  MEM_freeN(locations);
  if (prwdgt->table) {
    MEM_freeN(prwdgt->table);
  }
  prwdgt->table = new_table;
}

/** Creates the table of points with the sampled curves, this time for the samples used by the user. */
/* HANS-TODO: Remove if it doesn't work */
static void profilewidget_make_table_samples(ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH MAKE TABLE SAMPLES\n");
#endif
  ProfilePoint *new_table;
  float *locations;
  int i, n_samples;

  /* Get locations of samples from the sampling function */
  n_samples = prwdgt->totsegments;
  locations = MEM_callocN((size_t)n_samples * 2 * sizeof(float), "temp loc storage");
  profilewidget_create_samples(prwdgt, locations, n_samples - 1, true);

  /* Put the locations in the new table */
  new_table = MEM_callocN((size_t)n_samples * sizeof(ProfilePoint), "samples table");
  for (i = 0; i < n_samples; i++) {
    new_table[i].x = locations[2 * i];
    new_table[i].y = locations[2 * i + 1];
  }

  MEM_freeN(locations);
  if (prwdgt->samples) {
    MEM_freeN(prwdgt->samples);
  }
  prwdgt->samples = new_table;
}

/** Should be called after the widget is changed. Does profile and remove double checks and more
 * importantly recreates the display / evaluation / samples tables */
void profilewidget_changed(ProfileWidget *prwdgt, const bool remove_double)
{
  ProfilePoint *points = prwdgt->path;
  rctf *clipr = &prwdgt->clip_rect;
  float thresh;
  float dx = 0.0f, dy = 0.0f;
  int i;

  prwdgt->changed_timestamp++;

#if DEBUG_PRWDGT
  printf("PROFILEWIDGET CHANGED/n");
#endif

  /* Clamp with the clipping rect in case something got past */
  /* HANS-TODO: I thought this was done elsewhere too */
  if (prwdgt->flag & PROF_DO_CLIP) {
    /* Move points inside the clip rectangle */
    for (i = 0; i < prwdgt->totpoint; i++) {
      points[i].x = max_ff(points[i].x, clipr->xmin);
      points[i].x = min_ff(points[i].x, clipr->xmax);
      points[i].y = max_ff(points[i].y, clipr->ymin);
      points[i].y = min_ff(points[i].y, clipr->ymax);
    }
    /* Ensure zoom-level respects clipping */
    if (BLI_rctf_size_x(&prwdgt->view_rect) > BLI_rctf_size_x(&prwdgt->clip_rect)) {
      prwdgt->view_rect.xmin = prwdgt->clip_rect.xmin;
      prwdgt->view_rect.xmax = prwdgt->clip_rect.xmax;
    }
    if (BLI_rctf_size_y(&prwdgt->view_rect) > BLI_rctf_size_y(&prwdgt->clip_rect)) {
      prwdgt->view_rect.ymin = prwdgt->clip_rect.ymin;
      prwdgt->view_rect.ymax = prwdgt->clip_rect.ymax;
    }
  }

  /* Remove doubles with a threshold set on 1% of default range */
  thresh = 0.01f * BLI_rctf_size_x(clipr);
  if (remove_double && prwdgt->totpoint > 2) {
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

  /* Create the high resolution table for drawing and some evaluation functions */
  profilewidget_make_table(prwdgt);

  /* Store a table of samples to display a preview of them in the widget */
  /* HANS-TODO: Get this working or delete it. */
  if (prwdgt->totsegments > 0) {
    profilewidget_make_table_samples(prwdgt);
  }
}

/** Refreshes the higher resolution table sampled from the input points. Needed before evaluation
 * functions that use the table */
void profilewidget_initialize(ProfileWidget *prwdgt, short nsegments)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH INITIALIZE\n");
#endif
  BLI_assert(prwdgt);

  prwdgt->totsegments = nsegments;

  /* Calculate the high resolution table for display and evaluation */
  profilewidget_changed(prwdgt, false);
}

/** Gives the distance to the next point in the widget's sampled table, in other words the length
 * of the ith edge of the table.
 * \note Requires profilewidget_initialize or profilewidget_changed call before to fill table */
float profilewidget_distance_to_next_point(const ProfileWidget *prwdgt, int i)
{
  BLI_assert(prwdgt != NULL);
  BLI_assert(i >= 0);
  BLI_assert(i < prwdgt->totpoint);

  float x = prwdgt->table[i].x;
  float y = prwdgt->table[i].y;
  float x_next = prwdgt->table[i + 1].x;
  float y_next = prwdgt->table[i + 1].y;

  return sqrtf(powf(y_next - y, 2) + powf(x_next - x, 2));
}

/** Calculates the total length of the profile, including the curves sampled in the table.
 * \note Requires profilewidget_initialize or profilewidget_changed call before to fill table */
float profilewidget_total_length(const ProfileWidget *prwdgt)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH TOTAL LENGTH\n");
#endif
  float loc1[2], loc2[2];
  float total_length = 0;

  for (int i = 0; i < PROF_TABLE_SIZE; i++) {
    loc1[0] = prwdgt->table[i].x;
    loc1[1] = prwdgt->table[i].y;
    loc2[0] = prwdgt->table[i].x;
    loc2[1] = prwdgt->table[i].y;
    total_length += len_v2v2(loc1, loc2);
  }
  return total_length;
}

/** Samples evenly spaced positions along the profile widget's table (generated from path). Fills
 * an entire table at once for a speedup if all of the results are going to be used anyway.
 * \note Requires profilewidget_initialize or profilewidget_changed call before to fill table */
/* HANS-TODO: Enable this for an "even length sampling" option (and debug it). */
void profilewidget_fill_segment_table(const ProfileWidget *prwdgt,
                                      double *x_table_out,
                                      double *y_table_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH FILL SEGMENT TABLE\n");
#endif
  const float total_length = profilewidget_total_length(prwdgt);
  const float segment_length = total_length / prwdgt->totsegments;
  float length_travelled = 0.0f;
  float distance_to_next_point = profilewidget_distance_to_next_point(prwdgt, 0);
  float distance_to_previous_point = 0.0f;
  float travelled_since_last_point = 0.0f;
  float segment_left = segment_length;
  float f;
  int i_point = 0;

  /* Travel along the path, recording locations of segments as we pass where they should be */
  for (int i = 0; i < prwdgt->totsegments; i++) {
    /* Travel over all of the points that could be inside this segment */
    while (distance_to_next_point > segment_length * (i + 1) - length_travelled) {
      length_travelled += distance_to_next_point;
      segment_left -= distance_to_next_point;
      travelled_since_last_point += distance_to_next_point;
      i_point++;
      distance_to_next_point = profilewidget_distance_to_next_point(prwdgt, i_point);
      distance_to_previous_point = 0.0f;
    }
    /* We're now at the last point that fits inside the current segment */
    f = segment_left / (distance_to_previous_point + distance_to_next_point);
    x_table_out[i] = (double)interpf(prwdgt->table[i_point].x, prwdgt->table[i_point + 1].x, f);
    y_table_out[i] = (double)interpf(prwdgt->table[i_point].x, prwdgt->table[i_point + 1].x, f);
    distance_to_next_point -= segment_left;
    distance_to_previous_point += segment_left;

    length_travelled += segment_left;
  }
}

/** Does a single evaluation along the profile's path. Travels down length_portion * path length
 * and returns the position at that point
 * \param length_portion: The portion (0 to 1) of the path's full length to sample at.
 * \note Requires profilewidget_initialize or profilewidget_changed call before to fill table */
void profilewidget_evaluate_portion(const ProfileWidget *prwdgt,
                                    float length_portion,
                                    float *x_out,
                                    float *y_out)
{
#if DEBUG_PRWDGT
  printf("PROFILEPATH EVALUATE\n");
#endif
  const float total_length = profilewidget_total_length(prwdgt);
  float requested_length = length_portion * total_length;

  /* Find the last point along the path with a lower length portion than the input */
  int i = 0;
  float length_travelled = 0.0f;
  while (length_travelled < requested_length) {
    /* Check if we reached the last point before the final one */
    if (i == PROF_TABLE_SIZE - 2) {
      break;
    }
    float new_length = profilewidget_distance_to_next_point(prwdgt, i);
    if (length_travelled + new_length >= requested_length) {
      break;
    }
    length_travelled += new_length;
    i++;
  }

  /* Now travel the remaining distance of length portion down the path to the next point and
   * find the location where we stop */
  float distance_to_next_point = profilewidget_distance_to_next_point(prwdgt, i);
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

  *x_out = interpf(prwdgt->table[i].x, prwdgt->table[i + 1].x, lerp_factor);
  *y_out = interpf(prwdgt->table[i].y, prwdgt->table[i + 1].y, lerp_factor);
}

#undef DEBUG_PRWDGT
#undef DEBUG_PRWDGT_TABLE

