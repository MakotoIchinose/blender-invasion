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

#ifndef __VAMR_TYPES_H__
#define __VAMR_TYPES_H__

typedef struct VAMR_Context__ {
  int dummy;
} * VAMR_ContextHandle;

typedef enum { VAMR_Failure = 0, VAMR_Success } VAMR_TSuccess;

/**
 * The XR view (i.e. the OpenXR runtime) may require a different graphics library than OpenGL. An
 * offscreen texture of the viewport will then be drawn into using OpenGL, but the final texture
 * draw call will happen through another lib (say DirectX).
 *
 * This enum defines the possible graphics bindings to attempt to enable.
 */
typedef enum {
  VAMR_GraphicsBindingTypeUnknown = 0,
  VAMR_GraphicsBindingTypeOpenGL,
#ifdef WIN32
  VAMR_GraphicsBindingTypeD3D11,
#endif
  /* For later */
  //  VAMR_GraphicsBindingVulkan,
} VAMR_GraphicsBindingType;
/* An array of VAMR_GraphicsBindingType items defining the candidate bindings to use. The first
 * available candidate will be chosen, so order defines priority. */
typedef const VAMR_GraphicsBindingType *VAMR_GraphicsBindingCandidates;

typedef struct {
  float position[3];
  /* Blender convention (w, x, y, z) */
  float orientation_quat[4];
} VAMR_Pose;

enum {
  VAMR_ContextDebug = (1 << 0),
  VAMR_ContextDebugTime = (1 << 1),
};

typedef struct {
  const VAMR_GraphicsBindingCandidates gpu_binding_candidates;
  unsigned int gpu_binding_candidates_count;

  unsigned int context_flag;
} VAMR_ContextCreateInfo;

typedef struct {
  VAMR_Pose base_pose;
} VAMR_SessionBeginInfo;

typedef struct {
  int ofsx, ofsy;
  int width, height;

  VAMR_Pose pose;

  struct {
    float angle_left, angle_right;
    float angle_up, angle_down;
  } fov;

  /** Set if the buffer should be submitted with a srgb transfer applied. */
  char expects_srgb_buffer;
} VAMR_DrawViewInfo;

typedef struct {
  const char *user_message;

  /** File path and line number the error was found at. */
  const char *source_location;

  void *customdata;
} VAMR_Error;

typedef void (*VAMR_ErrorHandlerFn)(const VAMR_Error *);

typedef void *(*VAMR_GraphicsContextBindFn)(VAMR_GraphicsBindingType graphics_lib);
typedef void (*VAMR_GraphicsContextUnbindFn)(VAMR_GraphicsBindingType graphics_lib,
                                             void *graphics_context);
typedef void (*VAMR_DrawViewFn)(const VAMR_DrawViewInfo *draw_view, void *customdata);

#endif  // __VAMR_TYPES_H__
