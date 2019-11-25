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

/** \file BKE_asset_engine.h
 *  \ingroup bke
 */

#ifndef __BKE_ASSET_ENGINE_H__
#define __BKE_ASSET_ENGINE_H__

#ifdef __cplusplus
extern "C" {
#endif

struct AssetUUID;
struct Main;

#define ASSETUUID_SUB_EQUAL(_uuida, _uuidb, _member) \
  (memcmp((_uuida)->_member, (_uuidb)->_member, sizeof((_uuida)->_member)) == 0)

#define ASSETUUID_EQUAL(_uuida, _uuidb) \
  (ASSETUUID_SUB_EQUAL(_uuida, _uuidb, uuid_repository) && \
   ASSETUUID_SUB_EQUAL(_uuida, _uuidb, uuid_asset) && \
   ASSETUUID_SUB_EQUAL(_uuida, _uuidb, uuid_variant) && \
   ASSETUUID_SUB_EQUAL(_uuida, _uuidb, uuid_revision) && \
   ASSETUUID_SUB_EQUAL(_uuida, _uuidb, uuid_view))

/* Various helpers */
unsigned int BKE_asset_uuid_hash(const void *key);
bool BKE_asset_uuid_cmp(const void *a, const void *b);
void BKE_asset_uuid_print(const struct AssetUUID *uuid);

void BKE_asset_main_search(struct Main *bmain, struct AssetUUID *uuid);

#ifdef __cplusplus
}
#endif

#endif /* __BKE_ASSET_ENGINE_H__ */
