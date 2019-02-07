
#include "WM_api.h"
#include "WM_types.h"

#include "DNA_space_types.h"

#ifndef __IO_COMMON_H__
#define __IO_COMMON_H__

struct bContext;
struct Depsgraph;

typedef struct ExportSettings {

	/* Alembic */

	struct Scene *scene;
	struct ViewLayer *view_layer;  // Scene layer to export; all its objects will be exported, unless selected_only=true
	struct Depsgraph *depsgraph;
	/* SimpleLogger logger; */

	char filepath[FILE_MAX];

	bool selected_only;
	bool visible_only;
	bool renderable_only;

	double frame_start;
	double frame_end;
	double frame_samples_xform;
	double frame_samples_shape;
	double shutter_open;
	double shutter_close;

	bool flatten_hierarchy;

	bool export_normals;
	bool export_uvs;
	bool export_vcolors;
	bool export_face_sets;
	bool export_vweigths;
	bool export_particles;
	bool export_hair;
	bool export_child_hairs;

	bool apply_subdiv;
	bool curves_as_mesh;
	bool pack_uv;
	bool triangulate;
	int quad_method;
	int ngon_method;

	float global_scale;

	/* bool use_subdiv_schema; */
	/* bool export_ogawa; */
	/* bool do_convert_axis; */
	/* float convert_matrix[3][3]; */


	/* Collada */

	/* bool apply_modifiers; */
	/* BC_export_mesh_type export_mesh_type; */

	/* bool include_children; */
	/* bool include_armatures; */
	/* bool include_shapekeys; */
	/* bool include_animations; */
	/* bool include_all_actions; */
	/* bool deform_bones_only; */
	/* int sampling_rate; */
	/* bool keep_smooth_curves; */
	/* bool keep_keyframes; */
	/* bool keep_flat_curves; */

	/* bool active_uv_only; */
	/* /\* BC_export_animation_type export_animation_type; *\/ */
	/* bool use_texture_copies; */

	/* bool triangulate; */
	/* bool use_object_instantiation; */
	/* bool use_blender_profile; */
	/* bool sort_by_name; */
	/* BC_export_transformation_type export_transformation_type; */

	/* bool open_sim; */
	/* bool limit_precision; */
	/* bool keep_bind_info; */

	/* char *filepath; */
	/* LinkNode *export_set; */
} ExportSettings;

void io_common_default_declare_export(struct wmOperatorType *ot,
                                      eFileSel_File_Types file_type);

void io_common_default_declare_import(struct wmOperatorType *ot);

bool io_common_export_check(struct bContext *UNUSED(C), wmOperator *op, const char *ext);
int io_common_export_invoke(struct bContext *C, wmOperator *op,
                            const wmEvent *UNUSED(event), const char *ext);
int io_common_export_exec(struct bContext *C, struct wmOperator *op,
                          bool (*exporter)(struct bContext *C, ExportSettings *settings));

ExportSettings * io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op);
#endif  /* __IO_COMMON_H__ */
