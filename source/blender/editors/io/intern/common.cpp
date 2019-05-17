extern "C" {
#include "BLI_listbase.h"
#include "BLI_math_matrix.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_modifier_types.h"

#include "BKE_modifier.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "../io_common.h"

#include <stdio.h>
}

#include "common.hpp"

// Anonymous namespace for internal functions
namespace {
	void name_compat(std::string& ob_name, const std::string& mesh_name) {
		if(ob_name.compare(mesh_name) != 0) {
			ob_name += "_" + mesh_name;
		}
		size_t start_pos;
		while((start_pos = ob_name.find(" ")) != std::string::npos)
			ob_name.replace(start_pos, 1, "_");
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

	/**
	 * Returns whether this object should be exported into the Alembic file.
	 *
	 * \param settings: export settings, used for options like 'selected only'.
	 * \param ob: the object's base in question.
	 * This ignores layer visibility,
	 * and assumes that the dupli-object itself (e.g. the group-instantiating empty) is exported.
	 */
	bool should_export_object(const ExportSettings * const settings, const Object * const eob) {
		// If the object is a dupli, it's export satus depends on the parent
		if (!(eob->flag & BASE_FROM_DUPLI)) {
			/* !(eob->parent != NULL && eob->parent->transflag & OB_DUPLI) */

			/* These tests only make sense when the object isn't being instanced
			 * into the scene. When it is, its exportability is determined by
			 * its dupli-object and the DupliObject::no_draw property. */
			return  (settings->selected_only && (eob->flag & BASE_SELECTED) != 0) ||
				// FIXME Sybren: handle these cleanly (maybe just remove code),
				// now using active scene layer instead.
				(settings->visible_only && (eob->flag & BASE_VISIBLE) != 0) ||
				(settings->renderable_only && (eob->flag & BASE_ENABLED_RENDER) != 0);
		}

		return should_export_object(settings, eob->parent);
	}


	bool object_type_is_exportable(const Object * const ob) {
		switch (ob->type) {
		case OB_MESH:
			return !object_is_smoke_sim(ob);
			/* case OB_EMPTY: */
			/* case OB_CURVE: */
			/* case OB_SURF: */
			/* case OB_CAMERA: */
			/* return false; */
		default:
			printf("Export for this object type not defined %s\n", ob->id.name);
			return false;
		}
	}

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *eob, Mesh **mesh /* out */) {
		/* From abc_mesh.cc */

		/* Temporarily disable subdivs if we shouldn't apply them */
		if (!settings->apply_modifiers)
			for (ModifierData *md = (ModifierData *) eob->modifiers.first;
			     md; md = md->next)
				// if (md->type == eModifierType_Subsurf)
				md->mode |= eModifierMode_DisableTemporary;

		float scale_mat[4][4];
		scale_m4_fl(scale_mat, settings->global_scale);
		// TODO someone Unsure if necessary
		// scale_mat[3][3] = m_settings.global_scale;  /* also scale translation */
		mul_m4_m4m4((float (*)[4]) eob->obmat, eob->obmat, scale_mat);
		// yup_mat[3][3] /= m_settings.global_scale;  /* normalise the homogeneous component */

		if (determinant_m4(eob->obmat) < 0.0)
			;               /* TODO someone flip normals */

		/* Object *eob = DEG_get_evaluated_object(settings->depsgraph, ob); */
		//*mesh = mesh_get_eval_final(settings->depsgraph, (Scene *) escene,
		//                            (Object *)eob, &CD_MASK_MESH);

		if (settings->render_modifiers) // vv Depends on depsgraph type
			*mesh = mesh_create_eval_final_render(settings->depsgraph, (Scene *) escene,
			                                 (Object *) eob, &CD_MASK_MESH);
		else
			*mesh = mesh_create_eval_final_view(settings->depsgraph, (Scene *) escene,
			                               (Object *) eob, &CD_MASK_MESH);

		if (!settings->apply_modifiers)
			for (ModifierData *md = (ModifierData *) eob->modifiers.first;
			     md; md = md->next)
				// if (md->type == eModifierType_Subsurf)
				md->mode &= ~eModifierMode_DisableTemporary;

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

	std::string get_object_name(const Object * const eob, const Mesh * const mesh) {
		std::string name{eob->id.name + 2};
		std::string mesh_name{mesh->id.name + 2};
		name_compat(name /* modifies */, mesh_name);
		return name;
	}
}
