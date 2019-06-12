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

#include <pxr/pxr.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/xform.h>

#include <cmath>
#include <time.h>

extern "C" {
#include "BKE_anim.h"
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

bool USDExporter::export_object(Object *ob_eval, const DEGObjectIterData &data_)
{
  const pxr::SdfPath root("/");
  Mesh *mesh = ob_eval->runtime.mesh_eval;
  pxr::SdfPath parent_path;
  USDAbstractWriter *parent_writer = NULL;

  if (mesh == NULL) {
    printf("USD-\033[34mSKIPPING\033[0m object %s  type=%d mesh = %p\n",
           ob_eval->id.name,
           ob_eval->type,
           mesh);
    return false;
  }
  if (data_.dupli_object_current != NULL) {
    printf("USD-\033[34mSKIPPING\033[0m object %s  instance of %s  type=%d mesh = %p\n",
           ob_eval->id.name,
           data_.dupli_object_current->ob->id.name,
           ob_eval->type,
           mesh);
    return false;
  }

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
    parent_writer = usd_writers[parent_path];
  }

  USDAbstractWriter *xformWriter = new USDTransformWriter(
      m_stage, parent_path, ob_eval, data_, parent_writer);

  USDAbstractWriter *meshWriter = new USDMeshWriter(
      m_stage, parent_path, ob_eval, data_, parent_writer);

  usd_object_paths[ob_eval] = xformWriter->usd_path();
  usd_writers[xformWriter->usd_path()] = xformWriter;
  usd_writers[meshWriter->usd_path()] = meshWriter;

  xformWriter->write();
  meshWriter->write();

  return true;
}
