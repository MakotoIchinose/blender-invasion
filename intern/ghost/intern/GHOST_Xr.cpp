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
 *
 * Abstraction for XR (VR, AR, MR, ..) access via OpenXR.
 */

#include <cassert>
#include <string>

#include "GHOST_C-api.h"

#include "GHOST_Xr_intern.h"

static PFN_xrCreateDebugUtilsMessengerEXT g_xrCreateDebugUtilsMessengerEXT_fn = nullptr;
static PFN_xrDestroyDebugUtilsMessengerEXT g_xrDestroyDebugUtilsMessengerEXT_fn = nullptr;

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
static bool openxr_gather_extensions_ex(const GHOST_XrContext *xr_context,
                                        std::vector<XrExtensionProperties> &extensions,
                                        const char *layer_name)
{
  const unsigned long old_extension_count = extensions.size();
  uint32_t extension_count = 0;

  /* Get count for array creation/init first. */
  if (XR_FAILED(
          xrEnumerateInstanceExtensionProperties(layer_name, 0, &extension_count, nullptr))) {
    return false;
  }

  if (extension_count == 0) {
    /* Extensions are optional, can successfully exit. */
    return true;
  }

  for (uint32_t i = 0; i < extension_count; i++) {
    XrExtensionProperties ext{};

    ext.type = XR_TYPE_EXTENSION_PROPERTIES;
    extensions.push_back(ext);
  }

  if (layer_name) {
    XR_DEBUG_PRINTF(xr_context, "Layer: %s\n", layer_name);
  }

  /* Actually get the extensions. */
  xrEnumerateInstanceExtensionProperties(
      layer_name, extension_count, &extension_count, extensions.data());
  XR_DEBUG_ONLY_BEGIN(xr_context);
  for (uint32_t i = 0; i < extension_count; i++) {
    XR_DEBUG_PRINTF(
        xr_context, "Extension: %s\n", extensions[i + old_extension_count].extensionName);
  }
  XR_DEBUG_ONLY_END;

  return true;
}
static bool openxr_gather_extensions(const GHOST_XrContext *xr_context, OpenXRData *oxr)
{
  return openxr_gather_extensions_ex(xr_context, oxr->extensions, nullptr);
}

static bool openxr_gather_api_layers(const GHOST_XrContext *xr_context, OpenXRData *oxr)
{
  uint32_t layer_count = 0;

  /* Get count for array creation/init first. */
  if (XR_FAILED(xrEnumerateApiLayerProperties(0, &layer_count, nullptr))) {
    return false;
  }

  if (layer_count == 0) {
    /* Layers are optional, can safely exit. */
    return true;
  }

  oxr->layers = std::vector<XrApiLayerProperties>(layer_count);
  for (XrApiLayerProperties &layer : oxr->layers) {
    layer.type = XR_TYPE_API_LAYER_PROPERTIES;
  }

  /* Actually get the layers. */
  xrEnumerateApiLayerProperties(layer_count, &layer_count, oxr->layers.data());
  for (XrApiLayerProperties &layer : oxr->layers) {
    XR_DEBUG_PRINTF(xr_context, "Layer: %s\n", layer.layerName);

    /* Each layer may have own extensions */
    openxr_gather_extensions_ex(xr_context, oxr->extensions, layer.layerName);
  }

  return true;
}

static bool openxr_layer_is_available(const OpenXRData *oxr, const std::string &layer_name)
{
  for (const XrApiLayerProperties &layer : oxr->layers) {
    if (layer.layerName == layer_name) {
      return true;
    }
  }

  return false;
}
static bool openxr_extension_is_available(const OpenXRData *oxr, const std::string &extension_name)
{
  for (const XrExtensionProperties &ext : oxr->extensions) {
    if (ext.extensionName == extension_name) {
      return true;
    }
  }

  return false;
}

static const char *openxr_ext_name_from_wm_gpu_binding(GHOST_TXrGraphicsBinding binding)
{
  switch (binding) {
    case GHOST_kXrGraphicsOpenGL:
      return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
#ifdef WIN32
    case GHOST_kXrGraphicsD3D11:
      return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
#endif
    case GHOST_kXrGraphicsUnknown:
      assert(false);
      return nullptr;
  }

  return nullptr;
}

/**
 * Decide which graphics binding extension to use based on
 * #GHOST_XrContextCreateInfo.gpu_binding_candidates and available extensions.
 */
static GHOST_TXrGraphicsBinding openxr_graphics_extension_to_enable_get(
    const OpenXRData *oxr, const GHOST_XrContextCreateInfo *create_info)
{
  assert(create_info->gpu_binding_candidates != NULL);
  assert(create_info->gpu_binding_candidates_count > 0);

  for (uint32_t i = 0; i < create_info->gpu_binding_candidates_count; i++) {
    assert(create_info->gpu_binding_candidates[i] != GHOST_kXrGraphicsUnknown);
    const char *ext_name = openxr_ext_name_from_wm_gpu_binding(
        create_info->gpu_binding_candidates[i]);
    if (openxr_extension_is_available(oxr, ext_name)) {
      return create_info->gpu_binding_candidates[i];
    }
  }

  return GHOST_kXrGraphicsUnknown;
}

