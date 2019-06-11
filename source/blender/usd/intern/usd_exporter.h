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

#ifndef __USD_EXPORTER_H__
#define __USD_EXPORTER_H__

#include "../usd.h"
#include "DEG_depsgraph_query.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

#include <map>

struct Depsgraph;
struct Main;
struct Scene;
struct ViewLayer;

struct ExportSettings {
  Scene *scene;
  /** Scene layer to export; all its objects will be exported, unless selected_only=true. */
  ViewLayer *view_layer;
  Depsgraph *depsgraph;

  USDExportParams params;
};

class USDExporter {
  ExportSettings &m_settings;
  const char *m_filename;

  typedef std::map<Object *, pxr::SdfPath> USDPathMap;
  USDPathMap usd_object_paths;

  pxr::UsdStageRefPtr m_stage;

  bool export_object(Object *ob_eval, const DEGObjectIterData &data_);

 public:
  USDExporter(const char *filename, ExportSettings &settings);
  ~USDExporter();

  void operator()(float &progress, bool &was_canceled);
};

#endif /* __USD_EXPORTER_H__ */
