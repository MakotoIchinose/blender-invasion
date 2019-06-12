#ifndef __USD__USD_WRITER_ABSTRACT_H__
#define __USD__USD_WRITER_ABSTRACT_H__

#include "DEG_depsgraph_query.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <vector>

struct Main;
struct Object;

class USDAbstractWriter {
 protected:
  pxr::UsdStageRefPtr m_stage;
  pxr::SdfPath m_parent_path;
  Object *m_object;

  const DEGObjectIterData &m_degiter_data;
  std::vector<USDAbstractWriter *> m_children;
  pxr::SdfPath m_path;

 public:
  USDAbstractWriter(pxr::UsdStageRefPtr stage,
                    const pxr::SdfPath &parent_path,
                    Object *ob_eval,
                    const DEGObjectIterData &degiter_data,
                    USDAbstractWriter *parent = NULL);
  virtual ~USDAbstractWriter();

  void add_child(USDAbstractWriter *child);
  void write();

  const pxr::SdfPath &usd_path() const;

 protected:
  virtual void do_write() = 0;
};

#endif /* __USD__USD_WRITER_ABSTRACT_H__ */
