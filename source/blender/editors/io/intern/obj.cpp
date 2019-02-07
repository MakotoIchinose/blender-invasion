extern "C" {

#include "BKE_global.h"
#include "BKE_context.h"
#include "BKE_scene.h"
/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#define new extern_new
#include "BKE_screen.h"
#undef new

#include "DNA_space_types.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "MEM_guardedalloc.h"

#include "WM_api.h"
#include "WM_types.h"

#include "obj.h"
#include "../io_common.h"
#include "common.h"

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
	                                CTX_data_main(C),
	                                settings->scene,
	                                settings->view_layer);
	BKE_scene_graph_update_tagged(settings->depsgraph, CTX_data_main(C));

	for (Base *base = static_cast<Base *>(settings->view_layer->object_bases.first); base; base = base->next) {
		Object *ob = base->object;
		if (common_export_object_p(settings, base, false)) {
			Object *eob = DEG_get_evaluated_object(settings->depsgraph, ob);
			if (common_object_type_is_exportable(settings->scene, eob)) {
				// createTransformWriter(ob, parent, dupliObParent);
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
