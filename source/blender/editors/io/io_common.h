
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_context.h"

#include "DNA_space_types.h"

#ifndef __IO_COMMON_H__
#define __IO_COMMON_H__

struct bContext;
struct Depsgraph;

enum axis_remap { AXIS_X, AXIS_Y, AXIS_Z, AXIS_NEG_X, AXIS_NEG_Y, AXIS_NEG_Z };

typedef struct ExportSettings {

	/* Mostly From Alembic */

	struct Scene *scene;
	struct ViewLayer *view_layer;  // Scene layer to export; all its objects will be exported, unless selected_only=true
	struct Depsgraph *depsgraph;
	struct Main *main;
	/* SimpleLogger logger; */

	char filepath[FILE_MAX];

	enum axis_remap axis_forward;
	enum axis_remap axis_up;

	bool selected_only;
	bool visible_only;
	bool renderable_only;

	float frame_start;
	float frame_end;
	float frame_samples_xform;
	float frame_samples_shape;
	float shutter_open;
	float shutter_close;

	bool flatten_hierarchy;

	bool export_animations;
	bool export_normals;
	bool dedup_normals;
	float dedup_normals_threshold;
	bool export_uvs;
	bool dedup_uvs;
	float dedup_uvs_threshold;
	bool export_edges;
	bool export_materials;
	bool export_vcolors;
	bool export_face_sets;
	bool export_vweights;
	bool export_particles;
	bool export_hair;
	bool export_child_hairs;
	bool export_objects_as_objects;
	bool export_objects_as_groups;

	bool apply_modifiers;
	bool render_modifiers;
	bool curves_as_mesh;
	bool pack_uv;
	bool triangulate;
	int quad_method;
	int ngon_method;

	bool use_ascii;
	bool use_scene_units;
	float global_scale;

	/* bool (*should_export_object)(const ExportSettings * const settings, const Object * const eob); */

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
/* void io_common_export_draw(bContext *C, wmOperator *op); */

ExportSettings * io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op);

#endif  /* __IO_COMMON_H__ */
