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

/** \file \ingroup blenloader
 *
 * Support for version patching DNA, there are two kinds.
 *
 * - #DNA_MAKESDNA defined:
 *   Defines store files with old names but run-time uses new names.
 *
 *   Allows us to rename variables & structs without breaking compatibility.
 *
 * - #DNA_MAKESDNA undefined:
 *   Behave as if old files were written new names.
 *
 *   This means older versions of Blender won't have access to this data *USE WITH CARE*.
 *
 */

#ifndef DNA_MAKESDNA
#  include "BLI_compiler_attrs.h"
#  include "BLI_utildefines.h"

#  include "DNA_genfile.h"
#  include "DNA_listBase.h"

#  include "BLO_readfile.h"
#  include "readfile.h"
#endif

#ifdef DNA_MAKESDNA

/**
 * Included in DNA versioning code.
 *
 * Currently use 'old' names when replacing struct members.
 */
DNA_STRUCT_REPLACE(Lamp, Light)
DNA_STRUCT_MEMBER_REPLACE(Lamp, clipsta, clip_start)
DNA_STRUCT_MEMBER_REPLACE(Lamp, clipend, clip_end)
DNA_STRUCT_MEMBER_REPLACE(Camera, YF_dofdist, dof_dist)

#else /* !DNA_MAKESDNA */

/**
 * Manipulates SDNA before calling #DNA_struct_get_compareflags,
 * allowing us to rename structs and struct members.
 *
 * \attention Changes here will cause breakages in forward compatbility,
 * Use this only in the _rare_ cases when migrating to new naming is needed.
 */
void blo_do_versions_dna(SDNA *sdna, const int versionfile, const int subversionfile)
{
#define DNA_VERSION_ATLEAST(ver, subver) \
	(versionfile > (ver) || (versionfile == (ver) && (subversionfile >= (subver))))

	if (!DNA_VERSION_ATLEAST(280, 2)) {
		/* Version files created in the 'blender2.8' branch
		 * between October 2016, and November 2017 (>=280.0 and < 280.2). */
		if (versionfile >= 280) {
			DNA_sdna_patch_struct(sdna, "SceneLayer", "ViewLayer");
			DNA_sdna_patch_struct(sdna, "SceneLayerEngineData", "ViewLayerEngineData");
			DNA_sdna_patch_struct_member(sdna, "FileGlobal", "cur_render_layer", "cur_view_layer");
			DNA_sdna_patch_struct_member(sdna, "ParticleEditSettings", "scene_layer", "view_layer");
			DNA_sdna_patch_struct_member(sdna, "Scene", "active_layer", "active_view_layer");
			DNA_sdna_patch_struct_member(sdna, "Scene", "render_layers", "view_layers");
			DNA_sdna_patch_struct_member(sdna, "WorkSpace", "render_layer", "view_layer");
		}
	}

#undef DNA_VERSION_ATLEAST
}

#endif /* !DNA_MAKESDNA */
