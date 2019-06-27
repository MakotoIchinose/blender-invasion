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

/* general defines for kernel functions */
#define PROF_RESOL 32 /* HANS-QUESTION: What is this resolution parameter used for? */
#define PROF_TABLE_SIZE 256
#define PROF_TABLEDIV (1.0f / 256.0f)

typedef struct ProfilePoint {
  float x, y;
  /** Shorty for result lookup. */
  short flag;
  char _pad[2];
} ProfilePoint;

/* ProfilePoint->flag */
enum {
  PROF_SELECT = (1 << 0),
  PROF_HANDLE_VECTOR = (1 << 1),
  PROF_HANDLE_AUTO_ANIM = (1 << 2),
};

/* HANS-TODO: Combine into ProfileWidget probably */
typedef struct ProfilePath {
  short totpoint, flag;
  char _pad1[4];
  /** Sequence of points defining the shape of the curve  */
  ProfilePoint *path;
  /** Display and evaluation table at higher resolution for curves */
  ProfilePoint *table;

  /** The x-axis range for the table. */
  float mintable, maxtable;
  /** Total length of curve for path curves */
  float total_length;
  /** Range... */ /* HANS-TODO: Figure out if I need range */
  float range;
  /** Number of segments for sampled path */
  int nsegments;

  /** Sampled segment point location table */
  char _pad4[4];
} ProfilePath;

typedef struct ProfileWidget {
  /** Cur; for buttons, to show active curve. */
  int flag;
  /** Preset to use when reset */
  int preset;
  int changed_timestamp;
  /** Current rect, clip rect (is default rect too). */
  char _pad[4];
  rctf view_rect, clip_rect;
  /** The actual profile path information */
  ProfilePath *profile;
} ProfileWidget;

/* ProfileWidget->flag */
#define PROF_DO_CLIP (1 << 0)
#define PROF_USE_TABLE (1 << 1)

typedef enum eProfilePathPresets {
  PROF_PRESET_LINE = 0, /* Default simple line */
  PROF_PRESET_SUPPORTS = 1, /* Support loops for a regular curved profile */
  PROF_PRESET_EXAMPLE1 = 2, /* Molding type example of a supposed common use case */
} ProfilePathPresets;

#endif
