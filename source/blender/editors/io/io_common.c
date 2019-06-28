#include "io_common.h"

#include "DNA_modifier_types.h"

#include "BLI_string.h"
#include "BLI_path_util.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_report.h"
#include "BKE_scene.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

#include "ED_object.h"

#include "MEM_guardedalloc.h"

#include "UI_interface.h"
#include "UI_resources.h"

/* clang-format off */
const EnumPropertyItem axis_remap[] =
  {{AXIS_X,     "AXIS_X",     ICON_NONE, "X axis",  ""},
   {AXIS_Y,     "AXIS_Y",     ICON_NONE, "Y axis",  ""},
   {AXIS_Z,     "AXIS_Z",     ICON_NONE, "Z axis",  ""},
   {AXIS_NEG_X, "AXIS_NEG_X", ICON_NONE, "-X axis", ""},
   {AXIS_NEG_Y, "AXIS_NEG_Y", ICON_NONE, "-Y axis", ""},
   {AXIS_NEG_Z, "AXIS_NEG_Z", ICON_NONE, "-Z axis", ""},
   {0,          NULL,         0,         NULL,      NULL}};

const EnumPropertyItem path_reference_mode[] = {
    {AUTO,     "AUTO",     ICON_NONE, "Auto",       "Use Relative paths with subdirectories only"},
    {ABSOLUTE, "ABSOLUTE", ICON_NONE, "Absolute",   "Always write absolute paths"},
    {RELATIVE, "RELATIVE", ICON_NONE, "Relative",   "Always write relative paths (where possible)"},
    {MATCH,    "MATCH",    ICON_NONE, "Match",      "Match Absolute/Relative setting with input path"},
    {STRIP,    "STRIP",    ICON_NONE, "Strip Path", "Filename only"},
    {COPY,     "COPY",     ICON_NONE, "Copy",       "Copy the file to the destination path (or subdirectory)"},
    {0,        NULL,       0,         NULL,         NULL}};
/* clang-format on */

void io_common_default_declare_export(struct wmOperatorType *ot, eFileSel_File_Types file_type)
{
  // Defines "filepath"
  WM_operator_properties_filesel(ot,
                                 FILE_TYPE_FOLDER | file_type,
                                 FILE_BLENDER,
                                 FILE_SAVE,
                                 WM_FILESEL_FILEPATH,
                                 FILE_DEFAULTDISPLAY,
                                 FILE_SORT_ALPHA);

  RNA_def_enum(ot->srna,
               "axis_forward",
               axis_remap,
               AXIS_NEG_Z,  // From orientation helper, not sure why
               "Forward",
               "The axis to remap the forward axis to");
  RNA_def_enum(ot->srna,
               "axis_up",
               axis_remap,
               AXIS_Y,  // From orientation helper, not sure why
               "Up",
               "The axis to remap the up axis to");

  RNA_def_boolean(
      ot->srna, "selected_only", 0, "Selected Objects Only", "Export only selected objects");

  RNA_def_boolean(ot->srna,
                  "visible_only",
                  0,
                  "Visible Objects Only",
                  "Export only objects marked as visible");

  RNA_def_boolean(ot->srna,
                  "renderable_only",
                  1,
                  "Renderable Objects Only",
                  "Export only objects marked renderable in the outliner");

  RNA_def_boolean(
      ot->srna, "apply_modifiers", 0, "Apply Modifiers", "Apply modifiers before exporting");

  RNA_def_boolean(ot->srna,
                  "render_modifiers",
                  0,
                  "Use Render Modifiers",
                  "Whether to use Render or View modifier stettings");

  RNA_def_boolean(ot->srna,
                  "triangulate",
                  false,
                  "Triangulate",
                  "Export Polygons (Quads & NGons) as Triangles");

  RNA_def_enum(ot->srna,
               "quad_method",
               rna_enum_modifier_triangulate_quad_method_items,
               MOD_TRIANGULATE_QUAD_SHORTEDGE,
               "Quad Method",
               "Method for splitting the quads into triangles");

  RNA_def_enum(ot->srna,
               "ngon_method",
               rna_enum_modifier_triangulate_quad_method_items,
               MOD_TRIANGULATE_NGON_BEAUTY,
               "Polygon Method",
               "Method for splitting the polygons into triangles");

  RNA_def_float(ot->srna,
                "global_scale",
                1.0f,
                0.0001f,
                1000.0f,
                "Scale",
                "Value by which to enlarge or shrink the objects with"
                "respect to the world's origin",
                0.0001f,
                1000.0f);
}
void io_common_default_declare_import(struct wmOperatorType *UNUSED(ot))
{
}

