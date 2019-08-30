#ifndef __VAMR_CAPI_H__
#define __VAMR_CAPI_H__

#include "VAMR_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* xr-context */

/**
 * Set a custom callback to be executed whenever an error occurs. Should be set before calling
 * #VAMR_ContextCreate().
 */
void VAMR_ErrorHandler(VAMR_ErrorHandlerFn handler_fn, void *customdata);

VAMR_ContextHandle VAMR_ContextCreate(const VAMR_ContextCreateInfo *create_info);
void VAMR_ContextDestroy(VAMR_ContextHandle xr_context);

void VAMR_GraphicsContextBindFuncs(VAMR_ContextHandle xr_context,
                                   VAMR_GraphicsContextBindFn bind_fn,
                                   VAMR_GraphicsContextUnbindFn unbind_fn);

void VAMR_DrawViewFunc(VAMR_ContextHandle xr_context, VAMR_DrawViewFn draw_view_fn);

/* sessions */
int VAMR_SessionIsRunning(const VAMR_ContextHandle xr_context);
void VAMR_SessionStart(VAMR_ContextHandle xr_context, const VAMR_SessionBeginInfo *begin_info);
void VAMR_SessionEnd(VAMR_ContextHandle xr_context);
void VAMR_SessionDrawViews(VAMR_ContextHandle xr_context, void *customdata);

/* events */
VAMR_TSuccess VAMR_EventsHandle(VAMR_ContextHandle xr_context);

#ifdef __cplusplus
}
#endif

#endif  // __VAMR_CAPI_H__
