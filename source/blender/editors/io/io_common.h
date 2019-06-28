
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_context.h"

#include "DNA_space_types.h"

#ifndef __IO_COMMON_H__
#  define __IO_COMMON_H__

struct bContext;
struct Depsgraph;

enum axis_remap { AXIS_X, AXIS_Y, AXIS_Z, AXIS_NEG_X, AXIS_NEG_Y, AXIS_NEG_Z };

enum path_reference_mode { AUTO, ABSOLUTE, RELATIVE, MATCH, STRIP, COPY };

extern const EnumPropertyItem axis_remap[];
extern const EnumPropertyItem path_reference_mode[];

typedef struct ExportSettings {

  /* Mostly From Alembic */

  struct ViewLayer *view_layer;
  // Scene layer to export; all its objects will be exported, unless selected_only=true
  struct Scene *scene;
  struct Main *main;
  struct Depsgraph *depsgraph;

  char filepath[FILE_MAX];

  enum axis_remap axis_forward;
  enum axis_remap axis_up;

  bool selected_only;
  bool visible_only;
  bool renderable_only;

  bool export_normals;
  bool export_uvs;
  bool export_edges;
  bool export_materials;
  bool export_vcolors;
  bool export_vweights;
  bool export_particles;
  bool export_hair;
  bool export_child_hairs;
  bool export_curves;

  bool apply_modifiers;
  bool render_modifiers;
  bool pack_uv;
  bool triangulate;
  int quad_method;
  int ngon_method;

  bool use_scene_units;
  float global_scale;

  void *format_specific;  // Pointer to struct with extra settings that vary by exporter

  /* bool (*should_export_object)(const ExportSettings * const settings, const Object * const eob);
   */

} ExportSettings;

void io_common_default_declare_export(struct wmOperatorType *ot, eFileSel_File_Types file_type);

void io_common_default_declare_import(struct wmOperatorType *ot);

bool io_common_export_check(struct bContext *UNUSED(C), wmOperator *op, const char *ext);
int io_common_export_invoke(struct bContext *C,
                            wmOperator *op,
                            const wmEvent *UNUSED(event),
                            const char *ext);
int io_common_export_exec(struct bContext *C,
                          struct wmOperator *op,
                          ExportSettings *settings,
                          bool (*exporter)(struct bContext *C, ExportSettings *settings));
/* void io_common_export_draw(bContext *C, wmOperator *op); */

ExportSettings *io_common_construct_default_export_settings(struct bContext *C,
                                                            struct wmOperator *op);

#endif /* __IO_COMMON_H__ */
