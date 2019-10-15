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

#ifndef __BKE_PROFILEPATH_H__
#define __BKE_PROFILEPATH_H__

/** \file
 * \ingroup bke
 */

struct ProfileCurve;
struct ProfilePoint;

void BKE_profilecurve_set_defaults(struct ProfileCurve *prwdgt);

struct ProfileCurve *BKE_profilecurve_add(int preset);

void BKE_profilecurve_free_data(struct ProfileCurve *prwdgt);

void BKE_profilecurve_free(struct ProfileCurve *prwdgt);

void BKE_profilecurve_copy_data(struct ProfileCurve *target, const struct ProfileCurve *prwdgt);

struct ProfileCurve *BKE_profilecurve_copy(const struct ProfileCurve *prwdgt);

bool BKE_profilecurve_remove_point(struct ProfileCurve *prwdgt, struct ProfilePoint *point);

void BKE_profilecurve_remove_by_flag(struct ProfileCurve *prwdgt, const short flag);

struct ProfilePoint *BKE_profilecurve_insert(struct ProfileCurve *prwdgt, float x, float y);

void BKE_profilecurve_handle_set(struct ProfileCurve *prwdgt, int type_1, int type_2);

void BKE_profilecurve_reverse(struct ProfileCurve *prwdgt);

void BKE_profilecurve_reset(struct ProfileCurve *prwdgt);

void BKE_profilecurve_create_samples(struct ProfileCurve *prwdgt,
                                      int n_segments,
                                      bool sample_straight_edges,
                                      struct ProfilePoint *r_samples);

void BKE_profilecurve_initialize(struct ProfileCurve *prwdgt, short nsegments);

/* Called for a complete update of the widget after modifications */
void BKE_profilecurve_update(struct ProfileCurve *prwdgt, const bool rem_doubles);

/* Need to find the total length of the curve to sample a portion of it */
float BKE_profilecurve_total_length(const struct ProfileCurve *prwdgt);

void BKE_profilecurve_create_samples_even_spacing(struct ProfileCurve *prwdgt,
                                                   int n_segments,
                                                   struct ProfilePoint *r_samples);

/* Length portion is the fraction of the total path length where we want the location */
void BKE_profilecurve_evaluate_length_portion(const struct ProfileCurve *prwdgt,
                                               float length_portion,
                                               float *x_out,
                                               float *y_out);
#endif
