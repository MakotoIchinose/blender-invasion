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

const EnumPropertyItem axis_remap[] = {{AXIS_X, "AXIS_X", ICON_NONE, "X axis", ""},
                                       {AXIS_Y, "AXIS_Y", ICON_NONE, "Y axis", ""},
                                       {AXIS_Z, "AXIS_Z", ICON_NONE, "Z axis", ""},
                                       {AXIS_NEG_X, "AXIS_NEG_X", ICON_NONE, "-X axis", ""},
                                       {AXIS_NEG_Y, "AXIS_NEG_Y", ICON_NONE, "-Y axis", ""},
                                       {AXIS_NEG_Z, "AXIS_NEG_Z", ICON_NONE, "-Z axis", ""},
                                       {0, NULL, 0, NULL, NULL}};

void io_common_default_declare_export(struct wmOperatorType *ot, eFileSel_File_Types file_type)
{

  WM_operator_properties_filesel(ot,
                                 FILE_TYPE_FOLDER | file_type,
                                 FILE_BLENDER,
                                 FILE_SAVE,
                                 WM_FILESEL_FILEPATH,
                                 FILE_DEFAULTDISPLAY,
                                 FILE_SORT_ALPHA);

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

  RNA_def_int(ot->srna,
              "start",
              INT_MIN,
              INT_MIN,
              INT_MAX,
              "Start Frame",
              "Start frame of the export, use the default value to "
              "take the start frame of the current scene",
              INT_MIN,
              INT_MAX);

  RNA_def_int(ot->srna,
              "end",
              INT_MIN,
              INT_MIN,
              INT_MAX,
              "End Frame",
              "End frame of the export, use the default value to "
              "take the end frame of the current scene",
              INT_MIN,
              INT_MAX);

  RNA_def_int(ot->srna,
              "xsamples",
              1,
              1,
              128,
              "Transform Samples",
              "Number of times per frame transformations are sampled",
              1,
              128);

  RNA_def_int(ot->srna,
              "gsamples",
              1,
              1,
              128,
              "Geometry Samples",
              "Number of times per frame object data are sampled",
              1,
              128);

  RNA_def_float(ot->srna,
                "sh_open",
                0.0f,
                -1.0f,
                1.0f,
                "Shutter Open",
                "Time at which the shutter is open",
                -1.0f,
                1.0f);

  RNA_def_float(ot->srna,
                "sh_close",
                1.0f,
                -1.0f,
                1.0f,
                "Shutter Close",
                "Time at which the shutter is closed",
                -1.0f,
                1.0f);

  RNA_def_boolean(ot->srna,
                  "flatten_hierarchy",
                  0,
                  "Flatten Hierarchy",
                  "Do not preserve objects' parent/children relationship");

  RNA_def_boolean(ot->srna, "export_animations", 0, "Animations", "Export animations");

  RNA_def_boolean(ot->srna, "export_normals", 1, "Normals", "Export normals");

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

  RNA_def_boolean(ot->srna, "export_uvs", 1, "UVs", "Export UVs");

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

  RNA_def_boolean(ot->srna, "export_vcolors", 0, "Vertex Colors", "Export vertex colors");

  RNA_def_boolean(
      ot->srna, "export_face_sets", 0, "Face Sets", "Export per face shading group assignments");

  RNA_def_boolean(ot->srna, "export_vweights", 0, "Vertex Weights", "Exports vertex weights");

  RNA_def_boolean(
      ot->srna, "export_particles", 0, "Export Particles", "Exports non-hair particle systems");

  RNA_def_boolean(ot->srna,
                  "export_hair",
                  0,
                  "Export Hair",
                  "Exports hair particle systems as animated curves");

  RNA_def_boolean(ot->srna,
                  "export_child_hairs",
                  0,
                  "Export Child Hairs",
                  "Exports child hair particles as animated curves");

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
      ot->srna, "apply_modifiers", 0, "Apply Modifiers", "Apply modifiers before exporting");

  RNA_def_boolean(ot->srna,
                  "render_modifiers",
                  0,
                  "Use Render Modifiers",
                  "Whether to use Render or View modifier stettings");

  RNA_def_boolean(ot->srna,
                  "curves_as_mesh",
                  false,
                  "Curves as Mesh",
                  "Export curves and NURBS surfaces as meshes");

  RNA_def_boolean(ot->srna, "pack_uv", 1, "Pack UV Islands", "Export UVs with packed island");

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

  /* This dummy prop is used to check whether we need to init the start and
   * end frame values to that of the scene's, otherwise they are reset at
   * every change, draw update. */
  RNA_def_boolean(ot->srna, "init_scene_frame_range", false, "", "");
}
void io_common_default_declare_import(struct wmOperatorType *ot)
{
} /* TODO someone */

