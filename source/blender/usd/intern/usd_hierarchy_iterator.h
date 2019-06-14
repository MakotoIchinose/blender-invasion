#ifndef __USD__USD_HIERARCHY_ITERATOR_H__
#define __USD__USD_HIERARCHY_ITERATOR_H__

#include <map>
#include <string>

struct Depsgraph;
struct ViewLayer;

typedef void TEMP_WRITER_TYPE;

class AbstractHierarchyIterator {
 protected:
  Depsgraph *depsgraph;

  typedef std::map<std::string, TEMP_WRITER_TYPE *> WriterMap;
  WriterMap writers;

 public:
  AbstractHierarchyIterator(Dpesgraph *depsgraph);
  ~AbstractHierarchyIterator();

  void iterate();

 private:
  void visit_object(Base *base, Object *object, Object *parent, Object *dupliObParent);

  TEMP_WRITER_TYPE *export_object_and_parents(Object *ob, Object *parent, Object *dupliObParent);
  TEMP_WRITER_TYPE *get_writer(const std::string &name);

 protected:
  /* Not visiting means not exporting and also not expanding its duplis. */
  virtual bool should_visit_object(const Base *const base, bool is_duplicated) = 0;

  virtual TEMP_WRITER_TYPE *create_object_writer(const std::string &name,
                                                 Object *object,
                                                 void *parent_writer) = 0;
};

#endif /* __USD__USD_HIERARCHY_ITERATOR_H__ */
