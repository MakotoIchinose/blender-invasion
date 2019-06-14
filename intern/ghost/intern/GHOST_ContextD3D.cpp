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

#include "GHOST_ContextD3D.h"

HMODULE GHOST_ContextD3D::s_d3d_lib = NULL;
PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN GHOST_ContextD3D::s_D3D11CreateDeviceAndSwapChainFn = NULL;

GHOST_ContextD3D::GHOST_ContextD3D(bool stereoVisual, HWND hWnd)
    : GHOST_Context(stereoVisual), m_hWnd(hWnd)
{
}

GHOST_ContextD3D::~GHOST_ContextD3D()
{
}

GHOST_TSuccess GHOST_ContextD3D::swapBuffers()
{
  HRESULT res = m_swapchain->Present(0, 0);
  return (res == S_OK) ? GHOST_kSuccess : GHOST_kFailure;
}

GHOST_TSuccess GHOST_ContextD3D::activateDrawingContext()
{
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_ContextD3D::releaseDrawingContext()
{
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_ContextD3D::setupD3DLib()
{
  if (s_d3d_lib == NULL) {
    s_d3d_lib = LoadLibraryA("d3d11.dll");

    WIN32_CHK(s_d3d_lib != NULL);

    if (s_d3d_lib == NULL) {
      fprintf(stderr, "LoadLibrary(\"d3d11.dll\") failed!\n");
      return GHOST_kFailure;
    }
  }

  if (s_D3D11CreateDeviceAndSwapChainFn == NULL) {
    s_D3D11CreateDeviceAndSwapChainFn = (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)GetProcAddress(
        s_d3d_lib, "D3D11CreateDeviceAndSwapChain");

    WIN32_CHK(s_D3D11CreateDeviceAndSwapChainFn != NULL);

    if (s_D3D11CreateDeviceAndSwapChainFn == NULL) {
      fprintf(stderr, "GetProcAddress(s_d3d_lib, \"D3D11CreateDeviceAndSwapChain\") failed!\n");
      return GHOST_kFailure;
    }
  }

  return GHOST_kSuccess;
}

GHOST_TSuccess GHOST_ContextD3D::initializeDrawingContext()
{
  if (setupD3DLib() == GHOST_kFailure) {
    return GHOST_kFailure;
  }

  DXGI_SWAP_CHAIN_DESC sd{};

  sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 1;
  sd.OutputWindow = m_hWnd;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  HRESULT hres = s_D3D11CreateDeviceAndSwapChainFn(NULL,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   0,
                                                   D3D11_SDK_VERSION,
                                                   &sd,
                                                   &m_swapchain,
                                                   &m_device,
                                                   NULL,
                                                   &m_device_ctx);
  WIN32_CHK(hres == S_OK);

  Microsoft::WRL::ComPtr<ID3D11Resource> back_buffer = nullptr;
  m_swapchain->GetBuffer(0, __uuidof(ID3D11Resource), &back_buffer);
  m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, &m_backbuffer_view);

  return GHOST_kSuccess;
}

GHOST_TSuccess GHOST_ContextD3D::releaseNativeHandles()
{
  return GHOST_kFailure;
}

GHOST_TSuccess GHOST_ContextD3D::blitOpenGLOffscreenContext(GHOST_Context *offscreen_ctx)
{
  /* Just testing. OpenGL compatibility code goes here (using NV_DX_interop extensions) */

  const float col[] = {0.9f, 0.3f, 0.0f, 1.0f};
  m_device_ctx->ClearRenderTargetView(m_backbuffer_view.Get(), col);

  return GHOST_kSuccess;
}
