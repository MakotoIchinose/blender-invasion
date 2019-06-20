#include "usd_writer_abstract.h"

#include <pxr/base/tf/stringUtils.h>

USDAbstractWriter::USDAbstractWriter(const USDExporterContext &usd_export_context)
    : depsgraph(usd_export_context.depsgraph),
      stage(usd_export_context.stage),
      usd_path_(usd_export_context.usd_path)
{
}

USDAbstractWriter::~USDAbstractWriter()
{
}

bool USDAbstractWriter::is_supported() const
{
  return true;
}

void USDAbstractWriter::write(Object *object_eval)
{
  do_write(object_eval);
}

const pxr::SdfPath &USDAbstractWriter::usd_path() const
{
  return usd_path_;
}
