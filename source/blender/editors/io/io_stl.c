
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"

#include "DNA_space_types.h"

#include "io_stl.h"
#include "intern/stl.h"

static int wm_stl_export_invoke(bContext *C, wmOperator *op, const wmEvent *event) {
	return io_common_export_invoke(C, op, event, ".stl");
}
static int wm_stl_export_exec(bContext *C, wmOperator *op) {
	/* return io_common_export_exec(C, op, &STL_export /\* export function *\/); */
	return true;
}
static void wm_stl_export_draw(bContext *C, wmOperator *op) {
	io_common_export_draw(C, op);
}
static bool wm_stl_export_check(bContext *C, wmOperator *op) {
	return io_common_export_check(C, op, ".stl");
}

static int wm_stl_import_invoke(bContext *C, wmOperator *op, const wmEvent *event) {
	return OPERATOR_CANCELLED;
}
static int wm_stl_import_exec(bContext *C, wmOperator *op) {
	return OPERATOR_CANCELLED;
} /* TODO someone */
static void wm_stl_import_draw(bContext *UNUSED(C), wmOperator *op) {
	/* TODO someone */
}
static bool wm_stl_import_check(bContext *C, wmOperator *op) { return false; } /* TODO someone */

void WM_OT_stl_export(struct wmOperatorType *ot) {
	ot->name = "Export STL (.stl)";
	ot->description = "Export current scene in a .stl archive";
	ot->idname = "WM_OT_stl_export_new"; // TODO someone

	ot->invoke = wm_stl_export_invoke;
	ot->exec = wm_stl_export_exec;
	ot->poll = WM_operator_winactive;
	ot->ui = wm_stl_export_draw;
	ot->check = wm_stl_export_check;
	// TODO someone Does there need to be a new file type?
	io_common_default_declare_export(ot, FILE_TYPE_OBJ);
}
void WM_OT_stl_import(struct wmOperatorType *ot) {
	ot->name = "Import STL (.stl)";
	ot->description = "Import a .stl archive";
	ot->idname = "WM_OT_stl_import_new";

	ot->invoke = wm_stl_import_invoke;
	ot->exec = wm_stl_import_exec;
	ot->poll = WM_operator_winactive;
	ot->ui = wm_stl_import_draw;
	ot->check = wm_stl_import_check;
} /* TODO someone Importer */
