#include "abstract_hierarchy_iterator.h"

#include <string>

extern "C" {
#include "BKE_anim.h"

#include "BLI_assert.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"

#include "DEG_depsgraph_query.h"
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
  Scene *scene = DEG_get_evaluated_scene(depsgraph);

  // printf("====== Visiting objects:\n");
  DEG_OBJECT_ITER_BEGIN (depsgraph,
                         object,
                         DEG_ITER_OBJECT_FLAG_LINKED_DIRECTLY |
                             DEG_ITER_OBJECT_FLAG_LINKED_VIA_SET) {
    if (object->base_flag & BASE_HOLDOUT) {
      visit_object(object, object->parent, true);
      continue;
    }

    // Non-instanced objects always have their object-parent as export-parent.
    bool weak_export = !should_export_object(object);
    visit_object(object, object->parent, weak_export);

    if (weak_export) {
      // If a duplicator shouldn't be exported, its duplilist also shouldn't be.
      continue;
    }

    // Export the duplicated objects instanced by this object.
    ListBase *lb = object_duplilist(depsgraph, scene, object);
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
          export_parent = object;
        }

        visit_object(link->ob, export_parent, false);
      }
    }

    free_object_duplilist(lb);
  }
  DEG_OBJECT_ITER_END;

  // // For debug: print the export graph.
  // printf("====== Export graph pre-prune:\n");
  // for (auto it : export_graph) {
  //   printf("    OB %s:\n", it.first == nullptr ? "/" : (it.first->id.name + 2));
  //   for (auto child_it : it.second) {
  //     printf("       - %s (weak_export=%s)\n",
  //            child_it.object->id.name + 2,
  //            child_it.weak_export ? "true" : "false");
  //   }
  // }

  prune_export_graph();

  // // For debug: print the export graph.
  // printf("====== Export graph post-prune:\n");
  // for (auto it : export_graph) {
  //   printf("    OB %s (%p):\n", it.first == nullptr ? "/" : (it.first->id.name + 2), it.first);
  //   for (auto child_it : it.second) {
  //     printf("       - %s (weak_export=%s)\n",
  //            child_it.object->id.name + 2,
  //            child_it.weak_export ? "true" : "false");
  //   }
  // }

  // For debug: print the export paths.
  // printf("====== Export paths:\n");
  make_writers(nullptr, "", nullptr);
}

static bool prune_the_weak(const HierarchyContext &context,
                           AbstractHierarchyIterator::ExportGraph &modify,
                           const AbstractHierarchyIterator::ExportGraph &iterate)
{
  bool all_is_weak = context.weak_export;
  AbstractHierarchyIterator::ExportGraph::const_iterator child_iterator = iterate.find(
      context.object);

  if (child_iterator != iterate.end()) {
    for (const HierarchyContext &child_context : child_iterator->second) {
      bool child_tree_is_weak = prune_the_weak(child_context, modify, iterate);
      all_is_weak &= child_tree_is_weak;

      if (child_tree_is_weak) {
        // This subtree is all weak, so we can remove it from the current object's children.
        modify[context.object].erase(child_context);
      }
    }
  }

  if (all_is_weak) {
    // This node and all its children are weak, so it can be removed from the export graph.
    modify.erase(context.object);
  }

  return all_is_weak;
}

void AbstractHierarchyIterator::prune_export_graph()
{
  // Take a copy of the map so that we can modify while recursing.
  ExportGraph unpruned_export_graph = export_graph;
  const HierarchyContext root = {.object = nullptr};

  prune_the_weak(root, export_graph, unpruned_export_graph);
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

    // const char *color = context.weak_export ? "31;1" : "30";
    // printf("%s \033[%sm%s\033[0m\n",
    //        export_path.c_str(),
    //        color,
    //        context.weak_export ? "true" : "false");

    // Get or create the transform writer.
    xform_writer = ensure_xform_writer(context);
    if (xform_writer == nullptr) {
      // Unable to export, so there is nothing to attach any children to; just abort this entire
      // branch of the export hierarchy.
      return;
    }

    BLI_assert(DEG_is_evaluated_object(context.object));
    xform_writer->write(context.object);

    // Get or create the object data writer, but only if it is needed.
    if (!context.weak_export && context.object->data != nullptr) {
      ID *object_data = static_cast<ID *>(context.object->data);
      std::string data_path = path_concatenate(export_path, get_id_name(object_data));

      // Update the export context for writing the data.
      context.export_path = data_path;
      context.parent_writer = xform_writer;

      data_writer = ensure_data_writer(context);
      if (data_writer != nullptr) {
        data_writer->write(context.object);
      }
    }

    make_writers(context.object, export_path, xform_writer);
  }

  // TODO(Sybren): iterate over all unused writers and call unused_during_iteration() or something.
}

std::string AbstractHierarchyIterator::get_object_name(const Object *object) const
{
  if (object == nullptr) {
    return "";
  }

  return get_id_name(&object->id);
}

std::string AbstractHierarchyIterator::get_id_name(const ID *id) const
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

bool AbstractHierarchyIterator::should_visit_duplilink(const DupliObject *link) const
{
  // Removing link->no_draw hides things like custom bone shapes.
  return !link->no_draw;
}
bool AbstractHierarchyIterator::should_export_object(const Object * /*object*/) const
{
  return true;
}

void AbstractHierarchyIterator::visit_object(Object *object,
                                             Object *export_parent,
                                             bool weak_export)
{
  BLI_assert(DEG_is_evaluated_object(object));
  BLI_assert(export_parent == nullptr || DEG_is_evaluated_object(export_parent));

  HierarchyContext context = {
      .object = object,
      .export_parent = export_parent,
      .weak_export = weak_export,

      // Will be determined during the writer creation:
      .export_path = "",
      .parent_writer = nullptr,
  };
  export_graph[export_parent].insert(context);

  // std::string export_parent_name = export_parent ? get_object_name(export_parent) : "/";
  // printf("    OB %30s %p (parent=%s %p; xform-only=%s; instance=%s)\n",
  //        get_object_name(object).c_str(),
  //        object,
  //        export_parent_name.c_str(),
  //        export_parent,
  //        context.weak_export ? "\033[31;1mtrue\033[0m" : "false",
  //        context.object->base_flag & BASE_FROM_DUPLI ? "\033[35;1mtrue\033[0m" :
  //                                                      "\033[30;1mfalse\033[0m");
}

AbstractHierarchyWriter *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return nullptr;
  }
  return it->second;
}

AbstractHierarchyWriter *AbstractHierarchyIterator::ensure_xform_writer(
    const HierarchyContext &context)
{
  AbstractHierarchyWriter *xform_writer = get_writer(context.export_path);
  if (xform_writer != nullptr) {
    return xform_writer;
  }

  xform_writer = create_xform_writer(context);
  if (xform_writer != nullptr) {
    writers[context.export_path] = xform_writer;
  }
  return xform_writer;
}

AbstractHierarchyWriter *AbstractHierarchyIterator::ensure_data_writer(
    const HierarchyContext &context)
{
  AbstractHierarchyWriter *data_writer = get_writer(context.export_path);
  if (data_writer != nullptr) {
    return data_writer;
  }

  data_writer = create_data_writer(context);
  if (data_writer != nullptr) {
    writers[context.export_path] = data_writer;
  }
  return data_writer;
}
