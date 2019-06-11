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

#ifndef __WM_XR_H__
#define __WM_XR_H__

#if !defined(WITH_OPENXR)
static_assert(false, "WITH_OPENXR not defined, but wm_xr.c is being compiled.");
#endif

/**
 * The XR view (i.e. the OpenXR runtime) may require a different graphics library than OpenGL. An
 * offscreen texture of the viewport will then be drawn into using OpenGL, but the final texture
 * draw call will happen through another lib (say DirectX).
 *
 * This enum defines the possible graphics bindings to attempt to enable.
 */
typedef enum {
  WM_XR_GRAPHICS_UNKNOWN = 0,
  WM_XR_GRAPHICS_OPENGL,
#ifdef WIN32
  WM_XR_GRAPHICS_D3D11,
#endif
  /* For later */
  //  WM_XR_GRAPHICS_VULKAN,
} eWM_xrGraphicsBinding;

/* An array of eWM_xrGraphicsBinding items defining the candidate bindings to use. The first
 * available candidate will be chosen, so order defines priority. */
typedef const eWM_xrGraphicsBinding *wmXRGraphicsBindingCandidates;

typedef struct {
  const wmXRGraphicsBindingCandidates gpu_binding_candidates;
  unsigned int gpu_binding_candidates_count;
} wmXRContextCreateInfo;

/* xr-context */
struct wmXRContext *wm_xr_context_create(const wmXRContextCreateInfo *create_info)
    ATTR_WARN_UNUSED_RESULT ATTR_NONNULL();
void wm_xr_context_destroy(struct wmXRContext *xr_context);

/* sessions */
bool wm_xr_session_is_running(const struct wmXRContext *xr_context) ATTR_WARN_UNUSED_RESULT;
eWM_xrGraphicsBinding wm_xr_session_active_graphics_binding_type_get(
    const struct wmXRContext *xr_context) ATTR_NONNULL();
void wm_xr_session_start(struct wmXRContext *xr_context, void *ghost_context) ATTR_NONNULL();
void wm_xr_session_end(struct wmXRContext *xr_context) ATTR_NONNULL();

/* events */
bool wm_xr_events_handle(struct wmXRContext *xr_context) ATTR_NONNULL();

#endif /* __WM_XR_H__ */
