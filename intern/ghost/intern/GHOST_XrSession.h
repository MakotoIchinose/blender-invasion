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

#ifndef __GHOST_XRSESSION_H__
#define __GHOST_XRSESSION_H__

#include <map>
#include <memory>

#include "GHOST_Xr_openxr_includes.h"

class GHOST_XrSession {
 public:
  enum eLifeExpectancy {
    SESSION_KEEP_ALIVE,
    SESSION_DESTROY,
  };

  GHOST_XrSession(struct GHOST_XrContext *xr_context);
  ~GHOST_XrSession();

  void start(const GHOST_XrSessionBeginInfo *begin_info);
  void end();

  eLifeExpectancy handleStateChangeEvent(const struct XrEventDataSessionStateChanged *lifecycle);

  bool isRunning();
  bool isVisible();

  void unbindGraphicsContext(); /* public so context can ensure it's unbound as needed. */

  void draw(void *draw_customdata);

 private:
  /** Pointer back to context managing this session. Would be nice to avoid, but needed to access
   * custom callbacks set before session start. */
  struct GHOST_XrContext *m_context;

  std::unique_ptr<struct OpenXRSessionData> m_oxr; /* Could use stack, but PImpl is preferable */

  /** Active Ghost graphic context. Owned by Blender, not GHOST. */
  class GHOST_Context *m_gpu_ctx;
  std::unique_ptr<class GHOST_IXrGraphicsBinding> m_gpu_binding;

  /** Information on the currently drawn frame. Set while drawing only. */
  std::unique_ptr<struct GHOST_XrDrawFrame> m_draw_frame;

  void initSystem();

  void bindGraphicsContext();

  void prepareDrawing();
  XrCompositionLayerProjection drawLayer(
      std::vector<XrCompositionLayerProjectionView> &proj_layer_views, void *draw_customdata);
  void drawView(XrSwapchain swapchain,
                XrCompositionLayerProjectionView &proj_layer_view,
                XrView &view,
                void *draw_customdata);
  void beginFrameDrawing();
  void endFrameDrawing(std::vector<XrCompositionLayerBaseHeader *> *layers);
};

#endif /* GHOST_XRSESSION_H__ */
