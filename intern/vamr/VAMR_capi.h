#ifndef __VAMR_CAPI_H__
#define __VAMR_CAPI_H__

#include "VAMR_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* xr-context */

/**
 * Set a custom callback to be executed whenever an error occurs. Should be set before calling
 * #GHOST_XrContextCreate().
 */
void GHOST_XrErrorHandler(GHOST_XrErrorHandlerFn handler_fn, void *customdata);

GHOST_XrContextHandle GHOST_XrContextCreate(const GHOST_XrContextCreateInfo *create_info);
void GHOST_XrContextDestroy(GHOST_XrContextHandle xr_context);

void GHOST_XrGraphicsContextBindFuncs(GHOST_XrContextHandle xr_context,
                                      GHOST_XrGraphicsContextBindFn bind_fn,
                                      GHOST_XrGraphicsContextUnbindFn unbind_fn);

void GHOST_XrDrawViewFunc(GHOST_XrContextHandle xr_context, GHOST_XrDrawViewFn draw_view_fn);

/* sessions */
int GHOST_XrSessionIsRunning(const GHOST_XrContextHandle xr_context);
void GHOST_XrSessionStart(GHOST_XrContextHandle xr_context,
                          const GHOST_XrSessionBeginInfo *begin_info);
void GHOST_XrSessionEnd(GHOST_XrContextHandle xr_context);
void GHOST_XrSessionDrawViews(GHOST_XrContextHandle xr_context, void *customdata);

/* events */
GHOST_TSuccess GHOST_XrEventsHandle(GHOST_XrContextHandle xr_context);

#ifdef __cplusplus
}
#endif

#endif  // __VAMR_CAPI_H__
