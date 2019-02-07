/* #include "BLI_listbase.h" */

/* #include "DNA_mesh_types.h" */
/* #include "DNA_object_types.h" */

#include "DNA_layer_types.h"

#include "../io_common.h"

bool common_export_object_p(const ExportSettings * const settings, const Base * const ob_base,
                             bool is_duplicated);


bool common_object_type_is_exportable(Scene *scene, Object *ob);


static bool common_object_is_smoke_sim(Object *ob);
