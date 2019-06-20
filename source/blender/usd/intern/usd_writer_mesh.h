#ifndef __USD__USD_WRITER_MESH_H__
#define __USD__USD_WRITER_MESH_H__

#include "usd_writer_abstract.h"

#include <pxr/usd/usdGeom/mesh.h>

struct USDMeshData;

/* Writer for USD geometry. Does not assume the object is a mesh object. */
class USDGenericMeshWriter : public USDAbstractWriter {
 public:
  USDGenericMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual bool is_supported(const Object *object) const override;
  virtual void do_write(HierarchyContext &context) override;

  virtual Mesh *get_export_mesh(Object *object_eval, bool &r_needsfree) = 0;
  virtual void free_export_mesh(Mesh *mesh);

 private:
  void write_mesh(HierarchyContext &context, Mesh *mesh);
  void get_geometry_data(const Mesh *mesh, struct USDMeshData &usd_mesh_data);
  void assign_materials(const HierarchyContext &context,
                        pxr::UsdGeomMesh usd_mesh,
                        const std::map<short, pxr::VtIntArray> &usd_face_groups);
  void write_uv_maps(const Mesh *mesh, pxr::UsdGeomMesh usd_mesh);
  void write_normals(const Mesh *mesh, pxr::UsdGeomMesh usd_mesh);
  void write_surface_velocity(Object *object, const Mesh *mesh, pxr::UsdGeomMesh usd_mesh);
};

class USDMeshWriter : public USDGenericMeshWriter {
 public:
  USDMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual Mesh *get_export_mesh(Object *object_eval, bool &r_needsfree);
};

#endif /* __USD__USD_WRITER_MESH_H__ */
