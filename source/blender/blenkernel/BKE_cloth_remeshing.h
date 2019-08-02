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
 *
 * The Original Code is Copyright (C) Blender Foundation.
 * All rights reserved.
 */
#ifndef __BKE_CLOTH_REMESHING_H__
#define __BKE_CLOTH_REMESHING_H__

/** \file
 * \ingroup bke
 */

#include <float.h>
#include "BLI_math_inline.h"

#define DO_INLINE MALWAYS_INLINE

struct ClothModifierData;
struct Mesh;
struct Object;
struct ClothSizing;

#ifdef __cplusplus
extern "C" {
#endif

struct Mesh *cloth_remeshing_step(struct Depsgraph *depsgraph,
                                  struct Object *ob,
                                  struct ClothModifierData *clmd,
                                  struct Mesh *mesh);
CustomData_MeshMasks cloth_remeshing_get_cd_mesh_masks(void);

#ifdef __cplusplus
}

/**
 *The definition of sizing used for remeshing
 */

/* TODO(Ish): figure out how to write a wrapper that can be used in c when ClothSizing is converted
 * to a class */
typedef struct ClothSizing {
  float m[2][2];
  ClothSizing()
  {
    zero_m2(m);
  }
  ClothSizing(float a[2][2])
  {
    copy_m2_m2(m, a);
  }
  ClothSizing &operator+=(const ClothSizing &size);
  ClothSizing &operator/=(float value);
  ClothSizing operator*(float value);
} ClothSizing;

#endif
#endif
