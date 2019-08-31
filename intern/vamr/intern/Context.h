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
 * \ingroup VAMR
 */

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <memory>
#include <vector>

#include "VAMR_IContext.h"

namespace VAMR {

struct CustomFuncs {
  /** Function to retrieve (possibly create) a graphics context */
  VAMR_GraphicsContextBindFn gpu_ctx_bind_fn{nullptr};
  /** Function to release (possibly free) a graphics context */
  VAMR_GraphicsContextUnbindFn gpu_ctx_unbind_fn{nullptr};

  /** Custom per-view draw function for Blender side drawing. */
  VAMR_DrawViewFn draw_view_fn{nullptr};
};

/**
 * In some occasions, runtime specific handling is needed, e.g. to work around runtime bugs.
 */
enum OpenXRRuntimeID {
  OPENXR_RUNTIME_MONADO,
  OPENXR_RUNTIME_OCULUS,
  OPENXR_RUNTIME_WMR, /* Windows Mixed Reality */

  OPENXR_RUNTIME_UNKNOWN
};

/**
 * \brief Main VAMR container to manage OpenXR through.
 *
 * Creating a context using #VAMR_ContextCreate involves dynamically connecting to the OpenXR
 * runtime, likely reading the OS OpenXR configuration (i.e. active_runtime.json). So this is
 * something that should better be done using lazy-initialization.
 */
class Context : public VAMR::IContext {
 public:
  Context(const VAMR_ContextCreateInfo *create_info);
  ~Context();
  void initialize(const VAMR_ContextCreateInfo *create_info);

  void startSession(const VAMR_SessionBeginInfo *begin_info) override;
  void endSession() override;
  bool isSessionRunning() const override;
  void drawSessionViews(void *draw_customdata) override;

  static void setErrorHandler(VAMR_ErrorHandlerFn handler_fn, void *customdata);
  void dispatchErrorMessage(const class Exception *exception) const override;

  void setGraphicsContextBindFuncs(VAMR_GraphicsContextBindFn bind_fn,
                                   VAMR_GraphicsContextUnbindFn unbind_fn) override;
  void setDrawViewFunc(VAMR_DrawViewFn draw_view_fn) override;

  void handleSessionStateChange(const XrEventDataSessionStateChanged *lifecycle);

  OpenXRRuntimeID getOpenXRRuntimeID() const;
  const CustomFuncs *getCustomFuncs() const;
  VAMR_GraphicsBindingType getGraphicsBindingType() const;
  XrInstance getInstance() const;
  bool isDebugMode() const;
  bool isDebugTimeMode() const;

 private:
  std::unique_ptr<struct OpenXRInstanceData> m_oxr;

  OpenXRRuntimeID m_runtime_id{OPENXR_RUNTIME_UNKNOWN};

  /* The active VAMR XR Session. Null while no session runs. */
  std::unique_ptr<class Session> m_session;

  /** Active graphics binding type. */
  VAMR_GraphicsBindingType m_gpu_binding_type{VAMR_GraphicsBindingTypeUnknown};

  /** Names of enabled extensions */
  std::vector<const char *> m_enabled_extensions;
  /** Names of enabled API-layers */
  std::vector<const char *> m_enabled_layers;

  static VAMR_ErrorHandlerFn s_error_handler;
  static void *s_error_handler_customdata;
  CustomFuncs m_custom_funcs;

  /** Enable debug message prints and OpenXR API validation layers */
  bool m_debug{false};
  bool m_debug_time{false};

  void createOpenXRInstance();
  void storeInstanceProperties();
  void initDebugMessenger();

  void printInstanceInfo();
  void printAvailableAPILayersAndExtensionsInfo();
  void printExtensionsAndAPILayersToEnable();

  void enumerateApiLayers();
  void enumerateExtensions();
  void enumerateExtensionsEx(std::vector<XrExtensionProperties> &extensions,
                             const char *layer_name);
  void getAPILayersToEnable(std::vector<const char *> &r_ext_names);
  void getExtensionsToEnable(std::vector<const char *> &r_ext_names);
  VAMR_GraphicsBindingType determineGraphicsBindingTypeToEnable(
      const VAMR_ContextCreateInfo *create_info);
};

}  // namespace VAMR

#endif  // __CONTEXT_H__
