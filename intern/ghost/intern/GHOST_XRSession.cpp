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

#include <cassert>
#include <cstdio>
#include <cstring>

#include "GHOST_C-api.h"
#if defined(WITH_X11)
#  include "GHOST_ContextGLX.h"
#elif defined(WIN32)
#  include "GHOST_ContextWGL.h"
#  include "GHOST_ContextD3D.h"
#endif

#include "GHOST_XR_intern.h"

/** \file
 * \ingroup wm
 */

GHOST_TSuccess GHOST_XR_session_is_running(const GHOST_XRContext *xr_context)
{
  if ((xr_context == nullptr) || (xr_context->oxr.session == XR_NULL_HANDLE)) {
    return GHOST_kFailure;
  }
  switch (xr_context->oxr.session_state) {
    case XR_SESSION_STATE_RUNNING:
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED:
      return GHOST_kSuccess;
    default:
      return GHOST_kFailure;
  }
}

/**
 * A system in OpenXR the combination of some sort of HMD plus controllers and whatever other
 * devices are managed through OpenXR. So this attempts to init the HMD and the other devices.
 */
static void GHOST_XR_system_init(OpenXRData *oxr)
{
  assert(oxr->instance != XR_NULL_HANDLE);
  assert(oxr->system_id == XR_NULL_SYSTEM_ID);

  XrSystemGetInfo system_info{};
  system_info.type = XR_TYPE_SYSTEM_GET_INFO;
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  xrGetSystem(oxr->instance, &system_info, &oxr->system_id);
}

void GHOST_XRGraphicsBinding::initFromGhostContext(GHOST_TGraphicsBinding type,
                                                   GHOST_Context *ghost_ctx)
{
  switch (type) {
    case GHOST_kXRGraphicsOpenGL: {
#if defined(WITH_X11)
      GHOST_ContextGLX *ctx_glx = static_cast<GHOST_ContextGLX *>(ghost_ctx);
      XVisualInfo *visual_info = glXGetVisualFromFBConfig(ctx_glx->m_display, ctx_glx->m_fbconfig);

      oxr_binding.glx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
      oxr_binding.glx.xDisplay = ctx_glx->m_display;
      oxr_binding.glx.glxFBConfig = ctx_glx->m_fbconfig;
      oxr_binding.glx.glxDrawable = ctx_glx->m_window;
      oxr_binding.glx.glxContext = ctx_glx->m_context;
      oxr_binding.glx.visualid = visual_info->visualid;
#elif defined(WIN32)
      GHOST_ContextWGL *ctx_wgl = static_cast<GHOST_ContextWGL *>(ghost_ctx);

      oxr_binding.wgl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
      oxr_binding.wgl.hDC = ctx_wgl->m_hDC;
      oxr_binding.wgl.hGLRC = ctx_wgl->m_hGLRC;
#endif

      break;
    }
#ifdef WIN32
    case GHOST_kXRGraphicsD3D11: {
      GHOST_ContextD3D *ctx_d3d = static_cast<GHOST_ContextD3D *>(ghost_ctx);

      oxr_binding.d3d11.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
      oxr_binding.d3d11.device = ctx_d3d->m_device.Get();

      break;
    }
#endif
    default:
      assert(false);
  }
}

void GHOST_XR_session_start(GHOST_XRContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  assert(oxr->instance != XR_NULL_HANDLE);
  assert(oxr->session == XR_NULL_HANDLE);
  if (xr_context->gpu_ctx_bind_fn == nullptr) {
    fprintf(stderr,
            "Invalid API usage: No way to bind graphics context to the XR session. Call "
            "GHOST_XR_graphics_context_bind_funcs() with valid parameters before starting the "
            "session (through GHOST_XR_session_start()).");
    return;
  }

  GHOST_XR_system_init(oxr);

  GHOST_XR_graphics_context_bind(*xr_context);
  if (xr_context->gpu_ctx == nullptr) {
    fprintf(
        stderr,
        "Invalid API usage: No graphics context returned through the callback set with "
        "GHOST_XR_graphics_context_bind_funcs(). This is required for session starting (through "
        "GHOST_XR_session_start()).\n");
    return;
  }
  xr_context->gpu_binding = new GHOST_XRGraphicsBinding();
  xr_context->gpu_binding->initFromGhostContext(xr_context->gpu_binding_type, xr_context->gpu_ctx);

  XrSessionCreateInfo create_info{};
  create_info.type = XR_TYPE_SESSION_CREATE_INFO;
  create_info.systemId = oxr->system_id;
  create_info.next = &xr_context->gpu_binding->oxr_binding;

  xrCreateSession(oxr->instance, &create_info, &oxr->session);
}

void GHOST_XR_session_end(GHOST_XRContext *xr_context)
{
  xrEndSession(xr_context->oxr.session);
  GHOST_XR_graphics_context_unbind(*xr_context);
  delete xr_context->gpu_binding;
}

void GHOST_XR_session_state_change(OpenXRData *oxr,
                                   const XrEventDataSessionStateChanged &lifecycle)
{
  oxr->session_state = lifecycle.state;

  switch (lifecycle.state) {
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo begin_info{};

      begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
      begin_info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
      xrBeginSession(oxr->session, &begin_info);
      break;
    }
    case XR_SESSION_STATE_STOPPING: {
      assert(oxr->session != XR_NULL_HANDLE);
      xrEndSession(oxr->session);
    }
    default:
      break;
  }
}
