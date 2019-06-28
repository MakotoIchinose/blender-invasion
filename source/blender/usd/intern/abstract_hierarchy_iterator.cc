#include "abstract_hierarchy_iterator.h"

#include <string>

extern "C" {
#include "BKE_anim.h"

#include "BLI_assert.h"
#include "BLI_math_matrix.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"

#include "DEG_depsgraph_query.h"
}

const HierarchyContext &HierarchyContext::root()
{
  static const HierarchyContext root_hierarchy_context = {.object = nullptr};
  return root_hierarchy_context;
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
  construct_export_graph();

  // // For debug: print the export graph.
  // printf("====== Export graph pre-prune:\n");
  // for (auto it : export_graph) {
  //   const std::pair<Object *, Object *> &parent_info = it.first;
  //   Object *const export_parent = parent_info.first;
  //   Object *const duplicator = parent_info.second;

  //   if (duplicator != nullptr) {
  //     printf("    DU %s (as dupped by %s):\n",
  //            export_parent == nullptr ? "-null-" : (export_parent->id.name + 2),
  //            duplicator->id.name + 2);
  //   }
  //   else {
  //     printf("    OB %s:\n", export_parent == nullptr ? "-null-" : (export_parent->id.name +
  //     2));
  //   }

  //   for (auto child_it : it.second) {
  //     if (child_it.duplicator == nullptr) {
  //       printf("       - %s%s\n",
  //              child_it.object->id.name + 2,
  //              child_it.weak_export ? " (weak)" : "");
  //     }
  //     else {
  //       printf("       - %s (dup by %s%s)\n",
  //              child_it.object->id.name + 2,
  //              child_it.duplicator->id.name + 2,
  //              child_it.weak_export ? ", weak" : "");
  //     }
  //   }
  // }

  prune_export_graph();

  // // For debug: print the export graph.
  // printf("====== Export graph post-prune:\n");
  // for (auto it : export_graph) {
  //   const std::pair<Object *, Object *> &parent_info = it.first;
  //   Object *const export_parent = parent_info.first;
  //   Object *const duplicator = parent_info.second;

  //   if (duplicator != nullptr) {
  //     printf("    DU %s (as dupped by %s):\n",
  //            export_parent == nullptr ? "-null-" : (export_parent->id.name + 2),
  //            duplicator->id.name + 2);
  //   }
  //   else {
  //     printf("    OB %s:\n", export_parent == nullptr ? "-null-" : (export_parent->id.name +
  //     2));
  //   }

  //   for (auto child_it : it.second) {
  //     if (child_it.duplicator == nullptr) {
  //       printf("       - %s%s\n",
  //              child_it.object->id.name + 2,
  //              child_it.weak_export ? " (weak)" : "");
  //     }
  //     else {
  //       printf("       - %s (dup by %s%s)\n",
  //              child_it.object->id.name + 2,
  //              child_it.duplicator->id.name + 2,
  //              child_it.weak_export ? ", weak" : "");
  //     }
  //   }
  // }

  // For debug: print the export paths.
  // printf("====== Export paths:\n");
  make_writers(HierarchyContext::root(), nullptr);

  export_graph.clear();
}

void AbstractHierarchyIterator::construct_export_graph()
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

      for (link = static_cast<DupliObject *>(lb->first); link; link = link->next) {
        if (!should_visit_duplilink(link)) {
          continue;
        }

        visit_dupli_object(link, object, dupli_set);
      }
    }

    free_object_duplilist(lb);
  }
  DEG_OBJECT_ITER_END;
}

void AbstractHierarchyIterator::visit_object(Object *object,
                                             Object *export_parent,
                                             bool weak_export)
{
  HierarchyContext context;
  context.object = object;
  context.export_parent = export_parent;
  context.duplicator = nullptr;
  context.weak_export = weak_export;
  context.animation_check_include_parent = false;
  context.export_path = "";
  context.parent_writer = nullptr;
  copy_m4_m4(context.matrix_world, object->obmat);

  export_graph[std::make_pair(export_parent, nullptr)].insert(context);

  // std::string export_parent_name = export_parent ? get_object_name(export_parent) : "/";
  // printf("    OB %30s %p (export-parent=%s; world x = %f)\n",
  //        get_object_name(context.object).c_str(),
  //        context.object,
  //        export_parent_name.c_str(),
  //        context.matrix_world[3][0]);
}

