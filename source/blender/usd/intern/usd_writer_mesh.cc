#include "usd_writer_mesh.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>

extern "C" {
#include "BKE_anim.h"
#include "BKE_library.h"
#include "BKE_material.h"

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
    write_mesh(context, mesh);

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

void USDGenericMeshWriter::write_mesh(HierarchyContext &context, struct Mesh *mesh)
{
  pxr::UsdTimeCode timecode = get_export_time_code();
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

  std::map<short, pxr::VtIntArray> face_groups;
  for (int i = 0; i < mesh->totpoly; ++i, ++mpoly) {
    MLoop *loop = mloop + mpoly->loopstart;
    usd_face_vertex_counts.push_back(mpoly->totloop);
    for (int j = 0; j < mpoly->totloop; ++j, ++loop) {
      usd_face_indices.push_back(loop->v);
    }

    face_groups[mpoly->mat_nr].push_back(i);
  }
  usd_mesh.CreateFaceVertexCountsAttr().Set(usd_face_vertex_counts, timecode);
  usd_mesh.CreateFaceVertexIndicesAttr().Set(usd_face_indices, timecode);

  // TODO(Sybren): figure out what happens when the face groups change.
  if (frame_has_been_written_) {
    return;
  }

  // Define a geometry subset per material.
  bool mesh_material_bound = false;
  for (auto face_group_iter : face_groups) {
    short material_number = face_group_iter.first;
    const pxr::VtIntArray &face_indices = face_group_iter.second;

    Material *material = give_current_material(context.object, material_number + 1);
    if (material == nullptr) {
      continue;
    }

    pxr::UsdShadeMaterial usd_material = ensure_usd_material(material);
    pxr::TfToken material_name = usd_material.GetPath().GetNameToken();

    if (face_groups.size() > 1) {
      // Only bother with writing face groups if the object has multiple materials.
      pxr::UsdShadeMaterialBindingAPI api = pxr::UsdShadeMaterialBindingAPI(usd_mesh);
      pxr::UsdGeomSubset usd_face_subset = api.CreateMaterialBindSubset(material_name,
                                                                        face_indices);
      usd_material.Bind(usd_face_subset.GetPrim());
    }

    /* Binding a material to a geometry subset isn't supported by the Hydra GL viewport yet, which
     * is why we also bind the first material to the entire mesh. See
     * https://github.com/PixarAnimationStudios/USD/issues/542 for more info. */
    if (!mesh_material_bound) {
      usd_material.Bind(usd_mesh.GetPrim());
      mesh_material_bound = true;
    }
  }
}

USDMeshWriter::USDMeshWriter(const USDExporterContext &ctx) : USDGenericMeshWriter(ctx)
{
}

Mesh *USDMeshWriter::get_export_mesh(Object *object_eval, bool & /*r_needsfree*/)
{
  return object_eval->runtime.mesh_eval;
}
