extern "C" {

#include "BLI_sys_types.h"
#include "BLI_math.h"
#include "BLT_translation.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_global.h"
#include "BKE_context.h"
#include "BKE_scene.h"
#include "BKE_library.h"
#include "BKE_customdata.h"

#include "DNA_ID.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"

#include "DEG_depsgraph.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_object.h"
#include "bmesh.h"
#include "bmesh_tools.h"

#include "obj.h"
#include "../io_common.h"
}

#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "iterators.hpp"

/*
  TODO someone: () not done, -- done, # maybe add, ? unsure
  presets
  axis remap -- doesn't work. Does it need to update, somehow?
    DEG_id_tag_update(&mesh->id, 0); obedit->id.recalc & ID_RECALC_ALL
  --selection only
  animation - partly
  --apply modifiers
  --render modifiers -- mesh_create_derived_{view,render}, deg_get_mode
  --edges
  smooth groups
  bitflag smooth groups?
  --normals
  --uvs
  materials
  material groups
  path mode -- python repr
  --triangulate
  nurbs
  polygroups?
  --obj objects
  --obj groups
  -?vertex order
  --scale
  # units?
  # removing duplicates with a threshold and as an option
  TODO someone filter_glob : StringProp weird Python syntax

 */

namespace {

using namespace common;

// bool obj_export_materials(bcontext *unused(c),
//                           exportsettings *unused(settings),
//                           std::fstream &fs,
//                           std::map<std::string, const material *const> materials)
// {
//   fs << "# blender mtl file\n"; /* todo someone filename necessary? */
//   fs << "# material count: " << materials.size() << '\n';
//   for (auto p : materials) {
//     const material *const mat = p.second;
//     bli_assert(mat != nullptr);
//     fs << "newmtl " << (mat->id.name + 2) << '\n';
//     // todo someone ka seems to no longer make sense, without bi
//     fs << "kd " << mat->r << mat->g << mat->b << '\n';
//     fs << "ks " << ((1 - mat->roughness) * mat->r) << ((1 - mat->roughness) * mat->g)
//        << ((1 - mat->roughness) * mat->b) << '\n';
//     // todo someone it doens't seem to make sense too use this,
//     // unless i use the principled bsdf settings, instead of the viewport
//   }
//   return true;
// }

bool OBJ_export_mesh(bContext *UNUSED(C),
                     ExportSettings *settings,
                     std::FILE *file,
                     const Object *eob,
                     Mesh *mesh,
                     ulong &vertex_total,
                     ulong &uv_total,
                     ulong &no_total,
                     dedup_pair_t<uv_key_t> &uv_mapping_pair /* IN OUT */,
                     dedup_pair_t<no_key_t> &no_mapping_pair /* IN OUT */)
{

  auto &uv_mapping = uv_mapping_pair.second;
  auto &no_mapping = no_mapping_pair.second;

  ulong uv_initial_count = uv_mapping.size();
  ulong no_initial_count = no_mapping.size();

  if (mesh->totvert == 0)
    return true;

  if (settings->export_objects_as_objects || settings->export_objects_as_groups) {
    std::string name = common::get_object_name(eob, mesh);
    if (settings->export_objects_as_objects)
      fprintf(file, "o %s\n", name.c_str());
    else
      fprintf(file, "g %s\n", name.c_str());
  }

  for (const MVert &v : common::vert_iter{mesh})
    fprintf(file, "v %.6g %.6g %.6g\n", v.co[0], v.co[1], v.co[2]);
  // auto vxs = common::get_vertices(mesh);
  // for (const auto &v : vxs)
  //   fs << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';

  if (settings->export_uvs) {
    // TODO someone Is T47010 still relevant?
    if (settings->dedup_uvs)
      for (const std::array<float, 2> &uv :
           common::deduplicated_uv_iter(mesh, uv_total, uv_mapping_pair))
        fprintf(file, "vt %.6g %.6g\n", uv[0], uv[1]);
    else
      for (const std::array<float, 2> &uv : common::uv_iter{mesh})
        fprintf(file, "vt %.6g %.6g\n", uv[0], uv[1]);
    // auto uvs = common::get_uv(mesh);
    // for (const auto &uv : uvs)
    //   fs << "vt " << uv[0] << ' ' << uv[1] << '\n';
  }

  if (settings->export_normals) {
    if (settings->dedup_normals)
      for (const std::array<float, 3> &no :
           common::deduplicated_normal_iter{mesh, no_total, no_mapping_pair})
        fprintf(file, "vn %.4g %.4g %.4g\n", no[0], no[1], no[2]);
    else
      for (const std::array<float, 3> &no : common::normal_iter{mesh}) {
        fprintf(file, "vn %.4g %.4g %.4g\n", no[0], no[1], no[2]);
      }
    // auto nos = common::get_normals(mesh);
    // for (const auto &no : nos)
    //   fs << "vn " << no[0] << ' ' << no[1] << ' ' << no[2] << '\n';
  }

  std::cerr << "Totals: " << uv_total << " " << no_total << "\nSizes: " << uv_mapping.size() << " "
            << no_mapping.size() << '\n';

  for (const MPoly &p : common::poly_iter(mesh)) {
    fputc('f', file);
    // Loop index
    int li = p.loopstart;
    for (const MLoop &l : common::loop_of_poly_iter(mesh, p)) {
      ulong vx = vertex_total + l.v;
      ulong uv = 0;
      ulong no = 0;
      if (settings->export_uvs) {
        if (settings->dedup_uvs)
          uv = uv_mapping[uv_initial_count + li]->second;
        else
          uv = uv_initial_count + li;
      }
      if (settings->export_normals) {
        if (settings->dedup_normals)
          no = no_mapping[no_initial_count + l.v]->second;
        else
          no = no_initial_count + l.v;
      }
      if (settings->export_uvs && settings->export_normals)
        fprintf(file, " %lu/%lu/%lu", vx, uv, no);
      else if (settings->export_uvs)
        fprintf(file, " %lu/%lu", vx, uv);
      else if (settings->export_normals)
        fprintf(file, " %lu//%lu", vx, no);
      else
        fprintf(file, " %lu", vx);
    }
    fputc('\n', file);
  }

  if (settings->export_edges) {
    for (const MEdge &e : common::loose_edge_iter{mesh})
      fprintf(file, "l %lu %lu\n", vertex_total + e.v1, vertex_total + e.v2);
  }

  vertex_total += mesh->totvert;
  uv_total += mesh->totloop;
  no_total += mesh->totvert;
  return true;
}

bool OBJ_export_object(bContext *C,
                       ExportSettings *const settings,
                       Scene *scene,
                       const Object *ob,
                       std::FILE *file,
                       ulong &vertex_total,
                       ulong &uv_total,
                       ulong &no_total,
                       dedup_pair_t<uv_key_t> &uv_mapping_pair,
                       dedup_pair_t<no_key_t> &no_mapping_pair)
{
  // TODO someone Should it be evaluated first? Is this expensive? Breaks mesh_create_eval_final
  // Object *eob = DEG_get_evaluated_object(settings->depsgraph, base->object);

  struct Mesh *mesh = nullptr;
  bool needs_free = false;

  switch (ob->type) {
    case OB_MESH:
      needs_free = common::get_final_mesh(settings, scene, ob, &mesh /* OUT */);

      if (!OBJ_export_mesh(C,
                           settings,
                           file,
                           ob,
                           mesh,
                           vertex_total,
                           uv_total,
                           no_total,
                           uv_mapping_pair,
                           no_mapping_pair))
        return false;

      common::free_mesh(mesh, needs_free);
      return true;
    default:
      // TODO someone Probably abort, it shouldn't be possible to get here (once finished)
      std::cerr << "OBJ Export for the object \"" << ob->id.name << "\" is not implemented\n";
      // BLI_assert(false);
      return false;
  }
}

void OBJ_export_start(bContext *C, ExportSettings *const settings)
{
  common::export_start(C, settings);

  std::FILE *file = std::fopen(settings->filepath, "w");
  if (file == nullptr) {
    std::cerr << "Couldn't open file: " << settings->filepath << '\n';
    return;
  }
  fprintf(file, "# %s\n# www.blender.org\n", common::get_version_string().c_str());

  // If not exporting animations, the start and end are the same
  for (int frame = settings->frame_start; frame <= settings->frame_end; ++frame) {
    BKE_scene_frame_set(settings->scene, frame);
    BKE_scene_graph_update_for_newframe(settings->depsgraph, settings->main);
    Scene *escene = DEG_get_evaluated_scene(settings->depsgraph);
    ulong vertex_total = 0, uv_total = 0, no_total = 0;

    auto uv_mapping_pair = common::make_deduplicate_set<uv_key_t>(settings->dedup_uvs_threshold);
    auto no_mapping_pair = common::make_deduplicate_set<no_key_t>(
        settings->dedup_normals_threshold);

    // TODO someone if not exporting as objects, do they need to all be merged?
    for (const Object *const ob : common::exportable_object_iter{settings->view_layer, settings})
      if (!OBJ_export_object(C,
                             settings,
                             escene,
                             ob,
                             file,
                             vertex_total,
                             uv_total,
                             no_total,
                             uv_mapping_pair,
                             no_mapping_pair))
        return;
  }
}

bool OBJ_export_end(bContext *C, ExportSettings *const settings)
{
  return common::export_end(C, settings);
}
}  // namespace

extern "C" {
bool OBJ_export(bContext *C, ExportSettings *const settings)
{
  return common::time_export(C, settings, &OBJ_export_start, &OBJ_export_end);
}
}  // extern
