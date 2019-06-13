#include "usd_writer_transform.h"

#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/xform.h>

extern "C" {
#include "BLI_math_matrix.h"
}

USDTransformWriter::USDTransformWriter(pxr::UsdStageRefPtr stage,
                                       const pxr::SdfPath &parent_path,
                                       Object *ob_eval,
                                       const DEGObjectIterData &degiter_data)
    : USDAbstractWriter(stage, parent_path, ob_eval, degiter_data)
{
}

void USDTransformWriter::do_write()
{
  float dupliparent_relative_matrix[4][4];
  float parent_relative_matrix[4][4];  // The object matrix relative to the parent.

  if (m_degiter_data.dupli_parent != NULL && m_degiter_data.dupli_parent != m_object) {
    invert_m4_m4(m_degiter_data.dupli_parent->imat, m_degiter_data.dupli_parent->obmat);
    mul_m4_m4m4(dupliparent_relative_matrix, m_degiter_data.dupli_parent->imat, m_object->obmat);
  }
  else {
    copy_m4_m4(dupliparent_relative_matrix, m_object->obmat);
  }

  // Get the object matrix relative to the parent.
  if (m_object->parent == NULL) {
    copy_m4_m4(parent_relative_matrix, dupliparent_relative_matrix);
  }
  else {
    invert_m4_m4(m_object->parent->imat, m_object->parent->obmat);
    mul_m4_m4m4(parent_relative_matrix, m_object->parent->imat, dupliparent_relative_matrix);
  }

  printf("USD-\033[32mexporting\033[0m XForm %s → %s   isinstance=%d type=%d\n",
         m_object->id.name,
         m_path.GetString().c_str(),
         m_degiter_data.dupli_object_current != NULL,
         m_object->type);

  // Write the transform relative to the parent.
  pxr::UsdGeomXform xform = pxr::UsdGeomXform::Define(m_stage, m_path);
  xform.AddTransformOp().Set(pxr::GfMatrix4d(parent_relative_matrix));
}
