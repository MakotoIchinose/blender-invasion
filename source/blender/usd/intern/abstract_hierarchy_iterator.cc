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
                                             AbstractHierarchyWriter *parent_writer)
{
  AbstractHierarchyWriter *xform_writer = nullptr;
  AbstractHierarchyWriter *data_writer = nullptr;

  for (HierarchyContext context : export_graph[parent_object]) {
    std::string export_path = path_concatenate(parent_path, get_object_name(context.object));
    context.parent_writer = parent_writer;
    context.export_path = export_path;

    const char *color = context.xform_only ? "31;1" : "30";
    printf("%s \033[%sm%s\033[0m\n",
           export_path.c_str(),
           color,
           context.xform_only ? "true" : "false");

    // Get or create the transform writer.
    xform_writer = get_writer(export_path);
    if (xform_writer == nullptr) {
      xform_writer = create_xform_writer(context);
      if (xform_writer != nullptr) {
        writers[export_path] = xform_writer;
      }
    }
    if (xform_writer == nullptr) {
      // Unable to export, so there is nothing to attach any children to; just abort this entire
      // branch of the export hierarchy.
      return;
    }

    Object *object_eval = DEG_get_evaluated_object(depsgraph, context.object);
    xform_writer->write(object_eval);

    // Get or create the object data writer, but only if it is needed.
    if (!context.xform_only && context.object->data != nullptr) {
      ID *object_data = static_cast<ID *>(context.object->data);
      std::string data_path = path_concatenate(export_path, get_id_name(object_data));

      // Update the export context for writing the data.
      context.export_path = data_path;
      context.parent_writer = xform_writer;

      data_writer = get_writer(data_path);
      if (data_writer == nullptr) {
        data_writer = create_data_writer(context);
        if (data_writer != nullptr) {
          writers[data_path] = data_writer;
        }
      }

      if (data_writer != nullptr) {
        data_writer->write(object_eval);
      }
    }

    make_writers(context.object, export_path, xform_writer);
  }

  // TODO(Sybren): iterate over all unused writers and call unused_during_iteration() or something.
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

std::string AbstractHierarchyIterator::path_concatenate(const std::string &parent_path,
                                                        const std::string &child_path) const
{
  return parent_path + "/" + child_path;
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

  BLI_assert(export_parent == nullptr || DEG_is_original_object(export_parent));
  if (export_parent != NULL && export_graph.find(export_parent) == export_graph.end()) {
    // The parent is not (yet) exported, to remember to export it as transform-only.
    xform_onlies.insert(export_parent);
  }

  // This object is exported for real (so not "transform-only").
  xform_onlies.erase(object);

  HierarchyContext context = {
      .object = object,
      .export_parent = export_parent,
      .xform_only = xform_only || object->type == OB_EMPTY,

      // Will be determined during the writer creation:
      .export_path = "",
      .parent_writer = nullptr,
  };
  export_graph[export_parent].insert(context);

  std::string export_parent_name = export_parent ? get_object_name(export_parent) : "/";
  printf("    OB %20s (parent=%s; xform-only=%s)\n",
         get_object_name(object).c_str(),
         export_parent_name.c_str(),
         context.xform_only ? "true" : "false");
}

AbstractHierarchyWriter *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return nullptr;
  }
  return it->second;
}
