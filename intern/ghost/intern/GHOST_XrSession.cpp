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

#include "GHOST_IXrGraphicsBinding.h"
#include "GHOST_XrSession.h"
#include "GHOST_Xr_intern.h"

struct OpenXRSessionData {
  XrSystemId system_id{XR_NULL_SYSTEM_ID};
  XrSession session{XR_NULL_HANDLE};
  XrSessionState session_state{XR_SESSION_STATE_UNKNOWN};

  // Only stereo rendering supported now.
  const XrViewConfigurationType view_type{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
  std::vector<XrView> views;
  std::vector<XrSwapchain> swapchains;
  std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader *>> swapchain_images;
  int32_t swapchain_image_width, swapchain_image_height;
};

struct GHOST_XrDrawFrame {
  XrFrameState frame_state;
};

GHOST_XrSession::GHOST_XrSession(GHOST_XrContext *xr_context)
    : m_context(xr_context), m_oxr(new OpenXRSessionData())
{
}

GHOST_XrSession::~GHOST_XrSession()
{
  assert(m_oxr->session != XR_NULL_HANDLE);

  for (XrSwapchain &swapchain : m_oxr->swapchains) {
    xrDestroySwapchain(swapchain);
  }
  m_oxr->swapchains.clear();
  xrDestroySession(m_oxr->session);

  m_oxr->session = XR_NULL_HANDLE;
  m_oxr->session_state = XR_SESSION_STATE_UNKNOWN;
}

bool GHOST_XrSession::isRunning()
{
  if (m_oxr->session == XR_NULL_HANDLE) {
    return false;
  }
  switch (m_oxr->session_state) {
    case XR_SESSION_STATE_RUNNING:
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED:
      return true;
    default:
      return false;
  }
}
bool GHOST_XrSession::isVisible()
{
  if (m_oxr->session == XR_NULL_HANDLE) {
    return false;
  }
  switch (m_oxr->session_state) {
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED:
      return true;
    default:
      return false;
  }
}

/**
 * A system in OpenXR the combination of some sort of HMD plus controllers and whatever other
 * devices are managed through OpenXR. So this attempts to init the HMD and the other devices.
 */
void GHOST_XrSession::initSystem()
{
  assert(m_context->oxr.instance != XR_NULL_HANDLE);
  assert(m_oxr->system_id == XR_NULL_SYSTEM_ID);

  XrSystemGetInfo system_info{};
  system_info.type = XR_TYPE_SYSTEM_GET_INFO;
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  xrGetSystem(m_context->oxr.instance, &system_info, &m_oxr->system_id);
}

void GHOST_XrSession::bindGraphicsContext()
{
  assert(m_context->gpu_ctx_bind_fn);
  m_gpu_ctx = static_cast<GHOST_Context *>(
      m_context->gpu_ctx_bind_fn(m_context->gpu_binding_type));
}
void GHOST_XrSession::unbindGraphicsContext()
{
  if (m_context->gpu_ctx_unbind_fn) {
    m_context->gpu_ctx_unbind_fn(m_context->gpu_binding_type, m_gpu_ctx);
  }
  m_gpu_ctx = nullptr;
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

void GHOST_XrSession::prepareDrawing()
{
  std::vector<XrViewConfigurationView> view_configs;
  uint32_t view_count;

  xrEnumerateViewConfigurationViews(
      m_context->oxr.instance, m_oxr->system_id, m_oxr->view_type, 0, &view_count, nullptr);
  view_configs.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(m_context->oxr.instance,
                                    m_oxr->system_id,
                                    m_oxr->view_type,
                                    view_configs.size(),
                                    &view_count,
                                    view_configs.data());

  for (const XrViewConfigurationView &view : view_configs) {
    XrSwapchain swapchain = swapchain_create(m_oxr->session, m_gpu_binding.get(), &view);
    auto images = swapchain_images_create(swapchain, m_gpu_binding.get());

    m_oxr->swapchain_image_width = view.recommendedImageRectWidth;
    m_oxr->swapchain_image_height = view.recommendedImageRectHeight;
    m_oxr->swapchains.push_back(swapchain);
    m_oxr->swapchain_images.insert(std::make_pair(swapchain, std::move(images)));
  }

  m_oxr->views.resize(view_count, {XR_TYPE_VIEW});
}

void GHOST_XrSession::start()
{
  assert(m_context->oxr.instance != XR_NULL_HANDLE);
  assert(m_oxr->session == XR_NULL_HANDLE);
  if (m_context->gpu_ctx_bind_fn == nullptr) {
    fprintf(stderr,
            "Invalid API usage: No way to bind graphics context to the XR session. Call "
            "GHOST_XrGraphicsContextBindFuncs() with valid parameters before starting the "
            "session (through GHOST_XrSessionStart()).");
    return;
  }

  initSystem();

  bindGraphicsContext();
  if (m_gpu_ctx == nullptr) {
    fprintf(stderr,
            "Invalid API usage: No graphics context returned through the callback set with "
            "GHOST_XrGraphicsContextBindFuncs(). This is required for session starting (through "
            "GHOST_XrSessionStart()).\n");
    return;
  }
  m_gpu_binding = std::unique_ptr<GHOST_IXrGraphicsBinding>(
      GHOST_XrGraphicsBindingCreateFromType(m_context->gpu_binding_type));
  m_gpu_binding->initFromGhostContext(m_gpu_ctx);

  XrSessionCreateInfo create_info{};
  create_info.type = XR_TYPE_SESSION_CREATE_INFO;
  create_info.systemId = m_oxr->system_id;
  create_info.next = &m_gpu_binding->oxr_binding;

  xrCreateSession(m_context->oxr.instance, &create_info, &m_oxr->session);

  prepareDrawing();
}

void GHOST_XrSession::end()
{
  assert(m_oxr->session != XR_NULL_HANDLE);

  xrEndSession(m_oxr->session);
  unbindGraphicsContext();
}

GHOST_XrSession::eLifeExpectancy GHOST_XrSession::handleStateChangeEvent(
    const XrEventDataSessionStateChanged *lifecycle)
{
  m_oxr->session_state = lifecycle->state;

  /* Runtime may send events for apparently destroyed session. Our handle should be NULL then. */
  assert((m_oxr->session == XR_NULL_HANDLE) || (m_oxr->session == lifecycle->session));

  switch (lifecycle->state) {
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo begin_info{};

      begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
      begin_info.primaryViewConfigurationType = m_oxr->view_type;
      xrBeginSession(m_oxr->session, &begin_info);
      break;
    }
    case XR_SESSION_STATE_STOPPING:
      /* Runtime will change state to STATE_EXITING, don't destruct session yet. */
      end();
      break;
    case XR_SESSION_STATE_EXITING:
      return SESSION_DESTROY;
    default:
      break;
  }

  return SESSION_KEEP_ALIVE;
}

void GHOST_XrSession::beginFrameDrawing()
{
  XrFrameWaitInfo wait_info{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameBeginInfo begin_info{XR_TYPE_FRAME_BEGIN_INFO};
  XrFrameState frame_state{XR_TYPE_FRAME_STATE};

  // TODO Blocking call. Does this intefer with other drawing?
  xrWaitFrame(m_oxr->session, &wait_info, &frame_state);

  xrBeginFrame(m_oxr->session, &begin_info);

  m_draw_frame = std::unique_ptr<GHOST_XrDrawFrame>(new GHOST_XrDrawFrame());
  m_draw_frame->frame_state = frame_state;
}

void GHOST_XrSession::endFrameDrawing(std::vector<XrCompositionLayerBaseHeader *> *layers)
{
  XrFrameEndInfo end_info{XR_TYPE_FRAME_END_INFO};

  end_info.displayTime = m_draw_frame->frame_state.predictedDisplayTime;
  end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  end_info.layerCount = layers->size();
  end_info.layers = layers->data();

  xrEndFrame(m_oxr->session, &end_info);
  m_draw_frame = nullptr;
}

void GHOST_XrSession::draw(void *draw_customdata)
{
  XrReferenceSpaceCreateInfo refspace_info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  std::vector<XrCompositionLayerProjectionView>
      projection_layer_views;  // Keep alive until xrEndFrame() call!
  XrCompositionLayerProjection proj_layer;
  std::vector<XrCompositionLayerBaseHeader *> layers;
  XrSpace space;

  beginFrameDrawing();

  refspace_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  // TODO Use viewport pose here.
  refspace_info.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};
  refspace_info.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
  xrCreateReferenceSpace(m_oxr->session, &refspace_info, &space);

  if (isVisible()) {
    proj_layer = drawLayer(space, projection_layer_views, draw_customdata);
    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&proj_layer));
  }

  endFrameDrawing(&layers);
  xrDestroySpace(space);
}

