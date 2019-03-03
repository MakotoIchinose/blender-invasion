#include "io_common.h"

#include "DNA_modifier_types.h"

#include "BLI_string.h"
#include "BLI_path_util.h"

#include "BLT_translation.h"

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

#include "UI_interface.h"
#include "UI_resources.h"

#include "ED_object.h"

#include "MEM_guardedalloc.h"

void io_common_default_declare_export(struct wmOperatorType *ot,
                                      eFileSel_File_Types file_type) {

	WM_operator_properties_filesel(ot, FILE_TYPE_FOLDER | file_type,
	                               FILE_BLENDER, FILE_SAVE, WM_FILESEL_FILEPATH,
	                               FILE_DEFAULTDISPLAY, FILE_SORT_ALPHA);

	RNA_def_boolean(ot->srna, "selected_only", 0,
	                "Selected Objects Only", "Export only selected objects");

	RNA_def_boolean(ot->srna, "visible_only", 0,
	                "Visible Objects Only",
	                "Export only objects marked as visible");

	RNA_def_boolean(ot->srna, "renderable_only", 1,
	                "Renderable Objects Only",
	                "Export only objects marked renderable in the outliner");


	RNA_def_int(ot->srna, "start", INT_MIN, INT_MIN, INT_MAX,
	            "Start Frame",
	            "Start frame of the export, use the default value to "
	            "take the start frame of the current scene",
	            INT_MIN, INT_MAX);

	RNA_def_int(ot->srna, "end", INT_MIN, INT_MIN, INT_MAX,
	            "End Frame",
	            "End frame of the export, use the default value to "
	            "take the end frame of the current scene",
	            INT_MIN, INT_MAX);

	RNA_def_int(ot->srna, "xsamples", 1, 1, 128,
	            "Transform Samples",
	            "Number of times per frame transformations are sampled", 1, 128);

	RNA_def_int(ot->srna, "gsamples", 1, 1, 128,
	            "Geometry Samples", "Number of times per frame object data are sampled", 1, 128);

	RNA_def_float(ot->srna, "sh_open", 0.0f, -1.0f, 1.0f,
	              "Shutter Open", "Time at which the shutter is open", -1.0f, 1.0f);

	RNA_def_float(ot->srna, "sh_close", 1.0f, -1.0f, 1.0f,
	              "Shutter Close", "Time at which the shutter is closed", -1.0f, 1.0f);

	RNA_def_boolean(ot->srna, "flatten_hierarchy", 0,
	                "Flatten Hierarchy",
	                "Do not preserve objects' parent/children relationship");

	RNA_def_boolean(ot->srna, "export_animations", 0, "Animations",
	                "Export animations");

	RNA_def_boolean(ot->srna, "export_normals", 1, "Normals",
	                "Export normals");

	RNA_def_boolean(ot->srna, "export_uvs", 1, "UVs", "Export UVs");

	RNA_def_boolean(ot->srna, "export_materials", 0, "Materials", "Export Materials");

	RNA_def_boolean(ot->srna, "export_vcolors", 0, "Vertex Colors",
	                "Export vertex colors");

	RNA_def_boolean(ot->srna, "export_face_sets", 0, "Face Sets",
	                "Export per face shading group assignments");

	RNA_def_boolean(ot->srna, "export_vweights", 0, "Vertex Weights",
	                "Exports vertex weights");

	RNA_def_boolean(ot->srna, "export_particles", 0, "Export Particles",
	                "Exports non-hair particle systems");

	RNA_def_boolean(ot->srna, "export_hair", 0, "Export Hair",
	                "Exports hair particle systems as animated curves");

	RNA_def_boolean(ot->srna, "export_child_hairs", 0, "Export Child Hairs",
	                "Exports child hair particles as animated curves");

	RNA_def_boolean(ot->srna, "export_objects_as_objects", 1,
	                "Export as Objects named like Blender Objects",
	                "Export Blender Object as named objects");

	RNA_def_boolean(ot->srna, "export_objects_as_groups", 0,
	                "Export as Groups named like Blender Objects",
	                "Export Blender Objects as named groups");

	RNA_def_boolean(ot->srna, "apply_subdiv", 0,
	                "Apply Subsurf", "Export subdivision surfaces as meshes");

	RNA_def_boolean(ot->srna, "curves_as_mesh", false,
	                "Curves as Mesh", "Export curves and NURBS surfaces as meshes");

	RNA_def_boolean(ot->srna, "pack_uv", 1, "Pack UV Islands",
	                "Export UVs with packed island");

	RNA_def_boolean(ot->srna, "triangulate", false, "Triangulate",
	                "Export Polygons (Quads & NGons) as Triangles");

	RNA_def_enum(ot->srna, "quad_method", rna_enum_modifier_triangulate_quad_method_items,
	             MOD_TRIANGULATE_QUAD_SHORTEDGE, "Quad Method",
	             "Method for splitting the quads into triangles");

	RNA_def_enum(ot->srna, "ngon_method", rna_enum_modifier_triangulate_quad_method_items,
	             MOD_TRIANGULATE_NGON_BEAUTY, "Polygon Method",
	             "Method for splitting the polygons into triangles");

	RNA_def_float(ot->srna, "global_scale", 1.0f, 0.0001f, 1000.0f, "Scale",
	              "Value by which to enlarge or shrink the objects with"
	              "respect to the world's origin",
	              0.0001f, 1000.0f);

	/* This dummy prop is used to check whether we need to init the start and
	 * end frame values to that of the scene's, otherwise they are reset at
	 * every change, draw update. */
	RNA_def_boolean(ot->srna, "init_scene_frame_range", false, "", "");
}
void io_common_default_declare_import(struct wmOperatorType *ot) {} /* TODO someone */

