#include "usd_hierarchy_iterator.h"
#include "usd_writer_abstract.h"
#include "usd_writer_mesh.h"
#include "usd_writer_transform.h"

#include <string>

#include <pxr/base/tf/stringUtils.h>

extern "C" {
#include "BKE_anim.h"

#include "BLI_assert.h"
#include "BLI_utildefines.h"

#include "DEG_depsgraph_query.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
}

USDHierarchyIterator::USDHierarchyIterator(Depsgraph *depsgraph, pxr::UsdStageRefPtr stage)
    : AbstractHierarchyIterator(depsgraph), stage(stage)
{
}

void USDHierarchyIterator::delete_object_writer(TEMP_WRITER_TYPE *writer)
{
  delete static_cast<USDAbstractWriter *>(writer);
}

std::string USDHierarchyIterator::get_id_name(const ID *const id) const
{
  BLI_assert(id != nullptr);
  std::string name(id->name + 2);
  return pxr::TfMakeValidIdentifier(name);
}

TEMP_WRITER_TYPE *USDHierarchyIterator::create_xform_writer(const std::string &name,
                                                            Object *object,
                                                            void *UNUSED(parent_writer))
{
  printf("\033[32;1mCREATE\033[0m %s at %s\n", object->id.name, name.c_str());
  pxr::SdfPath usd_path("/" + name);

  USDExporterContext ctx = {stage, usd_path, object, NULL};
  USDAbstractWriter *xform_writer = new USDTransformWriter(ctx);
  xform_writer->write();
  return xform_writer;
}

TEMP_WRITER_TYPE *USDHierarchyIterator::create_data_writer(const std::string &name,
                                                           Object *object,
                                                           void *UNUSED(parent_writer))
{
  pxr::SdfPath usd_path("/" + name);
  std::string data_name(get_id_name((ID *)object->data));

  USDExporterContext ctx = {stage, usd_path.AppendPath(pxr::SdfPath(data_name)), object, NULL};
  USDAbstractWriter *data_writer = NULL;

  switch (ctx.ob_eval->type) {
    case OB_MESH:
      data_writer = new USDMeshWriter(ctx);
      break;
    default:
      printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (no data writer)\n",
             ctx.ob_eval->id.name,
             ctx.ob_eval->type);
      return nullptr;
  }

  if (!data_writer->is_supported()) {
    printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (data writer rejects the data)\n",
           ctx.ob_eval->id.name,
           ctx.ob_eval->type);
    delete data_writer;
    return nullptr;
  }

  data_writer->write();

  return data_writer;
}
