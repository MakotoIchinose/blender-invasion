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
 * \ingroup wm
 */

#ifndef __WM_XR_INTERN_H__
#define __WM_XR_INTERN_H__

typedef struct OpenXRData {
  XrInstance instance;

  XrExtensionProperties *extensions;
  uint32_t extension_count;

  struct XrApiLayerProperties *layers;
  uint32_t layer_count;

  XrSystemId system_id;
  XrSession session;
  XrSessionState session_state;
} OpenXRData;

typedef struct wmXRContext {
  OpenXRData oxr;

  /** Active graphics binding type. */
  eWM_xrGraphicsBinding gpu_binding;
  /** Function to retrieve (possibly create) a graphics context */
  wmXRGraphicsContextBindFn gpu_ctx_bind_fn;
  /** Function to release (possibly free) a graphics context */
  wmXRGraphicsContextUnbindFn gpu_ctx_unbind_fn;
  /** Active Ghost graphic context. */
  void *gpu_ctx;

  /** Names of enabled extensions */
  const char **enabled_extensions;
} wmXRContext;

void wm_xr_graphics_context_bind(wmXRContext *xr_context);
void wm_xr_graphics_context_unbind(wmXRContext *xr_context);

void wm_xr_session_state_change(OpenXRData *oxr, const XrEventDataSessionStateChanged *lifecycle);

#endif /* __WM_XR_INTERN_H__ */
