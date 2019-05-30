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

#if !defined(WITH_OPENXR)
static_assert(false, "WITH_OPENXR not defined, but wm_xr.c is being compiled.");
#endif

typedef struct wmXRContext {
  struct OpenXRData {
    XrInstance instance;
  } oxr;
} wmXRContext;

wmXRContext *wm_xr_context_init(void)
{
  XrInstanceCreateInfo create_info = {.type = XR_TYPE_INSTANCE_CREATE_INFO};
  XrInstance instance = {0};

  wmXRContext *wm_context = MEM_callocN(sizeof(*wm_context), "wmXRContext");

  BLI_assert(wm_context->oxr.instance == XR_NULL_HANDLE);

  BLI_strncpy(
      create_info.applicationInfo.applicationName, "Blender", XR_MAX_APPLICATION_NAME_SIZE);
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  xrCreateInstance(&create_info, &instance);

  return wm_context;
}

void wm_xr_context_destroy(wmXRContext *wm_context)
{
  xrDestroyInstance(wm_context->oxr.instance);
  MEM_SAFE_FREE(wm_context);
}
