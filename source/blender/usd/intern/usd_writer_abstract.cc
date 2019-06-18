#include "usd_writer_abstract.h"

#include <pxr/base/tf/stringUtils.h>

USDAbstractWriter::USDAbstractWriter(const USDExporterContext &ctx)
    : depsgraph(ctx.depsgraph), stage(ctx.stage), usd_path_(ctx.usd_path), object(ctx.ob_eval)
{
}

USDAbstractWriter::~USDAbstractWriter()
{
}

bool USDAbstractWriter::is_supported() const
{
  return true;
}

void USDAbstractWriter::write()
{
  do_write();
}

const pxr::SdfPath &USDAbstractWriter::usd_path() const
{
  return usd_path_;
}
