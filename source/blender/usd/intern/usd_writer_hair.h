#ifndef __USD__USD_WRITER_HAIR_H__
#define __USD__USD_WRITER_HAIR_H__

#include "usd_writer_abstract.h"

/* Writer for writing hair particle data as USD curves. */
class USDHairWriter : public USDAbstractWriter {
 public:
  USDHairWriter(const USDExporterContext &ctx);

 protected:
  virtual void do_write(HierarchyContext &context) override;
  virtual bool check_is_animated(const HierarchyContext &context) const override;
};

#endif /* __USD__USD_WRITER_HAIR_H__ */