/**
 * Gather an array of names for the API-layers to enable.
 */
static void openxr_layers_to_enable_get(const GHOST_XrContext *xr_context,
                                        const OpenXRData *oxr,
                                        std::vector<const char *> &r_ext_names)
{
  static std::vector<std::string> try_layers;

  XR_DEBUG_ONLY_BEGIN(xr_context);
  try_layers.push_back("XR_APILAYER_LUNARG_core_validation");
  XR_DEBUG_ONLY_END;

  r_ext_names.reserve(try_layers.size());

  for (const std::string &layer : try_layers) {
    if (openxr_layer_is_available(oxr, layer)) {
      r_ext_names.push_back(layer.c_str());
      XR_DEBUG_PRINTF(xr_context, "Enabling OpenXR API-Layer %s\n", layer.c_str());
    }
  }
}
/**
 * Gather an array of names for the extensions to enable.
 */
static void openxr_extensions_to_enable_get(const GHOST_XrContext *context,
                                            const OpenXRData *oxr,
                                            std::vector<const char *> &r_ext_names)
{
  assert(context->gpu_binding_type != GHOST_kXrGraphicsUnknown);

  const char *gpu_binding = openxr_ext_name_from_wm_gpu_binding(context->gpu_binding_type);
  static std::vector<std::string> try_ext;
  const auto add_ext = [context, &r_ext_names](const char *ext_name) {
    r_ext_names.push_back(ext_name);
    XR_DEBUG_PRINTF(context, "Enabling OpenXR Extension: %s\n", ext_name);
  };

  XR_DEBUG_ONLY_BEGIN(context);
  /* Try enabling debug extension */
  try_ext.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
  XR_DEBUG_ONLY_END;

  r_ext_names.reserve(try_ext.size() + 1); /* + 1 for graphics binding extension. */

  /* Add graphics binding extension. */
  assert(gpu_binding);
  assert(openxr_extension_is_available(oxr, gpu_binding));
  add_ext(gpu_binding);

  for (const std::string &ext : try_ext) {
    if (openxr_extension_is_available(oxr, ext)) {
      add_ext(ext.c_str());
    }
  }
}

