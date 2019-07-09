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
#include "GHOST_XrSession.h"

typedef struct OpenXRData {
  XrInstance instance;

  std::vector<XrExtensionProperties> extensions;
  std::vector<XrApiLayerProperties> layers;
} OpenXRData;

/**
 * \brief Main GHOST container to manage OpenXR through.
 *
 * Creating a context using #GHOST_XrContextCreate involves dynamically connecting to the OpenXR
 * runtime, likely reading the OS OpenXR configuration (i.e. active_runtime.json). So this is
 * something that should better be done using lazy-initialization.
 */
typedef struct GHOST_XrContext {
  OpenXRData oxr;

  /* The active GHOST XR Session. Null while no session runs. */
  std::unique_ptr<GHOST_XrSession> session;

  /** Active graphics binding type. */
  GHOST_TXrGraphicsBinding gpu_binding_type;
  /** Function to retrieve (possibly create) a graphics context */
  GHOST_XrGraphicsContextBindFn gpu_ctx_bind_fn;
  /** Function to release (possibly free) a graphics context */
  GHOST_XrGraphicsContextUnbindFn gpu_ctx_unbind_fn;

  /** Custom per-view draw function for Blender side drawing. */
  GHOST_XrDrawViewFn draw_view_fn;

  /** Names of enabled extensions */
  std::vector<const char *> enabled_extensions;
  /** Names of enabled API-layers */
  std::vector<const char *> enabled_layers;
} GHOST_XrContext;

void GHOST_XrSessionStateChange(GHOST_XrContext *xr_context,
                                const XrEventDataSessionStateChanged *lifecycle);

#endif /* __GHOST_XR_INTERN_H__ */
