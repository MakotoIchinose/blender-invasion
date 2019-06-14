#include "usd_writer_abstract.h"

#include <pxr/base/tf/stringUtils.h>

USDAbstractWriter::USDAbstractWriter(const USDExporterContext &ctx)
    : m_stage(ctx.stage),
      m_parent_path(ctx.parent_path),
      _path(),
      m_object(ctx.ob_eval),
      m_instanced_by(ctx.instanced_by)
{
}

USDAbstractWriter::~USDAbstractWriter()
{
}

std::string USDAbstractWriter::usd_name() const
{
  return m_object->id.name + 2;
}

const pxr::SdfPath &USDAbstractWriter::usd_path() const
{
  /* Lazy-evaluation; this isn't done in the constructor to allow overriding usd_name() in a
  /* subclass. */
  if (_path.IsEmpty()) {
    std::string my_usd_name = pxr::TfMakeValidIdentifier(usd_name());
    _path = m_parent_path.AppendPath(pxr::SdfPath(my_usd_name));
  }
  return _path;
}

bool USDAbstractWriter::is_supported() const
{
  return true;
}

void USDAbstractWriter::write()
{
  do_write();
}

USDAbstractObjectDataWriter::USDAbstractObjectDataWriter(const USDExporterContext &ctx)
    : USDAbstractWriter(ctx)
{
}

std::string USDAbstractObjectDataWriter::usd_name() const
{
  ID *data = static_cast<ID *>(m_object->data);
  return data->name + 2;
}
