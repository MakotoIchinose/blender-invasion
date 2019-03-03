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
#include "common.h"


static bool common_object_is_smoke_sim(Object *ob) {
	ModifierData *md = modifiers_findByType(ob, eModifierType_Smoke);
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
 * \param is_duplicated: Normally false; true when the object is instanced
 * into the scene by a dupli-object (e.g. part of a dupligroup).
 * This ignores selection and layer visibility,
 * and assumes that the dupli-object itself (e.g. the group-instantiating empty) is exported.
 */
bool common_should_export_object(const ExportSettings * const settings, const Base * const ob_base,
                             bool is_duplicated) {
	/* From alembic */
	if (!is_duplicated) {
		/* These two tests only make sense when the object isn't being instanced
		 * into the scene. When it is, its exportability is determined by
		 * its dupli-object and the DupliObject::no_draw property. */
		if (settings->selected_only && (ob_base->flag & BASE_SELECTED) != 0) {
			return false;
		}
		// FIXME Sybren: handle these cleanly (maybe just remove code), now using active scene layer instead.
		if (settings->visible_only && (ob_base->flag & BASE_VISIBLE) == 0) {
			return false;
		}

		if (settings->renderable_only && (ob_base->flag & BASE_ENABLED_RENDER)) {
			return false;
		}
	}

	return true;
}


bool common_object_type_is_exportable(Object *ob) {
	switch (ob->type) {
	case OB_MESH:
		return !common_object_is_smoke_sim(ob);
	case OB_EMPTY:
	case OB_CURVE:
	case OB_SURF:
	case OB_CAMERA:
		return true;
	default:
		return false;
	}
}

bool get_final_mesh(ExportSettings *settings, Scene *escene, Object *eob, Mesh *mesh) {
	/* From abc_mesh.cc */

	/* Temporarily disable subdivs if we shouldn't apply them */
	if (!settings->apply_subdiv)
		for (ModifierData *md = eob->modifiers.first; md; md = md->next)
			if (md->type == eModifierType_Subsurf)
				md->mode |= eModifierMode_DisableTemporary;

	float scale_mat[4][4];
	scale_m4_fl(scale_mat, settings->global_scale);
	// TODO someone Unsure if necessary
	// scale_mat[3][3] = m_settings.global_scale;  /* also scale translation */
	mul_m4_m4m4(eob->obmat, eob->obmat, scale_mat);
	// yup_mat[3][3] /= m_settings.global_scale;  /* normalise the homogeneous component */

	if (determinant_m4(eob->obmat) < 0.0)
		;               /* TODO someone flip normals */

	/* Object *eob = DEG_get_evaluated_object(settings->depsgraph, ob); */
	mesh = mesh_get_eval_final(settings->depsgraph, escene, eob, CD_MASK_MESH);

	if (!settings->apply_subdiv)
		for (ModifierData *md = eob->modifiers.first; md; md = md->next)
			if (md->type == eModifierType_Subsurf)
				md->mode &= ~eModifierMode_DisableTemporary;

	if (settings->triangulate) {
		struct BMeshCreateParams bmcp = {false};
		struct BMeshFromMeshParams bmfmp = {true, false, false, 0};
		BMesh *bm = BKE_mesh_to_bmesh_ex(mesh, &bmcp, &bmfmp);

		BM_mesh_triangulate(bm, settings->quad_method, settings->ngon_method,
		                    false /* tag_only */, NULL, NULL, NULL);

		Mesh *result = BKE_mesh_from_bmesh_for_eval_nomain(bm, 0);
		BM_mesh_free(bm);

		// TODO someone Does it need to be freed?
		// It seems from abc_mesh that the result of mesh_get_eval_final
		// doesn't need to be freed
		/* if (*needs_free) { */
		/* 	BKE_id_free(NULL, mesh); */
		/* } */

		mesh = result;
		return true;
	}

	// TODO someone Does it need to be freed?
	// It seems from abc_mesh that the result of mesh_get_eval_final doesn't need to be freed
	return false;
}
