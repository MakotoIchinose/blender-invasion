
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"

#include "DNA_space_types.h"

#include "io_obj.h"
#include "intern/obj.h"

static int wm_obj_export_invoke(bContext *C, wmOperator *op, const wmEvent *event) {
	return io_common_export_invoke(C, op, event, ".obj");
}
static int wm_obj_export_exec(bContext *C, wmOperator *op) {
	return io_common_export_exec(C, op, &OBJ_export /* export function */);
}
static void wm_obj_export_draw(bContext *C, wmOperator *op) {
	return io_common_export_draw(C, op);
}
static bool wm_obj_export_check(bContext *C, wmOperator *op) {
	return io_common_export_check(C, op, ".obj");
}

static int wm_obj_import_invoke(bContext *C, wmOperator *op, const wmEvent *event) {
	return OPERATOR_CANCELLED;
}
static int wm_obj_import_exec(bContext *C, wmOperator *op) {
	return OPERATOR_CANCELLED;
} /* TODO someone */
static void wm_obj_import_draw(bContext *UNUSED(C), wmOperator *op) {
	/* TODO someone */
}
static bool wm_obj_import_check(bContext *C, wmOperator *op) { return false; } /* TODO someone */

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
