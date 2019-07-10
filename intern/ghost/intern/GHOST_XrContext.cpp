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

#include "GHOST_Types.h"
#include "GHOST_Xr_intern.h"
#include "GHOST_XrSession.h"

#include "GHOST_XrContext.h"

struct OpenXRInstanceData {
  XrInstance instance{XR_NULL_HANDLE};

  std::vector<XrExtensionProperties> extensions;
  std::vector<XrApiLayerProperties> layers;

  static PFN_xrCreateDebugUtilsMessengerEXT s_xrCreateDebugUtilsMessengerEXT_fn;
  static PFN_xrDestroyDebugUtilsMessengerEXT s_xrDestroyDebugUtilsMessengerEXT_fn;

  XrDebugUtilsMessengerEXT debug_messenger;
};

PFN_xrCreateDebugUtilsMessengerEXT OpenXRInstanceData::s_xrCreateDebugUtilsMessengerEXT_fn =
    nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT OpenXRInstanceData::s_xrDestroyDebugUtilsMessengerEXT_fn =
    nullptr;

GHOST_XrContext::GHOST_XrContext(const GHOST_XrContextCreateInfo *create_info)
    : m_oxr(new OpenXRInstanceData()), m_debug(create_info->context_flag & GHOST_kXrContextDebug)
{
}
GHOST_XrContext::~GHOST_XrContext()
{
  if (m_oxr->debug_messenger != XR_NULL_HANDLE) {
    assert(m_oxr->s_xrDestroyDebugUtilsMessengerEXT_fn != nullptr);
    m_oxr->s_xrDestroyDebugUtilsMessengerEXT_fn(m_oxr->debug_messenger);
  }
  if (m_oxr->instance != XR_NULL_HANDLE) {
    xrDestroyInstance(m_oxr->instance);
  }
}

GHOST_TSuccess GHOST_XrContext::initialize(const GHOST_XrContextCreateInfo *create_info)
{
  XR_DEBUG_PRINTF(this, "Available OpenXR API-layers/extensions:\n");
  if (!enumerateApiLayers() || !enumerateExtensions()) {
    return GHOST_kFailure;
  }
  XR_DEBUG_PRINTF(this, "Done printing OpenXR API-layers/extensions.\n");

  m_gpu_binding_type = determineGraphicsBindingTypeToEnable(create_info);

  assert(m_oxr->instance == XR_NULL_HANDLE);
  createOpenXRInstance();
  printInstanceInfo();
  XR_DEBUG_ONLY_BEGIN(this);
  initDebugMessenger();
  XR_DEBUG_ONLY_END;

  return GHOST_kSuccess;
}

