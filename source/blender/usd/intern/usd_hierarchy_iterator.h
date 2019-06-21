#ifndef __USD__USD_HIERARCHY_ITERATOR_H__
#define __USD__USD_HIERARCHY_ITERATOR_H__

#include "abstract_hierarchy_iterator.h"
#include "usd_exporter_context.h"

#include <string>

#include <pxr/usd/usd/common.h>

struct Depsgraph;
struct ID;
struct Object;
struct USDAbstractWriter;

class USDHierarchyIterator : public AbstractHierarchyIterator {
 private:
  pxr::UsdStageRefPtr stage;
  const USDExportParams &params;

 public:
  USDHierarchyIterator(Depsgraph *depsgraph,
                       pxr::UsdStageRefPtr stage,
                       const USDExportParams &params);

 protected:
  virtual bool should_export_object(const Object *object) const override;

  virtual AbstractHierarchyWriter *create_xform_writer(const HierarchyContext &context) override;
  virtual AbstractHierarchyWriter *create_data_writer(const HierarchyContext &context) override;

  virtual std::string get_id_name(const ID *const id) const override;
  virtual void delete_object_writer(AbstractHierarchyWriter *writer) override;
};

#endif /* __USD__USD_HIERARCHY_ITERATOR_H__ */
