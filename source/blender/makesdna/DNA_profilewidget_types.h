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

#define PROF_TABLE_SIZE 256
/* HANS-TODO: Switch to variable table size based on resolution and number of points, mostly for a
 * speedup in the drawing and evaluation code if it's needed */
//#define PROF_N_TABLE(n_pts) min_ff(512, (((n_pts) - 1) * PROF_RESOL)) /* n_pts is prwdgt->totpoint */
//#define PROF_RESOL 16

typedef struct ProfilePoint {
  /** Location of the point */
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
  ProfilePoint *samples;
  /** Cur; for buttons, to show active curve. */
  int flag;
  /** Used for keeping track how many times the widget is changed */
  int changed_timestamp;
  /** Current rect, clip rect (is default rect too). */
  rctf view_rect, clip_rect;
} ProfileWidget;

/** ProfileWidget->flag */
enum {
  PROF_DO_CLIP = (1 << 0),
  PROF_USE_TABLE = (1 << 1),
  PROF_SAMPLE_STRAIGHT_EDGES = (1 << 2),
};

typedef enum eProfileWidgetPresets {
  PROF_PRESET_LINE = 0, /* Default simple line */
  PROF_PRESET_SUPPORTS = 1, /* Support loops for a regular curved profile */
  PROF_PRESET_EXAMPLE1 = 2, /* Molding type example of a supposed common use case */
} ProfileWiedgetPresets;

#endif
