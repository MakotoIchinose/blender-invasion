#include "abstract_hierarchy_iterator.h"

#include <string>

extern "C" {
#include "BKE_anim.h"

#include "BLI_assert.h"

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

const AbstractHierarchyIterator::WriterMap &AbstractHierarchyIterator::writer_map() const
{
  return writers;
}

void AbstractHierarchyIterator::release_writers()
{
  for (WriterMap::value_type it : writers) {
    delete_object_writer(it.second);
  }
  writers.clear();
}

void AbstractHierarchyIterator::iterate()
{
  ViewLayer *view_layer = DEG_get_input_view_layer(depsgraph);

  printf("====== Visiting objects:\n");
  Scene *scene = DEG_get_input_scene(depsgraph);
  for (Base *base = static_cast<Base *>(view_layer->object_bases.first); base; base = base->next) {
    if (base->flag & BASE_HOLDOUT) {
      continue;
    }

    // Non-instanced objects always have their object-parent as export-parent.
    visit_object(base, base->object, base->object->parent, false);

    // Object *evaluated_ob = DEG_get_evaluated_object(depsgraph, object);
    // export_object_and_parents(ob, parent, dupliObParent);

    ListBase *lb = object_duplilist(depsgraph, scene, base->object);
    if (lb) {
      DupliObject *link = nullptr;

      std::set<Object *> dupli_set;
      for (link = static_cast<DupliObject *>(lb->first); link; link = link->next) {
        if (!should_visit_duplilink(link)) {
          continue;
        }
        dupli_set.insert(link->ob);
      }

      Object *export_parent = nullptr;
      for (link = static_cast<DupliObject *>(lb->first); link; link = link->next) {
        if (!should_visit_duplilink(link)) {
          continue;
        }
        // If the dupli-object's scene parent is also instanced by this object, use that as the
        // export parent. Otherwise use the dupli-parent as export parent.
        if (link->ob->parent != nullptr && dupli_set.find(link->ob->parent) != dupli_set.end()) {
          export_parent = link->ob->parent;
        }
        else {
          export_parent = base->object;
        }
        visit_object(base, link->ob, export_parent, false);
      }
    }

    free_object_duplilist(lb);
  }

  printf("====== adding xform-onlies:\n");
  while (!xform_onlies.empty()) {
    std::set<Object *>::iterator first = xform_onlies.begin();

    Object *xform_only = *first;
    visit_object(nullptr, xform_only, xform_only->parent, true);

    xform_onlies.erase(xform_only);
  }

  printf("====== Export graph:\n");
  for (auto it : export_graph) {
    printf("    OB %s:\n", it.first == nullptr ? "/" : (it.first->id.name + 2));
    for (auto child_it : it.second) {
      printf("       - %s (xform_only=%s)\n",
             child_it.object->id.name + 2,
             child_it.xform_only ? "true" : "false");
    }
  }

  printf("====== Export paths:\n");
  make_paths(nullptr, "");
}

void AbstractHierarchyIterator::make_paths(Object *for_object, const std::string &at_path)
{
  for (auto it : export_graph[for_object]) {
    std::string usd_path = at_path + "/" + get_object_name(it.object);

    const char *colour = it.xform_only ? "31;1" : "30";
    printf("%s \033[%sm%s\033[0m\n", usd_path.c_str(), colour, it.xform_only ? "true" : "false");

    make_paths(it.object, usd_path);
  }
}

std::string AbstractHierarchyIterator::get_object_name(const Object *const object) const
{
  if (object == nullptr) {
    return "";
  }

  return get_id_name(&object->id);
}

std::string AbstractHierarchyIterator::get_id_name(const ID *const id) const
{
  if (id == nullptr) {
    return "";
  }

  return std::string(id->name + 2);
}

/**
 * \brief get_object_dag_path_name returns the name under which the object
 *  will be exported in the Alembic file. It is of the form
 *  "[../grandparent/]parent/object" if dupli_parent is nullptr, or
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

bool AbstractHierarchyIterator::should_visit_object(const Base * /*base*/,
                                                    bool /*is_duplicated*/) const
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
                                             Object *export_parent,
                                             bool xform_only)
{
  /* If an object isn't exported itself, its duplilist shouldn't be
   * exported either. */
  if (!should_visit_object(base, false)) {
    return;
  }

  BLI_assert(DEG_is_original_object(export_parent));
  if (export_parent != NULL && export_graph.find(export_parent) == export_graph.end()) {
    // If the export-parent is not an exportable object, it should be exported as XForm-only.
    xform_onlies.insert(export_parent);
  }
  xform_onlies.erase(object);

  ExportInfo export_info = {
      .object = object,
      .xform_only = xform_only || object->type == OB_EMPTY,
  };
  export_graph[export_parent].insert(export_info);

  std::string export_parent_name = export_parent ? get_object_name(export_parent) : "/";
  printf("    OB %20s (parent=%s; xform-only=%s)\n",
         get_object_name(object).c_str(),
         export_parent_name.c_str(),
         export_info.xform_only ? "true" : "false");

  // Object *evaluated_ob = DEG_get_evaluated_object(depsgraph, object);
  // export_object_and_parents(ob, parent, dupliObParent);

  // ListBase *lb = object_duplilist(depsgraph, DEG_get_input_scene(depsgraph), ob);

  // if (lb) {
  //   DupliObject *link = static_cast<DupliObject *>(lb->first);
  //   Object *dupli_ob = nullptr;
  //   Object *dupli_parent = nullptr;

  //   for (; link; link = link->next) {
  //     if (!should_visit_duplilink(link)) {
  //       continue;
  //     }

  //     dupli_ob = link->ob;
  //     dupli_parent = (dupli_ob->parent) ? dupli_ob->parent : ob;
  //     visit_object(base, dupli_ob, dupli_parent, ob);
  //   }

  //   free_object_duplilist(lb);
  // }
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::export_object_and_parents(Object *ob,
                                                                       Object *parent,
                                                                       Object *dupliObParent)
{
  /* An object should not be its own parent, or we'll get infinite loops. */
  BLI_assert(ob != parent);
  BLI_assert(ob != dupliObParent);

  std::string name = get_object_dag_path_name(ob, dupliObParent);

  /* check if we have already created a transform writer for this object */
  TEMP_WRITER_TYPE *xform_writer = get_writer(name);
  if (xform_writer != nullptr) {
    return xform_writer;
  }

  TEMP_WRITER_TYPE *parent_writer = nullptr;
  if (parent != nullptr) {
    /* Since there are so many different ways to find parents (as evident
     * in the number of conditions below), we can't really look up the
     * parent by name. We'll just call export_object_and_parents(), which will
     * return the parent's writer pointer. */
    if (parent->parent) {
      if (parent == dupliObParent) {
        parent_writer = export_object_and_parents(parent, parent->parent, nullptr);
      }
      else {
        parent_writer = export_object_and_parents(parent, parent->parent, dupliObParent);
      }
    }
    else if (parent == dupliObParent) {
      if (dupliObParent->parent == nullptr) {
        parent_writer = export_object_and_parents(parent, nullptr, nullptr);
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
  if (xform_writer != nullptr) {
    writers[name] = xform_writer;
  }

  if (ob->data != nullptr) {
    TEMP_WRITER_TYPE *data_writer = create_data_writer(name, ob, xform_writer);
    if (data_writer != nullptr) {
      writers[name] = data_writer;
    }
  }

  return xform_writer;
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return nullptr;
  }
  return it->second;
}
