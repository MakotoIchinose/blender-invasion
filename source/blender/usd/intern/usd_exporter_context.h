#ifndef __USD__USD_EXPORTER_CONTEXT_H__
#define __USD__USD_EXPORTER_CONTEXT_H__

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

struct Depsgraph;
struct Object;

struct USDExporterContext {
  Depsgraph *depsgraph;
  pxr::UsdStageRefPtr stage;
  pxr::SdfPath usd_path;
};

#endif /* __USD__USD_EXPORTER_CONTEXT_H__ */
