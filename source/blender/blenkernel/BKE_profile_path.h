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
 * The Original Code is Copyright (C) 2006 Blender Foundation.
 * All rights reserved.
 */
#ifdef __NO_NO_NO__
#define __NO_NO_NO_
//#ifndef __BKE_PROFILEPATH_H__
//#define __BKE_PROFILEPATH_H__

/** \file
 * \ingroup bke
 */

struct CurveMap;
struct CurveMapPoint;
struct CurveMapping;

void profilepath_set_defaults(
    struct CurveMapping *cumap, int tot, float minx, float miny, float maxx, float maxy);
struct CurveMapping *profilepath_add(int tot, float minx, float miny, float maxx, float maxy);
void profilepath_free_data(struct CurveMapping *cumap);
void profilepath_free(struct CurveMapping *cumap);
void profilepath_copy_data(struct CurveMapping *target, const struct CurveMapping *cumap);
struct CurveMapping *profilepath_copy(const struct CurveMapping *cumap);
void profilepath_set_black_white_ex(const float black[3], const float white[3], float r_bwmul[3]);
void profilepath_set_black_white(struct CurveMapping *cumap,
                                  const float black[3],
                                  const float white[3]);

/* Used for a path where the curve isn't necessarily a function. */

/* Initialized with the number of segments to fill the table with */
void profilepath_initialize(
    struct CurveMapping *cumap,
    int nsegments);  // HANS-TODO: Hmm, this assumes there is only one curve
/* Evaluates along the length of the path rather than with X coord */
void profilepath_evaluate(const struct CurveMapping *cumap,
                                int segment,
                                float *x_out,
                                float *y_out);
/* Length portion is the fraction of the total path length where we want the location */
void profile_evaluate(const struct CurveMap *cuma,
                            float length_portion,
                            float *x_out,
                            float *y_out);
/* Need to find the total length of the curve to sample a portion of it */
float profile_total_length(const struct CurveMap *cuma);
/* Distance in 2D to the next point */
float profile_linear_distance_to_next_point(const struct CurveMap *cuma, int i);

enum {
  CURVEMAP_SLOPE_NEGATIVE = 0,
  CURVEMAP_SLOPE_POSITIVE = 1,
  CURVEMAP_SLOPE_POS_NEG = 2,
};

void profile_reset(struct CurveMap *cuma, const struct rctf *clipr, int preset, int slope);
void profile_remove(struct CurveMap *cuma, const short flag);
bool profile_remove_point(struct CurveMap *cuma, struct CurveMapPoint *cmp);
struct CurveMapPoint *curvemap_insert(struct CurveMap *cuma, float x, float y);
void profile_handle_set(struct CurveMap *cuma, int type);

void profilepath_changed(struct CurveMapping *cumap, const bool rem_doubles);
void profilepath_changed_all(struct CurveMapping *cumap);

/* call before _all_ evaluation functions */
void profilepath_initialize2(struct CurveMapping *cumap);

/* keep these (const CurveMap) - to help with thread safety */
/* single curve, no table check */
float profile_evaluateF(const struct CurveMap *cuma, float value);
/* single curve, with table check */
float profilepath_evaluateF(const struct CurveMapping *cumap, int cur, float value);
/* non-const, these modify the curve */
void profilepath_premultiply(struct CurveMapping *cumap, int restore);

#endif
