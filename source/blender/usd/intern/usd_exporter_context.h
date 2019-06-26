#ifndef __USD__USD_EXPORTER_CONTEXT_H__
#define __USD__USD_EXPORTER_CONTEXT_H__

#include "../usd.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

struct Depsgraph;
struct Object;
class USDHierarchyIterator;

struct USDExporterContext {
  Depsgraph *depsgraph;
  pxr::UsdStageRefPtr stage;
  pxr::SdfPath usd_path;
  USDHierarchyIterator *hierarchy_iterator;
  const USDExportParams &export_params;
};

#endif /* __USD__USD_EXPORTER_CONTEXT_H__ */
