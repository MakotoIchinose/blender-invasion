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

#include "VAMR_Types.h"
#include "VAMR_capi.h"
#include "VAMR_IContext.h"

#include "Exception.h"

#define VAMR_CAPI_CALL(call, ctx) \
  try { \
    call; \
  } \
  catch (VAMR::Exception & e) { \
    (ctx)->dispatchErrorMessage(&e); \
  }

#define VAMR_CAPI_CALL_RET(call, ctx) \
  try { \
    return call; \
  } \
  catch (VAMR::Exception & e) { \
    (ctx)->dispatchErrorMessage(&e); \
  }

void VAMR_SessionStart(VAMR_ContextHandle xr_contexthandle,
                       const VAMR_SessionBeginInfo *begin_info)
{
  VAMR::IContext *xr_context = (VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL(xr_context->startSession(begin_info), xr_context);
}

void VAMR_SessionEnd(VAMR_ContextHandle xr_contexthandle)
{
  VAMR::IContext *xr_context = (VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL(xr_context->endSession(), xr_context);
}

int VAMR_SessionIsRunning(const VAMR_ContextHandle xr_contexthandle)
{
  const VAMR::IContext *xr_context = (const VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL_RET(xr_context->isSessionRunning(), xr_context);
  return 0;  // Only reached if exception is thrown.
}

void VAMR_SessionDrawViews(VAMR_ContextHandle xr_contexthandle, void *draw_customdata)
{
  VAMR::IContext *xr_context = (VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL(xr_context->drawSessionViews(draw_customdata), xr_context);
}

void VAMR_GraphicsContextBindFuncs(VAMR_ContextHandle xr_contexthandle,
                                   VAMR_GraphicsContextBindFn bind_fn,
                                   VAMR_GraphicsContextUnbindFn unbind_fn)
{
  VAMR::IContext *xr_context = (VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL(xr_context->setGraphicsContextBindFuncs(bind_fn, unbind_fn), xr_context);
}

void VAMR_DrawViewFunc(VAMR_ContextHandle xr_contexthandle, VAMR_DrawViewFn draw_view_fn)
{
  VAMR::IContext *xr_context = (VAMR::IContext *)xr_contexthandle;
  VAMR_CAPI_CALL(xr_context->setDrawViewFunc(draw_view_fn), xr_context);
}
