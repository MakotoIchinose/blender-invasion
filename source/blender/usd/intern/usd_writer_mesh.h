#ifndef __USD__USD_WRITER_MESH_H__
#define __USD__USD_WRITER_MESH_H__

#include "usd_writer_abstract.h"

/* Writer for USD geometry. Does not assume the object is a mesh object. */
class USDGenericMeshWriter : public USDAbstractWriter {
 public:
  USDGenericMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual void do_write(HierarchyContext &context) override;

  virtual Mesh *get_export_mesh(Object *object_eval, bool &r_needsfree) = 0;
  virtual void free_export_mesh(Mesh *mesh);

 private:
  void write_mesh(Mesh *mesh);
};

class USDMeshWriter : public USDGenericMeshWriter {
 public:
  USDMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual Mesh *get_export_mesh(Object *object_eval, bool &r_needsfree);
};

#endif /* __USD__USD_WRITER_MESH_H__ */
