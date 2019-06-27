#ifndef __USD__USD_WRITER_TRANSFORM_H__
#define __USD__USD_WRITER_TRANSFORM_H__

#include "usd_writer_abstract.h"

#include <pxr/usd/usdGeom/xform.h>

class USDTransformWriter : public USDAbstractWriter {
 private:
  pxr::UsdGeomXformOp xformOp_;

 public:
  USDTransformWriter(const USDExporterContext &ctx);

 protected:
  void do_write(HierarchyContext &context) override;
  bool check_is_animated(const HierarchyContext &context) const override;
};

#endif /* __USD__USD_WRITER_TRANSFORM_H__ */
