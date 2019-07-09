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

#include <iostream>

#include "GHOST_C-api.h"
#include "GHOST_Xr_intern.h"

static bool GHOST_XrEventPollNext(OpenXRData *oxr, XrEventDataBuffer &r_event_data)
{
  /* (Re-)initialize as required by specification */
  r_event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
  r_event_data.next = nullptr;

  return (xrPollEvent(oxr->instance, &r_event_data) == XR_SUCCESS);
}

GHOST_TSuccess GHOST_XrEventsHandle(GHOST_XrContext *xr_context)
{
  OpenXRData *oxr = &xr_context->oxr;
  XrEventDataBuffer event_buffer; /* structure big enought to hold all possible events */

  if (xr_context == NULL) {
    return GHOST_kFailure;
  }

  while (GHOST_XrEventPollNext(oxr, event_buffer)) {
    XrEventDataBaseHeader *event = (XrEventDataBaseHeader *)&event_buffer; /* base event struct */

    switch (event->type) {
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        GHOST_XrSessionStateChange(xr_context, (XrEventDataSessionStateChanged *)event);
        return GHOST_kSuccess;

      default:
        XR_DEBUG_PRINTF(xr_context, "Unhandled event: %i\n", event->type);
        return GHOST_kFailure;
    }
  }

  return GHOST_kFailure;
}
