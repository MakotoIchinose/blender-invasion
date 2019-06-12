#ifndef __USD__USD_WRITER_TRANSFORM_H__
#define __USD__USD_WRITER_TRANSFORM_H__

#include "usd_writer_abstract.h"

class USDTransformWriter : public USDAbstractWriter {
 public:
  USDTransformWriter(pxr::UsdStageRefPtr stage,
                     const pxr::SdfPath &parent_path,
                     Object *ob_eval,
                     const DEGObjectIterData &degiter_data,
                     USDAbstractWriter *parent = NULL);

 protected:
  void do_write();
};

#endif /* __USD__USD_WRITER_TRANSFORM_H__ */
