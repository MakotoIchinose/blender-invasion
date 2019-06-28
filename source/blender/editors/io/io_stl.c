
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"

#include "MEM_guardedalloc.h"

#include "DNA_space_types.h"

#include "io_stl.h"
#include "intern/stl.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "UI_interface.h"
#include "UI_resources.h"

/* bool STL_export(bContext *C, ExportSettings * const settings); */

static int wm_stl_export_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  return io_common_export_invoke(C, op, event, ".stl");
}
static int wm_stl_export_exec(bContext *C, wmOperator *op)
{
  ExportSettings *settings = io_common_construct_default_export_settings(C, op);
  settings->use_scene_units = RNA_boolean_get(op->ptr, "use_scene_units");

  settings->format_specific = MEM_mallocN(sizeof(STLExportSettings), "STLExportSettings");
  STLExportSettings *format_specific = settings->format_specific;

  format_specific->use_ascii = RNA_boolean_get(op->ptr, "use_ascii");

  return io_common_export_exec(C, op, settings, &STL_export /* export function */);
}
static void wm_stl_export_draw(bContext *UNUSED(C), wmOperator *op)
{
  PointerRNA ptr;
  RNA_pointer_create(NULL, op->type->srna, op->properties, &ptr);

  /* uiLayout *box; */
  /* uiLayout *sub_box; */
  /* box = uiLayoutBox(op->layout); */

  uiLayout *row;

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "axis_forward", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "axis_up", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "selected_only", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "visible_only", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "renderable_only", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "use_ascii", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "global_scale", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "use_scene_units", 0, NULL, ICON_NONE);

  row = uiLayoutRow(op->layout, false);
  uiItemR(row, &ptr, "apply_modifiers", 0, NULL, ICON_NONE);

  const bool modifiers = RNA_boolean_get(&ptr, "apply_modifiers");

  row = uiLayoutRow(op->layout, false);
  uiLayoutSetEnabled(row, modifiers);
  uiItemR(row, &ptr, "render_modifiers", 0, NULL, ICON_NONE);
}

static bool wm_stl_export_check(bContext *C, wmOperator *op)
{
  return io_common_export_check(C, op, ".stl");
}

static int wm_stl_import_invoke(bContext *UNUSED(C),
                                wmOperator *UNUSED(op),
                                const wmEvent *UNUSED(event))
{
  return OPERATOR_CANCELLED;
}

static int wm_stl_import_exec(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
  return OPERATOR_CANCELLED;
}

static void wm_stl_import_draw(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
}

static bool wm_stl_import_check(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
  return false;
}

void WM_OT_stl_export(struct wmOperatorType *ot)
{
  ot->name = "Export STL (.stl)";
  ot->description = "Export current scene in a .stl archive";
  ot->idname = "WM_OT_stl_export_c";  // TODO someone

  ot->invoke = wm_stl_export_invoke;
  ot->exec = wm_stl_export_exec;
  ot->poll = WM_operator_winactive;
  ot->ui = wm_stl_export_draw;
  ot->check = wm_stl_export_check;

  // TODO someone Does there need to be a new file type?
  io_common_default_declare_export(ot, FILE_TYPE_OBJ);

  RNA_def_boolean(ot->srna,
                  "use_ascii",
                  0,
                  "Use ASCII format",
                  "Whether to use the ASCII or the binary variant");

  RNA_def_boolean(ot->srna,
                  "use_scene_units",
                  0,
                  "Use scene units",
                  "Whether to use the scene's units as a scaling factor");
}
void WM_OT_stl_import(struct wmOperatorType *ot)
{
  ot->name = "Import STL (.stl)";
  ot->description = "Import a .stl archive";
  ot->idname = "WM_OT_stl_import_c";

  ot->invoke = wm_stl_import_invoke;
  ot->exec = wm_stl_import_exec;
  ot->poll = WM_operator_winactive;
  ot->ui = wm_stl_import_draw;
  ot->check = wm_stl_import_check;
} /* TODO someone Importer */
