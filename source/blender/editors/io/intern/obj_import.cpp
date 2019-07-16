extern "C" {

#include "BLI_sys_types.h"
#include "BLI_math.h"
#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_global.h"
#include "BKE_library.h"
#include "BKE_mesh.h"
#include "BKE_mesh_mapping.h"
#include "BKE_mesh_runtime.h"
#include "BKE_scene.h"

#include "DNA_curve_types.h"
#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "WM_api.h"
#include "WM_types.h"

#ifdef WITH_PYTHON
#  include "BPY_extern.h"
#endif

#include "ED_object.h"
#include "bmesh.h"
#include "bmesh_tools.h"

#include "obj.h"
#include "../io_common.h"
#include "../io_obj.h"
}

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "iterators.hpp"

namespace {
void OBJ_import_start(bContext *C, ImportSettings *const settings)
{
  common::import_start(C, settings);
  std::cerr << "IMPORTING\n";
}

bool OBJ_import_end(bContext *C, ImportSettings *const settings)
{
  return common::import_end(C, settings);
}
}  // namespace

extern "C" {
bool OBJ_import(bContext *C, ImportSettings *const settings)
{
  return common::time(C, settings, &OBJ_import_start, &OBJ_import_end);
}
}
