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

#include <string.h>

#include "BLI_compiler_attrs.h"
#include "BLI_utildefines.h"

#include "GHOST_C-api.h"

#include "MEM_guardedalloc.h"

#include "wm_xr_openxr_includes.h"

#include "wm_xr.h"
#include "wm_xr_intern.h"

/** \file
 * \ingroup wm
 */

bool wm_xr_session_is_running(const wmXRContext *xr_context)
{
  const OpenXRData *oxr = &xr_context->oxr;

  if (oxr->session == XR_NULL_HANDLE) {
    return false;
  }

  return ELEM(oxr->session_state,
              XR_SESSION_STATE_RUNNING,
              XR_SESSION_STATE_VISIBLE,
              XR_SESSION_STATE_FOCUSED);
}

/**
 * A system in OpenXR the combination of some sort of HMD plus controllers and whatever other
 * devices are managed through OpenXR. So this attempts to init the HMD and the other devices.
 */
static void wm_xr_system_init(OpenXRData *oxr)
{
  BLI_assert(oxr->instance != XR_NULL_HANDLE);
  BLI_assert(oxr->system_id == XR_NULL_SYSTEM_ID);

  XrSystemGetInfo system_info = {.type = XR_TYPE_SYSTEM_GET_INFO};
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  xrGetSystem(oxr->instance, &system_info, &oxr->system_id);
}

eWM_xrGraphicsBinding wm_xr_session_active_graphics_binding_type_get(const wmXRContext *xr_context)
{
  return xr_context->gpu_binding;
}

static void *openxr_graphics_binding_create(const wmXRContext *xr_context,
                                            GHOST_ContextHandle UNUSED(ghost_context))
{
  void *binding_ptr = NULL;

  switch (xr_context->gpu_binding) {
    case WM_XR_GRAPHICS_OPENGL: {
#if defined(WITH_X11)
      XrGraphicsBindingOpenGLXlibKHR binding = {.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};

#elif defined(WIN32)
      XrGraphicsBindingOpenGLWin32KHR binding = {
          .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};

#endif

      binding_ptr = MEM_mallocN(sizeof(binding), __func__);
      memcpy(binding_ptr, &binding, sizeof(binding));

      break;
    }
#ifdef WIN32
    case WM_XR_GRAPHICS_D3D11: {
      XrGraphicsBindingD3D11KHR binding = {.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};

      binding_ptr = MEM_mallocN(sizeof(binding), __func__);
      memcpy(binding_ptr, &binding, sizeof(binding));

      break;
    }
#endif
    default:
      BLI_assert(false);
  }

  return binding_ptr;
}

void wm_xr_session_start(wmXRContext *xr_context, void *ghost_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  BLI_assert(oxr->instance != XR_NULL_HANDLE);
  BLI_assert(oxr->session == XR_NULL_HANDLE);

  wm_xr_system_init(oxr);

  XrSessionCreateInfo create_info = {.type = XR_TYPE_SESSION_CREATE_INFO};
  create_info.systemId = oxr->system_id;
  create_info.next = openxr_graphics_binding_create(xr_context, ghost_context);

  xrCreateSession(oxr->instance, &create_info, &oxr->session);
}

void wm_xr_session_end(wmXRContext *xr_context)
{
  xrEndSession(xr_context->oxr.session);
}

void wm_xr_session_state_change(OpenXRData *oxr, const XrEventDataSessionStateChanged *lifecycle)
{
  oxr->session_state = lifecycle->state;

  switch (lifecycle->state) {
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo begin_info = {
          .type = XR_TYPE_SESSION_BEGIN_INFO,
          .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
      xrBeginSession(oxr->session, &begin_info);
      break;
    }
    case XR_SESSION_STATE_STOPPING: {
      BLI_assert(oxr->session != XR_NULL_HANDLE);
      xrEndSession(oxr->session);
    }
    default:
      break;
  }
}
