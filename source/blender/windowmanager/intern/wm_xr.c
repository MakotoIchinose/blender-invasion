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
#include "BKE_report.h"
#include "BKE_screen.h"

#include "BLI_math_geom.h"
#include "BLI_math_matrix.h"
#include "BLI_threads.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_debug.h"
#include "DEG_depsgraph_query.h"

#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_view3d_types.h"

#include "DRW_engine.h"

#include "ED_view3d.h"

#include "GHOST_C-api.h"

#include "GPU_context.h"
#include "GPU_draw.h"
#include "GPU_matrix.h"
#include "GPU_viewport.h"

#include "MEM_guardedalloc.h"

#include "UI_interface.h"

#include "WM_types.h"
#include "WM_api.h"

#include "wm.h"
#include "wm_surface.h"
#include "wm_window.h"

#ifdef WIN32 /* Only WIN32 supported now */
//#  define USE_FORCE_WINDOWED_SESSION
#endif

#ifdef USE_FORCE_WINDOWED_SESSION
static void xr_session_window_create(bContext *C);
#endif

static wmSurface *g_xr_surface = NULL;
static Depsgraph *g_depsgraph = NULL;
ListBase g_threadpool;

typedef struct {
  GHOST_TXrGraphicsBinding gpu_binding_type;
  GPUOffScreen *offscreen;
  GPUViewport *viewport;
} wmXrSurfaceData;

typedef struct {
  wmWindowManager *wm;
  bContext *evil_C;
} wmXrErrorHandlerData;

static void wm_xr_error_handler(const GHOST_XrError *error)
{
  wmXrErrorHandlerData *handler_data = error->customdata;
  wmWindowManager *wm = handler_data->wm;

  BKE_reports_clear(&wm->reports);
  WM_report(RPT_ERROR, error->user_message);
  WM_report_banner_show();
  UI_popup_menu_reports(handler_data->evil_C, &wm->reports);

  if (wm->xr_context) {
    /* Just play safe and destroy the entire context. */
    GHOST_XrContextDestroy(wm->xr_context);
    wm->xr_context = NULL;
  }
}

bool wm_xr_context_ensure(bContext *C, wmWindowManager *wm)
{
  if (wm->xr_context) {
    return true;
  }
  static wmXrErrorHandlerData error_customdata;

  /* Set up error handling */
  error_customdata.wm = wm;
  error_customdata.evil_C = C;
  GHOST_XrErrorHandler(wm_xr_error_handler, &error_customdata);

  {
    const GHOST_TXrGraphicsBinding gpu_bindings_candidates[] = {
        GHOST_kXrGraphicsOpenGL,
#ifdef WIN32
        GHOST_kXrGraphicsD3D11,
#endif
    };
    GHOST_XrContextCreateInfo create_info = {
        .gpu_binding_candidates = gpu_bindings_candidates,
        .gpu_binding_candidates_count = ARRAY_SIZE(gpu_bindings_candidates)};

    if (G.debug & G_DEBUG_XR) {
      create_info.context_flag |= GHOST_kXrContextDebug;
    }
    if (G.debug & G_DEBUG_XR_TIME) {
      create_info.context_flag |= GHOST_kXrContextDebugTime;
    }

    wm->xr_context = GHOST_XrContextCreate(&create_info);
  }

  return wm->xr_context != NULL;
}

static void wm_xr_session_surface_draw(bContext *C)
{
#if 0
  wmWindowManager *wm = CTX_wm_manager(C);

  if (!GHOST_XrSessionIsRunning(wm->xr_context)) {
    return;
  }
  GHOST_XrSessionDrawViews(wm->xr_context, C);
#endif
}

static void wm_xr_session_free_data(wmSurface *surface)
{
  wmXrSurfaceData *data = surface->customdata;

  if (surface->secondary_ghost_ctx) {
#ifdef WIN32
    if (data->gpu_binding_type == GHOST_kXrGraphicsD3D11) {
      WM_directx_context_dispose(surface->secondary_ghost_ctx);
    }
#endif
  }
  WM_opengl_context_activate(surface->ghost_ctx);
  GPU_context_active_set(surface->gpu_ctx);

  if (data->viewport) {
    GPU_viewport_clear_from_offscreen(data->viewport);
    GPU_viewport_free(data->viewport);
  }
  if (data->offscreen) {
    GPU_offscreen_free(data->offscreen);
  }
  GPU_context_discard(g_xr_surface->gpu_ctx);
  GPU_context_active_set(NULL);
  WM_opengl_context_release(surface->ghost_ctx);
  WM_opengl_context_dispose(surface->ghost_ctx);

  DEG_graph_free(g_depsgraph);

  MEM_freeN(surface->customdata);

  g_xr_surface = NULL;
}