static bool openxr_instance_create(GHOST_XrContext *context)
{
  XrInstanceCreateInfo create_info{};
  OpenXRData *oxr = &context->oxr;

  create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
  std::string("Blender").copy(create_info.applicationInfo.applicationName,
                              XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  openxr_layers_to_enable_get(context, oxr, context->enabled_layers);
  openxr_extensions_to_enable_get(context, oxr, context->enabled_extensions);
  create_info.enabledApiLayerCount = context->enabled_layers.size();
  create_info.enabledApiLayerNames = context->enabled_layers.data();
  create_info.enabledExtensionCount = context->enabled_extensions.size();
  create_info.enabledExtensionNames = context->enabled_extensions.data();

  xrCreateInstance(&create_info, &oxr->instance);

  return true;
}

static void openxr_instance_log_print(const GHOST_XrContext *xr_context, OpenXRData *oxr)
{
  assert(oxr->instance != XR_NULL_HANDLE);

  XrInstanceProperties instanceProperties{};
  instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
  xrGetInstanceProperties(oxr->instance, &instanceProperties);

  XR_DEBUG_PRINTF(xr_context, "Connected to OpenXR runtime: %s\n", instanceProperties.runtimeName);
}

static XrBool32 debug_messenger_func(XrDebugUtilsMessageSeverityFlagsEXT /*messageSeverity*/,
                                     XrDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
                                     const XrDebugUtilsMessengerCallbackDataEXT *callbackData,
                                     void * /*userData*/)
{
  puts("OpenXR Debug Message:");
  puts(callbackData->message);
  return XR_FALSE;  // OpenXR spec suggests always returning false.
}

static void openxr_instance_init_debug_messenger(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  XrDebugUtilsMessengerCreateInfoEXT create_info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

  /* Extension functions need to be obtained through xrGetInstanceProcAddr */
  if (XR_FAILED(
          xrGetInstanceProcAddr(oxr->instance,
                                "xrCreateDebugUtilsMessengerEXT",
                                (PFN_xrVoidFunction *)&g_xrCreateDebugUtilsMessengerEXT_fn)) ||
      XR_FAILED(
          xrGetInstanceProcAddr(oxr->instance,
                                "xrDestroyDebugUtilsMessengerEXT",
                                (PFN_xrVoidFunction *)&g_xrDestroyDebugUtilsMessengerEXT_fn))) {
    fprintf(stderr, "Could not use XR_EXT_debug_utils to enable debug prints.");
    g_xrCreateDebugUtilsMessengerEXT_fn = nullptr;
    g_xrDestroyDebugUtilsMessengerEXT_fn = nullptr;
    return;
  }

  create_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.userCallback = debug_messenger_func;

  g_xrCreateDebugUtilsMessengerEXT_fn(oxr->instance, &create_info, &oxr->debug_messenger);
}

/**
 * \brief Initialize the window manager XR-Context.
 * Includes setting up the OpenXR instance, querying available extensions and API layers,
 * enabling extensions (currently graphics binding extension only) and API layers.
 */
GHOST_XrContext *GHOST_XrContextCreate(const GHOST_XrContextCreateInfo *create_info)
{
  GHOST_XrContext *xr_context = new GHOST_XrContext();
  OpenXRData *oxr = &xr_context->oxr;

  if (create_info->context_flag & GHOST_kXrContextDebug) {
    xr_context->debug = true;
  }

  XR_DEBUG_PRINTF(xr_context, "Available OpenXR API-layers/extensions:\n");
  if (!openxr_gather_api_layers(xr_context, oxr) || !openxr_gather_extensions(xr_context, oxr)) {
    delete xr_context;
    return nullptr;
  }
  XR_DEBUG_PRINTF(xr_context, "Done printing OpenXR API-layers/extensions.\n");

  xr_context->gpu_binding_type = openxr_graphics_extension_to_enable_get(oxr, create_info);

  assert(xr_context->oxr.instance == XR_NULL_HANDLE);
  openxr_instance_create(xr_context);
  openxr_instance_log_print(xr_context, oxr);
  XR_DEBUG_ONLY_BEGIN(xr_context);
  openxr_instance_init_debug_messenger(xr_context);
  XR_DEBUG_ONLY_END;

  return xr_context;
}

void GHOST_XrContextDestroy(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  xr_context->session = nullptr;

  if (oxr->debug_messenger != XR_NULL_HANDLE) {
    assert(g_xrDestroyDebugUtilsMessengerEXT_fn != nullptr);
    g_xrDestroyDebugUtilsMessengerEXT_fn(oxr->debug_messenger);
  }
  if (oxr->instance != XR_NULL_HANDLE) {
    xrDestroyInstance(oxr->instance);
  }

  delete xr_context;
}

void GHOST_XrSessionStart(GHOST_XrContext *xr_context, const GHOST_XrSessionBeginInfo *begin_info)
{
  if (xr_context->session == nullptr) {
    xr_context->session = std::unique_ptr<GHOST_XrSession>(new GHOST_XrSession(xr_context));
  }

  xr_context->session->start(begin_info);
}

void GHOST_XrSessionEnd(GHOST_XrContext *xr_context)
{
  xr_context->session->end();
  xr_context->session = nullptr;
}

GHOST_TSuccess GHOST_XrSessionIsRunning(const GHOST_XrContext *xr_context)
{
  return (xr_context->session && xr_context->session->isRunning()) ? GHOST_kSuccess :
                                                                     GHOST_kFailure;
}

void GHOST_XrSessionDrawViews(GHOST_XrContext *xr_context, void *draw_customdata)
{
  xr_context->session->draw(draw_customdata);
}

/**
 * Delegates event to session, allowing context to destruct the session if needed.
 */
void GHOST_XrSessionStateChange(GHOST_XrContext *xr_context,
                                const XrEventDataSessionStateChanged *lifecycle)
{
  if (xr_context->session &&
      xr_context->session->handleStateChangeEvent(lifecycle) == GHOST_XrSession::SESSION_DESTROY) {
    xr_context->session = nullptr;
  }
}

/**
 * Set context for binding and unbinding a graphics context for a session. The binding callback
 * may create a new context thereby. In fact that's the sole reason for this callback approach to
 * binding. Just make sure to have an unbind function set that properly destructs.
 *
 * \param bind_fn Function to retrieve (possibly create) a graphics context.
 * \param unbind_fn Function to release (possibly free) a graphics context.
 */
void GHOST_XrGraphicsContextBindFuncs(GHOST_XrContext *xr_context,
                                      GHOST_XrGraphicsContextBindFn bind_fn,
                                      GHOST_XrGraphicsContextUnbindFn unbind_fn)
{
  if (xr_context->session) {
    xr_context->session->unbindGraphicsContext();
  }
  xr_context->gpu_ctx_bind_fn = bind_fn;
  xr_context->gpu_ctx_unbind_fn = unbind_fn;
}

void GHOST_XrDrawViewFunc(struct GHOST_XrContext *xr_context, GHOST_XrDrawViewFn draw_view_fn)
{
  xr_context->draw_view_fn = draw_view_fn;
}
