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
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "BKE_anim.h"
#include "BKE_mesh_runtime.h"
#include "BKE_scene.h"

#include "BLI_iterator.h"

#include "DNA_layer_types.h"
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
  static const pxr::SdfPath root("/");
  Timer timer_("Writing to USD");

  r_progress = 0.0;
  r_was_canceled = false;

  // Create a stage with the Only Correct up-axis.
  m_stage = pxr::UsdStage::CreateNew(m_filename);
  m_stage->SetMetadata(pxr::UsdGeomTokens->upAxis, pxr::VtValue(pxr::UsdGeomTokens->z));

  USDHierarchyIterator iter(m_settings.depsgraph, m_stage);
  iter.iterate();

  m_stage->GetRootLayer()->Save();
  r_progress = 1.0;
}
