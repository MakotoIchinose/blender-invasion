extern "C" {

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

#include "BLI_math.h"

#include "BLT_translation.h"

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

extern "C" {
bool STL_export(bContext *C, ExportSettings *settings) {
	auto f = std::chrono::steady_clock::now();
	STL_export_start(C, settings);
	auto ret = STL_export_end(C, settings);
	std::cout << "Took " << (std::chrono::steady_clock::now() - f).count() << "ns\n";
	return ret;
}
} // extern


bool STL_export_start(bContext *C, ExportSettings *settings) {
	common::export_start(C, settings);

	std::fstream fs;
	fs.open(settings->filepath, std::ios::out);

	Scene *scene  = DEG_get_evaluated_scene(settings->depsgraph);

	common::for_each_base(settings->view_layer,
	                      [](Base *base) {

	                      });
	return true;
}

bool STL_export_end(bContext *C, ExportSettings *settings) {
	return common::export_end(C, settings);
}
