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

#include "BKE_context.h"

#include "GHOST_C-api.h"

#include "GPU_context.h"
#include "GPU_draw.h"

#include "MEM_guardedalloc.h"

#include "WM_types.h"
#include "WM_api.h"

#include "wm.h"
#include "wm_window.h"
#include "wm_surface.h"

static wmSurface *global_xr_surface = NULL;

typedef struct {
  GHOST_TXrGraphicsBinding gpu_binding_type;
} XrSurfaceData;

bool wm_xr_context_ensure(wmWindowManager *wm)
{
  if (wm->xr_context) {
    return true;
  }

  const GHOST_TXrGraphicsBinding gpu_bindings_candidates[] = {
      GHOST_kXrGraphicsOpenGL,
#ifdef WIN32
      GHOST_kXrGraphicsD3D11,
#endif
  };
  const GHOST_XrContextCreateInfo create_info = {
      .gpu_binding_candidates = gpu_bindings_candidates,
      .gpu_binding_candidates_count = ARRAY_SIZE(gpu_bindings_candidates)};

  wm->xr_context = GHOST_XrContextCreate(&create_info);

  return wm->xr_context != NULL;
}

static void wm_xr_session_surface_draw(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);

  if (!GHOST_XrSessionIsRunning(wm->xr_context)) {
    return;
  }
  GHOST_XrSessionDrawViews(wm->xr_context, C);
}

static void wm_xr_session_free_data(wmSurface *surface)
{
  WM_opengl_context_dispose(surface->ghost_ctx);
  GPU_context_active_set(surface->gpu_ctx);
  GPU_context_discard(surface->gpu_ctx);
  MEM_freeN(surface->customdata);

  global_xr_surface = NULL;
}

wmSurface *wm_xr_session_surface_create(wmWindowManager *wm, unsigned int gpu_binding_type)
{
  if (global_xr_surface) {
    BLI_assert(false);
    return global_xr_surface;
  }

  wmSurface *surface = MEM_callocN(sizeof(*surface), __func__);
  XrSurfaceData *data = MEM_callocN(sizeof(*data), "XrSurfaceData");
  unsigned int default_fb;

#ifndef WIN32
  BLI_assert(gpu_binding_type == GHOST_kXrGraphicsOpenGL);
#endif

  surface->draw = wm_xr_session_surface_draw;
  surface->free_data = wm_xr_session_free_data;

  data->gpu_binding_type = gpu_binding_type;
  surface->customdata = data;

  wm_window_clear_drawable(wm);
  wm_surface_clear_drawable();
  surface->ghost_ctx = WM_opengl_context_create();
  WM_opengl_context_activate(surface->ghost_ctx);

  switch (gpu_binding_type) {
    case GHOST_kXrGraphicsOpenGL:
      break;
#ifdef WIN32
    case GHOST_kXrGraphicsD3D11:
      surface->secondary_ghost_ctx = WM_directx_context_create();
      break;
#endif
  }

  default_fb = GHOST_GetContextDefaultOpenGLFramebuffer(surface->ghost_ctx);
  surface->gpu_ctx = GPU_context_create(default_fb);

  wm_surface_set_drawable(surface, false);
  GHOST_SwapContextBuffers(surface->ghost_ctx);
  GPU_state_init();

  wm_surface_clear_drawable();

  global_xr_surface = surface;

  return surface;
}

wmSurface *wm_xr_session_surface_get(void)
{
  return global_xr_surface;
}
