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

#ifndef __VAMR_ICONTEXT_H__
#define __VAMR_ICONTEXT_H__

#include "VAMR_Types.h"

namespace VAMR {

class IContext {
 public:
  virtual ~IContext() = default;

  virtual void startSession(const VAMR_SessionBeginInfo *begin_info) = 0;
  virtual void endSession() = 0;
  virtual bool isSessionRunning() const = 0;
  virtual void drawSessionViews(void *draw_customdata) = 0;

  virtual void dispatchErrorMessage(const class Exception *) const = 0;

  virtual void setGraphicsContextBindFuncs(VAMR_GraphicsContextBindFn bind_fn,
                                           VAMR_GraphicsContextUnbindFn unbind_fn) = 0;
  virtual void setDrawViewFunc(VAMR_DrawViewFn draw_view_fn) = 0;
};

}  // namespace VAMR

#endif  // __VAMR_ICONTEXT_H__
