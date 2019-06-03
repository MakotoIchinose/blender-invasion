

extern "C" {

#include "BLI_math.h"
#include "BLI_sys_types.h"
#include "BLT_translation.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_global.h"
#include "BKE_context.h"
#include "BKE_scene.h"
#include "BKE_library.h"
#include "BKE_customdata.h"

/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#define new extern_new
#include "BKE_screen.h"
#undef new

#include "DNA_space_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_ID.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "MEM_guardedalloc.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_object.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "stl.h"
#include "../io_common.h"
}

#include <chrono>
#include <iostream>
#include <fstream>

#include "common.hpp"

namespace {
	bool STL_export_object(bContext *UNUSED(C), ExportSettings * const settings, std::fstream &fs,
	                     Scene *escene, const Object *ob) {
		Mesh *mesh;
		settings->triangulate = true; // STL only really works with triangles
		bool needs_free = common::get_final_mesh(settings, escene, ob, &mesh);
		// TODO someone Is it ok to add the version info after a # in STL?
		const std::string name = common::get_object_name(ob, mesh) + " # " + common::get_version_string();

		fs << "solid " << name;

		for (const MPoly &mp : common::poly_iter{mesh}) {
			const std::array<float, 3> no = common::calculate_normal(mesh, mp);
			fs << "facet normal " << no[0] << no[1] << no[2]
			   << "\nouter loop\n";
			for (const MVert &v : common::vert_of_poly_iter{mesh, mp})
				fs << "vertex " << v.co[0]
				   << v.co[1]   << v.co[2];
			fs << "\nendloop\nendfacet\n";
		}

		fs << "endsolid " << name;

		common::free_mesh(mesh, needs_free);
		return true;
	}

	void STL_export_start(bContext *C, ExportSettings * const settings) {
		common::export_start(C, settings);

		std::fstream fs;
		fs.open(settings->filepath, std::ios::out);

		Scene *escene  = DEG_get_evaluated_scene(settings->depsgraph);

		for (const Object *ob : common::object_iter{settings->view_layer})
			if (!STL_export_object(C, settings, fs, escene, ob))
				return;
	}

	bool STL_export_end(bContext *C, ExportSettings * const settings) {
		return common::export_end(C, settings);
	}
}

extern "C" {
bool STL_export(bContext *C, ExportSettings * const settings) {
	return common::time_export(C, settings, &STL_export_start, &STL_export_end);
}
} // extern
