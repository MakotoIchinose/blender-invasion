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
 * \ingroup DNA
 */

#ifndef DNA_PROFILEPATH_TYPES_H
#define DNA_PROFILEPATH_TYPES_H

#include "DNA_vec_types.h"

/** Number of points in high resolution table is dynamic up to the maximum */
#define PROF_TABLE_MAX 512
/** Number of table points per control point */
#define PROF_RESOL 16
/** Dynamic size of widget's high resolution table, input should be prwdgt->totpoint */
#define PROF_N_TABLE(n_pts) min_ii(PROF_TABLE_MAX, (((n_pts - 1)) * PROF_RESOL) + 1)

typedef struct ProfilePoint {
  /** Location of the point, keep together */
  float x, y;
  /** Flag for handle type and selection state */
  short flag;
  char _pad[2];
} ProfilePoint;

/** ProfilePoint->flag */
enum {
  PROF_SELECT = (1 << 0),
  PROF_HANDLE_VECTOR = (1 << 1),
  PROF_HANDLE_AUTO = (1 << 2),
};

typedef struct ProfileWidget {
  /** Number of user-added points that define the profile */
  short totpoint;
  /** Number of sampled points */
  short totsegments;
  /** Preset to use when reset */
  int preset;
  /** Sequence of points defining the shape of the curve  */
  ProfilePoint *path;
  /** Display and evaluation table at higher resolution for curves */
  ProfilePoint *table;
  /** The positions of the sampled points. Used to display a preview of where they will be */
  ProfilePoint *segments;
  /** Flag for mode states, sampling options, etc... */
  int flag;
  /** Used for keeping track how many times the widget is changed */
  int changed_timestamp;
  /** Current rect, clip rect (is default rect too). */
  rctf view_rect, clip_rect;
} ProfileWidget;

/** ProfileWidget->flag */
enum {
  PROF_USE_CLIP = (1 << 0), /* Keep control points inside bounding rectangle */
  PROF_SYMMETRY_MODE = (1 << 1), /* HANS-TODO: Add symmetry mode */
  PROF_SAMPLE_STRAIGHT_EDGES = (1 << 2), /* Sample extra points on straight edges */
};

typedef enum eProfileWidgetPresets {
  PROF_PRESET_LINE = 0, /* Default simple line */
  PROF_PRESET_SUPPORTS = 1, /* Support loops for a regular curved profile */
  PROF_PRESET_EXAMPLE1 = 2, /* Molding type example of a supposed common use case */
} ProfileWiedgetPresets;

#endif
