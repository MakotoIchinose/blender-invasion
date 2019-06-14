#include "abstract_hierarchy_iterator.h"

#include <string>

extern "C" {
#include "BKE_anim.h"

#include "BLI_assert.h"
#include "BLI_utildefines.h"

#include "DEG_depsgraph_query.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
}

AbstractHierarchyIterator::AbstractHierarchyIterator(Depsgraph *depsgraph)
    : depsgraph(depsgraph), writers()
{
}

AbstractHierarchyIterator::~AbstractHierarchyIterator()
{
}

void AbstractHierarchyIterator::release()
{
  for (WriterMap::value_type it : writers) {
    delete_object_writer(it.second);
  }
  writers.clear();
}

void AbstractHierarchyIterator::iterate()
{
  ViewLayer *view_layer = DEG_get_input_view_layer(depsgraph);
  for (Base *base = static_cast<Base *>(view_layer->object_bases.first); base; base = base->next) {
    Object *ob = base->object;
    visit_object(base, ob, ob->parent, NULL);
  }
}

std::string AbstractHierarchyIterator::get_object_name(const Object *const object) const
{
  if (object == NULL) {
    return "";
  }

  return get_id_name(&object->id);
}

std::string AbstractHierarchyIterator::get_id_name(const ID *const id) const
{
  if (id == NULL) {
    return "";
  }

  return std::string(id->name + 2);
}

/**
 * \brief get_object_dag_path_name returns the name under which the object
 *  will be exported in the Alembic file. It is of the form
 *  "[../grandparent/]parent/object" if dupli_parent is NULL, or
 *  "dupli_parent/[../grandparent/]parent/object" otherwise.
 * \param ob:
 * \param dupli_parent:
 * \return
 */
std::string AbstractHierarchyIterator::get_object_dag_path_name(
    const Object *const ob, const Object *const dupli_parent) const
{
  std::string name = get_object_name(ob);

  for (Object *parent = ob->parent; parent; parent = parent->parent) {
    name = get_object_name(parent) + "/" + name;
  }

  if (dupli_parent && (ob != dupli_parent)) {
    // TODO(Sybren): shouldn't this call get_object_dag_path_name()?
    name = get_object_name(dupli_parent) + "/" + name;
  }

  return name;
}

bool AbstractHierarchyIterator::should_visit_object(const Base *const UNUSED(base),
                                                    bool UNUSED(is_duplicated)) const
{
  return true;
}

bool AbstractHierarchyIterator::should_visit_duplilink(const DupliObject *const link) const
{
  // Removing link->no_draw hides things like custom bone shapes.
  return !link->no_draw && link->type == OB_DUPLICOLLECTION;
}

void AbstractHierarchyIterator::visit_object(Base *base,
                                             Object *object,
                                             Object *parent,
                                             Object *dupliObParent)
{
  /* If an object isn't exported itself, its duplilist shouldn't be
   * exported either. */
  if (!should_visit_object(base, dupliObParent != NULL)) {
    return;
  }

  Object *ob = DEG_get_evaluated_object(depsgraph, object);
  export_object_and_parents(ob, parent, dupliObParent);

  ListBase *lb = object_duplilist(depsgraph, DEG_get_input_scene(depsgraph), ob);

  if (lb) {
    DupliObject *link = static_cast<DupliObject *>(lb->first);
    Object *dupli_ob = NULL;
    Object *dupli_parent = NULL;

    for (; link; link = link->next) {
      if (!should_visit_duplilink(link)) {
        continue;
      }

      dupli_ob = link->ob;
      dupli_parent = (dupli_ob->parent) ? dupli_ob->parent : ob;
      visit_object(base, dupli_ob, dupli_parent, ob);
    }

    free_object_duplilist(lb);
  }
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::export_object_and_parents(Object *ob,
                                                                       Object *parent,
                                                                       Object *dupliObParent)
{
  /* An object should not be its own parent, or we'll get infinite loops. */
  BLI_assert(ob != parent);
  BLI_assert(ob != dupliObParent);

  std::string name;
  //   if (m_settings.flatten_hierarchy) {
  //     name = get_id_name(ob);
  //   }
  //   else {
  name = get_object_dag_path_name(ob, dupliObParent);
  //   }

  /* check if we have already created a transform writer for this object */
  TEMP_WRITER_TYPE *xform_writer = get_writer(name);
  if (xform_writer != NULL) {
    return xform_writer;
  }

  TEMP_WRITER_TYPE *parent_writer = NULL;

  if (/* m_settings.flatten_hierarchy || */ parent == NULL) {
  }
  else {
    /* Since there are so many different ways to find parents (as evident
     * in the number of conditions below), we can't really look up the
     * parent by name. We'll just call export_object_and_parents(), which will
     * return the parent's writer pointer. */
    if (parent->parent) {
      if (parent == dupliObParent) {
        parent_writer = export_object_and_parents(parent, parent->parent, NULL);
      }
      else {
        parent_writer = export_object_and_parents(parent, parent->parent, dupliObParent);
      }
    }
    else if (parent == dupliObParent) {
      if (dupliObParent->parent == NULL) {
        parent_writer = export_object_and_parents(parent, NULL, NULL);
      }
      else {
        parent_writer = export_object_and_parents(
            parent, dupliObParent->parent, dupliObParent->parent);
      }
    }
    else {
      parent_writer = export_object_and_parents(parent, dupliObParent, dupliObParent);
    }

    BLI_assert(parent_writer);
  }

  xform_writer = create_xform_writer(name, ob, parent_writer);
  //   /* When flattening, the matrix of the dupliobject has to be added. */
  //   if (m_settings.flatten_hierarchy && dupliObParent) {
  //     xform_writer->m_proxy_from = dupliObParent;
  //   }
  if (xform_writer != NULL) {
    writers[name] = xform_writer;
  }

  if (ob->data != NULL) {
    TEMP_WRITER_TYPE *data_writer = create_data_writer(name, ob, xform_writer);
    if (data_writer != NULL) {
      writers[name] = data_writer;
    }
  }

  return xform_writer;
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return NULL;
  }
  return it->second;
}