static void ghost_xr_draw_view_info_from_view(const XrView &view, GHOST_XrDrawViewInfo &r_info)
{
  /* Set and convert to Blender coodinate space */
  r_info.pose.position[0] = -view.pose.position.x;
  r_info.pose.position[1] = view.pose.position.y;
  r_info.pose.position[2] = -view.pose.position.z;
  r_info.pose.orientation_quat[0] = view.pose.orientation.w;
  r_info.pose.orientation_quat[1] = view.pose.orientation.x;
  r_info.pose.orientation_quat[2] = -view.pose.orientation.y;
  r_info.pose.orientation_quat[3] = view.pose.orientation.z;

  r_info.fov.angle_left = view.fov.angleLeft;
  r_info.fov.angle_right = view.fov.angleRight;
  r_info.fov.angle_up = view.fov.angleUp;
  r_info.fov.angle_down = view.fov.angleDown;
}

void GHOST_XrSession::drawView(XrSwapchain swapchain,
                               XrCompositionLayerProjectionView &proj_layer_view,
                               XrView &view,
                               void *draw_customdata)
{
  XrSwapchainImageAcquireInfo acquire_info{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  XrSwapchainImageWaitInfo wait_info{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
  XrSwapchainImageReleaseInfo release_info{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
  XrSwapchainImageBaseHeader *swapchain_image;
  GHOST_XrDrawViewInfo draw_view_info{};
  GHOST_ContextHandle draw_ctx;
  uint32_t swapchain_idx;

  xrAcquireSwapchainImage(swapchain, &acquire_info, &swapchain_idx);
  wait_info.timeout = XR_INFINITE_DURATION;
  xrWaitSwapchainImage(swapchain, &wait_info);

  proj_layer_view.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
  proj_layer_view.pose = view.pose;
  proj_layer_view.fov = view.fov;
  proj_layer_view.subImage.swapchain = swapchain;
  proj_layer_view.subImage.imageRect.offset = {0, 0};
  proj_layer_view.subImage.imageRect.extent = {m_oxr->swapchain_image_width,
                                               m_oxr->swapchain_image_height};

  swapchain_image = m_oxr->swapchain_images[swapchain][swapchain_idx];

  draw_view_info.ofsx = proj_layer_view.subImage.imageRect.offset.x;
  draw_view_info.ofsy = proj_layer_view.subImage.imageRect.offset.y;
  draw_view_info.width = proj_layer_view.subImage.imageRect.extent.width;
  draw_view_info.height = proj_layer_view.subImage.imageRect.extent.height;
  ghost_xr_draw_view_info_from_view(view, draw_view_info);

  m_gpu_binding->drawViewBegin(swapchain_image);
  draw_ctx = m_context->draw_view_fn(&draw_view_info, draw_customdata);
  m_gpu_binding->drawViewEnd(swapchain_image, (GHOST_Context *)draw_ctx);

  xrReleaseSwapchainImage(swapchain, &release_info);
}

XrCompositionLayerProjection GHOST_XrSession::drawLayer(
    XrSpace space,
    std::vector<XrCompositionLayerProjectionView> &proj_layer_views,
    void *draw_customdata)
{
  XrViewLocateInfo viewloc_info{XR_TYPE_VIEW_LOCATE_INFO};
  XrViewState view_state{XR_TYPE_VIEW_STATE};
  XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  uint32_t view_count;

  viewloc_info.displayTime = m_draw_frame->frame_state.predictedDisplayTime;
  viewloc_info.space = space;

  xrLocateViews(m_oxr->session,
                &viewloc_info,
                &view_state,
                m_oxr->views.size(),
                &view_count,
                m_oxr->views.data());
  assert(m_oxr->swapchains.size() == view_count);

  proj_layer_views.resize(view_count);

  for (uint32_t view_idx = 0; view_idx < view_count; view_idx++) {
    drawView(m_oxr->swapchains[view_idx],
             proj_layer_views[view_idx],
             m_oxr->views[view_idx],
             draw_customdata);
  }

  layer.space = space;
  layer.viewCount = proj_layer_views.size();
  layer.views = proj_layer_views.data();

  return layer;
}
