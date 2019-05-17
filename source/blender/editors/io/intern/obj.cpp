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

#include "DNA_ID.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"

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
}

#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

#include "common.hpp"

/*
  TODO someone: () not done, -- done, # maybe add, ? unsure
  presets
  axis remap
  --selection only
  animation - partly
  --apply modifiers
  --render modifiers -- mesh_create_derived_{view,render}, deg_get_mode
  --edges
  smooth groups
  bitflag smooth groups?
  --normals
  --uvs
  materials
  --triangulate
  nurbs
  polygroups?
  --obj objects
  --obj groups
  material groups
  -?vertex order
  --scale
  path mode -- python repr
  # units?

  TODO someone filter_glob : StringProp weird Python syntax

 */

namespace {

	using namespace common;

	bool OBJ_export_material(bContext *UNUSED(C), ExportSettings *settings, std::fstream &fs,
	                         std::unordered_map<std::string, const Material *> materials) {

		fs << "# Blender MTL File\n"; /*TODO someone Filename necessary? */
		fs << "# Material Count: " << materials.size() << '\n';

		return true;
	}

	bool OBJ_export_mesh(bContext *UNUSED(C), ExportSettings *settings, std::fstream &fs,
	                     Object *eob, Mesh *mesh,
	                     ulong &vertex_total, ulong &uv_total, ulong &no_total,
	                     dedup_pair_t<uv_key_t> &uv_mapping_pair /* IN OUT */,
	                     dedup_pair_t<no_key_t> &no_mapping_pair /* IN OUT */) {

		auto &uv_mapping = uv_mapping_pair.second;
		auto &no_mapping = no_mapping_pair.second;

		ulong uv_initial_count = uv_mapping.size();
		ulong no_initial_count = no_mapping.size();

		if (mesh->totvert == 0)
			return true;

		if (settings->export_objects_as_objects || settings->export_objects_as_groups) {
			std::string name = common::get_object_name(eob, mesh);
			if (settings->export_objects_as_objects)
				fs << "o " << name << '\n';
			else
				fs << "g " << name << '\n';
		}

		fs << std::fixed << std::setprecision(6);
		common::for_each_vertex(mesh, [&fs](ulong UNUSED(i), const MVert &v) {
			                              fs << "v "
			                                 << v.co[0] << ' '
			                                 << v.co[1] << ' '
			                                 << v.co[2] << '\n';
		                              });

		if (settings->export_uvs) {
			// TODO someone Is T47010 still relevant?
			common::for_each_deduplicated_uv(mesh, /* modifies */ uv_total,
			                                 /* modifies */ uv_mapping_pair,
			                                 [&fs](ulong UNUSED(i),
			                                       const typename set_t<uv_key_t>::
			                                       iterator &v) {
				                                 fs << "vt " << v->first[0]
				                                    << ' '   << v->first[1] << '\n';
			                                 });
		}

		if (settings->export_normals) {
			fs << std::fixed << std::setprecision(4);
			common::for_each_deduplicated_normal(mesh, /* modifies */ no_total,
			                                     /* modifies */ no_mapping_pair,
			                                     [&fs](ulong UNUSED(i),
			                                           const typename set_t<no_key_t>::
			                                           iterator &v) {
				                                     fs << "vn " << v->first[0]
				                                        << ' '   << v->first[1]
				                                        << ' '   << v->first[2] << '\n';
			                                     });
		}

		if (settings->export_edges) {
			common::for_each_edge(mesh, [&fs](ulong UNUSED(i), const MEdge &e){
				                            if (e->flag & ME_LOOSEEDGE)
					                            fs << "l " << e->v1
					                               << ' '  << e->v2 << '\n';
			                            });
		}

		std::cerr << "Totals: "  << uv_total << " " << no_total
		          << "\nSizes: " << uv_mapping.size() << " " << no_mapping.size()  << '\n';

		for (int p_i = 0, p_e = mesh->totpoly; p_i < p_e; ++p_i) {
			fs << 'f';
			MPoly *p = mesh->mpoly + p_i;
			for (int l_i = p->loopstart, l_e = p->loopstart + p->totloop;
			     l_i < l_e; ++l_i) {
				MLoop *vxl = mesh->mloop + l_i;
				if (settings->export_uvs && settings->export_normals) {
					fs << ' '
					   << vertex_total + vxl->v << '/'
					   << uv_mapping[uv_initial_count + l_i]->second << '/'
					   << no_mapping[no_initial_count + vxl->v]->second;
				} else if (settings->export_uvs) {
					fs << ' '
					   << vertex_total + vxl->v << '/'
					   << uv_mapping[uv_initial_count + l_i]->second;
				} else if (settings->export_normals) {
					fs << ' '
					   << vertex_total + vxl->v << "//"
					   << no_mapping[no_initial_count + vxl->v]->second;
				} else {
					fs << ' '
					   << vertex_total + vxl->v;
				}
			}
			fs << '\n';
		}
		vertex_total += mesh->totvert;
		uv_total += mesh->totloop;
		no_total += mesh->totvert;
		return true;
	}


}

bool OBJ_export(bContext *C, ExportSettings *settings) {
	auto f = std::chrono::steady_clock::now();
	OBJ_export_start(C, settings);
	auto ret = OBJ_export_end(C, settings);
	std::cout << "Took " << (std::chrono::steady_clock::now() - f).count() << "ns\n";
	return ret;
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
	fs << "# Blender v2.8\n# www.blender.org\n"; // TODO someone add proper version
	// TODO someone add material export

	Scene *scene  = DEG_get_evaluated_scene(settings->depsgraph);
	ulong vertex_total = 0, uv_total = 0, no_total = 0;

	float frame = 0;

	if (!settings->export_animations)
		goto do_export;

	// TODO someone Check this, it seems wrong
	for (frame = settings->frame_start; frame <= settings->frame_end + 0.5; frame += 1.0) {
	do_export:
		BKE_scene_frame_set(scene, frame);
		BKE_scene_graph_update_for_newframe(settings->depsgraph, settings->main);

		auto uv_mapping_pair = common::make_deduplicate_set<uv_key_t>();
		auto no_mapping_pair = common::make_deduplicate_set<no_key_t>();

		// TODO someone if not exporting as objects, do they need to all be merged?

		for (Base *base = static_cast<Base *>(settings->view_layer->object_bases.first);
		     base; base = base->next) {
			if (!G.is_break) {

				Object *eob = DEG_get_evaluated_object(settings->depsgraph,
				                                       base->object); // TODO someone Is this expensive?
				if (!common::should_export_object(settings, eob) ||
				    !common::object_type_is_exportable(eob))
					continue;

				struct Mesh *mesh = nullptr;
				bool needs_free = false;

				switch (eob->type) {
				case OB_MESH:

					// std::cerr << "R: " << (**(*eob).mat).r
					//           << "G: " << (**(*eob).mat).g
					//           << "B: " << (**(*eob).mat).b << "\n";

					needs_free = common::get_final_mesh(settings, scene,
						                           eob, &mesh /* OUT */);

					if (!OBJ_export_mesh(C, settings, fs, eob, mesh,
					                     vertex_total, uv_total, no_total,
					                     uv_mapping_pair, no_mapping_pair))
						return false;

					if (needs_free)
						BKE_id_free(NULL, mesh);
					break;
				default:
					// TODO someone Probably abort, it shouldn't be possible to get here
					std::cerr << "OBJ Export for this Object Type not implemented" << eob->id.name << '\n';
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

	DEG_graph_free(settings->depsgraph);

	MEM_freeN(settings);

	return true;
}

// void io_obj_write_object() {

// }
