
#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"

#include "DNA_space_types.h"

#include "io_obj.h"
#include "intern/obj.h"

#include "BLT_translation.h"

#include "UI_interface.h"
#include "UI_resources.h"

static int wm_obj_export_invoke(bContext *C, wmOperator *op, const wmEvent *event) {
	return io_common_export_invoke(C, op, event, ".obj");
}
static int wm_obj_export_exec(bContext *C, wmOperator *op) {
	return io_common_export_exec(C, op, &OBJ_export /* export function */);
}
static void wm_obj_export_draw(bContext *C, wmOperator *op) {
	PointerRNA ptr;
	RNA_pointer_create(NULL, op->type->srna, op->properties, &ptr);

	/* Conveniently set start and end frame to match the scene's frame range. */
	Scene *scene = CTX_data_scene(C); // For SFRA and EFRA macros

	/* So we only do this once, use the dummy property */
	if (scene != NULL && RNA_boolean_get(&ptr, "init_scene_frame_range")) {
		RNA_int_set(&ptr, "start", SFRA);
		RNA_int_set(&ptr, "end", EFRA);
		RNA_boolean_set(&ptr, "init_scene_frame_range", false);
	}

	uiLayout *box;
	uiLayout *sub_box;
	uiLayout *row;

	/* Scene Options */
	box = uiLayoutBox(op->layout);
	row = uiLayoutRow(box, false);
	uiItemL(row, IFACE_("Scene Options:"), ICON_SCENE_DATA);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "axis_forward", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "axis_up",      0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_animations", 0, NULL, ICON_NONE);

	const bool animations = RNA_boolean_get(&ptr, "export_animations");

	row = uiLayoutRow(box, false);
	uiLayoutSetEnabled(row, animations);
	uiItemR(row, &ptr, "start", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiLayoutSetEnabled(row, animations);
	uiItemR(row, &ptr, "end", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "selected_only", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "visible_only", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "renderable_only", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "flatten_hierarchy", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "global_scale", 0, NULL, ICON_NONE);

	/* Object Data */
	box = uiLayoutBox(op->layout);
	row = uiLayoutRow(box, false);
	uiItemL(row, IFACE_("Object Options:"), ICON_OBJECT_DATA);

	row = uiLayoutRow(box, false);
	sub_box = uiLayoutBox(row);
	uiItemR(sub_box, &ptr, "export_normals", 0, NULL, ICON_NONE);

	const bool normals = RNA_boolean_get(&ptr, "export_normals");
	row = uiLayoutRow(sub_box, false);
	uiLayoutSetEnabled(row, normals);
	uiItemR(row, &ptr, "dedup_normals", 0, NULL, ICON_NONE);

	const bool dedup_normals = RNA_boolean_get(&ptr, "dedup_normals");
	row = uiLayoutRow(sub_box, false);
	uiLayoutSetEnabled(row, normals && dedup_normals);
	uiItemR(row, &ptr, "dedup_normals_threshold", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	sub_box = uiLayoutBox(row);
	uiItemR(sub_box, &ptr, "export_uvs", 0, NULL, ICON_NONE);

	const bool uvs = RNA_boolean_get(&ptr, "export_uvs");
	row = uiLayoutRow(sub_box, false);
	uiLayoutSetEnabled(row, uvs);
	uiItemR(row, &ptr, "dedup_uvs", 0, NULL, ICON_NONE);

	const bool dedup_uvs = RNA_boolean_get(&ptr, "dedup_uvs");
	row = uiLayoutRow(sub_box, false);
	uiLayoutSetEnabled(row, uvs && dedup_uvs);
	uiItemR(row, &ptr, "dedup_uvs_threshold", 0, NULL, ICON_NONE);

	row = uiLayoutRow(sub_box, false);
	uiLayoutSetEnabled(row, uvs);
	uiItemR(row, &ptr, "pack_uv", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_edges", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_materials", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_vcolors", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_face_sets", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "apply_modifiers", 1, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "render_modifiers", 1, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "curves_as_mesh", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "triangulate", 0, NULL, ICON_NONE);

	const bool triangulate = RNA_boolean_get(&ptr, "triangulate");

	row = uiLayoutRow(box, false);
	uiLayoutSetEnabled(row, triangulate);
	uiItemR(row, &ptr, "quad_method", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiLayoutSetEnabled(row, triangulate);
	uiItemR(row, &ptr, "ngon_method", 0, NULL, ICON_NONE);
}

static bool wm_obj_export_check(bContext *C, wmOperator *op) {
	return io_common_export_check(C, op, ".obj");
}

static int wm_obj_import_invoke(bContext *UNUSED(C), wmOperator *UNUSED(op), const wmEvent *UNUSED(event)) {
	return OPERATOR_CANCELLED;
}
static int wm_obj_import_exec(bContext *UNUSED(C), wmOperator *UNUSED(op)) {
	return OPERATOR_CANCELLED;
} /* TODO someone */
static void wm_obj_import_draw(bContext *UNUSED(C), wmOperator *UNUSED(op)) {
	/* TODO someone */
}
static bool wm_obj_import_check(bContext *UNUSED(C), wmOperator *UNUSED(op)) { return false; } /* TODO someone */

void WM_OT_obj_export(struct wmOperatorType *ot) {
	ot->name = "Export Wavefront (.obj)";
	ot->description = "Export current scene in an .obj archive";
	ot->idname = "WM_OT_obj_export_c";

	ot->invoke = wm_obj_export_invoke;
	ot->exec = wm_obj_export_exec;
	ot->poll = WM_operator_winactive;
	ot->ui = wm_obj_export_draw;
	ot->check = wm_obj_export_check;

	io_common_default_declare_export(ot, FILE_TYPE_OBJ);
}
void WM_OT_obj_import(struct wmOperatorType *ot) {
	ot->name = "Import Wavefront (.obj)";
	ot->description = "Import an .obj archive";
	ot->idname = "WM_OT_obj_import_c";

	ot->invoke = wm_obj_import_invoke;
	ot->exec = wm_obj_import_exec;
	ot->poll = WM_operator_winactive;
	ot->ui = wm_obj_import_draw;
	ot->check = wm_obj_import_check;
} /* TODO someone */
