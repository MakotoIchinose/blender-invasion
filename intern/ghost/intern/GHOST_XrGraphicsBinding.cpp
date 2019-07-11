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
#include <sstream>

#if defined(WITH_X11)
#  include "GHOST_ContextGLX.h"
#elif defined(WIN32)
#  include "GHOST_ContextWGL.h"
#  include "GHOST_ContextD3D.h"
#endif
#include "GHOST_C-api.h"
#include "GHOST_Xr_intern.h"

#include "GHOST_IXrGraphicsBinding.h"

static bool choose_swapchain_format_from_candidates(std::vector<int64_t> gpu_binding_formats,
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
  bool checkVersionRequirements(GHOST_Context *ghost_ctx,
                                XrInstance instance,
                                XrSystemId system_id,
                                std::string *r_requirement_info) const override
  {
#if defined(WITH_X11)
    GHOST_ContextGLX *ctx_gl = static_cast<GHOST_ContextGLX *>(ghost_ctx);
#else
    GHOST_ContextWGL *ctx_gl = static_cast<GHOST_ContextWGL *>(ghost_ctx);
#endif
    XrGraphicsRequirementsOpenGLKHR gpu_requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    const uint32_t gl_version = XR_MAKE_VERSION(
        ctx_gl->m_contextMajorVersion, ctx_gl->m_contextMinorVersion, 0);

    xrGetOpenGLGraphicsRequirementsKHR(instance, system_id, &gpu_requirements);

    if (r_requirement_info) {
      std::ostringstream strstream;
      strstream << "Min OpenGL version "
                << XR_VERSION_MAJOR(gpu_requirements.minApiVersionSupported) << "."
                << XR_VERSION_MINOR(gpu_requirements.minApiVersionSupported) << std::endl;
      strstream << "Max OpenGL version "
                << XR_VERSION_MAJOR(gpu_requirements.maxApiVersionSupported) << "."
                << XR_VERSION_MINOR(gpu_requirements.maxApiVersionSupported) << std::endl;

      *r_requirement_info = std::move(strstream.str());
    }

    return (gl_version >= gpu_requirements.minApiVersionSupported) &&
           (gl_version <= gpu_requirements.maxApiVersionSupported);
  }

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

  bool chooseSwapchainFormat(const std::vector<int64_t> &runtime_formats,
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
  void drawViewEnd(XrSwapchainImageBaseHeader *swapchain_image, GHOST_Context *ogl_ctx) override
  {
    // TODO
    (void)swapchain_image;
    (void)ogl_ctx;
  }

 private:
  std::list<std::vector<XrSwapchainImageOpenGLKHR>> m_image_cache;
};

#ifdef WIN32
class GHOST_XrGraphicsBindingD3D : public GHOST_IXrGraphicsBinding {
 public:
  bool checkVersionRequirements(GHOST_Context * /*ghost_ctx*/,
                                XrInstance /*instance*/,
                                XrSystemId /*system_id*/,
                                std::string * /*r_requirement_info*/) const override
  {
    // TODO
  }

  void initFromGhostContext(GHOST_Context *ghost_ctx) override
  {
    GHOST_ContextD3D *ctx_d3d = static_cast<GHOST_ContextD3D *>(ghost_ctx);

    oxr_binding.d3d11.type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
    oxr_binding.d3d11.device = ctx_d3d->m_device;
    m_ghost_ctx = ctx_d3d;
  }

  bool chooseSwapchainFormat(const std::vector<int64_t> &runtime_formats,
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

  void drawViewBegin(XrSwapchainImageBaseHeader * /*swapchain_image*/) override
  {
  }
  void drawViewEnd(XrSwapchainImageBaseHeader *swapchain_image, GHOST_Context *ogl_ctx) override
  {
    XrSwapchainImageD3D11KHR *d3d_swapchain_image = reinterpret_cast<XrSwapchainImageD3D11KHR *>(
        swapchain_image);

#  if 0
    /* Ideally we'd just create a render target view for the OpenXR swapchain image texture and
     * blit from the OpenGL context into it. The NV_DX_interop extension doesn't want to work with
     * this though. At least not with Optimus hardware. See:
     * https://github.com/mpv-player/mpv/issues/2949#issuecomment-197262807.
     * Note: Even if this worked, the blitting code only supports one shared resource by now, we'd
     * need at least two (for each eye). We could also entirely re-register shared resources all
     * the time. Also, the runtime might recreate the swapchain image so the shared resource would
     * have to be re-registered then as well. */

    ID3D11RenderTargetView *rtv;
    CD3D11_RENDER_TARGET_VIEW_DESC rtv_desc(D3D11_RTV_DIMENSION_TEXTURE2D,
                                            DXGI_FORMAT_R8G8B8A8_UNORM);
    D3D11_TEXTURE2D_DESC tex_desc;

    d3d_swapchain_image->texture->GetDesc(&tex_desc);

    m_ghost_ctx->m_device->CreateRenderTargetView(d3d_swapchain_image->texture, &rtv_desc, &rtv);
    m_ghost_ctx->blitOpenGLOffscreenContext(ogl_ctx, rtv, tex_desc.Width, tex_desc.Height);
#  else
    ID3D11Resource *res;
    ID3D11Texture2D *tex;
    D3D11_TEXTURE2D_DESC tex_desc;

    d3d_swapchain_image->texture->GetDesc(&tex_desc);

    ogl_ctx->activateDrawingContext();
    m_ghost_ctx->blitOpenGLOffscreenContext(ogl_ctx, tex_desc.Width, tex_desc.Height);

    m_ghost_ctx->m_backbuffer_view->GetResource(&res);
    res->QueryInterface<ID3D11Texture2D>(&tex);

    m_ghost_ctx->m_device_ctx->OMSetRenderTargets(0, nullptr, nullptr);
    m_ghost_ctx->m_device_ctx->CopyResource(d3d_swapchain_image->texture, tex);

    res->Release();
    tex->Release();
#  endif
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