void AbstractHierarchyIterator::visit_dupli_object(DupliObject *dupli_object,
                                                   Object *duplicator,
                                                   const std::set<Object *> &dupli_set)
{
  ExportGraph::key_type graph_index;
  bool animation_check_include_parent = false;

  HierarchyContext context;
  context.object = dupli_object->ob;
  context.duplicator = duplicator;
  context.weak_export = false;
  context.export_path = "";
  context.parent_writer = nullptr;

  /* If the dupli-object's scene parent is also instanced by this object, use that as the
   * export parent. Otherwise use the dupli-parent as export parent. */
  Object *parent = dupli_object->ob->parent;
  if (parent != nullptr && dupli_set.find(parent) != dupli_set.end()) {
    // The parent object is part of the duplicated collection.
    context.export_parent = parent;
    graph_index = std::make_pair(parent, duplicator);
  }
  else {
    /* The parent object is NOT part of the duplicated collection. This means that the world
     * transform of this dupliobject can be influenced by objects that are not part of its
     * export graph. */
    animation_check_include_parent = true;
    context.export_parent = duplicator;
    graph_index = std::make_pair(duplicator, nullptr);
  }

  context.animation_check_include_parent = animation_check_include_parent;
  copy_m4_m4(context.matrix_world, dupli_object->mat);

  export_graph[graph_index].insert(context);

  // std::string export_parent_name = export_parent ? get_object_name(export_parent) : "/";
  // printf("    DU %30s %p (export-parent=%s; duplicator = %s; world x = %f)\n",
  //        get_object_name(context.object).c_str(),
  //        context.object,
  //        export_parent_name.c_str(),
  //        duplicator->id.name + 2,
  //        context.matrix_world[3][0]);
}

static bool prune_the_weak(const HierarchyContext &context,
                           AbstractHierarchyIterator::ExportGraph &modify,
                           const AbstractHierarchyIterator::ExportGraph &iterate)
{
  bool all_is_weak = context.weak_export;
  const auto map_index = std::make_pair(context.object, context.duplicator);
  AbstractHierarchyIterator::ExportGraph::const_iterator child_iterator = iterate.find(map_index);

  if (child_iterator != iterate.end()) {
    for (const HierarchyContext &child_context : child_iterator->second) {
      bool child_tree_is_weak = prune_the_weak(child_context, modify, iterate);
      all_is_weak &= child_tree_is_weak;

      if (child_tree_is_weak) {
        // This subtree is all weak, so we can remove it from the current object's children.
        modify[map_index].erase(child_context);
      }
    }
  }

  if (all_is_weak) {
    // This node and all its children are weak, so it can be removed from the export graph.
    modify.erase(map_index);
  }

  return all_is_weak;
}

void AbstractHierarchyIterator::prune_export_graph()
{
  // Take a copy of the map so that we can modify while recursing.
  ExportGraph unpruned_export_graph = export_graph;
  prune_the_weak(HierarchyContext::root(), export_graph, unpruned_export_graph);
}

void AbstractHierarchyIterator::make_writers(const HierarchyContext &parent_context,
                                             AbstractHierarchyWriter *parent_writer)
{
  AbstractHierarchyWriter *xform_writer = nullptr;
  AbstractHierarchyWriter *data_writer = nullptr;
  float parent_matrix_inv_world[4][4];

  if (parent_context.object == nullptr) {
    unit_m4(parent_matrix_inv_world);
  }
  else {
    invert_m4_m4(parent_matrix_inv_world, parent_context.matrix_world);
  }

  for (HierarchyContext context :
       export_graph[std::make_pair(parent_context.object, parent_context.duplicator)]) {
    std::string export_path = path_concatenate(parent_context.export_path,
                                               get_object_name(context.object));
    context.parent_writer = parent_writer;
    context.export_path = export_path;
    copy_m4_m4(context.parent_matrix_inv_world, parent_matrix_inv_world);

    // const char *color = context.duplicator ? "32;1" : "30";
    // printf("%s \033[%sm%s\033[0m\n",
    //        export_path.c_str(),
    //        color,
    //        context.duplicator ? context.duplicator->id.name + 2 : "");

    // Get or create the transform writer.
    xform_writer = ensure_writer(context, &AbstractHierarchyIterator::create_xform_writer);
    if (xform_writer == nullptr) {
      // Unable to export, so there is nothing to attach any children to; just abort this entire
      // branch of the export hierarchy.
      return;
    }

    BLI_assert(DEG_is_evaluated_object(context.object));
    xform_writer->write(context);

    // Get or create the object data writer, but only if it is needed.
    if (!context.weak_export && context.object->data != nullptr) {
      HierarchyContext data_context = context;
      ID *object_data = static_cast<ID *>(context.object->data);
      std::string data_path = path_concatenate(export_path, get_id_name(object_data));

      data_context.export_path = data_path;
      data_context.parent_writer = xform_writer;

      data_writer = ensure_writer(data_context, &AbstractHierarchyIterator::create_data_writer);
      if (data_writer != nullptr) {
        data_writer->write(data_context);
      }
    }

    make_writers(context, xform_writer);
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

AbstractHierarchyWriter *AbstractHierarchyIterator::get_writer(const std::string &name)
{
  WriterMap::iterator it = writers.find(name);

  if (it == writers.end()) {
    return nullptr;
  }
  return it->second;
}

AbstractHierarchyWriter *AbstractHierarchyIterator::ensure_writer(
    const HierarchyContext &context, AbstractHierarchyIterator::create_writer_func create_func)
{
  AbstractHierarchyWriter *writer = get_writer(context.export_path);
  if (writer != nullptr) {
    return writer;
  }

  writer = (this->*create_func)(context);
  if (writer != nullptr) {
    writers[context.export_path] = writer;
  }
  return writer;
}
