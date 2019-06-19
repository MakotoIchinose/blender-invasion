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

static void *openxr_graphics_binding_create(const GHOST_XRContext *xr_context,
                                            GHOST_Context *ghost_ctx)
{
  static union {
#if defined(WITH_X11)
    XrGraphicsBindingOpenGLXlibKHR glx;
#elif defined(WIN32)
    XrGraphicsBindingOpenGLWin32KHR wgl;
    XrGraphicsBindingD3D11KHR d3d11;
#endif
  } binding;

  memset(&binding, 0, sizeof(binding));

  switch (xr_context->gpu_binding) {
    case GHOST_kXRGraphicsOpenGL: {
#if defined(WITH_X11)
      binding.glx.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
#elif defined(WIN32)
      GHOST_ContextWGL *ctx_wgl = static_cast<GHOST_ContextWGL *>(ghost_ctx);
      GHOST_ContextWGL::Info info = ctx_wgl->getInfo();

      binding.wgl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
      binding.wgl.hDC = info.hDC;
      binding.wgl.hGLRC = info.hGLRC;
#endif

      break;
    }
#ifdef WIN32
    case GHOST_kXRGraphicsD3D11: {
      GHOST_ContextD3D *ctx_d3d = static_cast<GHOST_ContextD3D *>(ghost_ctx);

      binding.d3d11.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
      binding.d3d11.device = ctx_d3d->getDevice();

      break;
    }
#endif
    default:
      assert(false);
  }

  return &binding;
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

  XrSessionCreateInfo create_info{};
  create_info.type = XR_TYPE_SESSION_CREATE_INFO;
  create_info.systemId = oxr->system_id;
  create_info.next = openxr_graphics_binding_create(xr_context, xr_context->gpu_ctx);

  xrCreateSession(oxr->instance, &create_info, &oxr->session);
}

void GHOST_XR_session_end(GHOST_XRContext *xr_context)
{
  xrEndSession(xr_context->oxr.session);
  GHOST_XR_graphics_context_unbind(*xr_context);
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