ExportSettings *io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op)
{
  ExportSettings *settings = MEM_mallocN(sizeof(ExportSettings), "ExportSettings");

  settings->scene = CTX_data_scene(C);
  settings->view_layer = CTX_data_view_layer(C);

  settings->apply_modifiers = RNA_boolean_get(op->ptr, "apply_modifiers");
  settings->render_modifiers = RNA_boolean_get(op->ptr, "render_modifiers");

  // If render_modifiers use render depsgraph, to get render modifiers
  settings->depsgraph = DEG_graph_new(settings->scene,
                                      settings->view_layer,
                                      settings->render_modifiers ? DAG_EVAL_RENDER :
                                                                   DAG_EVAL_VIEWPORT);

  settings->main = CTX_data_main(C);

  RNA_string_get(op->ptr, "filepath", settings->filepath);

  settings->axis_forward = RNA_enum_get(op->ptr, "axis_forward");
  settings->axis_up = RNA_enum_get(op->ptr, "axis_up");
  settings->selected_only = RNA_boolean_get(op->ptr, "selected_only");
  settings->visible_only = RNA_boolean_get(op->ptr, "visible_only");
  settings->renderable_only = RNA_boolean_get(op->ptr, "renderable_only");
  settings->frame_start = RNA_int_get(op->ptr, "start");
  settings->frame_end = RNA_int_get(op->ptr, "end");
  settings->frame_samples_xform = RNA_int_get(op->ptr, "xsamples");
  settings->frame_samples_shape = RNA_int_get(op->ptr, "gsamples");
  settings->shutter_open = RNA_float_get(op->ptr, "sh_open");
  settings->shutter_close = RNA_float_get(op->ptr, "sh_close");
  settings->flatten_hierarchy = RNA_boolean_get(op->ptr, "flatten_hierarchy");
  settings->export_animations = RNA_boolean_get(op->ptr, "export_animations");
  settings->export_normals = RNA_boolean_get(op->ptr, "export_normals");
  settings->dedup_normals = RNA_boolean_get(op->ptr, "dedup_normals");
  settings->dedup_normals_threshold = RNA_float_get(op->ptr, "dedup_normals_threshold");
  settings->export_uvs = RNA_boolean_get(op->ptr, "export_uvs");
  settings->dedup_uvs = RNA_boolean_get(op->ptr, "dedup_uvs");
  settings->dedup_uvs_threshold = RNA_float_get(op->ptr, "dedup_uvs_threshold");
  settings->export_edges = RNA_boolean_get(op->ptr, "export_edges");
  settings->export_materials = RNA_boolean_get(op->ptr, "export_materials");
  settings->export_vcolors = RNA_boolean_get(op->ptr, "export_vcolors");
  settings->export_face_sets = RNA_boolean_get(op->ptr, "export_face_sets");
  settings->export_vweights = RNA_boolean_get(op->ptr, "export_vweights");
  settings->export_particles = RNA_boolean_get(op->ptr, "export_particles");
  settings->export_hair = RNA_boolean_get(op->ptr, "export_hair");
  settings->export_child_hairs = RNA_boolean_get(op->ptr, "export_child_hairs");
  settings->export_objects_as_objects = RNA_boolean_get(op->ptr, "export_objects_as_objects");
  settings->export_objects_as_groups = RNA_boolean_get(op->ptr, "export_objects_as_groups");
  settings->curves_as_mesh = RNA_boolean_get(op->ptr, "curves_as_mesh");
  settings->pack_uv = RNA_boolean_get(op->ptr, "pack_uv");
  settings->triangulate = RNA_boolean_get(op->ptr, "triangulate");
  settings->quad_method = RNA_enum_get(op->ptr, "quad_method");
  settings->ngon_method = RNA_enum_get(op->ptr, "ngon_method");
  settings->global_scale = RNA_float_get(op->ptr, "global_scale");
  settings->use_ascii = RNA_boolean_get(op->ptr, "use_ascii");
  settings->use_scene_units = RNA_boolean_get(op->ptr, "use_scene_units");

  if (!settings->export_animations) {
    settings->frame_start = BKE_scene_frame_get(CTX_data_scene(C));
    ;
    settings->frame_end = settings->frame_start;
  }

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

  ExportSettings *settings = io_common_construct_default_export_settings(C, op);
  bool ok = exporter(C, settings);

  BKE_scene_frame_set(CTX_data_scene(C), orig_frame);

  return ok ? OPERATOR_FINISHED : OPERATOR_CANCELLED;
}
