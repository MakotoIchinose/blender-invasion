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
PFN_D3D11_CREATE_DEVICE GHOST_ContextD3D::s_D3D11CreateDeviceFn = NULL;

GHOST_ContextD3D::GHOST_ContextD3D(bool stereoVisual, HWND hWnd, HDC hDC)
    : GHOST_Context(stereoVisual), m_hWnd(hWnd), m_hDC(hDC)
{
}

GHOST_ContextD3D::~GHOST_ContextD3D()
{
}

GHOST_TSuccess GHOST_ContextD3D::swapBuffers()
{
  return GHOST_kFailure;
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

  if (s_D3D11CreateDeviceFn == NULL) {
    s_D3D11CreateDeviceFn = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(s_d3d_lib,
                                                                    "D3D11CreateDevice");

    WIN32_CHK(s_D3D11CreateDeviceFn != NULL);

    if (s_D3D11CreateDeviceFn == NULL) {
      fprintf(stderr, "GetProcAddress(s_d3d_lib, \"D3D11CreateDevice\") failed!\n");
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

  const D3D_FEATURE_LEVEL feature_level[] = {D3D_FEATURE_LEVEL_11_1};
  HRESULT hres = s_D3D11CreateDeviceFn(NULL,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       NULL,
                                       0,
                                       feature_level,
                                       std::size(feature_level),
                                       D3D11_SDK_VERSION,
                                       &m_d3d_device,
                                       NULL,
                                       &m_d3d_device_ctx);
  WIN32_CHK(hres == S_OK);

  return GHOST_kSuccess;
}

GHOST_TSuccess GHOST_ContextD3D::releaseNativeHandles()
{
  return GHOST_kFailure;
}
