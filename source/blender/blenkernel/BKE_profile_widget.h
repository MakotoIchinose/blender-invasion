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

#ifndef BKE_PROFILEPATH_H
#define BKE_PROFILEPATH_H

/** \file
 * \ingroup bke
 */

struct ProfileWidget;
struct ProfilePoint;

void profilewidget_set_defaults(struct ProfileWidget *prwdgt);

struct ProfileWidget *profilewidget_add(int preset);

void profilewidget_free_data(struct ProfileWidget *prwdgt);

void profilewidget_free(struct ProfileWidget *prwdgt);

void profilewidget_copy_data(struct ProfileWidget *target, const struct ProfileWidget *prwdgt);

struct ProfileWidget *profilewidget_copy(const struct ProfileWidget *prwdgt);

bool profilewidget_remove_point(struct ProfileWidget *prwdgt, struct ProfilePoint *point);

void profilewidget_remove(struct ProfileWidget *prwdgt, const short flag);

struct ProfilePoint *profilewidget_insert(struct ProfileWidget *prwdgt, float x, float y);

void profilewidget_handle_set(struct ProfileWidget *prwdgt, int type);

void profilewidget_reverse(struct ProfileWidget *prwdgt);

void profilewidget_reset(struct ProfileWidget *prwdgt);

void profilewidget_create_samples(const struct ProfileWidget *prwdgt,
                                  float *locations,
                                  int n_segments,
                                  bool sample_straight_edges);

void profilewidget_initialize(struct ProfileWidget *prwdgt, short nsegments);

/* Called for a complete update of the widget after modifications */
void profilewidget_changed(struct ProfileWidget *prwdgt, const bool rem_doubles);

/* Distance in 2D to the next point */
float profilewidget_distance_to_next_point(const struct ProfileWidget *prwdgt, int i);

/* Need to find the total length of the curve to sample a portion of it */
float profilewidget_total_length(const struct ProfileWidget *prwdgt);

void profilewidget_create_samples_even_spacing(const struct ProfileWidget *prwdgt,
                                      double *x_table_out,
                                      double *y_table_out);

/* Length portion is the fraction of the total path length where we want the location */
void profilewidget_evaluate_portion(const struct ProfileWidget *prwdgt,
                                    float length_portion,
                                    float *x_out,
                                    float *y_out);
#endif
