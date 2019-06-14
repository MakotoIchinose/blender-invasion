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
  pxr::UsdStageRefPtr m_stage;
  pxr::SdfPath m_parent_path;
  mutable pxr::SdfPath _path;  // use usd_path() to get

  Object *m_object;
  Object *m_instanced_by;  // The dupli-object that instanced this object.

 public:
  USDAbstractWriter(const USDExporterContext &ctx);
  virtual ~USDAbstractWriter();

  void write();
  const pxr::SdfPath &usd_path() const;

  /* Returns true iff the data to be written is actually supported. This would, for example, allow
   * a hypothetical camera writer accept a perspective camera but reject an orthogonal one. */
  virtual bool is_supported() const;

 protected:
  virtual void do_write() = 0;
  virtual std::string usd_name() const;
};

class USDAbstractObjectDataWriter : public USDAbstractWriter {
 public:
  USDAbstractObjectDataWriter(const USDExporterContext &ctx);

 protected:
  virtual std::string usd_name() const override;
};

#endif /* __USD__USD_WRITER_ABSTRACT_H__ */
