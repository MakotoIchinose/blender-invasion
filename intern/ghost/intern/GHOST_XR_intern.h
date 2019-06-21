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

#include <vector>

#include "GHOST_XR_openxr_includes.h"

typedef struct OpenXRData {
  XrInstance instance;

  std::vector<XrExtensionProperties> extensions;
  std::vector<XrApiLayerProperties> layers;

  XrSystemId system_id;
  XrSession session;
  XrSessionState session_state;
} OpenXRData;

class GHOST_XrGraphicsBinding {
 public:
  union {
#if defined(WITH_X11)
    XrGraphicsBindingOpenGLXlibKHR glx;
#elif defined(WIN32)
    XrGraphicsBindingOpenGLWin32KHR wgl;
    XrGraphicsBindingD3D11KHR d3d11;
#endif
  } oxr_binding;

  void initFromGhostContext(GHOST_TXrGraphicsBinding type, class GHOST_Context *ghost_ctx);
};

typedef struct GHOST_XrContext {
  OpenXRData oxr;

  /** Active graphics binding type. */
  GHOST_TXrGraphicsBinding gpu_binding_type;
  class GHOST_XrGraphicsBinding *gpu_binding;
  /** Function to retrieve (possibly create) a graphics context */
  GHOST_XrGraphicsContextBindFn gpu_ctx_bind_fn;
  /** Function to release (possibly free) a graphics context */
  GHOST_XrGraphicsContextUnbindFn gpu_ctx_unbind_fn;
  /** Active Ghost graphic context. */
  class GHOST_Context *gpu_ctx;

  /** Names of enabled extensions */
  std::vector<const char *> enabled_extensions;
} GHOST_XrContext;

void GHOST_XrGraphicsContextBind(GHOST_XrContext &xr_context);
void GHOST_XrGraphicsContextUnbind(GHOST_XrContext &xr_context);

void GHOST_XrSessionStateChange(OpenXRData *oxr, const XrEventDataSessionStateChanged &lifecycle);

#endif /* __GHOST_XR_INTERN_H__ */
