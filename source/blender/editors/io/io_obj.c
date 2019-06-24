#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_scene.h"

#include "MEM_guardedalloc.h"

#include "DNA_space_types.h"

#include "io_obj.h"
#include "intern/obj.h"

#include "BLT_translation.h"

#include "UI_interface.h"
#include "UI_resources.h"

static int wm_obj_export_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  return io_common_export_invoke(C, op, event, ".obj");
}
static int wm_obj_export_exec(bContext *C, wmOperator *op)
{
  ExportSettings *settings = io_common_construct_default_export_settings(C, op);
  settings->export_normals = RNA_boolean_get(op->ptr, "export_normals");
  settings->export_uvs = RNA_boolean_get(op->ptr, "export_uvs");
  settings->export_edges = RNA_boolean_get(op->ptr, "export_edges");
  settings->export_materials = RNA_boolean_get(op->ptr, "export_materials");
  settings->export_vcolors = RNA_boolean_get(op->ptr, "export_vcolors");
  settings->export_vweights = RNA_boolean_get(op->ptr, "export_vweights");
  settings->export_curves = RNA_boolean_get(op->ptr, "export_curves");
  settings->pack_uv = RNA_boolean_get(op->ptr, "pack_uv");

  settings->extra = MEM_mallocN(sizeof(OBJExportSettings), "OBJExportSettings");
  OBJExportSettings *extra = settings->extra;

  extra->start_frame = RNA_int_get(op->ptr, "start_frame");
  extra->end_frame = RNA_int_get(op->ptr, "end_frame");

  extra->export_animations = RNA_boolean_get(op->ptr, "export_animations");
  extra->dedup_normals = RNA_boolean_get(op->ptr, "dedup_normals");
  extra->dedup_normals_threshold = RNA_float_get(op->ptr, "dedup_normals_threshold");
  extra->dedup_uvs = RNA_boolean_get(op->ptr, "dedup_uvs");
  extra->dedup_uvs_threshold = RNA_float_get(op->ptr, "dedup_uvs_threshold");
  extra->export_face_sets = RNA_boolean_get(op->ptr, "export_face_sets");
  extra->path_mode = RNA_enum_get(op->ptr, "path_mode");
  extra->export_objects_as_objects = RNA_boolean_get(op->ptr, "export_objects_as_objects");
  extra->export_objects_as_groups = RNA_boolean_get(op->ptr, "export_objects_as_groups");

  if (!extra->export_animations) {
    extra->start_frame = BKE_scene_frame_get(CTX_data_scene(C));
    extra->end_frame = extra->start_frame;
  }

  return io_common_export_exec(C, op, settings, &OBJ_export /* export function */);
}
static void wm_obj_export_draw(bContext *C, wmOperator *op)
{
  PointerRNA ptr;
  RNA_pointer_create(NULL, op->type->srna, op->properties, &ptr);

  /* Conveniently set start and end frame to match the scene's frame range. */
  Scene *scene = CTX_data_scene(C);  // For SFRA and EFRA macros

  /* So we only do this once, use the dummy property */
  if (scene != NULL && RNA_boolean_get(&ptr, "init_scene_frame_range")) {
    RNA_int_set(&ptr, "start_frame", SFRA);
    RNA_int_set(&ptr, "end_frame", EFRA);
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
  uiItemR(row, &ptr, "axis_up", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  sub_box = uiLayoutBox(row);
  uiItemR(sub_box, &ptr, "export_animations", 0, NULL, ICON_NONE);

  const bool animations = RNA_boolean_get(&ptr, "export_animations");

  row = uiLayoutRow(sub_box, false);
  uiLayoutSetEnabled(row, animations);
  uiItemR(row, &ptr, "start_frame", 0, NULL, ICON_NONE);

  row = uiLayoutRow(sub_box, false);
  uiLayoutSetEnabled(row, animations);
  uiItemR(row, &ptr, "end_frame", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "selected_only", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "visible_only", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "renderable_only", 0, NULL, ICON_NONE);

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

  sub_box = uiLayoutBox(box);
  row = uiLayoutRow(sub_box, false);
  uiItemR(row, &ptr, "export_materials", 0, NULL, ICON_NONE);

  const bool materials = RNA_boolean_get(&ptr, "export_materials");
  row = uiLayoutRow(sub_box, false);
  uiLayoutSetEnabled(row, materials);
  uiItemR(row, &ptr, "path_mode", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "export_vcolors", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "export_face_sets", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  sub_box = uiLayoutBox(row);
  uiItemR(sub_box, &ptr, "apply_modifiers", 0, NULL, ICON_NONE);

  const bool modifiers = RNA_boolean_get(&ptr, "apply_modifiers");

  row = uiLayoutRow(sub_box, false);
  uiLayoutSetEnabled(row, modifiers);
  uiItemR(row, &ptr, "render_modifiers", 0, NULL, ICON_NONE);

  row = uiLayoutRow(box, false);
  uiItemR(row, &ptr, "export_curves", 0, NULL, ICON_NONE);

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

static bool wm_obj_export_check(bContext *C, wmOperator *op)
{
  return io_common_export_check(C, op, ".obj");
}

static int wm_obj_import_invoke(bContext *UNUSED(C),
                                wmOperator *UNUSED(op),
                                const wmEvent *UNUSED(event))
{
  return OPERATOR_CANCELLED;
}
static int wm_obj_import_exec(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
  return OPERATOR_CANCELLED;
} /* TODO someone */
static void wm_obj_import_draw(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
  /* TODO someone */
}
static bool wm_obj_import_check(bContext *UNUSED(C), wmOperator *UNUSED(op))
{
  return false;
} /* TODO someone */

void WM_OT_obj_export(struct wmOperatorType *ot)
{
  ot->name = "Export Wavefront (.obj)";
  ot->description = "Export current scene in an .obj archive";
  ot->idname = "WM_OT_obj_export_c";

  ot->invoke = wm_obj_export_invoke;
  ot->exec = wm_obj_export_exec;
  ot->poll = WM_operator_winactive;
  ot->ui = wm_obj_export_draw;
  ot->check = wm_obj_export_check;

  io_common_default_declare_export(ot, FILE_TYPE_OBJ);

  RNA_def_int(ot->srna,
              "start_frame",
              INT_MIN,
              INT_MIN,
              INT_MAX,
              "Start Frame",
              "Start frame of the export, use the default value to "
              "take the start frame of the current scene",
              INT_MIN,
              INT_MAX);

  RNA_def_int(ot->srna,
              "end_frame",
              INT_MIN,
              INT_MIN,
              INT_MAX,
              "End Frame",
              "End frame of the export, use the default value to "
              "take the end frame of the current scene",
              INT_MIN,
              INT_MAX);

  RNA_def_boolean(ot->srna, "export_animations", 0, "Animations", "Export animations");

  RNA_def_boolean(ot->srna, "export_normals", 1, "Normals", "Export normals");

  RNA_def_boolean(ot->srna, "export_uvs", 1, "UVs", "Export UVs");

  RNA_def_boolean(ot->srna,
                  "dedup_normals",
                  1,
                  "Deduplicate Normals",
                  "Remove duplicate normals");  // TODO someone add a threshold

  // The UI seems to make it so the minimum softlimit can't be smaller than 0.001,
  // but normals are only printed with four decimal places, so it doesn't matter too much
  RNA_def_float(ot->srna,
                "dedup_normals_threshold",
                0.001,
                FLT_MIN,
                FLT_MAX,
                "Threshold for deduplication of Normals",
                "The minimum difference so two Normals are considered different",
                0.001,
                10.0);

  RNA_def_boolean(ot->srna,
                  "dedup_uvs",
                  1,
                  "Deduplicate UVs",
                  "Remove duplicate UVs");  // TODO someone add a threshold

  RNA_def_float(ot->srna,
                "dedup_uvs_threshold",
                0.001,
                FLT_MIN,
                FLT_MAX,
                "Threshold for deduplication of UVs",
                "The minimum difference so two UVs are considered different",
                0.001,
                10.0);

  RNA_def_boolean(ot->srna, "export_edges", 0, "Edges", "Export Edges");

  RNA_def_boolean(ot->srna, "export_materials", 0, "Materials", "Export Materials");

  RNA_def_boolean(
      ot->srna, "export_face_sets", 0, "Face Sets", "Export per face shading group assignments");

  RNA_def_boolean(ot->srna, "export_vcolors", 0, "Vertex Colors", "Export vertex colors");

  RNA_def_boolean(ot->srna, "export_vweights", 0, "Vertex Weights", "Exports vertex weights");

  RNA_def_boolean(ot->srna,
                  "export_objects_as_objects",
                  1,
                  "Export as Objects named like Blender Objects",
                  "Export Blender Object as named objects");

  RNA_def_boolean(ot->srna,
                  "export_objects_as_groups",
                  0,
                  "Export as Groups named like Blender Objects",
                  "Export Blender Objects as named groups");

  RNA_def_boolean(
      ot->srna, "export_curves", false, "Export curves", "Export curves and NURBS surfaces");

  RNA_def_boolean(ot->srna, "pack_uv", 1, "Pack UV Islands", "Export UVs with packed island");

  RNA_def_enum(ot->srna,
               "path_mode",
               path_reference_mode,
               AUTO,
               "Path mode",
               "How external files (such as images) are treated");

  /* This dummy prop is used to check whether we need to init the start and
   * end frame values to that of the scene's, otherwise they are reset at
   * every change, draw update. */
  RNA_def_boolean(ot->srna, "init_scene_frame_range", true, "", "");
}
void WM_OT_obj_import(struct wmOperatorType *ot)
{
  ot->name = "Import Wavefront (.obj)";
  ot->description = "Import an .obj archive";
  ot->idname = "WM_OT_obj_import_c";

  ot->invoke = wm_obj_import_invoke;
  ot->exec = wm_obj_import_exec;
  ot->poll = WM_operator_winactive;
  ot->ui = wm_obj_import_draw;
  ot->check = wm_obj_import_check;
} /* TODO someone */
