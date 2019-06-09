extern "C" {
#include "BLI_listbase.h"
#include "BLI_math_matrix.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_modifier_types.h"

#include "BKE_modifier.h"
#include "BKE_modifier.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "../io_common.h"

#include <stdio.h>
}

#include <iostream>
#include <chrono>

#include "common.hpp"

// Anonymous namespace for internal functions
namespace {
	inline void name_compat(std::string& ob_name, const std::string& mesh_name) {
		if(ob_name.compare(mesh_name) != 0) {
			ob_name += "_" + mesh_name;
		}
		size_t start_pos;
		while((start_pos = ob_name.find(" ")) != std::string::npos)
			ob_name.replace(start_pos, 1, "_");
	}

	inline void change_single_axis_orientation(float (&mat)[4][4],
	                                     int axis_from, int axis_to) {
		bool negate = false;
		switch (axis_to) {
		case AXIS_X: // Fallthrough
		case AXIS_Y: // Fallthrough
		case AXIS_Z:
			break; // Just swap
		case AXIS_NEG_X: // Fallthrough
		case AXIS_NEG_Y: // Fallthrough
		case AXIS_NEG_Z:
			axis_to -= AXIS_NEG_X; // Transform the enum into an index
			negate = true;         // Swap and negate
			break;
		}
		for (size_t i = 0; i < 4; ++i) {
			float t = mat[i][axis_to];
			mat[i][axis_to] = (negate ? -1 : 1) * mat[i][axis_from];
			mat[i][axis_from] = t;
		}
	}
}

namespace common {

	bool object_is_smoke_sim(const Object * const ob) {
		ModifierData *md = modifiers_findByType((Object *) ob, eModifierType_Smoke);
		if (md) {
			SmokeModifierData *smd = (SmokeModifierData *) md;
			return (smd->type == MOD_SMOKE_TYPE_DOMAIN);
		}
		return false;
	}

	bool object_type_is_exportable(const Object * const ob) {
		switch (ob->type) {
		case OB_MESH:
			return !object_is_smoke_sim(ob);
			/* case OB_CURVE: */
			/* case OB_SURF: */
		case OB_LAMP:
		case OB_EMPTY:
		case OB_CAMERA:
			return false;
		default:
			// TODO someone Print in debug only
			fprintf(stderr, "Export for this object type is not yet defined %s\n", ob->id.name);
			return false;
		}
	}

	// Whether the object should be exported
	bool should_export_object(const ExportSettings * const settings, const Object * const ob) {
		if (!object_type_is_exportable(ob))
			return false;
		// If the object is a dupli, it's export satus depends on the parent
		if (!(ob->flag & BASE_FROM_DUPLI)) {
			/* These tests only make sense when the object isn't being instanced
			 * into the scene. When it is, its exportability is determined by
			 * its dupli-object and the DupliObject::no_draw property. */
			return  (settings->selected_only && (ob->flag & BASE_SELECTED) != 0) ||
				// FIXME Sybren: handle these cleanly (maybe just remove code),
				// now using active scene layer instead.
				(settings->visible_only && (ob->flag & BASE_VISIBLE) != 0) ||
				(settings->renderable_only && (ob->flag & BASE_ENABLED_RENDER) != 0);
		} else if (!(ob->parent != NULL && ob->parent->transflag & OB_DUPLI))
			return should_export_object(settings, ob->parent);
		else {
			BLI_assert(!"should_export_object");
			return false;
		}
	}

