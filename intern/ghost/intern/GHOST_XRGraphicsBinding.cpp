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
#include <list>

#if defined(WITH_X11)
#  include "GHOST_ContextGLX.h"
#elif defined(WIN32)
#  include "GHOST_ContextWGL.h"
#  include "GHOST_ContextD3D.h"
#endif
#include "GHOST_C-api.h"
#include "GHOST_XR_intern.h"

#include "GHOST_IXRGraphicsBinding.h"

bool choose_swapchain_format_from_candidates(std::vector<int64_t> gpu_binding_formats,
                                             std::vector<int64_t> runtime_formats,
                                             int64_t *r_result)
{
  if (gpu_binding_formats.empty()) {
    return false;
  }

  auto res = std::find_first_of(gpu_binding_formats.begin(),
                                gpu_binding_formats.end(),
                                runtime_formats.begin(),
                                runtime_formats.end());
  if (res == gpu_binding_formats.end()) {
    return false;
  }

  *r_result = *res;
  return true;
}

class GHOST_XrGraphicsBindingOpenGL : public GHOST_IXrGraphicsBinding {
 public:
  void initFromGhostContext(GHOST_Context *ghost_ctx) override
  {
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
  }

  bool chooseSwapchainFormat(std::vector<int64_t> runtime_formats,
                             int64_t *r_result) const override
  {
    std::vector<int64_t> gpu_binding_formats = {GL_RGBA8};
    return choose_swapchain_format_from_candidates(gpu_binding_formats, runtime_formats, r_result);
  }

  std::vector<XrSwapchainImageBaseHeader *> createSwapchainImages(uint32_t image_count) override
  {
    std::vector<XrSwapchainImageOpenGLKHR> ogl_images(image_count);
    std::vector<XrSwapchainImageBaseHeader *> base_images;

    // Need to return vector of base header pointers, so of a different type. Need to build a new
    // list with this type, and keep the initial one alive.
    for (XrSwapchainImageOpenGLKHR &image : ogl_images) {
      image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
      base_images.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
    }

    // Keep alive.
    m_image_cache.push_back(std::move(ogl_images));

    return base_images;
  }

  void drawViewBegin(XrSwapchainImageBaseHeader *swapchain_image) override
  {
    // TODO
    (void)swapchain_image;
  }
  void drawViewEnd(XrSwapchainImageBaseHeader *swapchain_image) override
  {
    // TODO
    (void)swapchain_image;
  }

 private:
  std::list<std::vector<XrSwapchainImageOpenGLKHR>> m_image_cache;
};

#ifdef WIN32
class GHOST_XrGraphicsBindingD3D : public GHOST_IXrGraphicsBinding {
 public:
  void initFromGhostContext(GHOST_Context *ghost_ctx) override
  {
    GHOST_ContextD3D *ctx_d3d = static_cast<GHOST_ContextD3D *>(ghost_ctx);

    oxr_binding.d3d11.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
    oxr_binding.d3d11.device = ctx_d3d->m_device;
    m_ghost_ctx = ctx_d3d;
  }

  bool chooseSwapchainFormat(std::vector<int64_t> runtime_formats,
                             int64_t *r_result) const override
  {
    std::vector<int64_t> gpu_binding_formats = {DXGI_FORMAT_R8G8B8A8_UNORM};
    return choose_swapchain_format_from_candidates(gpu_binding_formats, runtime_formats, r_result);
  }

  std::vector<XrSwapchainImageBaseHeader *> createSwapchainImages(uint32_t image_count) override
  {
    std::vector<XrSwapchainImageD3D11KHR> d3d_images(image_count);
    std::vector<XrSwapchainImageBaseHeader *> base_images;

    // Need to return vector of base header pointers, so of a different type. Need to build a new
    // list with this type, and keep the initial one alive.
    for (XrSwapchainImageD3D11KHR &image : d3d_images) {
      image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
      base_images.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
    }

    // Keep alive.
    m_image_cache.push_back(std::move(d3d_images));

    return base_images;
  }

  void drawViewBegin(XrSwapchainImageBaseHeader *swapchain_image) override
  {
    // Can't we simply use the backbuffer texture? Didn't work in initial test.

    XrSwapchainImageD3D11KHR *d3d_swapchain_image = reinterpret_cast<XrSwapchainImageD3D11KHR *>(
        swapchain_image);
    const CD3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc(D3D11_RTV_DIMENSION_TEXTURE2D,
                                                                 DXGI_FORMAT_R8G8B8A8_UNORM);
    ID3D11RenderTargetView *render_target_view;
    m_ghost_ctx->m_device->CreateRenderTargetView(d3d_swapchain_image->texture,
                                                  &render_target_view_desc,
                                                  &render_target_view);

    const float clear_col[] = {0.2f, 0.5f, 0.8f, 1.0f};
    m_ghost_ctx->m_device_ctx->ClearRenderTargetView(render_target_view, clear_col);

    render_target_view->Release();
  }
  void drawViewEnd(XrSwapchainImageBaseHeader *swapchain_image) override
  {
    // TODO
    (void)swapchain_image;
  }

 private:
  GHOST_ContextD3D *m_ghost_ctx;
  std::list<std::vector<XrSwapchainImageD3D11KHR>> m_image_cache;
};
#endif  // WIN32

std::unique_ptr<GHOST_IXrGraphicsBinding> GHOST_XrGraphicsBindingCreateFromType(
    GHOST_TXrGraphicsBinding type)
{
  switch (type) {
    case GHOST_kXrGraphicsOpenGL:
      return std::unique_ptr<GHOST_XrGraphicsBindingOpenGL>(new GHOST_XrGraphicsBindingOpenGL());
#ifdef WIN32
    case GHOST_kXrGraphicsD3D11:
      return std::unique_ptr<GHOST_XrGraphicsBindingD3D>(new GHOST_XrGraphicsBindingD3D());
#endif
    default:
      return nullptr;
  }
}
