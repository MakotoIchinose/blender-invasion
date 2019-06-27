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
static GHOST_TSuccess GHOST_XrSessionIsVisible(const GHOST_XrContext *xr_context)
{
  if ((xr_context == nullptr) || (xr_context->oxr.session == XR_NULL_HANDLE)) {
    return GHOST_kFailure;
  }
  switch (xr_context->oxr.session_state) {
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

static std::vector<XrSwapchainImageBaseHeader *> swapchain_images_create(
    XrSwapchain swapchain, GHOST_IXrGraphicsBinding *gpu_binding)
{
  std::vector<XrSwapchainImageBaseHeader *> images;
  uint32_t image_count;

  xrEnumerateSwapchainImages(swapchain, 0, &image_count, nullptr);
  images = gpu_binding->createSwapchainImages(image_count);
  xrEnumerateSwapchainImages(swapchain, images.size(), &image_count, images[0]);

  return images;
}

static XrSwapchain swapchain_create(const XrSession session,
                                    GHOST_IXrGraphicsBinding *gpu_binding,
                                    const XrViewConfigurationView *xr_view)
{
  XrSwapchainCreateInfo create_info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  XrSwapchain swapchain;
  uint32_t format_count = 0;
  int64_t chosen_format;

  xrEnumerateSwapchainFormats(session, 0, &format_count, nullptr);
  std::vector<int64_t> swapchain_formats(format_count);
  xrEnumerateSwapchainFormats(
      session, swapchain_formats.size(), &format_count, swapchain_formats.data());
  assert(swapchain_formats.size() == format_count);

  if (!gpu_binding->chooseSwapchainFormat(swapchain_formats, &chosen_format)) {
    fprintf(stderr, "Error: No format matching OpenXR runtime supported swapchain formats found.");
    return nullptr;
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
  xrCreateSwapchain(session, &create_info, &swapchain);

  return swapchain;
}

void GHOST_XrSessionRenderingPrepare(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  std::vector<XrViewConfigurationView> view_configs;
  uint32_t view_count;

  xrEnumerateViewConfigurationViews(
      oxr->instance, oxr->system_id, oxr->view_type, 0, &view_count, nullptr);
  view_configs.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(oxr->instance,
                                    oxr->system_id,
                                    oxr->view_type,
                                    view_configs.size(),
                                    &view_count,
                                    view_configs.data());

  for (const XrViewConfigurationView &view : view_configs) {
    XrSwapchain swapchain = swapchain_create(oxr->session, xr_context->gpu_binding.get(), &view);
    auto images = swapchain_images_create(swapchain, xr_context->gpu_binding.get());

    oxr->swapchain_image_width = view.recommendedImageRectWidth;
    oxr->swapchain_image_height = view.recommendedImageRectHeight;
    oxr->swapchains.push_back(swapchain);
    oxr->swapchain_images.insert(std::make_pair(swapchain, std::move(images)));
  }

  oxr->views.resize(view_count, {XR_TYPE_VIEW});
}

static void drawing_begin(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  XrFrameWaitInfo wait_info{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameBeginInfo begin_info{XR_TYPE_FRAME_BEGIN_INFO};
  XrFrameState frame_state{XR_TYPE_FRAME_STATE};

  // TODO Blocking call. Does this intefer with other drawing?
  xrWaitFrame(oxr->session, &wait_info, &frame_state);

  xrBeginFrame(oxr->session, &begin_info);

  xr_context->draw_frame = std::unique_ptr<GHOST_XrDrawFrame>(new GHOST_XrDrawFrame());
  xr_context->draw_frame->frame_state = frame_state;
}

void drawing_end(GHOST_XrContext *xr_context, std::vector<XrCompositionLayerBaseHeader *> *layers)
{
  XrFrameEndInfo end_info{XR_TYPE_FRAME_END_INFO};

  end_info.displayTime = xr_context->draw_frame->frame_state.predictedDisplayTime;
  end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  end_info.layerCount = layers->size();
  end_info.layers = layers->data();

  xrEndFrame(xr_context->oxr.session, &end_info);
  xr_context->draw_frame = nullptr;
}

static void draw_view(GHOST_XrContext *xr_context,
                      OpenXRData *oxr,
                      XrSwapchain swapchain,
                      XrCompositionLayerProjectionView &proj_layer_view,
                      XrView &view,
                      void *draw_customdata)
{
  XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  XrSwapchainImageBaseHeader *swapchain_image;
  GHOST_XrDrawViewInfo draw_view_info{};
  uint32_t swapchain_idx;

  xrAcquireSwapchainImage(swapchain, &acquire_info, &swapchain_idx);
  wait_info.timeout = XR_INFINITE_DURATION;
  xrWaitSwapchainImage(swapchain, &wait_info);

  proj_layer_view.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
  proj_layer_view.pose = view.pose;
  proj_layer_view.fov = view.fov;
  proj_layer_view.subImage.swapchain = swapchain;
  proj_layer_view.subImage.imageRect.offset = {0, 0};
  proj_layer_view.subImage.imageRect.extent = {oxr->swapchain_image_width,
                                               oxr->swapchain_image_height};

  swapchain_image = oxr->swapchain_images[swapchain][swapchain_idx];

  xr_context->gpu_binding->drawViewBegin(swapchain_image);
  xr_context->draw_view_fn(&draw_view_info, draw_customdata);
  xr_context->gpu_binding->drawViewEnd(swapchain_image);

  xrReleaseSwapchainImage(swapchain, &release_info);
}

static XrCompositionLayerProjection draw_layer(
    GHOST_XrContext *xr_context,
    OpenXRData *oxr,
    XrSpace space,
    std::vector<XrCompositionLayerProjectionView> &proj_layer_views,
    void *draw_customdata)
{
  XrViewLocateInfo viewloc_info{XR_TYPE_VIEW_LOCATE_INFO};
  XrViewState view_state{XR_TYPE_VIEW_STATE};
  XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  uint32_t view_count;

  viewloc_info.displayTime = xr_context->draw_frame->frame_state.predictedDisplayTime;
  viewloc_info.space = space;

  xrLocateViews(
      oxr->session, &viewloc_info, &view_state, oxr->views.size(), &view_count, oxr->views.data());
  assert(oxr->swapchains.size() == view_count);

  proj_layer_views.resize(view_count);

  for (uint32_t view_idx = 0; view_idx < view_count; view_idx++) {
    draw_view(xr_context,
              oxr,
              oxr->swapchains[view_idx],
              proj_layer_views[view_idx],
              oxr->views[view_idx],
              draw_customdata);
  }

  layer.space = space;
  layer.viewCount = proj_layer_views.size();
  layer.views = proj_layer_views.data();

  return layer;
}

void GHOST_XrSessionDrawViews(GHOST_XrContext *xr_context, void *draw_customdata)
{
  OpenXRData *oxr = &xr_context->oxr;
  XrReferenceSpaceCreateInfo refspace_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  std::vector<XrCompositionLayerProjectionView>
      projection_layer_views;  // Keep alive until xrEndFrame() call!
  XrCompositionLayerProjection proj_layer;
  std::vector<XrCompositionLayerBaseHeader *> layers;
  XrSpace space;

  drawing_begin(xr_context);

  refspace_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  // TODO Use viewport pose here.
  refspace_info.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};
  refspace_info.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
  xrCreateReferenceSpace(oxr->session, &refspace_info, &space);

  if (GHOST_XrSessionIsVisible(xr_context)) {
    proj_layer = draw_layer(xr_context, oxr, space, projection_layer_views, draw_customdata);
    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&proj_layer));
  }

  drawing_end(xr_context, &layers);
}
