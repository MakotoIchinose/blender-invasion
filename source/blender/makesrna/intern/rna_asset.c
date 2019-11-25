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
 * Contributor(s): Blender Foundation (2015)
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/makesrna/intern/rna_asset.c
 *  \ingroup RNA
 */

#include "BLI_utildefines.h"

#include "DNA_ID.h"

#include "RNA_define.h"

#include "rna_internal.h"

#include "BKE_asset_engine.h"

#ifdef RNA_RUNTIME

#  include "MEM_guardedalloc.h"

#  include "RNA_access.h"

/* AssetUUID */

static void rna_AssetUUID_preview_size_get(PointerRNA *ptr, int *values)
{
  AssetUUID *uuid = ptr->data;

  values[0] = uuid->width;
  values[1] = uuid->height;
}

static void rna_AssetUUID_preview_size_set(PointerRNA *ptr, const int *values)
{
  AssetUUID *uuid = ptr->data;

  uuid->width = values[0];
  uuid->height = values[1];

  MEM_SAFE_FREE(uuid->ibuff);
}

static int rna_AssetUUID_preview_pixels_get_length(PointerRNA *ptr,
                                                   int length[RNA_MAX_ARRAY_DIMENSION])
{
  AssetUUID *uuid = ptr->data;

  length[0] = (uuid->ibuff == NULL) ? 0 : uuid->width * uuid->height;

  return length[0];
}

static void rna_AssetUUID_preview_pixels_get(PointerRNA *ptr, int *values)
{
  AssetUUID *uuid = ptr->data;

  memcpy(values, uuid->ibuff, uuid->width * uuid->height * sizeof(unsigned int));
}

static void rna_AssetUUID_preview_pixels_set(PointerRNA *ptr, const int *values)
{
  AssetUUID *uuid = ptr->data;

  if (!uuid->ibuff) {
    uuid->ibuff = MEM_mallocN(sizeof(*uuid->ibuff) * 4 * uuid->width * uuid->height, __func__);
  }

  memcpy(uuid->ibuff, values, uuid->width * uuid->height * sizeof(unsigned int));
}

#else /* RNA_RUNTIME */

/* Much lighter version of asset/variant/revision identifier. */
static void rna_def_asset_uuid(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  int null_uuid[4] = {0};

  srna = RNA_def_struct(brna, "AssetUUID", NULL);
  RNA_def_struct_sdna(srna, "AssetUUID");
  RNA_def_struct_ui_text(
      srna, "Asset UUID", "A unique identifier of an asset (asset engine dependent!)");

  RNA_def_int_vector(srna,
                     "uuid_repository",
                     4,
                     null_uuid,
                     INT_MIN,
                     INT_MAX,
                     "Repository UUID",
                     "Unique identifier of the repository of this asset",
                     INT_MIN,
                     INT_MAX);

  RNA_def_int_vector(srna,
                     "uuid_asset",
                     4,
                     null_uuid,
                     INT_MIN,
                     INT_MAX,
                     "Asset UUID",
                     "Unique identifier of this asset",
                     INT_MIN,
                     INT_MAX);

  RNA_def_int_vector(srna,
                     "uuid_variant",
                     4,
                     null_uuid,
                     INT_MIN,
                     INT_MAX,
                     "Variant UUID",
                     "Unique identifier of this asset's variant",
                     INT_MIN,
                     INT_MAX);

  RNA_def_int_vector(srna,
                     "uuid_revision",
                     4,
                     null_uuid,
                     INT_MIN,
                     INT_MAX,
                     "Revision UUID",
                     "Unique identifier of this asset's revision",
                     INT_MIN,
                     INT_MAX);

  RNA_def_int_vector(srna,
                     "uuid_view",
                     4,
                     null_uuid,
                     INT_MIN,
                     INT_MAX,
                     "View UUID",
                     "Unique identifier of this asset's view",
                     INT_MIN,
                     INT_MAX);

  prop = RNA_def_boolean(srna,
                         "is_unknown_engine",
                         false,
                         "Unknown Asset Engine",
                         "This AssetUUID is referencing an unknown asset engine");
  RNA_def_property_boolean_sdna(prop, NULL, "tag", UUID_TAG_ENGINE_MISSING);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);

  prop = RNA_def_boolean(srna,
                         "is_asset_missing",
                         false,
                         "Missing Asset",
                         "This AssetUUID is no more known by its asset engine");
  RNA_def_property_boolean_sdna(prop, NULL, "tag", UUID_TAG_ASSET_MISSING);

  prop = RNA_def_boolean(srna,
                         "use_asset_reload",
                         false,
                         "Reload Asset",
                         "The data matching this AssetUUID should be reloaded");
  RNA_def_property_boolean_sdna(prop, NULL, "tag", UUID_TAG_ASSET_RELOAD);

  prop = RNA_def_boolean(
      srna, "has_asset_preview", false, "Valid Preview", "This asset has a valid preview");
  RNA_def_property_boolean_negative_sdna(prop, NULL, "tag", UUID_TAG_ASSET_NOPREVIEW);

  prop = RNA_def_int_vector(
      srna, "preview_size", 2, NULL, 0, 0, "Preview Size", "Width and height in pixels", 0, 0);
  RNA_def_property_subtype(prop, PROP_PIXEL);
  RNA_def_property_int_funcs(
      prop, "rna_AssetUUID_preview_size_get", "rna_AssetUUID_preview_size_set", NULL);

  prop = RNA_def_property(srna, "preview_pixels", PROP_INT, PROP_NONE);
  RNA_def_property_flag(prop, PROP_DYNAMIC);
  RNA_def_property_multi_array(prop, 1, NULL);
  RNA_def_property_ui_text(
      prop, "Preview Pixels", "Preview pixels, as bytes (always RGBA 32bits)");
  RNA_def_property_dynamic_array_funcs(prop, "rna_AssetUUID_preview_pixels_get_length");
  RNA_def_property_int_funcs(
      prop, "rna_AssetUUID_preview_pixels_get", "rna_AssetUUID_preview_pixels_set", NULL);

  prop = RNA_def_property(srna, "id", PROP_POINTER, PROP_NONE);
  RNA_def_property_struct_type(prop, "ID");
  RNA_def_property_flag(prop, PROP_PTR_NO_OWNERSHIP);
  RNA_def_property_ui_text(
      prop, "Loaded ID", "Pointer to linked/appended data-block, for load_post callback only");
}

void RNA_def_asset(BlenderRNA *brna)
{
  rna_def_asset_uuid(brna);
}

#endif /* RNA_RUNTIME */
