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

    // Export the duplicated objects instanced by this object.
    ListBase *lb = object_duplilist(depsgraph, scene, base->object);
    if (lb) {
      DupliObject *link = nullptr;

      // Construct the set of duplicated objects, so that later we can determine whether a parent
      // is also duplicated itself.
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

  // Add the parent objects that weren't included in this view layer as transform-only objects.
  // This ensures that the object hierarchy in Blender is reflected in the exported file.
  printf("====== adding xform-onlies:\n");
  while (!xform_onlies.empty()) {
    std::set<Object *>::iterator first = xform_onlies.begin();

    Object *xform_only = *first;
    visit_object(nullptr, xform_only, xform_only->parent, true);

    xform_onlies.erase(xform_only);
  }

  // For debug: print the export graph.
  printf("====== Export graph:\n");
  for (auto it : export_graph) {
    printf("    OB %s:\n", it.first == nullptr ? "/" : (it.first->id.name + 2));
    for (auto child_it : it.second) {
      printf("       - %s (xform_only=%s)\n",
             child_it.object->id.name + 2,
             child_it.xform_only ? "true" : "false");
    }
  }

  // For debug: print the export paths.
  printf("====== Export paths:\n");
  make_writers(nullptr, "", nullptr);
}

void AbstractHierarchyIterator::make_writers(Object *parent_object,
                                             const std::string &parent_path,
                                             TEMP_WRITER_TYPE *parent_writer)
{
  TEMP_WRITER_TYPE *xform_writer = nullptr;
  TEMP_WRITER_TYPE *data_writer = nullptr;

  for (const ExportInfo &export_info : export_graph[parent_object]) {
    // TODO(Sybren): make the separator overridable in a subclass.
    std::string usd_path = parent_path + "/" + get_object_name(export_info.object);

    const char *colour = export_info.xform_only ? "31;1" : "30";
    printf("%s \033[%sm%s\033[0m\n",
           usd_path.c_str(),
           colour,
           export_info.xform_only ? "true" : "false");

    xform_writer = create_xform_writer(usd_path, export_info.object, parent_writer);
    if (xform_writer != nullptr) {
      writers[usd_path] = xform_writer;
    }

    if (!export_info.xform_only && export_info.object->data != nullptr) {
      data_writer = create_data_writer(usd_path, export_info.object, xform_writer);
      if (data_writer != nullptr) {
        writers[usd_path] = data_writer;
      }
    }

    make_writers(export_info.object, usd_path, xform_writer);
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
    // The parent is not (yet) exported, to remember to export it as transform-only.
    xform_onlies.insert(export_parent);
  }

  // This object is exported for real (so not "transform-only").
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
}

TEMP_WRITER_TYPE *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return nullptr;
  }
  return it->second;
}
