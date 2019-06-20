#ifndef __USD__USD_WRITER_TRANSFORM_H__
#define __USD__USD_WRITER_TRANSFORM_H__

#include "usd_writer_abstract.h"

class USDTransformWriter : public USDAbstractWriter {
 public:
  USDTransformWriter(const USDExporterContext &ctx);

 protected:
  void do_write(Object *object_eval) override;
};

#endif /* __USD__USD_WRITER_TRANSFORM_H__ */
