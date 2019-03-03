/* #include "BLI_listbase.h" */

/* #include "DNA_mesh_types.h" */
/* #include "DNA_object_types.h" */

#include "DNA_layer_types.h"

#include "../io_common.h"

bool common_should_export_object(const ExportSettings * const settings, const Base * const ob_base,
                                 bool is_duplicated);


bool common_object_type_is_exportable(Object *ob);

bool get_final_mesh(ExportSettings *settings, Scene *escene, Object *eob, Mesh *mesh);
