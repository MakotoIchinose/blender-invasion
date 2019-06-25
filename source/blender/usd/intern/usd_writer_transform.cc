#include "usd_writer_transform.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/xform.h>

extern "C" {
#include "BKE_animsys.h"

#include "BLI_math_matrix.h"

#include "DNA_layer_types.h"
}

USDTransformWriter::USDTransformWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
}

void USDTransformWriter::do_write(HierarchyContext &context)
{
  float parent_relative_matrix[4][4];  // The object matrix relative to the parent.
  if (context.export_parent == NULL) {
    copy_m4_m4(parent_relative_matrix, context.matrix_world);
  }
  else {
    invert_m4_m4(context.export_parent->imat, context.export_parent->obmat);
    mul_m4_m4m4(parent_relative_matrix, context.export_parent->imat, context.matrix_world);
  }

  // Write the transform relative to the parent.
  pxr::UsdGeomXform xform = pxr::UsdGeomXform::Define(stage, usd_path_);
  if (!xformOp_) {
    xformOp_ = xform.AddTransformOp();
  }
  // TODO(Sybren): when not animated, write to the default timecode instead.
  xformOp_.Set(pxr::GfMatrix4d(parent_relative_matrix),
               hierarchy_iterator->get_export_time_code());
}

bool USDTransformWriter::check_is_animated(Object *object) const
{
  /* We should also check the animation state of parents that aren't part of the export hierarchy
   * (that is, when the animated parent is not instanced by the duplicator of the current object).
   * For now, just assume duplis are transform-animated. */
  // if (object->base_flag & BASE_FROM_DUPLI) {
  //   return true;
  // }
  return true;

  AnimData *adt = BKE_animdata_from_id(&object->id);
  /* TODO(Sybren): make this check more strict, as the AnimationData may
   * actually be empty (no fcurves, drivers, etc.) and thus effectively
   * have no animation at all. */
  if (adt != nullptr) {
    return true;
  };

  if (object->constraints.first != nullptr) {
    return true;
  }

  return false;
}