void GHOST_XrContext::createOpenXRInstance()
{
  XrInstanceCreateInfo create_info{};

  create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
  std::string("Blender").copy(create_info.applicationInfo.applicationName,
                              XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  getAPILayersToEnable(m_enabled_layers);
  getExtensionsToEnable(m_enabled_extensions);
  create_info.enabledApiLayerCount = m_enabled_layers.size();
  create_info.enabledApiLayerNames = m_enabled_layers.data();
  create_info.enabledExtensionCount = m_enabled_extensions.size();
  create_info.enabledExtensionNames = m_enabled_extensions.data();

  xrCreateInstance(&create_info, &m_oxr->instance);
}

void GHOST_XrContext::printInstanceInfo()
{
  assert(m_oxr->instance != XR_NULL_HANDLE);

  XrInstanceProperties instanceProperties{};
  instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
  xrGetInstanceProperties(m_oxr->instance, &instanceProperties);

  XR_DEBUG_PRINTF(this, "Connected to OpenXR runtime: %s\n", instanceProperties.runtimeName);
}

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
GHOST_TSuccess GHOST_XrContext::enumerateExtensionsEx(
    std::vector<XrExtensionProperties> &extensions, const char *layer_name)
{
  const unsigned long old_extension_count = extensions.size();
  uint32_t extension_count = 0;

  /* Get count for array creation/init first. */
  if (XR_FAILED(
          xrEnumerateInstanceExtensionProperties(layer_name, 0, &extension_count, nullptr))) {
    return GHOST_kFailure;
  }

  if (extension_count == 0) {
    /* Extensions are optional, can successfully exit. */
    return GHOST_kSuccess;
  }

  for (uint32_t i = 0; i < extension_count; i++) {
    XrExtensionProperties ext{XR_TYPE_EXTENSION_PROPERTIES};
    extensions.push_back(ext);
  }

  if (layer_name) {
    XR_DEBUG_PRINTF(this, "Layer: %s\n", layer_name);
  }

  /* Actually get the extensions. */
  xrEnumerateInstanceExtensionProperties(
      layer_name, extension_count, &extension_count, extensions.data());
  XR_DEBUG_ONLY_BEGIN(this);
  for (uint32_t i = 0; i < extension_count; i++) {
    XR_DEBUG_PRINTF(this, "Extension: %s\n", extensions[i + old_extension_count].extensionName);
  }
  XR_DEBUG_ONLY_END;

  return GHOST_kSuccess;
}
GHOST_TSuccess GHOST_XrContext::enumerateExtensions()
{
  return enumerateExtensionsEx(m_oxr->extensions, nullptr);
}

GHOST_TSuccess GHOST_XrContext::enumerateApiLayers()
{
  uint32_t layer_count = 0;

  /* Get count for array creation/init first. */
  if (XR_FAILED(xrEnumerateApiLayerProperties(0, &layer_count, nullptr))) {
    return GHOST_kFailure;
  }

  if (layer_count == 0) {
    /* Layers are optional, can safely exit. */
    return GHOST_kFailure;
  }

  m_oxr->layers = std::vector<XrApiLayerProperties>(layer_count);
  for (XrApiLayerProperties &layer : m_oxr->layers) {
    layer.type = XR_TYPE_API_LAYER_PROPERTIES;
  }

  /* Actually get the layers. */
  xrEnumerateApiLayerProperties(layer_count, &layer_count, m_oxr->layers.data());
  for (XrApiLayerProperties &layer : m_oxr->layers) {
    XR_DEBUG_PRINTF(this, "Layer: %s\n", layer.layerName);

    /* Each layer may have own extensions */
    enumerateExtensionsEx(m_oxr->extensions, layer.layerName);
  }

  return GHOST_kSuccess;
}

static bool openxr_layer_is_available(const std::vector<XrApiLayerProperties> layers_info,
                                      const std::string &layer_name)
{
  for (const XrApiLayerProperties &layer_info : layers_info) {
    if (layer_info.layerName == layer_name) {
      return true;
    }
  }

  return false;
}
static bool openxr_extension_is_available(const std::vector<XrExtensionProperties> extensions_info,
                                          const std::string &extension_name)
{
  for (const XrExtensionProperties &ext_info : extensions_info) {
    if (ext_info.extensionName == extension_name) {
      return true;
    }
  }

  return false;
}

/**
 * Gather an array of names for the API-layers to enable.
 */
void GHOST_XrContext::getAPILayersToEnable(std::vector<const char *> &r_ext_names)
{
  static std::vector<std::string> try_layers;

  XR_DEBUG_ONLY_BEGIN(this);
  try_layers.push_back("XR_APILAYER_LUNARG_core_validation");
  XR_DEBUG_ONLY_END;

  r_ext_names.reserve(try_layers.size());

  for (const std::string &layer : try_layers) {
    if (openxr_layer_is_available(m_oxr->layers, layer)) {
      r_ext_names.push_back(layer.c_str());
      XR_DEBUG_PRINTF(this, "Enabling OpenXR API-Layer %s\n", layer.c_str());
    }
  }
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
 * Gather an array of names for the extensions to enable.
 */
void GHOST_XrContext::getExtensionsToEnable(std::vector<const char *> &r_ext_names)
{
  assert(m_gpu_binding_type != GHOST_kXrGraphicsUnknown);

  const char *gpu_binding = openxr_ext_name_from_wm_gpu_binding(m_gpu_binding_type);
  static std::vector<std::string> try_ext;
  const auto add_ext = [this, &r_ext_names](const char *ext_name) {
    r_ext_names.push_back(ext_name);
    XR_DEBUG_PRINTF(this, "Enabling OpenXR Extension: %s\n", ext_name);
  };

  XR_DEBUG_ONLY_BEGIN(this);
  /* Try enabling debug extension */
  try_ext.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
  XR_DEBUG_ONLY_END;

  r_ext_names.reserve(try_ext.size() + 1); /* + 1 for graphics binding extension. */

  /* Add graphics binding extension. */
  assert(gpu_binding);
  assert(openxr_extension_is_available(m_oxr->extensions, gpu_binding));
  add_ext(gpu_binding);

  for (const std::string &ext : try_ext) {
    if (openxr_extension_is_available(m_oxr->extensions, ext)) {
      add_ext(ext.c_str());
    }
  }
}

/**
 * Decide which graphics binding extension to use based on
 * #GHOST_XrContextCreateInfo.gpu_binding_candidates and available extensions.
 */
GHOST_TXrGraphicsBinding GHOST_XrContext::determineGraphicsBindingTypeToEnable(
    const GHOST_XrContextCreateInfo *create_info)
{
  assert(create_info->gpu_binding_candidates != NULL);
  assert(create_info->gpu_binding_candidates_count > 0);

  for (uint32_t i = 0; i < create_info->gpu_binding_candidates_count; i++) {
    assert(create_info->gpu_binding_candidates[i] != GHOST_kXrGraphicsUnknown);
    const char *ext_name = openxr_ext_name_from_wm_gpu_binding(
        create_info->gpu_binding_candidates[i]);
    if (openxr_extension_is_available(m_oxr->extensions, ext_name)) {
      return create_info->gpu_binding_candidates[i];
    }
  }

  return GHOST_kXrGraphicsUnknown;
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

void GHOST_XrContext::initDebugMessenger()
{
  XrDebugUtilsMessengerCreateInfoEXT create_info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

  /* Extension functions need to be obtained through xrGetInstanceProcAddr */
  if (XR_FAILED(xrGetInstanceProcAddr(
          m_oxr->instance,
          "xrCreateDebugUtilsMessengerEXT",
          (PFN_xrVoidFunction *)&m_oxr->s_xrCreateDebugUtilsMessengerEXT_fn)) ||
      XR_FAILED(xrGetInstanceProcAddr(
          m_oxr->instance,
          "xrDestroyDebugUtilsMessengerEXT",
          (PFN_xrVoidFunction *)&m_oxr->s_xrDestroyDebugUtilsMessengerEXT_fn))) {
    fprintf(stderr, "Could not use XR_EXT_debug_utils to enable debug prints.");
    m_oxr->s_xrCreateDebugUtilsMessengerEXT_fn = nullptr;
    m_oxr->s_xrDestroyDebugUtilsMessengerEXT_fn = nullptr;
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

  m_oxr->s_xrCreateDebugUtilsMessengerEXT_fn(
      m_oxr->instance, &create_info, &m_oxr->debug_messenger);
}

void GHOST_XrContext::startSession(const GHOST_XrSessionBeginInfo *begin_info)
{
  if (m_session == nullptr) {
    m_session = std::unique_ptr<GHOST_XrSession>(new GHOST_XrSession(this));
  }

  m_session->start(begin_info);
}
void GHOST_XrContext::endSession()
{
  m_session->end();
  m_session = nullptr;
}

bool GHOST_XrContext::isSessionRunning() const
{
  return m_session && m_session->isRunning();
}

void GHOST_XrContext::drawSessionViews(void *draw_customdata)
{
  m_session->draw(draw_customdata);
}

/**
 * Delegates event to session, allowing context to destruct the session if needed.
 */
void GHOST_XrContext::handleSessionStateChange(const XrEventDataSessionStateChanged *lifecycle)
{
  if (m_session &&
      m_session->handleStateChangeEvent(lifecycle) == GHOST_XrSession::SESSION_DESTROY) {
    m_session = nullptr;
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
void GHOST_XrContext::setGraphicsContextBindFuncs(GHOST_XrGraphicsContextBindFn bind_fn,
                                                  GHOST_XrGraphicsContextUnbindFn unbind_fn)
{
  if (m_session) {
    m_session->unbindGraphicsContext();
  }
  m_custom_funcs.gpu_ctx_bind_fn = bind_fn;
  m_custom_funcs.gpu_ctx_unbind_fn = unbind_fn;
}

void GHOST_XrContext::setDrawViewFunc(GHOST_XrDrawViewFn draw_view_fn)
{
  m_custom_funcs.draw_view_fn = draw_view_fn;
}

GHOST_XrCustomFuncs *GHOST_XrContext::getCustomFuncs()
{
  return &m_custom_funcs;
}

GHOST_TXrGraphicsBinding GHOST_XrContext::getGraphicsBindingType()
{
  return m_gpu_binding_type;
}

XrInstance GHOST_XrContext::getInstance() const
{
  return m_oxr->instance;
}

bool GHOST_XrContext::isDebugMode() const
{
  return m_debug;
}
