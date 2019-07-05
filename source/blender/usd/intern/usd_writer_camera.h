#ifndef __USD__USD_WRITER_CAMERA_H__
#define __USD__USD_WRITER_CAMERA_H__

#include "usd_writer_abstract.h"

/* Writer for writing camera data to UsdGeomCamera. */
class USDCameraWriter : public USDAbstractWriter {
 public:
  USDCameraWriter(const USDExporterContext &ctx);

 protected:
  virtual bool is_supported(const Object *object) const override;
  virtual void do_write(HierarchyContext &context) override;
};

#endif /* __USD__USD_WRITER_CAMERA_H__ */