ExportSettings *io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op)
{
  ExportSettings *settings = MEM_mallocN(sizeof(ExportSettings), "ExportSettings");

  settings->scene = CTX_data_scene(C);
  settings->view_layer = CTX_data_view_layer(C);
  settings->main = CTX_data_main(C);
  settings->depsgraph = NULL;  // Defer building

  RNA_string_get(op->ptr, "filepath", settings->filepath);

  settings->axis_forward = RNA_enum_get(op->ptr, "axis_forward");
  settings->axis_up = RNA_enum_get(op->ptr, "axis_up");
  settings->selected_only = RNA_boolean_get(op->ptr, "selected_only");
  settings->visible_only = RNA_boolean_get(op->ptr, "visible_only");
  settings->renderable_only = RNA_boolean_get(op->ptr, "renderable_only");
  settings->apply_modifiers = RNA_boolean_get(op->ptr, "apply_modifiers");
  settings->render_modifiers = RNA_boolean_get(op->ptr, "render_modifiers");
  settings->triangulate = RNA_boolean_get(op->ptr, "triangulate");
  settings->quad_method = RNA_enum_get(op->ptr, "quad_method");
  settings->ngon_method = RNA_enum_get(op->ptr, "ngon_method");
  settings->global_scale = RNA_float_get(op->ptr, "global_scale");

  return settings;
}

bool io_common_export_check(bContext *UNUSED(C), wmOperator *op, const char *ext)
{
  char filepath[FILE_MAX];
  RNA_string_get(op->ptr, "filepath", filepath);

  if (!BLI_path_extension_check(filepath, ext)) {
    BLI_path_extension_ensure(filepath, FILE_MAX, ext);
    RNA_string_set(op->ptr, "filepath", filepath);
    return true;
  }

  return false;
}

int io_common_export_invoke(bContext *C,
                            wmOperator *op,
                            const wmEvent *UNUSED(event),
                            const char *ext)
{

  RNA_boolean_set(op->ptr, "init_scene_frame_range", true);

  if (!RNA_struct_property_is_set(op->ptr, "filepath")) {
    Main *bmain = CTX_data_main(C);
    char filepath[FILE_MAX];

    if (BKE_main_blendfile_path(bmain)[0] == '\0') {
      BLI_strncpy(filepath, "untitled", sizeof(filepath));
    }
    else {
      BLI_strncpy(filepath, BKE_main_blendfile_path(bmain), sizeof(filepath));
    }

    BLI_path_extension_replace(filepath, sizeof(filepath), ext);
    RNA_string_set(op->ptr, "filepath", filepath);
  }

  WM_event_add_fileselect(C, op);

  return OPERATOR_RUNNING_MODAL;
}

int io_common_export_exec(struct bContext *C,
                          struct wmOperator *op,
                          ExportSettings *settings,
                          bool (*exporter)(struct bContext *C, ExportSettings *settings))
{
  if (!RNA_struct_property_is_set(op->ptr, "filepath")) {
    BKE_report(op->reports, RPT_ERROR, "No filename given");
    return OPERATOR_CANCELLED;
  }

  Object *obedit = CTX_data_edit_object(C);
  if (obedit) {
    ED_object_mode_toggle(C, OB_MODE_EDIT);
  }

  float orig_frame = BKE_scene_frame_get(CTX_data_scene(C));

  bool ok = exporter(C, settings);

  BKE_scene_frame_set(CTX_data_scene(C), orig_frame);

  return ok ? OPERATOR_FINISHED : OPERATOR_CANCELLED;
}
