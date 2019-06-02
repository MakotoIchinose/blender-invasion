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
 * \ingroup wm
 *
 * Abstraction for XR (VR, AR, MR, ..) access via OpenXR.
 */

#include <string.h>

#include "CLG_log.h"

#include "BKE_context.h"

#include "DNA_windowmanager_types.h"

#include "MEM_guardedalloc.h"

#include "BLI_assert.h"
#include "BLI_compiler_attrs.h"
#include "BLI_string.h"

#include "openxr/openxr.h"

#include "WM_api.h"
#include "WM_types.h"
#include "wm.h"

/* Toggle printing of available OpenXR extensions and API-layers. Should probably be changed to use
 * CLOG at some point */
#define USE_EXT_LAYER_PRINTS

#if !defined(WITH_OPENXR)
static_assert(false, "WITH_OPENXR not defined, but wm_xr.c is being compiled.");
#endif

typedef struct wmXRContext {
  struct OpenXRData {
    XrInstance instance;

    XrExtensionProperties *extensions;
    uint32_t extension_count;

    XrApiLayerProperties *layers;
    uint32_t layer_count;
  } oxr;
} wmXRContext;

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
static void openxr_gather_extensions_ex(wmXRContext *context, const char *layer_name)
{
  uint32_t extension_count;

  context->oxr.extensions = NULL;

  /* Get count for array creation/init first. */
  xrEnumerateInstanceExtensionProperties(layer_name, 0, &extension_count, NULL);

  if (extension_count == 0) {
    /* Extensions are optional, can safely exit. */
    return;
  }

  context->oxr.extensions = MEM_calloc_arrayN(
      extension_count, sizeof(*context->oxr.extensions), "xrExtensionProperties");
  for (int i = 0; i < extension_count; i++) {
    context->oxr.extensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
  }

#ifdef USE_EXT_LAYER_PRINTS
  if (layer_name) {
    printf("Layer: %s\n", layer_name);
  }
#endif
  /* Actually get the extensions. */
  xrEnumerateInstanceExtensionProperties(
      layer_name, extension_count, &extension_count, context->oxr.extensions);
#ifdef USE_EXT_LAYER_PRINTS
  for (int i = 0; i < extension_count; i++) {
    printf("Extension: %s\n", context->oxr.extensions[i].extensionName);
  }
#endif
}
static void openxr_gather_extensions(wmXRContext *context)
{
  openxr_gather_extensions_ex(context, NULL);
}

static void openxr_gather_api_layers(wmXRContext *context)
{
  uint32_t layer_count;

  /* Get count for array creation/init first. */
  xrEnumerateApiLayerProperties(0, &layer_count, NULL);

  if (layer_count == 0) {
    /* Layers are optional, can safely exit. */
    return;
  }

  context->oxr.layers = MEM_calloc_arrayN(
      layer_count, sizeof(*context->oxr.layers), "XrApiLayerProperties");
  for (int i = 0; i < layer_count; i++) {
    context->oxr.layers[i].type = XR_TYPE_API_LAYER_PROPERTIES;
  }

  /* Actually get the layers. */
  xrEnumerateApiLayerProperties(layer_count, &layer_count, context->oxr.layers);
  for (int i = 0; i < layer_count; i++) {
#ifdef USE_EXT_LAYER_PRINTS
    printf("Layer: %s\n", context->oxr.layers[i].layerName);
#endif
    /* Each layer may have own extensions */
    openxr_gather_extensions_ex(context, context->oxr.layers[i].layerName);
  }
}

static bool openxr_instance_setup(wmXRContext *context) ATTR_NONNULL()
{
  XrInstanceCreateInfo create_info = {.type = XR_TYPE_INSTANCE_CREATE_INFO};

#ifdef USE_EXT_LAYER_PRINTS
  puts("Available OpenXR layers/extensions:");
#endif
  openxr_gather_api_layers(context);
  openxr_gather_extensions(context);
#ifdef USE_EXT_LAYER_PRINTS
  puts("Done printing OpenXR layers/extensions.");
#endif

  BLI_strncpy(
      create_info.applicationInfo.applicationName, "Blender", XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  // create_info.enabledExtensionCount = 1;
  create_info.enabledExtensionCount = 0;
  static const uchar *enabled_extensions[] = {// "XR_KHR_D3D11_enable",
                                              // "XR_KHR_opengl_enable"
                                              ""};
  create_info.enabledExtensionNames = enabled_extensions;

  xrCreateInstance(&create_info, &context->oxr.instance);

  return true;
}

wmXRContext *wm_xr_context_create(void)
{
  wmXRContext *wm_context = MEM_callocN(sizeof(*wm_context), "wmXRContext");

  BLI_assert(wm_context->oxr.instance == XR_NULL_HANDLE);
  openxr_instance_setup(wm_context);

  return wm_context;
}

void wm_xr_context_destroy(wmXRContext *wm_context)
{
  xrDestroyInstance(wm_context->oxr.instance);
  MEM_SAFE_FREE(wm_context->oxr.extensions);
  MEM_SAFE_FREE(wm_context->oxr.layers);
  MEM_SAFE_FREE(wm_context);
}
