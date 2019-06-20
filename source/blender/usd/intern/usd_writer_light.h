#ifndef __USD__USD_WRITER_LIGHT_H__
#define __USD__USD_WRITER_LIGHT_H__

#include "usd_writer_abstract.h"

class USDLightWriter : public USDAbstractWriter {
 public:
  USDLightWriter(const USDExporterContext &ctx);

 protected:
  virtual bool is_supported(const Object *object) const override;
  virtual void do_write(HierarchyContext &context) override;
};

#endif /* __USD__USD_WRITER_LIGHT_H__ */
