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
#include "BKE_global.h"
#include "BKE_main.h"

#include "GHOST_C-api.h"

#include "GPU_context.h"
#include "GPU_draw.h"

#include "MEM_guardedalloc.h"

#include "WM_types.h"
#include "WM_api.h"

#include "wm.h"
#include "wm_surface.h"
#include "wm_window.h"

#ifdef WIN32 /* Only WIN32 supported now */
//#  define USE_FORCE_WINDOWED_SESSION
#endif

static wmSurface *g_xr_surface = NULL;

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

  g_xr_surface = NULL;
}

static wmSurface *wm_xr_session_surface_create(wmWindowManager *wm, unsigned int gpu_binding_type)
{
  if (g_xr_surface) {
    BLI_assert(false);
    return g_xr_surface;
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

  g_xr_surface = surface;

  return surface;
}

static void wm_xr_draw_view(const GHOST_XrDrawViewInfo *UNUSED(draw_view), void *customdata)
{
}

static void *wm_xr_session_gpu_binding_context_create(GHOST_TXrGraphicsBinding graphics_binding)
{
#ifndef USE_FORCE_WINDOWED_SESSION
  wmSurface *surface = wm_xr_session_surface_create(G_MAIN->wm.first, graphics_binding);

  wm_surface_add(surface);

  return surface->secondary_ghost_ctx ? surface->secondary_ghost_ctx : surface->ghost_ctx;
#else
#  ifdef WIN32
  if (graphics_binding == GHOST_kXrGraphicsD3D11) {
    wmWindowManager *wm = G_MAIN->wm.first;
    for (wmWindow *win = wm->windows.first; win; win = win->next) {
      /* TODO better lookup? For now only one D3D window possible, but later? */
      if (GHOST_GetDrawingContextType(win->ghostwin) == GHOST_kDrawingContextTypeD3D) {
        return GHOST_GetWindowContext(win->ghostwin);
      }
    }
  }
#  endif
  return NULL;
#endif
}

static void wm_xr_session_gpu_binding_context_destroy(
    GHOST_TXrGraphicsBinding UNUSED(graphics_lib), void *UNUSED(context))
{
#ifndef USE_FORCE_WINDOWED_SESSION
  if (g_xr_surface) { /* Might have been freed already */
    wm_surface_remove(g_xr_surface);
  }
#endif
}

void wm_xr_session_toggle(struct GHOST_XrContext *xr_context)
{
  if (GHOST_XrSessionIsRunning(xr_context)) {
    GHOST_XrSessionEnd(xr_context);
  }
  else {
#if defined(USE_FORCE_WINDOWED_SESSION)
    xr_session_window_create(C);
#endif

    GHOST_XrGraphicsContextBindFuncs(xr_context,
                                     wm_xr_session_gpu_binding_context_create,
                                     wm_xr_session_gpu_binding_context_destroy);
    GHOST_XrDrawViewFunc(xr_context, wm_xr_draw_view);

    GHOST_XrSessionStart(xr_context);
    GHOST_XrSessionRenderingPrepare(xr_context);
  }
}

#if defined(USE_FORCE_WINDOWED_SESSION)
static void xr_session_window_create(bContext *C)
{
  Main *bmain = CTX_data_main(C);
  Scene *scene = CTX_data_scene(C);
  ViewLayer *view_layer = CTX_data_view_layer(C);
  wmWindow *win_prev = CTX_wm_window(C);

  rcti rect;
  wmWindow *win;
  bScreen *screen;
  ScrArea *sa;
  int screen_size[2];

  wm_get_screensize(&screen_size[0], &screen_size[1]);
  BLI_rcti_init(&rect, 0, screen_size[0], 0, screen_size[1]);
  BLI_rcti_scale(&rect, 0.8f);
  wm_window_check_position(&rect);

  win = WM_window_open_directx(C, &rect);

  if (WM_window_get_active_workspace(win) == NULL) {
    WorkSpace *workspace = WM_window_get_active_workspace(win_prev);
    BKE_workspace_active_set(win->workspace_hook, workspace);
  }

  /* add new screen layout */
  WorkSpace *workspace = WM_window_get_active_workspace(win);
  WorkSpaceLayout *layout = ED_workspace_layout_add(bmain, workspace, win, "VR Session");

  screen = BKE_workspace_layout_screen_get(layout);
  WM_window_set_active_layout(win, workspace, layout);

  /* Set scene and view layer to match original window. */
  STRNCPY(win->view_layer_name, view_layer->name);
  if (WM_window_get_active_scene(win) != scene) {
    ED_screen_scene_change(C, win, scene);
  }

  WM_check(C);

  CTX_wm_window_set(C, win);
  sa = screen->areabase.first;
  CTX_wm_area_set(C, sa);
  ED_area_newspace(C, sa, SPACE_VIEW3D, false);

  ED_screen_change(C, screen);
  ED_screen_refresh(CTX_wm_manager(C), win); /* test scale */

  if (win->ghostwin) {
    GHOST_SetTitle(win->ghostwin, "Blender VR Session View");
  }
  else {
    /* very unlikely! but opening a new window can fail */
    wm_window_close(C, CTX_wm_manager(C), win);
    CTX_wm_window_set(C, win_prev);
  }
}
#endif /* USE_FORCE_WINDOWED_SESSION */