	void change_orientation(float (&mat)[4][4], int forward, int up) {
		change_single_axis_orientation(mat, AXIS_X, forward);
		change_single_axis_orientation(mat, AXIS_Z, up);
	}

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *ob, Mesh **mesh /* out */) {
		/* Based on abc_mesh.cc */

		/* Temporarily disable modifiers if we shouldn't apply them */
		if (!settings->apply_modifiers)
			for(ModifierData &md : common::modifier_iter{ob})
				md.mode |= eModifierMode_DisableTemporary;

		float scale_mat[4][4];
		scale_m4_fl(scale_mat, settings->global_scale);

		change_orientation(scale_mat, settings->axis_forward, settings->axis_up);

		// TODO someone Unsure if necessary
		// scale_mat[3][3] = m_settings.global_scale;  /* also scale translation */

		// TODO someone Doesn't seem to do anyhing. Is it necessary to update the object somehow?
		mul_m4_m4m4((float (*)[4]) ob->obmat, ob->obmat, scale_mat);
		// yup_mat[3][3] /= m_settings.global_scale;  /* normalise the homogeneous component */

		if (determinant_m4(ob->obmat) < 0.0)
			;               /* TODO someone flip normals */

		/* Object *ob = DEG_get_evaluated_object(settings->depsgraph, ob); */
		// *mesh = mesh_get_eval_final(settings->depsgraph, (Scene *) escene,
		//                             (Object *) ob, &CD_MASK_MESH);

		if (settings->render_modifiers) // vv Depends on depsgraph type
			*mesh = mesh_create_eval_final_render(settings->depsgraph, (Scene *) escene,
			                                      (Object *) ob, &CD_MASK_MESH);
		else
			*mesh = mesh_create_eval_final_view(settings->depsgraph, (Scene *) escene,
			                                    (Object *) ob, &CD_MASK_MESH);

		if (!settings->apply_modifiers)
			for(ModifierData &md : common::modifier_iter{ob})
				md.mode &= ~eModifierMode_DisableTemporary;

		if (settings->triangulate) {
			struct BMeshCreateParams bmcp = {false};
			struct BMeshFromMeshParams bmfmp = {true, false, false, 0};
			BMesh *bm = BKE_mesh_to_bmesh_ex(*mesh, &bmcp, &bmfmp);

			BM_mesh_triangulate(bm, settings->quad_method, settings->ngon_method, 4,
			                    false /* tag_only */, NULL, NULL, NULL);

			Mesh *result = BKE_mesh_from_bmesh_for_eval_nomain(bm, 0);
			BM_mesh_free(bm);

			*mesh = result;
			/* Needs to free? */
			return true;
		}
		/* Needs to free? */
		return false;
	}

	void free_mesh(Mesh *mesh, bool needs_free) {
		if (needs_free)
			BKE_id_free(NULL, mesh); // TODO someoene null? (alembic)
	}

	std::string get_object_name(const Object * const eob, const Mesh * const mesh) {
		std::string name{eob->id.name + 2};
		std::string mesh_name{mesh->id.name + 2};
		name_compat(name /* modifies */, mesh_name);
		return name;
	}

	std::string get_version_string() {
		return ""; // TODO someone implement
	}

	void export_start(bContext *UNUSED(C), ExportSettings * const settings) {
		/* From alembic_capi.cc
		 * XXX annoying hack: needed to prevent data corruption when changing
		 * scene frame in separate threads
		 */
		G.is_rendering = true; // TODO someone Should use BKE_main_lock(bmain);?
		BKE_spacedata_draw_locks(true);
		DEG_graph_build_from_view_layer(settings->depsgraph,
		                                settings->main,
		                                settings->scene,
		                                settings->view_layer);
		BKE_scene_graph_update_tagged(settings->depsgraph, settings->main);
	}

	bool export_end(bContext *UNUSED(C), ExportSettings * const settings) {
		G.is_rendering = false;
		BKE_spacedata_draw_locks(false);
		DEG_graph_free(settings->depsgraph);
		MEM_freeN(settings);
		return true;
	}

	bool time_export(bContext *C, ExportSettings * const settings,
	                 void (*start)(bContext *C, ExportSettings * const settings),
	                 void (*end)(bContext *C, ExportSettings * const settings) {
		auto f = std::chrono::steady_clock::now();
		start(C, settings);
		auto ret = end(C, settings);
		std::cout << "Took " << (std::chrono::steady_clock::now() - f).count() << "ns\n";
		return ret;
	}

	const std::array<float, 3> calculate_normal(const Mesh * const mesh,
	                                            const MPoly &mp) {
		float no[3];
		BKE_mesh_calc_poly_normal(&mp, mesh->mloop + mp.loopstart,
		                          mesh->mvert, no);
		return std::array<float, 3>{no[0], no[1], no[2]};
	}
}
