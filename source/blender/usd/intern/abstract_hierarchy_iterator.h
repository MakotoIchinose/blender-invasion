#ifndef __USD__ABSTRACT_HIERARCHY_ITERATOR_H__
#define __USD__ABSTRACT_HIERARCHY_ITERATOR_H__

#include <map>
#include <string>

struct Base;
struct Depsgraph;
struct DupliObject;
struct ID;
struct Object;
struct ViewLayer;

typedef void TEMP_WRITER_TYPE;

class AbstractHierarchyIterator {
 public:
  typedef std::map<std::string, TEMP_WRITER_TYPE *> WriterMap;

 protected:
  Depsgraph *depsgraph;
  WriterMap writers;

 public:
  explicit AbstractHierarchyIterator(Depsgraph *depsgraph);
  virtual ~AbstractHierarchyIterator();

  void iterate();
  const WriterMap &writer_map() const;
  void release_writers();

 private:
  void visit_object(Base *base, Object *object, Object *parent, Object *dupliObParent);

  std::string get_object_name(const Object *const object) const;
  std::string get_object_dag_path_name(const Object *const ob,
                                       const Object *const dupli_parent) const;

  TEMP_WRITER_TYPE *export_object_and_parents(Object *ob, Object *parent, Object *dupliObParent);
  TEMP_WRITER_TYPE *get_writer(const std::string &name);

 protected:
  /* Not visiting means not exporting and also not expanding its duplis. */
  virtual bool should_visit_object(const Base *const base, bool is_duplicated) const;
  virtual bool should_visit_duplilink(const DupliObject *const link) const;

  virtual TEMP_WRITER_TYPE *create_xform_writer(const std::string &name,
                                                Object *object,
                                                void *parent_writer) = 0;
  virtual TEMP_WRITER_TYPE *create_data_writer(const std::string &name,
                                               Object *object,
                                               void *parent_writer) = 0;

  virtual void delete_object_writer(TEMP_WRITER_TYPE *writer) = 0;

  virtual std::string get_id_name(const ID *const id) const = 0;
};

#endif /* __USD__ABSTRACT_HIERARCHY_ITERATOR_H__ */
