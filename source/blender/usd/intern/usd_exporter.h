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
struct Scene;
struct ViewLayer;
class USDAbstractWriter;

struct ExportSettings {
  Scene *scene;
  /** Scene layer to export; all its objects will be exported, unless selected_only=true. */
  ViewLayer *view_layer;
  Depsgraph *depsgraph;

  USDExportParams params;
};

// Temporary class for timing stuff.
#include <ctime>
class Timer {
  timespec ts_begin;
  std::string label;

 public:
  explicit Timer(std::string label) : label(label)
  {
    clock_gettime(CLOCK_MONOTONIC, &ts_begin);
  }
  ~Timer()
  {
    timespec ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double duration = double(ts_end.tv_sec - ts_begin.tv_sec) +
                      double(ts_end.tv_nsec - ts_begin.tv_nsec) / 1e9;
    printf("%s took %.3f sec wallclock time\n", label.c_str(), duration);
  }
};

class USDExporter {
  ExportSettings &m_settings;
  const char *m_filename;

  typedef std::map<Object *, pxr::SdfPath> USDPathMap;
  typedef std::map<pxr::SdfPath, USDAbstractWriter *> USDWriterMap;

  USDPathMap usd_object_paths;
  USDWriterMap usd_writers;

  pxr::UsdStageRefPtr m_stage;

 private:
  bool export_object(Object *ob_eval, const DEGObjectIterData &data_);
  pxr::SdfPath parent_usd_path(Object *ob_eval, const DEGObjectIterData &degiter_data);

  USDAbstractWriter *export_object_xform(const pxr::SdfPath &parent_path,
                                         Object *ob_eval,
                                         const DEGObjectIterData &degiter_data);
  USDAbstractWriter *export_object_data(const pxr::SdfPath &parent_path,
                                        Object *ob_eval,
                                        const DEGObjectIterData &degiter_data);

 public:
  USDExporter(const char *filename, ExportSettings &settings);
  ~USDExporter();

  void operator()(float &progress, bool &was_canceled);
};

#endif /* __USD_EXPORTER_H__ */
