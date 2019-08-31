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
 *
 * Abstraction for XR (VR, AR, MR, ..) access via OpenXR.
 */

#include <cassert>
#include <string>

#include "VAMR_capi.h"

#include "openxr_includes.h"

#include "Context.h"
#include "Exception.h"
#include "utils.h"

using namespace VAMR;

/**
 * \brief Initialize the XR-Context.
 * Includes setting up the OpenXR instance, querying available extensions and API layers,
 * enabling extensions (currently graphics binding extension only) and API layers.
 */
VAMR_ContextHandle VAMR_ContextCreate(const VAMR_ContextCreateInfo *create_info)
{
  Context *xr_context = new Context(create_info);

  // TODO VAMR_Context's should probably be owned by the GHOST_System, which will handle context
  // creation and destruction. Try-catch logic can be moved to C-API then.
  try {
    xr_context->initialize(create_info);
  }
  catch (Exception &e) {
    xr_context->dispatchErrorMessage(&e);
    delete xr_context;

    return nullptr;
  }

  return (VAMR_ContextHandle)xr_context;
}

void VAMR_ContextDestroy(VAMR_ContextHandle xr_contexthandle)
{
  delete (Context *)xr_contexthandle;
}

void VAMR_ErrorHandler(VAMR_ErrorHandlerFn handler_fn, void *customdata)
{
  Context::setErrorHandler(handler_fn, customdata);
}

VAMR_TSuccess VAMR_EventsHandle(VAMR_ContextHandle xr_contexthandle)
{
  return VAMR_EventsHandle((Context *)xr_contexthandle);
}
