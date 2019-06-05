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

    XrSystemId system_id;
    XrSession session;
    XrSessionState session_state;
  } oxr;
} wmXRContext;

/**
 * \param layer_name May be NULL for extensions not belonging to a specific layer.
 */
static void openxr_gather_extensions_ex(wmXRContext *context, const char *layer_name)
{
  uint32_t extension_count = 0;

  context->oxr.extensions = NULL;

  /* Get count for array creation/init first. */
  xrEnumerateInstanceExtensionProperties(layer_name, 0, &extension_count, NULL);

  if (extension_count == 0) {
    /* Extensions are optional, can safely exit. */
    return;
  }

  context->oxr.extensions = MEM_calloc_arrayN(
      extension_count, sizeof(*context->oxr.extensions), "xrExtensionProperties");
  context->oxr.extension_count = extension_count;
  for (uint i = 0; i < extension_count; i++) {
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
  for (uint i = 0; i < extension_count; i++) {
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
  uint32_t layer_count = 0;

  /* Get count for array creation/init first. */
  xrEnumerateApiLayerProperties(0, &layer_count, NULL);

  if (layer_count == 0) {
    /* Layers are optional, can safely exit. */
    return;
  }

  context->oxr.layers = MEM_calloc_arrayN(
      layer_count, sizeof(*context->oxr.layers), "XrApiLayerProperties");
  context->oxr.layer_count = layer_count;
  for (uint i = 0; i < layer_count; i++) {
    context->oxr.layers[i].type = XR_TYPE_API_LAYER_PROPERTIES;
  }

  /* Actually get the layers. */
  xrEnumerateApiLayerProperties(layer_count, &layer_count, context->oxr.layers);
  for (uint i = 0; i < layer_count; i++) {
#ifdef USE_EXT_LAYER_PRINTS
    printf("Layer: %s\n", context->oxr.layers[i].layerName);
#endif
    /* Each layer may have own extensions */
    openxr_gather_extensions_ex(context, context->oxr.layers[i].layerName);
  }
}

ATTR_NONNULL()
ATTR_WARN_UNUSED_RESULT
static bool openxr_extension_is_available(const wmXRContext *context, const char *extension_name)
{
  for (uint i = 0; i < context->oxr.extension_count; i++) {
    if (STREQ(context->oxr.extensions[i].extensionName, extension_name)) {
      return true;
    }
  }

  return false;
}

/**
 * Gather an array of names for the extensions to enable.
 */
static void openxr_extensions_to_enable_get(const wmXRContext *context,
                                            char ***r_ext_names,
                                            uint *r_ext_count)
{
  const static char *try_ext[] = {
      "XR_KHR_opengl_enable",
#ifdef WIN32
      "XR_KHR_D3D11_enable",
#endif
  };
  const uint try_ext_count = ARRAY_SIZE(try_ext);

  BLI_assert(r_ext_names != NULL);
  BLI_assert(r_ext_count != NULL);

  *r_ext_names = MEM_malloc_arrayN(try_ext_count, sizeof(char *), __func__);

  for (uint i = 0, j = 0; j < try_ext_count; j++) {
    if (openxr_extension_is_available(context, try_ext[j])) {
      *r_ext_names[i++] = try_ext[j];
    }
  }
}

ATTR_NONNULL()
static bool openxr_instance_setup(wmXRContext *context)
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

  openxr_extensions_to_enable_get(
      context, &create_info.enabledExtensionNames, &create_info.enabledExtensionCount);

  xrCreateInstance(&create_info, &context->oxr.instance);

  return true;
}

ATTR_NONNULL()
static void openxr_instance_log_print(wmXRContext *context)
{
  BLI_assert(context->oxr.instance != XR_NULL_HANDLE);

  XrInstanceProperties instanceProperties = {XR_TYPE_INSTANCE_PROPERTIES};
  xrGetInstanceProperties(context->oxr.instance, &instanceProperties);

  printf("Connected to OpenXR runtime: %s\n", instanceProperties.runtimeName);
}

wmXRContext *wm_xr_context_create(void)
{
  wmXRContext *xr_context = MEM_callocN(sizeof(*xr_context), "wmXRContext");

  xr_context->oxr.session_state = XR_SESSION_STATE_UNKNOWN;

  BLI_assert(xr_context->oxr.instance == XR_NULL_HANDLE);
  openxr_instance_setup(xr_context);
  openxr_instance_log_print(xr_context);

  return xr_context;
}

void wm_xr_context_destroy(wmXRContext *xr_context)
{
  xrDestroySession(xr_context->oxr.session);
  xrDestroyInstance(xr_context->oxr.instance);

  MEM_SAFE_FREE(xr_context->oxr.extensions);
  MEM_SAFE_FREE(xr_context->oxr.layers);

  MEM_SAFE_FREE(xr_context);
}

bool wm_xr_session_is_running(const wmXRContext *xr_context)
{
  if (xr_context->oxr.session == XR_NULL_HANDLE) {
    return false;
  }

  return ELEM(xr_context->oxr.session_state,
              XR_SESSION_STATE_RUNNING,
              XR_SESSION_STATE_VISIBLE,
              XR_SESSION_STATE_FOCUSED);
}

/**
 * A system in OpenXR the combination of some sort of HMD plus controllers and whatever other
 * devices are managed through OpenXR. So this attempts to init the HMD and the other devices.
 */
static void wm_xr_system_init(wmXRContext *xr_context)
{
  BLI_assert(xr_context->oxr.instance != XR_NULL_HANDLE);
  BLI_assert(xr_context->oxr.system_id == XR_NULL_SYSTEM_ID);

  XrSystemGetInfo system_info = {.type = XR_TYPE_SYSTEM_GET_INFO};
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

  xrGetSystem(xr_context->oxr.instance, &system_info, &xr_context->oxr.system_id);
}

void wm_xr_session_start(wmXRContext *xr_context)
{
  BLI_assert(xr_context->oxr.instance != XR_NULL_HANDLE);
  BLI_assert(xr_context->oxr.session == XR_NULL_HANDLE);

  wm_xr_system_init(xr_context);

  XrSessionCreateInfo create_info = {.type = XR_TYPE_SESSION_CREATE_INFO};
  create_info.systemId = xr_context->oxr.system_id;
  /* TODO .next needs to point to graphics extension structure  XrGraphicsBindingFoo */
  // create_info.next = ;

  xrCreateSession(xr_context->oxr.instance, &create_info, &xr_context->oxr.session);
}

void wm_xr_session_end(wmXRContext *xr_context)
{
  xrEndSession(xr_context->oxr.session);
  xrDestroySession(xr_context->oxr.session);
}

static void wm_xr_session_state_change(wmXRContext *xr_context,
                                       const XrEventDataSessionStateChanged *lifecycle)
{
  xr_context->oxr.session_state = lifecycle->type;

  switch (lifecycle->type) {
    case XR_SESSION_STATE_READY: {
      XrSessionBeginInfo begin_info = {
          .type = XR_TYPE_SESSION_BEGIN_INFO,
          .primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
      xrBeginSession(xr_context->oxr.session, &begin_info);
      break;
    }
    case XR_SESSION_STATE_STOPPING: {
      BLI_assert(xr_context->oxr.session != XR_NULL_HANDLE);
      xrEndSession(xr_context->oxr.session);
    }
  }
}

static bool wm_xr_event_poll_next(wmXRContext *xr_context, XrEventDataBuffer *r_event_data)
{
  /* (Re-)initialize as required by specification */
  r_event_data->type = XR_TYPE_EVENT_DATA_BUFFER;
  r_event_data->next = NULL;

  return (xrPollEvent(xr_context->oxr.instance, r_event_data) == XR_SUCCESS);
}

bool wm_xr_events_handle(wmXRContext *xr_context)
{
  XrEventDataBuffer event_buffer; /* structure big enought to hold all possible events */

  if (!wm_xr_session_is_running(xr_context)) {
    return false;
  }

  while (wm_xr_event_poll_next(xr_context, &event_buffer)) {
    XrEventDataBaseHeader *event = (XrEventDataBaseHeader *)&event_buffer; /* base event struct */

    switch (event->type) {
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        wm_xr_session_state_change(xr_context, (XrEventDataSessionStateChanged *)&event);
        return true;

      default:
        printf("Unhandled event: %i\n", event->type);
        return false;
    }
  }

  return false;
}
