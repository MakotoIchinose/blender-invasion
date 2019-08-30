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

typedef struct GHOST_XrContext *GHOST_XrContextHandle;

typedef enum { GHOST_kFailure = 0, GHOST_kSuccess } GHOST_TSuccess;

/**
 * The XR view (i.e. the OpenXR runtime) may require a different graphics library than OpenGL. An
 * offscreen texture of the viewport will then be drawn into using OpenGL, but the final texture
 * draw call will happen through another lib (say DirectX).
 *
 * This enum defines the possible graphics bindings to attempt to enable.
 */
typedef enum {
  GHOST_kXrGraphicsUnknown = 0,
  GHOST_kXrGraphicsOpenGL,
#ifdef WIN32
  GHOST_kXrGraphicsD3D11,
#endif
  /* For later */
  //  GHOST_kXrGraphicsVulkan,
} GHOST_TXrGraphicsBinding;
/* An array of GHOST_TXrGraphicsBinding items defining the candidate bindings to use. The first
 * available candidate will be chosen, so order defines priority. */
typedef const GHOST_TXrGraphicsBinding *GHOST_XrGraphicsBindingCandidates;

typedef struct {
  float position[3];
  /* Blender convention (w, x, y, z) */
  float orientation_quat[4];
} GHOST_XrPose;

enum {
  GHOST_kXrContextDebug = (1 << 0),
  GHOST_kXrContextDebugTime = (1 << 1),
};

typedef struct {
  const GHOST_XrGraphicsBindingCandidates gpu_binding_candidates;
  unsigned int gpu_binding_candidates_count;

  unsigned int context_flag;
} GHOST_XrContextCreateInfo;

typedef struct {
  GHOST_XrPose base_pose;
} GHOST_XrSessionBeginInfo;

typedef struct {
  int ofsx, ofsy;
  int width, height;

  GHOST_XrPose pose;

  struct {
    float angle_left, angle_right;
    float angle_up, angle_down;
  } fov;

  /** Set if the buffer should be submitted with a srgb transfer applied. */
  char expects_srgb_buffer;
} GHOST_XrDrawViewInfo;

typedef struct {
  const char *user_message;

  /** File path and line number the error was found at. */
  const char *source_location;

  void *customdata;
} GHOST_XrError;

typedef void (*GHOST_XrErrorHandlerFn)(const GHOST_XrError *);

typedef void *(*GHOST_XrGraphicsContextBindFn)(GHOST_TXrGraphicsBinding graphics_lib);
typedef void (*GHOST_XrGraphicsContextUnbindFn)(GHOST_TXrGraphicsBinding graphics_lib,
                                                void *graphics_context);
typedef void (*GHOST_XrDrawViewFn)(const GHOST_XrDrawViewInfo *draw_view, void *customdata);

#endif  // __VAMR_TYPES_H__
