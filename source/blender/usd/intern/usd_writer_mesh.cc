#include "usd_writer_mesh.h"

#include <pxr/usd/usdGeom/mesh.h>

extern "C" {
#include "BLI_utildefines.h"

#include "BKE_anim.h"
#include "BKE_library.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
}

USDGenericMeshWriter::USDGenericMeshWriter(pxr::UsdStageRefPtr stage,
                                           const pxr::SdfPath &parent_path,
                                           Object *ob_eval,
                                           const DEGObjectIterData &degiter_data)
    : USDAbstractWriter(stage, parent_path, ob_eval, degiter_data)
{
}

void USDGenericMeshWriter::do_write()
{
  bool needsfree = false;
  struct Mesh *mesh = get_evaluated_mesh(needsfree);

  try {
    write_mesh(mesh);

    if (needsfree) {
      free_evaluated_mesh(mesh);
    }
  }
  catch (...) {
    if (needsfree) {
      free_evaluated_mesh(mesh);
    }
    throw;
  }
}

void USDGenericMeshWriter::free_evaluated_mesh(struct Mesh *mesh)
{
  BKE_id_free(NULL, mesh);
}

void USDGenericMeshWriter::write_mesh(struct Mesh *mesh)
{
  printf("USD-\033[32mexporting\033[0m data %s → %s   isinstance=%d type=%d mesh = %p\n",
         mesh->id.name,
         m_path.GetString().c_str(),
         m_degiter_data.dupli_object_current != NULL,
         m_object->type,
         mesh);

  pxr::UsdGeomMesh usd_mesh = pxr::UsdGeomMesh::Define(m_stage, m_path);

  const MVert *verts = mesh->mvert;

  // TODO(Sybren): there is probably a more C++-y way to do this, which avoids copying the entire
  // mesh to a different structure. I haven't seen the approach below in the USD exporters for
  // Maya/Houdini, but it's simple and it works for now.
  pxr::VtArray<pxr::GfVec3f> usd_points;
  usd_points.reserve(mesh->totvert);
  for (int i = 0; i < mesh->totvert; ++i) {
    usd_points.push_back(pxr::GfVec3f(verts[i].co));
  }
  usd_mesh.CreatePointsAttr().Set(usd_points);

  pxr::VtArray<int> usd_face_vertex_counts, usd_face_indices;
  usd_face_vertex_counts.reserve(mesh->totpoly);
  usd_face_indices.reserve(mesh->totloop);

  MLoop *mloop = mesh->mloop;
  MPoly *mpoly = mesh->mpoly;

  for (int i = 0; i < mesh->totpoly; ++i, ++mpoly) {
    MLoop *loop = mloop + mpoly->loopstart;
    usd_face_vertex_counts.push_back(mpoly->totloop);
    for (int j = 0; j < mpoly->totloop; ++j, ++loop) {
      usd_face_indices.push_back(loop->v);
    }
  }
  usd_mesh.CreateFaceVertexCountsAttr().Set(usd_face_vertex_counts);
  usd_mesh.CreateFaceVertexIndicesAttr().Set(usd_face_indices);
}

USDMeshWriter::USDMeshWriter(pxr::UsdStageRefPtr stage,
                             const pxr::SdfPath &parent_path,
                             Object *ob_eval,
                             const DEGObjectIterData &degiter_data)
    : USDGenericMeshWriter(stage, parent_path, ob_eval, degiter_data)
{
}

Mesh *USDMeshWriter::get_evaluated_mesh(bool &UNUSED(r_needsfree))
{
  if (m_degiter_data.dupli_object_current != NULL) {
    printf("USD-\033[34mSKIPPING\033[0m object %s  instance of %s  type=%d mesh = %p\n",
           m_object->id.name,
           m_degiter_data.dupli_object_current->ob->id.name,
           m_object->type,
           m_object->runtime.mesh_eval);
    return NULL;
  }

  return m_object->runtime.mesh_eval;
}
