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

#include <iostream>

#include "VAMR_Types.h"
#include "VAMR_intern.h"
#include "VAMR_capi.h"
#include "VAMR_Context.h"

namespace VAMR {

static bool VAMR_EventPollNext(XrInstance instance, XrEventDataBuffer &r_event_data)
{
  /* (Re-)initialize as required by specification */
  r_event_data.type = XR_TYPE_EVENT_DATA_BUFFER;
  r_event_data.next = nullptr;

  return (xrPollEvent(instance, &r_event_data) == XR_SUCCESS);
}

::VAMR_TSuccess VAMR_EventsHandle(Context *xr_context)
{
  XrEventDataBuffer event_buffer; /* structure big enought to hold all possible events */

  if (xr_context == NULL) {
    return VAMR_Failure;
  }

  while (VAMR_EventPollNext(xr_context->getInstance(), event_buffer)) {
    XrEventDataBaseHeader *event = (XrEventDataBaseHeader *)&event_buffer; /* base event struct */

    switch (event->type) {
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
        xr_context->handleSessionStateChange((XrEventDataSessionStateChanged *)event);
        return VAMR_Success;

      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
        VAMR_ContextDestroy((VAMR_ContextHandle)xr_context);
        return VAMR_Success;
      default:
        XR_DEBUG_PRINTF(xr_context, "Unhandled event: %i\n", event->type);
        return VAMR_Failure;
    }
  }

  return VAMR_Failure;
}

}  // namespace VAMR
