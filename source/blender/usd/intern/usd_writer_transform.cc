#include "usd_writer_transform.h"

#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/xform.h>

extern "C" {
#include "BLI_math_matrix.h"
}

USDTransformWriter::USDTransformWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
}

void USDTransformWriter::do_write()
{
  float parent_relative_matrix[4][4];  // The object matrix relative to the parent.

  // Get the object matrix relative to the parent.
  if (object->parent == NULL) {
    copy_m4_m4(parent_relative_matrix, object->obmat);
  }
  else {
    invert_m4_m4(object->parent->imat, object->parent->obmat);
    mul_m4_m4m4(parent_relative_matrix, object->parent->imat, object->obmat);
  }

  printf("USD-\033[32mexporting\033[0m XForm %s → %s   type=%d   addr = %p\n",
         object->id.name,
         usd_path_.GetString().c_str(),
         object->type,
         object);

  // Write the transform relative to the parent.
  pxr::UsdGeomXform xform = pxr::UsdGeomXform::Define(stage, usd_path_);
  xform.AddTransformOp().Set(pxr::GfMatrix4d(parent_relative_matrix));
}
