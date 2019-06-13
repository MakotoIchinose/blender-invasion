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
  pxr::SdfPath parent_path = parent_usd_path(ob_eval, degiter_data);
  if (parent_path.IsEmpty()) {
    return false;
  }

  USDAbstractWriter *xform_writer, *data_writer;
  xform_writer = export_object_xform(parent_path, ob_eval, degiter_data);
  data_writer = export_object_data(xform_writer->usd_path(), ob_eval, degiter_data);

  return data_writer != NULL;
}

pxr::SdfPath USDExporter::parent_usd_path(Object *ob_eval, const DEGObjectIterData &degiter_data)
{
  static const pxr::SdfPath root("/");
  pxr::SdfPath parent_path(root);

  // Prepend any dupli-parent USD path.
  if (degiter_data.dupli_parent != NULL && degiter_data.dupli_parent != ob_eval) {
    USDPathMap::iterator path_it = usd_object_paths.find(degiter_data.dupli_parent);
    if (path_it == usd_object_paths.end()) {
      printf(
          "USD-\033[31mSKIPPING\033[0m object %s because dupli-parent %s hasn't been seen yet\n",
          ob_eval->id.name,
          degiter_data.dupli_parent->id.name);
      return pxr::SdfPath();
    }
    parent_path = path_it->second;
  }

  if (ob_eval->parent == NULL) {
    return parent_path;
  }

  // Append the parent object's USD path.
  USDPathMap::iterator path_it = usd_object_paths.find(ob_eval->parent);
  if (path_it == usd_object_paths.end()) {
    printf("USD-\033[31mSKIPPING\033[0m object %s because parent %s hasn't been seen yet\n",
           ob_eval->id.name,
           ob_eval->parent->id.name);
    return pxr::SdfPath();
  }

  return parent_path.AppendPath(path_it->second.MakeRelativePath(root));
}

/* Write the transform. This is always done, even when we don't write the data, as it makes it
 * possible to reference collection-instantiating empties. */
USDAbstractWriter *USDExporter::export_object_xform(const pxr::SdfPath &parent_path,
                                                    Object *ob_eval,
                                                    const DEGObjectIterData &degiter_data)
{
  USDAbstractWriter *xform_writer = new USDTransformWriter(
      m_stage, parent_path, ob_eval, degiter_data);

  const pxr::SdfPath &xform_usd_path = xform_writer->usd_path();
  usd_object_paths[ob_eval] = xform_usd_path;
  usd_writers[xform_usd_path] = xform_writer;
  xform_writer->write();

  return xform_writer;
}

/* Write the object data, if we know how. */
USDAbstractWriter *USDExporter::export_object_data(const pxr::SdfPath &parent_path,
                                                   Object *ob_eval,
                                                   const DEGObjectIterData &degiter_data)
{
  USDAbstractWriter *data_writer = NULL;

  switch (ob_eval->type) {
    case OB_MESH:
      data_writer = new USDMeshWriter(m_stage, parent_path, ob_eval, degiter_data);
      break;
    default:
      printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (no data writer)\n",
             ob_eval->id.name,
             ob_eval->type);
      return NULL;
  }

  if (!data_writer->is_supported()) {
    printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (data writer rejects the data)\n",
           ob_eval->id.name,
           ob_eval->type);
    delete data_writer;
    return NULL;
  }

  usd_writers[data_writer->usd_path()] = data_writer;
  data_writer->write();

  return data_writer;
}
