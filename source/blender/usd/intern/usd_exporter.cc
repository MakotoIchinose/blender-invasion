/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup USD
 */

#include "usd_exporter.h"

#include <pxr/pxr.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>

#include <cmath>
#include <time.h>

extern "C" {
#include "BKE_mesh_runtime.h"
#include "BKE_scene.h"

#include "BLI_iterator.h"
#include "BLI_math_matrix.h"

#include "DEG_depsgraph_query.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_object_types.h"
}

USDExporter::USDExporter(const char *filename, ExportSettings &settings)
    : m_settings(settings), m_filename(filename)
{
}

USDExporter::~USDExporter()
{
}

void USDExporter::operator()(float &r_progress, bool &r_was_canceled)
{
  timespec ts_begin;
  clock_gettime(CLOCK_MONOTONIC, &ts_begin);

  r_progress = 0.0;
  r_was_canceled = false;

  // Create a stage with the Only Correct up-axis.
  m_stage = pxr::UsdStage::CreateNew(m_filename);
  m_stage->SetMetadata(pxr::UsdGeomTokens->upAxis, pxr::VtValue(pxr::UsdGeomTokens->z));

  // Approach copied from draw manager.
  DEG_OBJECT_ITER_FOR_RENDER_ENGINE_BEGIN (m_settings.depsgraph, ob_eval) {
    if (export_object(ob_eval, data_)) {
      // printf("Breaking out of USD export loop after first object\n");
      // break;
    }
  }
  DEG_OBJECT_ITER_FOR_RENDER_ENGINE_END;

  m_stage->GetRootLayer()->Save();

  timespec ts_end;
  clock_gettime(CLOCK_MONOTONIC, &ts_end);
  double duration = double(ts_end.tv_sec - ts_begin.tv_sec) +
                    double(ts_end.tv_nsec - ts_begin.tv_nsec) / 1e9;
  printf("Export to USD took %.3f sec wallclock time\n", duration);

  r_progress = 1.0;
}

bool USDExporter::export_object(Object *ob_eval, const DEGObjectIterData &data_)
{
  const pxr::SdfPath root("/");
  Mesh *mesh = ob_eval->runtime.mesh_eval;
  pxr::SdfPath parent_path;
  float parent_relative_matrix[4][4];

  if (mesh == NULL || data_.dupli_object_current != NULL) {
    printf("USD-\033[34mSKIPPING\033[0m object %s  isinstance=%d type=%d mesh = %p\n",
           ob_eval->id.name,
           data_.dupli_object_current != NULL,
           ob_eval->type,
           mesh);
    return false;
  }
  printf("USD-\033[32mexporting\033[0m object %s  isinstance=%d type=%d mesh = %p\n",
         ob_eval->id.name,
         data_.dupli_object_current != NULL,
         ob_eval->type,
         mesh);

  // Compute the parent's SdfPath and get the object matrix relative to the parent.
  if (ob_eval->parent == NULL) {
    parent_path = root;
    copy_m4_m4(parent_relative_matrix, ob_eval->obmat);
  }
  else {
    USDPathMap::iterator path_it = usd_object_paths.find(ob_eval->parent);
    if (path_it == usd_object_paths.end()) {
      printf("USD-\033[31mSKIPPING\033[0m object %s because parent %s hasn't been seen yet\n",
             ob_eval->id.name,
             ob_eval->parent->id.name);
      return false;
    }
    parent_path = path_it->second;

    invert_m4_m4(ob_eval->imat, ob_eval->obmat);
    mul_m4_m4m4(parent_relative_matrix, ob_eval->parent->imat, ob_eval->obmat);
  }
  pxr::SdfPath xform_path = parent_path.AppendPath(pxr::SdfPath(ob_eval->id.name + 2));
  usd_object_paths[ob_eval] = xform_path;

  // Write the transform relative to the parent.
  pxr::UsdGeomXform xform = pxr::UsdGeomXform::Define(m_stage, xform_path);
  xform.AddTransformOp().Set(pxr::GfMatrix4d(parent_relative_matrix));

  // Write the mesh.
  pxr::SdfPath mesh_path(xform_path.AppendPath(pxr::SdfPath(std::string(mesh->id.name))));
  pxr::UsdGeomMesh usd_mesh = pxr::UsdGeomMesh::Define(m_stage, mesh_path);

  const MVert *verts = mesh->mvert;

  // TODO(Sybren): there is probably a more C++-y way to do this, which avoids copying the entire
  // mesh to a different structure. I haven't seen the approach below in the USD exporters for
  // Maya/Houdini, but it's simple and it works for now.
  pxr::VtArray<pxr::GfVec3f> usd_points;
  usd_points.reserve(mesh->totvert);
  for (int i = 0; i < mesh->totvert; ++i) {
    usd_points.push_back(pxr::GfVec3f(verts[i].co));
  }
  usd_mesh.CreatePointsAttr().Set(usd_points);

  pxr::VtArray<int> usd_face_vertex_counts, usd_face_indices;
  usd_face_vertex_counts.reserve(mesh->totpoly);
  usd_face_indices.reserve(mesh->totloop);

  MLoop *mloop = mesh->mloop;
  MPoly *mpoly = mesh->mpoly;

  for (int i = 0; i < mesh->totpoly; ++i, ++mpoly) {
    MLoop *loop = mloop + mpoly->loopstart;
    usd_face_vertex_counts.push_back(mpoly->totloop);
    for (int j = 0; j < mpoly->totloop; ++j, ++loop) {
      usd_face_indices.push_back(loop->v);
    }
  }
  usd_mesh.CreateFaceVertexCountsAttr().Set(usd_face_vertex_counts);
  usd_mesh.CreateFaceVertexIndicesAttr().Set(usd_face_indices);

  return true;
}
