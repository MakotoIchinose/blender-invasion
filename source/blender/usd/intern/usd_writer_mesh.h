#ifndef __USD__USD_WRITER_MESH_H__
#define __USD__USD_WRITER_MESH_H__

#include "usd_writer_abstract.h"

/* Writer for USD geometry. Does not assume the object is a mesh object. */
class USDGenericMeshWriter : public USDAbstractObjectDataWriter {
 public:
  USDGenericMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual void do_write() override;

  virtual Mesh *get_evaluated_mesh(bool &r_needsfree) = 0;
  virtual void free_evaluated_mesh(struct Mesh *mesh);

 private:
  void write_mesh(struct Mesh *mesh);
};

class USDMeshWriter : public USDGenericMeshWriter {
 public:
  USDMeshWriter(const USDExporterContext &ctx);

 protected:
  virtual Mesh *get_evaluated_mesh(bool &r_needsfree);
};

#endif /* __USD__USD_WRITER_MESH_H__ */