static wmSurface *wm_xr_session_surface_create(wmWindowManager *UNUSED(wm),
                                               unsigned int gpu_binding_type)
{
  if (g_xr_surface) {
    BLI_assert(false);
    return g_xr_surface;
  }

  wmSurface *surface = MEM_callocN(sizeof(*surface), __func__);
  wmXrSurfaceData *data = MEM_callocN(sizeof(*data), "XrSurfaceData");

#ifndef WIN32
  BLI_assert(gpu_binding_type == GHOST_kXrGraphicsOpenGL);
#endif

  surface->draw = wm_xr_session_surface_draw;
  surface->free_data = wm_xr_session_free_data;

  data->gpu_binding_type = gpu_binding_type;
  surface->customdata = data;

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

  WM_opengl_context_release(surface->ghost_ctx);
  GPU_context_active_set(NULL);
  wm_window_reset_drawable();

  BLI_threadpool_end(&g_threadpool);

  g_xr_surface = surface;

  return surface;
}

static void wm_xr_draw_matrices_create(const Scene *scene,
                                       const GHOST_XrDrawViewInfo *draw_view,
                                       const float clip_start,
                                       const float clip_end,
                                       float r_view_mat[4][4],
                                       float r_proj_mat[4][4])
{
  float scalemat[4][4], quat[4];
  float temp[4][4];

  perspective_m4_fov(r_proj_mat,
                     draw_view->fov.angle_left,
                     draw_view->fov.angle_right,
                     draw_view->fov.angle_up,
                     draw_view->fov.angle_down,
                     clip_start,
                     clip_end);

  scale_m4_fl(scalemat, 1.0f);
  invert_qt_qt_normalized(quat, draw_view->pose.orientation_quat);
  quat_to_mat4(temp, quat);
  translate_m4(temp,
               -draw_view->pose.position[0],
               -draw_view->pose.position[1],
               -draw_view->pose.position[2]);

  if (scene->camera) {
    invert_m4_m4(scene->camera->imat, scene->camera->obmat);
    mul_m4_m4m4(r_view_mat, temp, scene->camera->imat);
  }
  else {
    copy_m4_m4(r_view_mat, temp);
  }
}

static bool wm_xr_session_surface_offscreen_ensure(const GHOST_XrDrawViewInfo *draw_view)
{
  wmXrSurfaceData *surface_data = g_xr_surface->customdata;
  char err_out[256] = "unknown";
  bool failure = false;

  if (surface_data->offscreen &&
      (GPU_offscreen_width(surface_data->offscreen) == draw_view->width) &&
      (GPU_offscreen_height(surface_data->offscreen) == draw_view->height)) {
    BLI_assert(surface_data->viewport);
    return true;
  }

  if (surface_data->offscreen) {
    GPU_viewport_clear_from_offscreen(surface_data->viewport);
    GPU_viewport_free(surface_data->viewport);
    GPU_offscreen_free(surface_data->offscreen);
  }

  if (!(surface_data->offscreen = GPU_offscreen_create(
            draw_view->width, draw_view->height, 0, true, false, err_out))) {
    failure = true;
  }
  if ((failure == false) &&
      !(surface_data->viewport = GPU_viewport_create_from_offscreen(surface_data->offscreen))) {
    GPU_offscreen_free(surface_data->offscreen);
    failure = true;
  }

  if (failure) {
    fprintf(stderr, "%s: failed to get buffer, %s\n", __func__, err_out);
    return false;
  }

  return true;
}

static GHOST_ContextHandle wm_xr_draw_view(const GHOST_XrDrawViewInfo *draw_view, void *customdata)
{
  bContext *C = customdata;
  wmXrSurfaceData *surface_data = g_xr_surface->customdata;
  const float clip_start = 0.01, clip_end = 500.0f;
  const float lens = 50.0f; /* TODO get from OpenXR */
  const rcti rect = {
      .xmin = 0, .ymin = 0, .xmax = draw_view->width - 1, .ymax = draw_view->height - 1};

  GPUOffScreen *offscreen;
  GPUViewport *viewport;
  View3DShading shading;
  float viewmat[4][4], winmat[4][4];

  wm_xr_draw_matrices_create(CTX_data_scene(C), draw_view, clip_start, clip_end, viewmat, winmat);

  DRW_opengl_render_context_enable(g_xr_surface->ghost_ctx);
  DRW_gawain_render_context_enable(g_xr_surface->gpu_ctx);

  DEG_graph_relations_update(
      g_depsgraph, CTX_data_main(C), CTX_data_scene(C), CTX_data_view_layer(C));
  DEG_evaluate_on_refresh(g_depsgraph);

  if (!wm_xr_session_surface_offscreen_ensure(draw_view)) {
    // TODO disable correctly.
    return NULL;
  }

  offscreen = surface_data->offscreen;
  viewport = surface_data->viewport;
  GPU_viewport_bind(viewport, &rect);
  glClear(GL_DEPTH_BUFFER_BIT);

  BKE_screen_view3d_shading_init(&shading);
  shading.flag |= V3D_SHADING_WORLD_ORIENTATION;
  shading.background_type = V3D_SHADING_BACKGROUND_WORLD;
  ED_view3d_draw_offscreen_simple(g_depsgraph,
                                  DEG_get_evaluated_scene(g_depsgraph),
                                  &shading,
                                  OB_SOLID,
                                  draw_view->width,
                                  draw_view->height,
                                  /* Draw floor for better orientation */
                                  V3D_OFSDRAW_OVERRIDE_SCENE_SETTINGS | V3D_OFSDRAW_SHOW_GRIDFLOOR,
                                  viewmat,
                                  winmat,
                                  clip_start,
                                  clip_end,
                                  lens,
                                  true,
                                  true,
                                  NULL,
                                  false,
                                  offscreen,
                                  viewport);

  GPU_framebuffer_restore();

  GPUTexture *texture = GPU_offscreen_color_texture(offscreen);

  wm_draw_offscreen_texture_parameters(offscreen);
  GPU_depth_test(false);

  GPU_matrix_push();
  GPU_matrix_push_projection();
  wmViewport(&rect);

  if (g_xr_surface->secondary_ghost_ctx &&
      GHOST_isUpsideDownContext(g_xr_surface->secondary_ghost_ctx)) {
    GPU_texture_bind(texture, 0);
    wm_draw_upside_down(draw_view->width, draw_view->height);
    GPU_texture_unbind(texture);
  }
  else {
    GPU_viewport_draw_to_screen(viewport, &rect);
  }

  GPU_matrix_pop_projection();
  GPU_matrix_pop();
  GPU_viewport_unbind(viewport);

  DRW_gawain_render_context_disable(g_xr_surface->gpu_ctx);
  DRW_opengl_render_context_disable(g_xr_surface->ghost_ctx);

  return g_xr_surface->ghost_ctx;
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

  wm_window_reset_drawable();
}

