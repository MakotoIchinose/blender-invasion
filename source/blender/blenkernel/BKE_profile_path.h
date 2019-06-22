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
/* HANS-TODO: Check right copyright info for new files */
#ifndef BKE_PROFILEPATH_H
#define BKE_PROFILEPATH_H

/** \file
 * \ingroup bke
 */

struct ProfileWidget;
struct ProfilePath;
struct ProfilePoint;

/* HANS-TODO: Organize */

void profilewidget_set_defaults(struct ProfileWidget *prwdgt);

struct ProfileWidget *profilewidget_add(int preset);

void profilewidget_free_data(struct ProfileWidget *prwdgt);

void profilewidget_free(struct ProfileWidget *prwdgt);

void profilewidget_copy_data(struct ProfileWidget *target, const struct ProfileWidget *prwdgt);

struct ProfileWidget *profilewidget_copy(const struct ProfileWidget *prwdgt);

/* Evaluates along the length of the path rather than with X coord */
void profilewidget_evaluate(const struct ProfileWidget *prwdgt,
                            int segment,
                            float *x_out,
                            float *y_out);

/* Length portion is the fraction of the total path length where we want the location */
void profilepath_evaluate(const struct ProfilePath *prpath,
                          float length_portion,
                          float *x_out,
                          float *y_out);

/* Need to find the total length of the curve to sample a portion of it */
float profilepath_total_length(const struct ProfilePath *prpath);

/* Distance in 2D to the next point */
float profilepath_linear_distance_to_next_point(const struct ProfilePath *prpath, int i);


void profilepath_reset(struct ProfilePath *prpath, int preset);

void profilepath_remove(struct ProfilePath *prpath, const short flag);

bool profilepath_remove_point(struct ProfilePath *prpath, struct ProfilePoint *point);

struct ProfilePoint *profilepath_insert(struct ProfilePath *prpath, float x, float y);

void profilepath_handle_set(struct ProfilePath *prpath, int type);

/* Called for a complete update of the widget after modifications */
void profilewidget_changed(struct ProfileWidget *prwdgt, const bool rem_doubles);

/* call before _all_ evaluation functions */
void profilewidget_initialize(struct ProfileWidget *prwdgt, int nsegments);

#endif
