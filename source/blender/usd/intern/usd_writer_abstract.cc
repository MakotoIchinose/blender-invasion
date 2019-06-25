#include "usd_writer_abstract.h"

#include <pxr/base/tf/stringUtils.h>

USDAbstractWriter::USDAbstractWriter(const USDExporterContext &usd_export_context)
    : depsgraph(usd_export_context.depsgraph),
      stage(usd_export_context.stage),
      usd_path_(usd_export_context.usd_path),
      hierarchy_iterator(usd_export_context.hierarchy_iterator)
{
}

USDAbstractWriter::~USDAbstractWriter()
{
}

bool USDAbstractWriter::is_supported() const
{
  return true;
}

void USDAbstractWriter::write(HierarchyContext &context)
{
  // TODO(Sybren): deal with animatedness of objects and only calling do_write() when this is
  // either the first call or the object is animated.
  do_write(context);
}

const pxr::SdfPath &USDAbstractWriter::usd_path() const
{
  return usd_path_;
}
