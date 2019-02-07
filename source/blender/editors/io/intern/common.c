#include "BLI_listbase.h"

#include "DNA_mesh_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_modifier_types.h"

#include "BKE_modifier.h"

#include "../io_common.h"
#include "common.h"

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
bool common_export_object_p(const ExportSettings * const settings, const Base * const ob_base,
                             bool is_duplicated) {
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


bool common_object_type_is_exportable(Scene *scene, Object *ob) {
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

static bool common_object_is_smoke_sim(Object *ob) {
	ModifierData *md = modifiers_findByType(ob, eModifierType_Smoke);
	if (md) {
		SmokeModifierData *smd = md;
		return (smd->type == MOD_SMOKE_TYPE_DOMAIN);
	}
	return false;
}
