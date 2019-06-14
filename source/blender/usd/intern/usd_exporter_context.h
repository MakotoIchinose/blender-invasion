#ifndef __USD__USD_EXPORTER_CONTEXT_H__
#define __USD__USD_EXPORTER_CONTEXT_H__

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

struct Object;

struct USDExporterContext {
  pxr::UsdStageRefPtr stage;
  pxr::SdfPath parent_path;
  Object *ob_eval;
  Object *instanced_by;  // The dupli-object that instanced this object.
};

#endif /* __USD__USD_EXPORTER_CONTEXT_H__ */
