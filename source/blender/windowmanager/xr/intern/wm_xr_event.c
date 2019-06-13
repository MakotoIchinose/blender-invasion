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
 */

#include <stdio.h>

#include "BLI_compiler_attrs.h"
#include "BLI_utildefines.h"

#include "wm_xr_openxr_includes.h"

#include "wm_xr.h"
#include "wm_xr_intern.h"

static bool wm_xr_event_poll_next(OpenXRData *oxr, XrEventDataBuffer *r_event_data)
{
  /* (Re-)initialize as required by specification */
  r_event_data->type = XR_TYPE_EVENT_DATA_BUFFER;
  r_event_data->next = NULL;

  return (xrPollEvent(oxr->instance, r_event_data) == XR_SUCCESS);
}

bool wm_xr_events_handle(wmXRContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  XrEventDataBuffer event_buffer; /* structure big enought to hold all possible events */

  if (!wm_xr_session_is_running(xr_context)) {
    return false;
  }

  while (wm_xr_event_poll_next(oxr, &event_buffer)) {
    XrEventDataBaseHeader *event = (XrEventDataBaseHeader *)&event_buffer; /* base event struct */

    switch (event->type) {
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        wm_xr_session_state_change(oxr, (XrEventDataSessionStateChanged *)&event);
        return true;

      default:
        printf("Unhandled event: %u\n", event->type);
        return false;
    }
  }

  return false;
}
