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
#include "usd_writer_mesh.h"
#include "usd_writer_transform.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "BKE_mesh_runtime.h"
#include "BKE_scene.h"

#include "BLI_iterator.h"
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
  Timer timer_("Writing to USD");

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

  r_progress = 1.0;
}

bool USDExporter::export_object(Object *ob_eval, const DEGObjectIterData &degiter_data)
{
  const pxr::SdfPath root("/");
  pxr::SdfPath parent_path;

  // Compute the parent's SdfPath.
  if (ob_eval->parent == NULL) {
    parent_path = root;
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
  }

  // Write the transform. This is always done, even when we don't write the data, as it makes it
  // possible to reference collection-instantiating empties.
  USDAbstractWriter *xform_writer = new USDTransformWriter(
      m_stage, parent_path, ob_eval, degiter_data);
  const pxr::SdfPath &xform_usd_path = xform_writer->usd_path();
  usd_object_paths[ob_eval] = xform_usd_path;
  usd_writers[xform_usd_path] = xform_writer;
  xform_writer->write();

  // Write the object data, if we know how.
  // TODO: let the writer determine whether the data is actually supported.
  USDAbstractWriter *data_writer = NULL;
  switch (ob_eval->type) {
    case OB_MESH:
      data_writer = new USDMeshWriter(m_stage, xform_usd_path, ob_eval, degiter_data);
      break;
    default:
      printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (no data writer)\n",
             ob_eval->id.name,
             ob_eval->type);
      return false;
  }

  if (!data_writer->is_supported()) {
    printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (data writer rejects the data)\n",
           ob_eval->id.name,
           ob_eval->type);
  }
  else {
    usd_writers[data_writer->usd_path()] = data_writer;
    data_writer->write();
  }

  return true;
}
