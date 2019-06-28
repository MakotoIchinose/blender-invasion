extern "C" {

#include "BLI_math.h"
#include "BLI_sys_types.h"
#include "BLT_translation.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_global.h"
#include "BKE_context.h"
#include "BKE_scene.h"
#include "BKE_library.h"
#include "BKE_customdata.h"

/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#define new extern_new
#include "BKE_screen.h"
#undef new

#include "DNA_space_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
#include "DNA_ID.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "MEM_guardedalloc.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_object.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "stl.h"
#include "../io_common.h"
#include "../io_stl.h"
}

#include <chrono>
#include <iostream>
#include <ios>
#include <fstream>
#include <cstdint>

#include "common.hpp"
#include "iterators.hpp"

namespace {
bool STL_export_mesh_ascii(bContext *UNUSED(C),
                           ExportSettings *const UNUSED(settings),
                           std::fstream &fs,
                           const Object *const ob,
                           const Mesh *const mesh)
{
  // TODO someone Is it ok to add the version info after a # in STL?
  const std::string name = common::get_object_name(ob, mesh) + " # " +
                           common::get_version_string();
  fs << std::scientific;
  fs << "solid " << name;
  for (const MPoly &mp : common::poly_iter{mesh}) {
    const std::array<float, 3> no = common::calculate_normal(mesh, mp);
    fs << "facet normal " << no[0] << ' ' << no[1] << ' ' << no[2] << "\nouter loop";
    for (const MVert &v : common::vert_of_poly_iter{mesh, mp})
      fs << "\nvertex " << v.co[0] << ' ' << v.co[1] << ' ' << v.co[2];
    fs << "\nendloop\nendfacet\n";
  }
  fs << "endsolid " << name;
  return true;
}

const int __one__ = 1;
// CPU endianness
const bool little_endian = 1 == *(char *)(&__one__);

template<typename T, size_t size = sizeof(T)>
std::fstream &operator<<(std::fstream &fs, const T &v)
{
  if (little_endian) {
    std::cerr << std::scientific << " ";
    std::cerr << v;
    fs.write((char *)&v, size);
  }
  else {
    char bytes[size], *pv = (char *)&v;
    for (int i = 0; i < size; ++i)
      bytes[i] = pv[size - 1 - i];
    fs.write(bytes, size);
  }
  return fs;
}

bool STL_export_mesh_bin(bContext *UNUSED(C),
                         ExportSettings *const UNUSED(settings),
                         std::fstream &fs,
                         const Object *const UNUSED(ob),
                         const Mesh *const mesh)
{
  char header[80] = {'\0'};
  std::uint32_t tri = mesh->totpoly;
  std::uint16_t attribute = 0;
  fs << header << tri;
  for (const MPoly &mp : common::poly_iter{mesh}) {
    const std::array<float, 3> no = common::calculate_normal(mesh, mp);
    fs << no[0] << no[1] << no[2];
    for (const MVert &v : common::vert_of_poly_iter{mesh, mp})
      fs << v.co[0] << v.co[1] << v.co[2];
    fs << attribute;
  }
  return true;
}

void STL_export_start(bContext *C, ExportSettings *const settings)
{
  common::export_start(C, settings);
  std::fstream fs;
  fs.open(settings->filepath, std::ios::out | std::ios::trunc);
  Scene *escene = DEG_get_evaluated_scene(settings->depsgraph);
  for (const Object *const ob : common::exportable_object_iter{settings->view_layer, settings}) {
    Mesh *mesh;
    settings->triangulate = true;  // STL only really works with triangles
    float mat[4][4];
    bool needs_free = common::get_final_mesh(settings, escene, ob, &mesh, &mat);
    if (((STLExportSettings *)settings->format_specific)->use_ascii) {
      if (!STL_export_mesh_ascii(C, settings, fs, ob, mesh))
        return;
    }
    else {
      if (!STL_export_mesh_bin(C, settings, fs, ob, mesh))
        return;
    }
    common::free_mesh(mesh, needs_free);
  }
}

bool STL_export_end(bContext *C, ExportSettings *const settings)
{
  return common::export_end(C, settings);
}
}  // namespace

extern "C" {
bool STL_export(bContext *C, ExportSettings *const settings)
{
  return common::time_export(C, settings, &STL_export_start, &STL_export_end);
}
}  // extern
