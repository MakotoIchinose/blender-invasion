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

#include "GHOST_XR_intern.h"

/* Toggle printing of available OpenXR extensions and API-layers. Should probably be changed to use
 * CLOG at some point */
#define USE_EXT_LAYER_PRINTS

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
static bool openxr_gather_extensions_ex(std::vector<XrExtensionProperties> &extensions,
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

#ifdef USE_EXT_LAYER_PRINTS
  if (layer_name) {
    printf("Layer: %s\n", layer_name);
  }
#endif
  /* Actually get the extensions. */
  xrEnumerateInstanceExtensionProperties(
      layer_name, extension_count, &extension_count, extensions.data());
#ifdef USE_EXT_LAYER_PRINTS
  for (uint32_t i = 0; i < extension_count; i++) {
    printf("Extension: %s\n", extensions[i + old_extension_count].extensionName);
  }
#endif

  return true;
}
static bool openxr_gather_extensions(OpenXRData *oxr)
{
  return openxr_gather_extensions_ex(oxr->extensions, nullptr);
}

static bool openxr_gather_api_layers(OpenXRData *oxr)
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
#ifdef USE_EXT_LAYER_PRINTS
    printf("Layer: %s\n", layer.layerName);
#endif
    /* Each layer may have own extensions */
    openxr_gather_extensions_ex(oxr->extensions, layer.layerName);
  }

  return true;
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

static const char *openxr_ext_name_from_wm_gpu_binding(GHOST_TGraphicsBinding binding)
{
  switch (binding) {
    case GHOST_kXRGraphicsOpenGL:
      return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
#ifdef WIN32
    case GHOST_kXRGraphicsD3D11:
      return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
#endif
    case GHOST_kXRGraphicsUnknown:
      assert(false);
      return nullptr;
  }

  return nullptr;
}

/**
 * Decide which graphics binding extension to use based on
 * #GHOST_XRContextCreateInfo.gpu_binding_candidates and available extensions.
 */
static GHOST_TGraphicsBinding openxr_graphics_extension_to_enable_get(
    const OpenXRData *oxr, const GHOST_XRContextCreateInfo *create_info)
{
  assert(create_info->gpu_binding_candidates != NULL);
  assert(create_info->gpu_binding_candidates_count > 0);

  for (uint32_t i = 0; i < create_info->gpu_binding_candidates_count; i++) {
    assert(create_info->gpu_binding_candidates[i] != GHOST_kXRGraphicsUnknown);
    const char *ext_name = openxr_ext_name_from_wm_gpu_binding(
        create_info->gpu_binding_candidates[i]);
    if (openxr_extension_is_available(oxr, ext_name)) {
      return create_info->gpu_binding_candidates[i];
    }
  }

  return GHOST_kXRGraphicsUnknown;
}

/**
 * Gather an array of names for the extensions to enable.
 */
static void openxr_extensions_to_enable_get(const GHOST_XRContext *context,
                                            const OpenXRData *oxr,
                                            std::vector<const char *> &r_ext_names)
{
  assert(context->gpu_binding != GHOST_kXRGraphicsUnknown);

  const char *gpu_binding = openxr_ext_name_from_wm_gpu_binding(context->gpu_binding);
  const static std::vector<std::string> try_ext; /* None yet */

  assert(gpu_binding);

  r_ext_names.reserve(try_ext.size() + 1); /* + 1 for graphics binding extension. */

  /* Add graphics binding extension. */
  r_ext_names.push_back(gpu_binding);

  for (const std::string &ext : try_ext) {
    if (openxr_extension_is_available(oxr, ext)) {
      r_ext_names.push_back(ext.c_str());
    }
  }
}

static bool openxr_instance_create(GHOST_XRContext *context)
{
  XrInstanceCreateInfo create_info{};
  OpenXRData *oxr = &context->oxr;

  create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
  std::string("Blender").copy(create_info.applicationInfo.applicationName,
                              XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  openxr_extensions_to_enable_get(context, oxr, context->enabled_extensions);
  create_info.enabledExtensionCount = context->enabled_extensions.size();
  create_info.enabledExtensionNames = context->enabled_extensions.data();

  xrCreateInstance(&create_info, &oxr->instance);

  return true;
}

static void openxr_instance_log_print(OpenXRData *oxr)
{
  assert(oxr->instance != XR_NULL_HANDLE);

  XrInstanceProperties instanceProperties{};
  instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
  xrGetInstanceProperties(oxr->instance, &instanceProperties);

  printf("Connected to OpenXR runtime: %s\n", instanceProperties.runtimeName);
}

/**
 * \brief Initialize the window manager XR-Context.
 * Includes setting up the OpenXR instance, querying available extensions and API layers, enabling
 * extensions (currently graphics binding extension only) and API layers.
 */
GHOST_XRContext *GHOST_XR_context_create(const GHOST_XRContextCreateInfo *create_info)
{
  GHOST_XRContext *xr_context = new GHOST_XRContext();
  OpenXRData *oxr = &xr_context->oxr;

#ifdef USE_EXT_LAYER_PRINTS
  puts("Available OpenXR layers/extensions:");
#endif
  if (!openxr_gather_api_layers(oxr) || !openxr_gather_extensions(oxr)) {
    delete xr_context;
    return nullptr;
  }
#ifdef USE_EXT_LAYER_PRINTS
  puts("Done printing OpenXR layers/extensions.");
#endif

  xr_context->gpu_binding = openxr_graphics_extension_to_enable_get(oxr, create_info);

  assert(xr_context->oxr.instance == XR_NULL_HANDLE);
  openxr_instance_create(xr_context);
  openxr_instance_log_print(oxr);

  oxr->session_state = XR_SESSION_STATE_UNKNOWN;

  return xr_context;
}

void GHOST_XR_context_destroy(GHOST_XRContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  /* Unbinding may involve destruction, so call here too */
  GHOST_XR_graphics_context_unbind(*xr_context);

  if (oxr->session != XR_NULL_HANDLE) {
    xrDestroySession(oxr->session);
  }
  if (oxr->instance != XR_NULL_HANDLE) {
    xrDestroyInstance(oxr->instance);
  }

  delete xr_context;
}

/**
 * Set context for binding and unbinding a graphics context for a session. The binding callback may
 * create a new context thereby. In fact that's the sole reason for this callback approach to
 * binding. Just make sure to have an unbind function set that properly destructs.
 *
 * \param bind_fn Function to retrieve (possibly create) a graphics context.
 * \param unbind_fn Function to release (possibly free) a graphics context.
 */
void GHOST_XR_graphics_context_bind_funcs(GHOST_XRContext *xr_context,
                                          GHOST_XRGraphicsContextBindFn bind_fn,
                                          GHOST_XRGraphicsContextUnbindFn unbind_fn)
{
  GHOST_XR_graphics_context_unbind(*xr_context);
  xr_context->gpu_ctx_bind_fn = bind_fn;
  xr_context->gpu_ctx_unbind_fn = unbind_fn;
}

void GHOST_XR_graphics_context_bind(GHOST_XRContext &xr_context)
{
  assert(xr_context.gpu_ctx_bind_fn);
  xr_context.gpu_ctx = static_cast<GHOST_Context *>(
      xr_context.gpu_ctx_bind_fn(xr_context.gpu_binding));
}

void GHOST_XR_graphics_context_unbind(GHOST_XRContext &xr_context)
{
  if (xr_context.gpu_ctx_unbind_fn) {
    xr_context.gpu_ctx_unbind_fn(xr_context.gpu_binding, xr_context.gpu_ctx);
  }
  xr_context.gpu_ctx = nullptr;
}
