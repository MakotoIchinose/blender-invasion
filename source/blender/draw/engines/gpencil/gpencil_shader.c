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
 * Copyright 2019, Blender Foundation.
 */

/** \file
 * \ingroup draw
 */
#include "DRW_render.h"

#include "gpencil_engine.h"

extern char datatoc_gpencil_common_lib_glsl[];
extern char datatoc_gpencil_frag_glsl[];
extern char datatoc_gpencil_vert_glsl[];
extern char datatoc_gpencil_composite_frag_glsl[];

extern char datatoc_common_colormanagement_lib_glsl[];
extern char datatoc_common_view_lib_glsl[];

struct GPUShader *GPENCIL_shader_geometry_get(GPENCIL_e_data *e_data)
{
  if (!e_data->gpencil_sh) {
    e_data->gpencil_sh = GPU_shader_create_from_arrays({
        .vert =
            (const char *[]){
                datatoc_common_view_lib_glsl,
                datatoc_gpencil_common_lib_glsl,
                datatoc_gpencil_vert_glsl,
                NULL,
            },
        .frag =
            (const char *[]){
                datatoc_common_colormanagement_lib_glsl,
                datatoc_gpencil_common_lib_glsl,
                datatoc_gpencil_frag_glsl,
                NULL,
            },
        .defs =
            (const char *[]){
                "#define GPENCIL_MATERIAL_BUFFER_LEN " STRINGIFY(GPENCIL_MATERIAL_BUFFER_LEN) "\n",
                NULL,
            },
    });
  }
  return e_data->gpencil_sh;
}

struct GPUShader *GPENCIL_shader_composite_get(GPENCIL_e_data *e_data)
{
  if (!e_data->composite_sh) {
    e_data->composite_sh = DRW_shader_create_fullscreen(datatoc_gpencil_composite_frag_glsl, NULL);
  }
  return e_data->composite_sh;
}
