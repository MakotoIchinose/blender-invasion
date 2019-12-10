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
 * \ingroup draw_engine
 */

#include "DRW_render.h"

#include "BLI_memblock.h"

#include "GPU_uniformbuffer.h"

#include "gpencil_engine.h"

static GPENCIL_MaterialPool *gpencil_material_pool_add(GPENCIL_PrivateData *pd)
{
  GPENCIL_MaterialPool *matpool = BLI_memblock_alloc(pd->gp_material_pool);
  matpool->next = NULL;
  if (matpool->ubo == NULL) {
    matpool->ubo = GPU_uniformbuffer_create(sizeof(matpool->mat_data), NULL, NULL);
  }
  pd->last_material_pool = matpool;
  return matpool;
}

/**
 * Creates a linked list of material pool containing all materials assigned for a given object.
 * We merge the material pools together if object does not contain a huge amount of materials.
 * Also return an offset to the first material of the object in the ubo.
 **/
GPENCIL_MaterialPool *gpencil_material_pool_create(GPENCIL_PrivateData *pd, Object *ob, int *ofs)
{
  GPENCIL_MaterialPool *matpool = pd->last_material_pool;

  /* TODO reuse matpool for objects with small material count. */
  if (true) {
    matpool = gpencil_material_pool_add(pd);
  }

  GPENCIL_MaterialPool *pool = matpool;
  for (int i = 0; i < ob->totcol; i++) {
    if ((i > 0) && (i % GPENCIL_MATERIAL_BUFFER_LEN) == 0) {
      pool->next = gpencil_material_pool_add(pd);
      pool = pool->next;
    }
    gpMaterial *mat_data = &pool->mat_data[i % GPENCIL_MATERIAL_BUFFER_LEN];
    MaterialGPencilStyle *gp_style = BKE_material_gpencil_settings_get(ob, i + 1);
    copy_v4_v4(mat_data->stroke_color, gp_style->stroke_rgba);
    copy_v4_v4(mat_data->fill_color, gp_style->fill_rgba);
  }

  *ofs = 0;

  return matpool;
}

GPUUniformBuffer *gpencil_material_ubo_get(GPENCIL_MaterialPool *first_pool, int mat_id)
{
  GPENCIL_MaterialPool *matpool = first_pool;
  int pool_id = mat_id / GPENCIL_MATERIAL_BUFFER_LEN;
  for (int i = 0; i < pool_id; i++) {
    matpool = matpool->next;
  }
  return matpool->ubo;
}

void gpencil_material_pool_free(void *storage)
{
  GPENCIL_MaterialPool *matpool = (GPENCIL_MaterialPool *)storage;
  DRW_UBO_FREE_SAFE(matpool->ubo);
}

static void gpencil_view_layer_data_free(void *storage)
{
  GPENCIL_ViewLayerData *vldata = (GPENCIL_ViewLayerData *)storage;

  BLI_memblock_destroy(vldata->gp_material_pool, gpencil_material_pool_free);
  BLI_memblock_destroy(vldata->gp_object_pool, NULL);
  BLI_memblock_destroy(vldata->gp_layer_pool, NULL);
  BLI_memblock_destroy(vldata->gp_vfx_pool, NULL);
}

GPENCIL_ViewLayerData *GPENCIL_view_layer_data_ensure(void)
{
  GPENCIL_ViewLayerData **vldata = (GPENCIL_ViewLayerData **)DRW_view_layer_engine_data_ensure(
      &draw_engine_gpencil_type, gpencil_view_layer_data_free);

  /* NOTE(fclem) Putting this stuff in viewlayer means it is shared by all viewports.
   * For now it is ok, but in the future, it could become a problem if we implement
   * the caching system. */
  if (*vldata == NULL) {
    *vldata = MEM_callocN(sizeof(**vldata), "GPENCIL_ViewLayerData");

    (*vldata)->gp_material_pool = BLI_memblock_create(sizeof(GPENCIL_MaterialPool));
    (*vldata)->gp_object_pool = BLI_memblock_create(sizeof(GPENCIL_tObject));
    (*vldata)->gp_layer_pool = BLI_memblock_create(sizeof(GPENCIL_tLayer));
    (*vldata)->gp_vfx_pool = BLI_memblock_create(sizeof(GPENCIL_tVfx));
  }

  return *vldata;
}
