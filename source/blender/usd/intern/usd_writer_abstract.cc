#include "usd_writer_abstract.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/base/tf/stringUtils.h>

extern "C" {
#include "BKE_animsys.h"
#include "BKE_key.h"

#include "DNA_modifier_types.h"
}

USDAbstractWriter::USDAbstractWriter(const USDExporterContext &usd_export_context)
    : depsgraph(usd_export_context.depsgraph),
      stage(usd_export_context.stage),
      usd_path_(usd_export_context.usd_path),
      hierarchy_iterator(usd_export_context.hierarchy_iterator),
      export_params(usd_export_context.export_params),
      frame_has_been_written_(false),
      is_animated_(false)
{
}

USDAbstractWriter::~USDAbstractWriter()
{
}

bool USDAbstractWriter::is_supported() const
{
  return true;
}

pxr::UsdTimeCode USDAbstractWriter::get_export_time_code() const
{
  if (is_animated_) {
    return hierarchy_iterator->get_export_time_code();
  }
  // By using the default timecode USD won't even write a single `timeSample` for non-animated
  // data. Instead, it writes it as non-timesampled.
  static pxr::UsdTimeCode default_timecode = pxr::UsdTimeCode::Default();
  return default_timecode;
}

void USDAbstractWriter::write(HierarchyContext &context)
{
  if (frame_has_been_written_) {
    if (!is_animated_) {
      return;
    }
  }
  else {
    is_animated_ = export_params.do_animation && check_is_animated(context.object);
    // printf("%sANIMATION\033[0m: %20s: %s\n",
    //        is_animated_ ? "\033[32;1m" : "\033[31;1m",
    //        context.export_path.c_str(),
    //        is_animated_ ? "true" : "false");
  }

  do_write(context);

  frame_has_been_written_ = true;
}

bool USDAbstractWriter::check_is_animated(Object *object) const
{
  if (object->data != nullptr) {
    AnimData *adt = BKE_animdata_from_id(static_cast<ID *>(object->data));
    /* TODO(Sybren): make this check more strict, as the AnimationData may
     * actually be empty (no fcurves, drivers, etc.) and thus effectively
     * have no animation at all. */
    if (adt != nullptr) {
      return true;
    }
  }
  if (BKE_key_from_object(object) != nullptr) {
    return true;
  }

  /* Test modifiers. */
  ModifierData *md = static_cast<ModifierData *>(object->modifiers.first);
  while (md) {
    if (md->type != eModifierType_Subsurf) {
      return true;
    }
    md = md->next;
  }

  return false;
}

const pxr::SdfPath &USDAbstractWriter::usd_path() const
{
  return usd_path_;
}
