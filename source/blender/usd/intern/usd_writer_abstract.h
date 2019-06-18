#ifndef __USD__USD_WRITER_ABSTRACT_H__
#define __USD__USD_WRITER_ABSTRACT_H__

#include "usd_exporter_context.h"

#include "DEG_depsgraph_query.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <vector>

struct Main;
struct Object;

class USDAbstractWriter {
 protected:
  Depsgraph *depsgraph;
  pxr::UsdStageRefPtr stage;
  pxr::SdfPath usd_path_;
  Object *object;

 public:
  USDAbstractWriter(const USDExporterContext &ctx);
  virtual ~USDAbstractWriter();

  void write();

  /* Returns true iff the data to be written is actually supported. This would, for example, allow
   * a hypothetical camera writer accept a perspective camera but reject an orthogonal one. */
  virtual bool is_supported() const;

  const pxr::SdfPath &usd_path() const;

 protected:
  virtual void do_write() = 0;
};

#endif /* __USD__USD_WRITER_ABSTRACT_H__ */
