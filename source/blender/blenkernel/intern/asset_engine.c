/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
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
 * The Original Code is Copyright (C) 2015 Blender Foundation.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/blenkernel/intern/asset_engine.c
 *  \ingroup bke
 */

#include <string.h>

#include "BLI_utildefines.h"
#include "BLI_hash_mm2a.h"
#include "BLI_listbase.h"

#include "BKE_asset_engine.h"
#include "BKE_main.h"

#include "IMB_imbuf.h"

#include "DNA_ID.h"

#include "MEM_guardedalloc.h"

#ifdef WITH_PYTHON
#  include "BPY_extern.h"
#endif

/* Various helpers */
void BKE_asset_uuid_runtime_reset(AssetUUID *uuid) {
  uuid->ibuff = NULL;
  uuid->width = uuid->height = 0;
  uuid->tag = 0;
  uuid->id = NULL;
}

unsigned int BKE_asset_uuid_hash(const void *key)
{
  BLI_HashMurmur2A mm2a;
  const AssetUUID *uuid = key;
  BLI_hash_mm2a_init(&mm2a, 0);
  BLI_hash_mm2a_add(&mm2a, (const uchar *)uuid->uuid_repository, sizeof(uuid->uuid_repository));
  BLI_hash_mm2a_add(&mm2a, (const uchar *)uuid->uuid_asset, sizeof(uuid->uuid_asset));
  BLI_hash_mm2a_add(&mm2a, (const uchar *)uuid->uuid_variant, sizeof(uuid->uuid_variant));
  BLI_hash_mm2a_add(&mm2a, (const uchar *)uuid->uuid_revision, sizeof(uuid->uuid_revision));
  BLI_hash_mm2a_add(&mm2a, (const uchar *)uuid->uuid_view, sizeof(uuid->uuid_view));
  return BLI_hash_mm2a_end(&mm2a);
}

bool BKE_asset_uuid_cmp(const void *a, const void *b)
{
  const AssetUUID *uuid1 = a;
  const AssetUUID *uuid2 = b;
  return !ASSETUUID_EQUAL(uuid1, uuid2); /* Expects false when compared equal. */
}

void BKE_asset_uuid_print(const AssetUUID *uuid)
{
  /* TODO print nicer (as 128bit hexadecimal?). */
  printf("[%.10d,%.10d,%.10d,%.10d]"
         "[%.10d,%.10d,%.10d,%.10d]"
         "[%.10d,%.10d,%.10d,%.10d]"
         "[%.10d,%.10d,%.10d,%.10d]"
         "[%.10d,%.10d,%.10d,%.10d]\n",
         uuid->uuid_repository[0],
         uuid->uuid_repository[1],
         uuid->uuid_repository[2],
         uuid->uuid_repository[3],
         uuid->uuid_asset[0],
         uuid->uuid_asset[1],
         uuid->uuid_asset[2],
         uuid->uuid_asset[3],
         uuid->uuid_variant[0],
         uuid->uuid_variant[1],
         uuid->uuid_variant[2],
         uuid->uuid_variant[3],
         uuid->uuid_revision[0],
         uuid->uuid_revision[1],
         uuid->uuid_revision[2],
         uuid->uuid_revision[3],
         uuid->uuid_view[0],
         uuid->uuid_view[1],
         uuid->uuid_view[2],
         uuid->uuid_view[3]);
}

/** Search the whole Main for a given asset uuid.
 *
 * \note if found, ID is put into uuid->id pointer. */
void BKE_asset_main_search(Main *bmain, AssetUUID *uuid)
{
  uuid->id = NULL;

  ID *id;
  FOREACH_MAIN_ID_BEGIN (bmain, id) {
    if (id->uuid == NULL) {
      continue;
    }
    if (ASSETUUID_EQUAL(id->uuid, uuid)) {
      uuid->id = id;
      return;
    }
  }
  FOREACH_MAIN_ID_END;
}
