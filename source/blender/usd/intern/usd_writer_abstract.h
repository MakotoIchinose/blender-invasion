#ifndef __USD__USD_WRITER_ABSTRACT_H__
#define __USD__USD_WRITER_ABSTRACT_H__

#include "usd_exporter_context.h"
#include "abstract_hierarchy_iterator.h"

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/material.h>

#include <vector>

extern "C" {
#include "DEG_depsgraph_query.h"
#include "DNA_material_types.h"
}

struct Main;
struct Material;
struct Object;

class USDAbstractWriter : public AbstractHierarchyWriter {
 protected:
  const USDExporterContext usd_export_context_;

  bool frame_has_been_written_;
  bool is_animated_;

 public:
  USDAbstractWriter(const USDExporterContext &usd_export_context);
  virtual ~USDAbstractWriter();

  virtual void write(HierarchyContext &context) override;

  /* Returns true iff the data to be written is actually supported. This would, for example, allow
   * a hypothetical camera writer accept a perspective camera but reject an orthogonal one. */
  virtual bool is_supported(const Object *object) const;

  const pxr::SdfPath &usd_path() const;

 protected:
  virtual void do_write(HierarchyContext &context) = 0;
  virtual bool check_is_animated(const HierarchyContext &context) const;
  pxr::UsdTimeCode get_export_time_code() const;

  pxr::UsdShadeMaterial ensure_usd_material(Material *material);
};

#endif /* __USD__USD_WRITER_ABSTRACT_H__ */
