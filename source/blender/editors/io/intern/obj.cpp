extern "C" {

#include "BKE_mesh_runtime.h"
#include "BKE_global.h"
#include "BKE_context.h"
#include "BKE_scene.h"
#include "BKE_library.h"

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

#include "obj.h"
#include "../io_common.h"
#include "common.h"

}

#include <iostream>
#include <fstream>
#include <iomanip>



namespace {
	void name_compat(std::string& ob_name, std::string& mesh_name) {
		if(ob_name.compare(mesh_name) != 0) {
			ob_name += "_" + mesh_name;
		}
		size_t start_pos = ob_name.find(" ");
		while(start_pos != std::string::npos)
			ob_name.replace(start_pos, 1, "_");
	}

	bool OBJ_export_mesh(bContext *UNUSED(C), ExportSettings *settings, std::fstream& fs,
	                    Object *eob, Mesh *mesh) {

		std::cout << "Foo\n";

		if (mesh->totvert == 0)
			return  true;

		if (settings->export_objects_as_objects || settings->export_objects_as_groups) {
			std::string name{eob->id.name};
			std::string mesh_name{mesh->id.name};
			name_compat(name, mesh_name);
			if (settings->export_objects_as_objects)
				fs << "o " << name << '\n';
			else
				fs << "g " << name << '\n';
		}

		// TODO someone Should this use iterators? Unsure if those are only for BMesh
		for (int i = 0, e = mesh->totvert; i < e; ++i)
			fs << "v "
			   << mesh->mvert[i].co[0]
			   << mesh->mvert[i].co[1]
			   << mesh->mvert[i].co[2]
			   << '\n';

		if (settings->export_uvs) {
			// TODO someone Is T47010 still relevant?
			for (int i = 0, e = mesh->totloop; i < e; ++i)
				fs << "vt "
				   << mesh->mloopuv[i].uv[0]
				   << mesh->mloopuv[i].uv[1]
				   << '\n';

		}

		return true;
	}


}

bool OBJ_export(bContext *C, ExportSettings *settings) {

	OBJ_export_start(C, settings);
	return OBJ_export_end(C, settings);
}

bool OBJ_export_start(bContext *C, ExportSettings *settings) {
	/* From alembic_capi.cc
	 * XXX annoying hack: needed to prevent data corruption when changing
	 * scene frame in separate threads
	 */
	G.is_rendering = true;
	BKE_spacedata_draw_locks(true);


	DEG_graph_build_from_view_layer(settings->depsgraph,
	                                settings->main,
	                                settings->scene,
	                                settings->view_layer);
	BKE_scene_graph_update_tagged(settings->depsgraph, settings->main);

	std::fstream fs;
	fs.open(settings->filepath, std::ios::out);
	fs << std::fixed << std::setprecision(6);
	fs << "# Blender v2.8\n# www.blender.org\n"; // TODO someone add proper version
	// TODO someone add material export

	Scene *scene  = DEG_get_evaluated_scene(settings->depsgraph);

	for (float frame = settings->frame_start; frame != settings->frame_end + 1.0; frame += 1.0) {
		BKE_scene_frame_set(scene, frame);
		BKE_scene_graph_update_for_newframe(settings->depsgraph, settings->main);

		for (Base *base = static_cast<Base *>(settings->view_layer->object_bases.first);
		     base; base = base->next) {
			if (!G.is_break) {
				// Object *ob = base->object;
				Object *eob = DEG_get_evaluated_object(settings->depsgraph,
				                                       base->object); // Is this expensive?
				if (!common_should_export_object(settings, base,
				                                 // TODO someone Currently ignoring
				                                 // all dupli objects;
				                                 // unsure if expected behavior
				                                 eob->parent != NULL &&
				                                 eob->parent->transflag & OB_DUPLI)

				    || !common_object_type_is_exportable(eob))
					continue;


				struct Mesh *mesh = nullptr;
				bool needs_free = false;

				std::cout << "BAR\n";

				switch (eob->type) {
				case OB_MESH:
					needs_free = get_final_mesh(settings, scene,
					                            eob, mesh /* OUT */);
					if (!OBJ_export_mesh(C, settings, fs, eob,
					                    mesh))
						return false;

					BKE_id_free(NULL, mesh);
					break;
				default:
					std::cerr << "OBJ Export Object Type not implemented\n";
					return false;
				}
			}
		}

	}

	G.is_break = false;
	return true;
}

bool OBJ_export_end(bContext *UNUSED(C), ExportSettings *settings) {
	G.is_rendering = false;
	BKE_spacedata_draw_locks(false);

	MEM_freeN(settings);
	return true;
}

// void io_obj_write_object() {

// }
