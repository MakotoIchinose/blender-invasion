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
 */

/** \file
 * \ingroup USD
 */

#ifndef __USD__DEBUG_TIMER_H__
#define __USD__DEBUG_TIMER_H__

// Temporary class for timing stuff.
#include <stdio.h>
#include <string>
#include <time.h>

class Timer {
  timespec ts_begin;
  std::string label;

 public:
  explicit Timer(std::string label) : label(label)
  {
    clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  }
  ~Timer()
  {
    timespec ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double duration = double(ts_end.tv_sec - ts_begin.tv_sec) +
                      double(ts_end.tv_nsec - ts_begin.tv_nsec) / 1e9;
    printf("%s took %.3f sec wallclock time\n", label.c_str(), duration);
  }
};

#endif /* __USD__DEBUG_TIMER_H__ */