static void *wm_xr_session_drawthread_main(void *data)
{
  bContext *C = data;
  wmWindowManager *wm = CTX_wm_manager(C);
  Main *bmain = CTX_data_main(C);
  Scene *scene = CTX_data_scene(C);

  g_depsgraph = DEG_graph_new(scene, CTX_data_view_layer(C), DAG_EVAL_VIEWPORT);
  DEG_debug_name_set(g_depsgraph, "VR SESSION");
  DEG_graph_build_from_view_layer(g_depsgraph, bmain, scene, CTX_data_view_layer(C));
  //  DEG_evaluate_on_framechange(bmain, g_depsgraph, CFRA);
  DEG_graph_tag_relations_update(g_depsgraph);

  WM_opengl_context_activate(g_xr_surface->ghost_ctx);
  g_xr_surface->gpu_ctx = GPU_context_create(
      GHOST_GetContextDefaultOpenGLFramebuffer(g_xr_surface->ghost_ctx));
  WM_opengl_context_release(g_xr_surface->ghost_ctx);

  /* Sort of the session's main loop. */
  while (GHOST_XrHasSession(wm->xr_context)) {
    if (!GHOST_XrSessionIsRunning(wm->xr_context)) {
      continue;
    }

    GHOST_XrSessionDrawViews(wm->xr_context, C);
  }

  return NULL;
}

static void wm_xr_session_drawthread_spawn(bContext *C)
{
  BLI_threadpool_init(&g_threadpool, wm_xr_session_drawthread_main, 1);
  BLI_threadpool_insert(&g_threadpool, C);
}

static void wm_xr_session_begin_info_create(const Scene *scene,
                                            GHOST_XrSessionBeginInfo *begin_info)
{
  if (scene->camera) {
    copy_v3_v3(begin_info->base_pose.position, scene->camera->loc);
    /* TODO will only work if rotmode is euler */
    eul_to_quat(begin_info->base_pose.orientation_quat, scene->camera->rot);
  }
  else {
    copy_v3_fl(begin_info->base_pose.position, 0.0f);
    unit_qt(begin_info->base_pose.orientation_quat);
  }
}

void wm_xr_session_toggle(bContext *C, void *xr_context_ptr)
{
  GHOST_XrContextHandle xr_context = xr_context_ptr;

  if (xr_context && GHOST_XrSessionIsRunning(xr_context)) {
    GHOST_XrSessionEnd(xr_context);
  }
  else {
    GHOST_XrSessionBeginInfo begin_info;

#if defined(USE_FORCE_WINDOWED_SESSION)
    xr_session_window_create(C);
#endif
    wm_xr_session_begin_info_create(CTX_data_scene(C), &begin_info);

    GHOST_XrGraphicsContextBindFuncs(xr_context,
                                     wm_xr_session_gpu_binding_context_create,
                                     wm_xr_session_gpu_binding_context_destroy);
    GHOST_XrDrawViewFunc(xr_context, wm_xr_draw_view);

    GHOST_XrSessionStart(xr_context, &begin_info);
    wm_xr_session_drawthread_spawn(C);
  }
}

#if defined(USE_FORCE_WINDOWED_SESSION)

#  include "BLI_rect.h"
#  include "DNA_workspace_types.h"
#  include "BKE_workspace.h"
#  include "ED_screen.h"
#  include "BLI_string.h"

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
