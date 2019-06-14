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

 public:
  USDHierarchyIterator(Depsgraph *depsgraph, pxr::UsdStageRefPtr stage);

 protected:
  virtual TEMP_WRITER_TYPE *create_xform_writer(const std::string &name,
                                                Object *object,
                                                void *parent_writer) override;
  virtual TEMP_WRITER_TYPE *create_data_writer(const std::string &name,
                                               Object *object,
                                               void *parent_writer) override;

  virtual std::string get_id_name(const ID *const id) const override;
  virtual void delete_object_writer(TEMP_WRITER_TYPE *writer) override;
};

#endif /* __USD__USD_HIERARCHY_ITERATOR_H__ */
