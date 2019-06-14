#include "usd_hierarchy_iterator.h"

#include <string>

#include <pxr/base/tf/stringUtils.h>

extern "C" {
#include "BKE_anim.h"
#include "BLI_assert.h"

#include "DEG_depsgraph_query.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
}

static std::string get_id_name(const Object *const ob)
{
  if (!ob) {
    return "";
  }

  return get_id_name(&ob->id);
}

static std::string get_id_name(const ID *const id)
{
  if (!id)
    return "";

  std::string name(id->name + 2);
  return pxr::TfMakeValidIdentifier(name);
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
static std::string get_object_dag_path_name(const Object *const ob, Object *dupli_parent)
{
  std::string name = get_id_name(ob);

  Object *p = ob->parent;

  while (p) {
    name = get_id_name(p) + "/" + name;
    p = p->parent;
  }

  if (dupli_parent && (ob != dupli_parent)) {
    name = get_id_name(dupli_parent) + "/" + name;
  }

  return name;
}

void AbstractHierarchyIterator::iterate()
{
  ViewLayer *view_layer = DEG_get_input_view_layer(depsgraph);
  for (Base *base = static_cast<Base *>(view_layer->object_bases.first); base; base = base->next) {
    Object *ob = base->object;
    visit_object(base, ob, ob->parent, NULL);
  }
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
      //   /* This skips things like custom bone shapes. */
      //   if (m_settings.renderable_only && link->no_draw) {
      //     continue;
      //   }

      if (link->type == OB_DUPLICOLLECTION) {
        dupli_ob = link->ob;
        dupli_parent = (dupli_ob->parent) ? dupli_ob->parent : ob;

        visit_object(base, dupli_ob, dupli_parent, ob);
      }
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
  TEMP_WRITER_TYPE *my_writer = get_writer(name);
  if (my_writer != NULL) {
    return my_writer;
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

  my_writer = create_object_writer(name, ob, parent_writer);

  //   /* When flattening, the matrix of the dupliobject has to be added. */
  //   if (m_settings.flatten_hierarchy && dupliObParent) {
  //     my_writer->m_proxy_from = dupliObParent;
  //   }

  writers[name] = my_writer;
  return my_writer;
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return NULL;
  }
  return it->second;
}