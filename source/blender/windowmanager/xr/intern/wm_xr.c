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

#include "MEM_guardedalloc.h"

#include "BLI_assert.h"
#include "BLI_compiler_attrs.h"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "wm_xr_openxr_includes.h"

#include "wm_xr.h"
#include "wm_xr_intern.h"

/* Toggle printing of available OpenXR extensions and API-layers. Should probably be changed to use
 * CLOG at some point */
#define USE_EXT_LAYER_PRINTS

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
static void openxr_gather_extensions_ex(OpenXRData *oxr, const char *layer_name)
{
  uint32_t extension_count = 0;

  oxr->extensions = NULL;

  /* Get count for array creation/init first. */
  xrEnumerateInstanceExtensionProperties(layer_name, 0, &extension_count, NULL);

  if (extension_count == 0) {
    /* Extensions are optional, can safely exit. */
    return;
  }

  oxr->extensions = MEM_calloc_arrayN(
      extension_count, sizeof(*oxr->extensions), "xrExtensionProperties");
  oxr->extension_count = extension_count;
  for (uint i = 0; i < extension_count; i++) {
    oxr->extensions[i].type = XR_TYPE_EXTENSION_PROPERTIES;
  }

#ifdef USE_EXT_LAYER_PRINTS
  if (layer_name) {
    printf("Layer: %s\n", layer_name);
  }
#endif
  /* Actually get the extensions. */
  xrEnumerateInstanceExtensionProperties(
      layer_name, extension_count, &extension_count, oxr->extensions);
#ifdef USE_EXT_LAYER_PRINTS
  for (uint i = 0; i < extension_count; i++) {
    printf("Extension: %s\n", oxr->extensions[i].extensionName);
  }
#endif
}
static void openxr_gather_extensions(OpenXRData *oxr)
{
  openxr_gather_extensions_ex(oxr, NULL);
}

static void openxr_gather_api_layers(OpenXRData *oxr)
{
  uint32_t layer_count = 0;

  /* Get count for array creation/init first. */
  xrEnumerateApiLayerProperties(0, &layer_count, NULL);

  if (layer_count == 0) {
    /* Layers are optional, can safely exit. */
    return;
  }

  oxr->layers = MEM_calloc_arrayN(layer_count, sizeof(*oxr->layers), "XrApiLayerProperties");
  oxr->layer_count = layer_count;
  for (uint i = 0; i < layer_count; i++) {
    oxr->layers[i].type = XR_TYPE_API_LAYER_PROPERTIES;
  }

  /* Actually get the layers. */
  xrEnumerateApiLayerProperties(layer_count, &layer_count, oxr->layers);
  for (uint i = 0; i < layer_count; i++) {
#ifdef USE_EXT_LAYER_PRINTS
    printf("Layer: %s\n", oxr->layers[i].layerName);
#endif
    /* Each layer may have own extensions */
    openxr_gather_extensions_ex(oxr, oxr->layers[i].layerName);
  }
}

ATTR_NONNULL()
ATTR_WARN_UNUSED_RESULT
static bool openxr_extension_is_available(const OpenXRData *oxr, const char *extension_name)
{
  for (uint i = 0; i < oxr->extension_count; i++) {
    if (STREQ(oxr->extensions[i].extensionName, extension_name)) {
      return true;
    }
  }

  return false;
}

static const char *openxr_ext_name_from_wm_gpu_binding(eWM_xrGraphicsBinding binding)
{
  switch (binding) {
    case WM_XR_GRAPHICS_OPENGL:
      return XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
#ifdef WIN32
    case WM_XR_GRAPHICS_D3D11:
      return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
#endif
    case WM_XR_GRAPHICS_UNKNOWN:
      BLI_assert(false);
  }

  return NULL;
}

/**
 * Decide which graphics binding extension to use based on
 * #wmXRContextCreateInfo.gpu_binding_candidates and available extensions.
 */
static eWM_xrGraphicsBinding openxr_graphics_extension_to_enable_get(
    const OpenXRData *oxr, const wmXRContextCreateInfo *create_info)
{
  BLI_assert(create_info->gpu_binding_candidates != NULL);
  BLI_assert(create_info->gpu_binding_candidates_count > 0);

  for (uint i = 0; i < create_info->gpu_binding_candidates_count; i++) {
    BLI_assert(create_info->gpu_binding_candidates[i] != WM_XR_GRAPHICS_UNKNOWN);
    const char *ext_name = openxr_ext_name_from_wm_gpu_binding(
        create_info->gpu_binding_candidates[i]);
    if (openxr_extension_is_available(oxr, ext_name)) {
      return create_info->gpu_binding_candidates[i];
    }
  }

  return WM_XR_GRAPHICS_UNKNOWN;
}

