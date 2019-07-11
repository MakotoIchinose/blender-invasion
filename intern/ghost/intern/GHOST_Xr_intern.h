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
 * \ingroup GHOST
 */

#ifndef __GHOST_XR_INTERN_H__
#define __GHOST_XR_INTERN_H__

#include <memory>
#include <vector>

#include "GHOST_Xr_openxr_includes.h"

#define XR_DEBUG_ONLY_BEGIN(ctx) \
  if ((ctx)->isDebugMode()) { \
    (void)0
#define XR_DEBUG_ONLY_END \
  } \
  (void)0

#define XR_DEBUG_PRINTF(ctx, ...) \
  if ((ctx)->isDebugMode()) { \
    printf(__VA_ARGS__); \
  } \
  (void)0

#define XR_DEBUG_ONLY_CALL(ctx, call) \
  if ((ctx)->isDebugMode()) { \
    call; \
  } \
  (void)0

#endif /* __GHOST_XR_INTERN_H__ */
