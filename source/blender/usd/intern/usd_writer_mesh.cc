#include "usd_writer_mesh.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usdGeom/mesh.h>

extern "C" {
#include "BKE_anim.h"
#include "BKE_library.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
}

USDGenericMeshWriter::USDGenericMeshWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
}

void USDGenericMeshWriter::do_write(HierarchyContext &context)
{
  Object *object_eval = context.object;
  bool needsfree = false;
  struct Mesh *mesh = get_export_mesh(object_eval, needsfree);

  if (mesh == NULL) {
    printf("USD-\033[31mSKIPPING\033[0m object %s  type=%d mesh = NULL\n",
           object_eval->id.name,
           object_eval->type);
    return;
  }

  try {
    write_mesh(mesh);

    if (needsfree) {
      free_export_mesh(mesh);
    }
  }
  catch (...) {
    if (needsfree) {
      free_export_mesh(mesh);
    }
    throw;
  }
}

void USDGenericMeshWriter::free_export_mesh(struct Mesh *mesh)
{
  BKE_id_free(NULL, mesh);
}

void USDGenericMeshWriter::write_mesh(struct Mesh *mesh)
{
  pxr::UsdTimeCode timecode = hierarchy_iterator->get_export_time_code();
  // printf("USD-\033[32mexporting\033[0m mesh  %s → %s  mesh = %p\n",
  //        mesh->id.name,
  //        usd_path_.GetString().c_str(),
  //        mesh);

  pxr::UsdGeomMesh usd_mesh = pxr::UsdGeomMesh::Define(stage, usd_path_);

  const MVert *verts = mesh->mvert;

  // TODO(Sybren): there is probably a more C++-y way to do this, which avoids copying the entire
  // mesh to a different structure. I haven't seen the approach below in the USD exporters for
  // Maya/Houdini, but it's simple and it works for now.
  pxr::VtArray<pxr::GfVec3f> usd_points;
  usd_points.reserve(mesh->totvert);
  for (int i = 0; i < mesh->totvert; ++i) {
    usd_points.push_back(pxr::GfVec3f(verts[i].co));
  }
  usd_mesh.CreatePointsAttr().Set(usd_points, timecode);

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
  usd_mesh.CreateFaceVertexCountsAttr().Set(usd_face_vertex_counts, timecode);
  usd_mesh.CreateFaceVertexIndicesAttr().Set(usd_face_indices, timecode);
}

USDMeshWriter::USDMeshWriter(const USDExporterContext &ctx) : USDGenericMeshWriter(ctx)
{
}

Mesh *USDMeshWriter::get_export_mesh(Object *object_eval, bool & /*r_needsfree*/)
{
  return object_eval->runtime.mesh_eval;
}