/**
 * Gather an array of names for the extensions to enable.
 */
static void openxr_extensions_to_enable_get(const wmXRContext *context,
                                            const OpenXRData *oxr,
                                            const char ***r_ext_names,
                                            uint *r_ext_count)
{
  BLI_assert(context->gpu_binding != WM_XR_GRAPHICS_UNKNOWN);

  const char *gpu_binding = openxr_ext_name_from_wm_gpu_binding(context->gpu_binding);
  BLI_assert(gpu_binding);

  {
/* Zero sized array is GCC extension, so disable until it grows. */
#if 0
    const static char *try_ext[] = {};
    const uint try_ext_count = ARRAY_SIZE(try_ext);
#else
    const static char **try_ext = NULL;
    const uint try_ext_count = 0;
#endif

    BLI_assert(r_ext_names != NULL);
    BLI_assert(r_ext_count != NULL);

    /* try_ext_count + 1 for graphics binding extension. */
    *r_ext_names = MEM_malloc_arrayN(try_ext_count + 1, sizeof(char *), __func__);

    /* Add graphics binding extension. */
    *r_ext_names[0] = gpu_binding;
    *r_ext_count = 1;

    for (uint i = 1, j = 0; j < try_ext_count; j++) {
      if (openxr_extension_is_available(oxr, try_ext[j])) {
        *r_ext_names[i++] = try_ext[j];
        (*r_ext_count)++;
      }
    }
  }
}

ATTR_NONNULL()
static bool openxr_instance_create(wmXRContext *context)
{
  XrInstanceCreateInfo create_info = {.type = XR_TYPE_INSTANCE_CREATE_INFO};
  OpenXRData *oxr = &context->oxr;

  BLI_strncpy(
      create_info.applicationInfo.applicationName, "Blender", XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  openxr_extensions_to_enable_get(
      context, oxr, &context->enabled_extensions, &create_info.enabledExtensionCount);
  create_info.enabledExtensionNames = context->enabled_extensions;

  xrCreateInstance(&create_info, &oxr->instance);

  return true;
}

ATTR_NONNULL()
static void openxr_instance_log_print(OpenXRData *oxr)
{
  BLI_assert(oxr->instance != XR_NULL_HANDLE);

  XrInstanceProperties instanceProperties = {.type = XR_TYPE_INSTANCE_PROPERTIES};
  xrGetInstanceProperties(oxr->instance, &instanceProperties);

  printf("Connected to OpenXR runtime: %s\n", instanceProperties.runtimeName);
}

/**
 * \brief Initialize the window manager XR-Context.
 * Includes setting up the OpenXR instance, querying available extensions and API layers, enabling
 * extensions (currently graphics binding extension only) and API layers.
 */
wmXRContext *wm_xr_context_create(const wmXRContextCreateInfo *create_info)
{
  wmXRContext *xr_context = MEM_callocN(sizeof(*xr_context), "wmXRContext");
  OpenXRData *oxr = &xr_context->oxr;

#ifdef USE_EXT_LAYER_PRINTS
  puts("Available OpenXR layers/extensions:");
#endif
  openxr_gather_api_layers(oxr);
  openxr_gather_extensions(oxr);
#ifdef USE_EXT_LAYER_PRINTS
  puts("Done printing OpenXR layers/extensions.");
#endif

  xr_context->gpu_binding = openxr_graphics_extension_to_enable_get(oxr, create_info);

  BLI_assert(xr_context->oxr.instance == XR_NULL_HANDLE);
  openxr_instance_create(xr_context);
  openxr_instance_log_print(oxr);

  oxr->session_state = XR_SESSION_STATE_UNKNOWN;

  return xr_context;
}

void wm_xr_context_destroy(wmXRContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;

  if (oxr->session != XR_NULL_HANDLE) {
    xrDestroySession(oxr->session);
  }
  if (oxr->instance != XR_NULL_HANDLE) {
    xrDestroyInstance(oxr->instance);
  }

  MEM_SAFE_FREE(oxr->extensions);
  MEM_SAFE_FREE(oxr->layers);

  MEM_SAFE_FREE(xr_context->enabled_extensions);

  MEM_SAFE_FREE(xr_context);
}
