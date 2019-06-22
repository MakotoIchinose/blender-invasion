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

#include <algorithm>
#include <cassert>
#include <cstdio>

#include "GHOST_C-api.h"

#include "GHOST_IXRGraphicsBinding.h"

#include "GHOST_XR_intern.h"

GHOST_TSuccess GHOST_XrSessionIsRunning(const GHOST_XrContext *xr_context)
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
static void GHOST_XrSystemInit(OpenXRData *oxr)
{
  assert(oxr->instance != XR_NULL_HANDLE);
  assert(oxr->system_id == XR_NULL_SYSTEM_ID);

  XrSystemGetInfo system_info{};
  system_info.type = XR_TYPE_SYSTEM_GET_INFO;
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  xrGetSystem(oxr->instance, &system_info, &oxr->system_id);
}

void GHOST_XrSessionStart(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  assert(oxr->instance != XR_NULL_HANDLE);
  assert(oxr->session == XR_NULL_HANDLE);
  if (xr_context->gpu_ctx_bind_fn == nullptr) {
    fprintf(stderr,
            "Invalid API usage: No way to bind graphics context to the XR session. Call "
            "GHOST_XrGraphicsContextBindFuncs() with valid parameters before starting the "
            "session (through GHOST_XrSessionStart()).");
    return;
  }

  GHOST_XrSystemInit(oxr);

  GHOST_XrGraphicsContextBind(*xr_context);
  if (xr_context->gpu_ctx == nullptr) {
    fprintf(stderr,
            "Invalid API usage: No graphics context returned through the callback set with "
            "GHOST_XrGraphicsContextBindFuncs(). This is required for session starting (through "
            "GHOST_XrSessionStart()).\n");
    return;
  }
  xr_context->gpu_binding = GHOST_XrGraphicsBindingCreateFromType(xr_context->gpu_binding_type);
  xr_context->gpu_binding->initFromGhostContext(xr_context->gpu_ctx);

  XrSessionCreateInfo create_info{};
  create_info.type = XR_TYPE_SESSION_CREATE_INFO;
  create_info.systemId = oxr->system_id;
  create_info.next = &xr_context->gpu_binding->oxr_binding;

  xrCreateSession(oxr->instance, &create_info, &oxr->session);
}

void GHOST_XrSessionEnd(GHOST_XrContext *xr_context)
{
  xrEndSession(xr_context->oxr.session);
  GHOST_XrGraphicsContextUnbind(*xr_context);
}

void GHOST_XrSessionStateChange(OpenXRData *oxr, const XrEventDataSessionStateChanged &lifecycle)
{
  oxr->session_state = lifecycle.state;

  switch (lifecycle.state) {
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo begin_info{};

      begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
      begin_info.primaryViewConfigurationType = oxr->view_type;
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

static bool swapchain_create(const GHOST_XrContext *xr_context,
                             OpenXRData *oxr,
                             const XrViewConfigurationView *xr_view)
{
  XrSwapchainCreateInfo create_info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  XrSwapchain swapchain;
  uint32_t format_count = 0;
  int64_t chosen_format;

  xrEnumerateSwapchainFormats(oxr->session, 0, &format_count, nullptr);
  std::vector<int64_t> swapchain_formats(format_count);
  xrEnumerateSwapchainFormats(
      oxr->session, swapchain_formats.size(), &format_count, swapchain_formats.data());
  assert(swapchain_formats.size() == format_count);

  if (!xr_context->gpu_binding->chooseSwapchainFormat(swapchain_formats, &chosen_format)) {
    fprintf(stderr, "Error: No format matching OpenXR runtime supported swapchain formats found.");
    return false;
  }

  create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                           XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
  create_info.format = chosen_format;
  create_info.sampleCount = xr_view->recommendedSwapchainSampleCount;
  create_info.width = xr_view->recommendedImageRectWidth;
  create_info.height = xr_view->recommendedImageRectHeight;
  create_info.faceCount = 1;
  create_info.arraySize = 1;
  create_info.mipCount = 1;
  xrCreateSwapchain(oxr->session, &create_info, &swapchain);

  return true;
}

void GHOST_XrSessionRenderingPrepare(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  std::vector<XrViewConfigurationView> views;
  uint32_t view_count;

  xrEnumerateViewConfigurationViews(
      oxr->instance, oxr->system_id, oxr->view_type, 0, &view_count, nullptr);
  views.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(
      oxr->instance, oxr->system_id, oxr->view_type, views.size(), &view_count, views.data());

  for (const XrViewConfigurationView &view : views) {
    swapchain_create(xr_context, oxr, &view);
  }
}