ExportSettings * io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op) {
	ExportSettings *settings = MEM_mallocN(sizeof(ExportSettings), "ExportSettings");

	settings->scene               = CTX_data_scene(C);
	settings->view_layer          = CTX_data_view_layer(C);
	settings->depsgraph           = DEG_graph_new(settings->scene, settings->view_layer,
	                                              DAG_EVAL_RENDER);
	settings->main                = CTX_data_main(C);

	RNA_string_get(op->ptr, "filepath", settings->filepath);

	settings->selected_only            = RNA_boolean_get(op->ptr, "selected_only");
	settings->visible_only             = RNA_boolean_get(op->ptr, "visible_only");
	settings->renderable_only          = RNA_boolean_get(op->ptr, "renderable_only");

	settings->frame_start              = RNA_int_get(op->ptr, "start");
	settings->frame_end                = RNA_int_get(op->ptr, "end");
	settings->frame_samples_xform      = RNA_int_get(op->ptr, "xsamples");
	settings->frame_samples_shape      = RNA_int_get(op->ptr, "gsamples");
	settings->shutter_open             = RNA_float_get(op->ptr, "sh_open");
	settings->shutter_close            = RNA_float_get(op->ptr, "sh_close");

	settings->flatten_hierarchy        = RNA_boolean_get(op->ptr, "flatten_hierarchy");

	settings->export_animations        = RNA_boolean_get(op->ptr, "export_animations");
	settings->export_normals           = RNA_boolean_get(op->ptr, "export_normals");
	settings->export_uvs               = RNA_boolean_get(op->ptr, "export_uvs");
	settings->export_materials         = RNA_boolean_get(op->ptr, "export_materials");
	settings->export_vcolors           = RNA_boolean_get(op->ptr, "export_vcolors");
	settings->export_face_sets         = RNA_boolean_get(op->ptr, "export_face_sets");
	settings->export_vweights          = RNA_boolean_get(op->ptr, "export_vweights");
	settings->export_particles         = RNA_boolean_get(op->ptr, "export_particles");
	settings->export_hair              = RNA_boolean_get(op->ptr, "export_hair");
	settings->export_child_hairs       = RNA_boolean_get(op->ptr, "export_child_hairs");
	settings->export_objects_as_objects= RNA_boolean_get(op->ptr, "export_objects_as_objects");
	settings->export_objects_as_groups = RNA_boolean_get(op->ptr, "export_objects_as_groups");


	settings->apply_subdiv             = RNA_boolean_get(op->ptr, "apply_subdiv");
	settings->curves_as_mesh           = RNA_boolean_get(op->ptr, "curves_as_mesh");
	settings->pack_uv                  = RNA_boolean_get(op->ptr, "pack_uv");
	settings->triangulate              = RNA_boolean_get(op->ptr, "triangulate");
	settings->quad_method              = RNA_enum_get(op->ptr, "quad_method");
	settings->ngon_method              = RNA_enum_get(op->ptr, "ngon_method");

	settings->global_scale             = RNA_float_get(op->ptr, "global_scale");

	if(!settings->export_animations) {
		settings->frame_start = BKE_scene_frame_get(CTX_data_scene(C));;
		settings->frame_end   = settings->frame_start;
	}

	return settings;
}

bool io_common_export_check(bContext *UNUSED(C), wmOperator *op, const char *ext) {
	char filepath[FILE_MAX];
	RNA_string_get(op->ptr, "filepath", filepath);

	if (!BLI_path_extension_check(filepath, ext)) {
		BLI_path_extension_ensure(filepath, FILE_MAX, ext);
		RNA_string_set(op->ptr, "filepath", filepath);
		return true;
	}

	return false;
}

int io_common_export_invoke(bContext *C, wmOperator *op,
                            const wmEvent *UNUSED(event), const char *ext) {

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

void io_common_export_draw(bContext *C, wmOperator *op) {
	PointerRNA ptr;
	RNA_pointer_create(NULL, op->type->srna, op->properties, &ptr);

	/* Conveniently set start and end frame to match the scene's frame range. */
	Scene *scene = CTX_data_scene(C);

	/* So we only do this once, use the dummy property */
	if (scene != NULL && RNA_boolean_get(&ptr, "init_scene_frame_range")) {
		RNA_int_set(&ptr, "start", SFRA);
		RNA_int_set(&ptr, "end", EFRA);

		RNA_boolean_set(&ptr, "init_scene_frame_range", false);
	}

	/* ui_alembic_export_settings(op->layout, &&ptr); */

	uiLayout *box;
	uiLayout *row;
	/* uiLayout *col; */

	/* Scene Options */
	box = uiLayoutBox(op->layout);
	row = uiLayoutRow(box, false);
	uiItemL(row, IFACE_("Scene Options:"), ICON_SCENE_DATA);

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
	uiItemR(row, &ptr, "export_normals", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_uvs", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "pack_uv", 0, NULL, ICON_NONE);
	uiLayoutSetEnabled(row, RNA_boolean_get(&ptr, "export_uvs"));

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_materials", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_vcolors", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "export_face_sets", 0, NULL, ICON_NONE);

	row = uiLayoutRow(box, false);
	uiItemR(row, &ptr, "apply_subdiv", 0, NULL, ICON_NONE);

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

int io_common_export_exec(struct bContext *C, struct wmOperator *op,
                          bool (*exporter)(struct bContext *C, ExportSettings *settings)) {
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

