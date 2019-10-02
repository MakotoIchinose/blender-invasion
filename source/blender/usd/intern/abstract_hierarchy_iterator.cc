#include "abstract_hierarchy_iterator.h"

#include <iostream>
#include <limits.h>
#include <sstream>
#include <string>

extern "C" {
#include "BKE_anim.h"
#include "BKE_particle.h"

#include "BLI_assert.h"
#include "BLI_math_matrix.h"

#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_particle_types.h"

#include "DEG_depsgraph_query.h"
}

const HierarchyContext *HierarchyContext::root()
{
  return nullptr;
}

bool HierarchyContext::operator<(const HierarchyContext &other) const
{
  if (object != other.object) {
    return object < other.object;
  }
  if (duplicator != NULL && duplicator == other.duplicator) {
    // Only resort to string comparisons when both objects are created by the same duplicator.
    return export_name < other.export_name;
  }

  return export_parent < other.export_parent;
}

bool HierarchyContext::is_instance() const
{
  return !original_export_path.empty();
}
void HierarchyContext::mark_as_instance_of(const std::string &reference_export_path)
{
  original_export_path = reference_export_path;
}
void HierarchyContext::mark_as_not_instanced()
{
  original_export_path.clear();
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

void AbstractHierarchyIterator::debug_print_export_graph() const
{
  size_t total_graph_size = 0;
  for (const ExportGraph::value_type &map_iter : export_graph) {
    const std::pair<Object *, Object *> &parent_info = map_iter.first;
    Object *const export_parent = parent_info.first;
    Object *const duplicator = parent_info.second;

    if (duplicator != nullptr) {
      printf("    DU %s (as dupped by %s):\n",
             export_parent == nullptr ? "-null-" : (export_parent->id.name + 2),
             duplicator->id.name + 2);
    }
    else {
      printf("    OB %s:\n", export_parent == nullptr ? "-null-" : (export_parent->id.name + 2));
    }

    total_graph_size += map_iter.second.size();
    for (HierarchyContext *child_ctx : map_iter.second) {
      if (child_ctx->duplicator == nullptr) {
        printf("       - %s%s%s\n",
               child_ctx->object->id.name + 2,
               child_ctx->weak_export ? " \033[97m(weak)\033[0m" : "",
               child_ctx->original_export_path.size() ?
                   (std::string("ref ") + child_ctx->original_export_path).c_str() :
                   "");
      }
      else {
        printf("       - %s (dup by %s%s) %s\n",
               child_ctx->object->id.name + 2,
               child_ctx->duplicator->id.name + 2,
               child_ctx->weak_export ? ", \033[97mweak\033[0m" : "",
               child_ctx->original_export_path.size() ?
                   (std::string("ref ") + child_ctx->original_export_path).c_str() :
                   "");
      }
    }
  }
  printf("    (Total graph size: %lu objects\n", total_graph_size);
}

void AbstractHierarchyIterator::iterate()
{
  export_graph_construct();
  export_graph_prune();
  determine_export_paths(HierarchyContext::root());
  determine_duplication_references(HierarchyContext::root(), "");
  make_writers(HierarchyContext::root());
  export_graph_clear();
}

void AbstractHierarchyIterator::export_graph_construct()
{
  Scene *scene = DEG_get_evaluated_scene(depsgraph);

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
  HierarchyContext *context = new HierarchyContext();
  context->object = object;
  context->export_name = get_object_name(object);
  context->export_parent = export_parent;
  context->duplicator = nullptr;
  context->weak_export = weak_export;
  context->animation_check_include_parent = false;
  context->export_path = "";
  context->original_export_path = "";
  copy_m4_m4(context->matrix_world, object->obmat);

  export_graph[std::make_pair(export_parent, nullptr)].insert(context);
}

void AbstractHierarchyIterator::visit_dupli_object(DupliObject *dupli_object,
                                                   Object *duplicator,
                                                   const std::set<Object *> &dupli_set)
{
  ExportGraph::key_type graph_index;
  bool animation_check_include_parent = false;

  HierarchyContext *context = new HierarchyContext();
  context->object = dupli_object->ob;
  context->duplicator = duplicator;
  context->weak_export = false;
  context->export_path = "";
  context->original_export_path = "";

  /* If the dupli-object's scene parent is also instanced by this object, use that as the
   * export parent. Otherwise use the dupli-parent as export parent. */
  Object *parent = dupli_object->ob->parent;
  if (parent != nullptr && dupli_set.find(parent) != dupli_set.end()) {
    // The parent object is part of the duplicated collection.
    context->export_parent = parent;
    graph_index = std::make_pair(parent, duplicator);
  }
  else {
    /* The parent object is NOT part of the duplicated collection. This means that the world
     * transform of this dupliobject can be influenced by objects that are not part of its
     * export graph. */
    animation_check_include_parent = true;
    context->export_parent = duplicator;
    graph_index = std::make_pair(duplicator, nullptr);
  }

  context->animation_check_include_parent = animation_check_include_parent;
  copy_m4_m4(context->matrix_world, dupli_object->mat);

  // Construct export name for the dupli-instance.
  std::stringstream suffix_stream;
  suffix_stream << std::hex;
  for (int i = 0; i < MAX_DUPLI_RECUR && dupli_object->persistent_id[i] != INT_MAX; i++) {
    suffix_stream << "-" << dupli_object->persistent_id[i];
  }
  context->export_name = make_valid_name(get_object_name(context->object) + suffix_stream.str());

  export_graph[graph_index].insert(context);
}

static bool prune_the_weak(const HierarchyContext *context,
                           AbstractHierarchyIterator::ExportGraph &modify,
                           const AbstractHierarchyIterator::ExportGraph &iterate)
{
  bool all_is_weak = context != nullptr && context->weak_export;
  Object *object = context != nullptr ? context->object : nullptr;
  Object *duplicator = context != nullptr ? context->duplicator : nullptr;

  const auto map_index = std::make_pair(object, duplicator);
  AbstractHierarchyIterator::ExportGraph::const_iterator child_iterator = iterate.find(map_index);

  if (child_iterator != iterate.end()) {
    for (HierarchyContext *child_context : child_iterator->second) {
      bool child_tree_is_weak = prune_the_weak(child_context, modify, iterate);
      all_is_weak &= child_tree_is_weak;

      if (child_tree_is_weak) {
        // This subtree is all weak, so we can remove it from the current object's children.
        modify[map_index].erase(child_context);
        delete child_context;
      }
    }
  }

  if (all_is_weak) {
    // This node and all its children are weak, so it can be removed from the export graph.
    modify.erase(map_index);
  }

  return all_is_weak;
}

void AbstractHierarchyIterator::export_graph_prune()
{
  // Take a copy of the map so that we can modify while recursing.
  ExportGraph unpruned_export_graph = export_graph;
  prune_the_weak(HierarchyContext::root(), export_graph, unpruned_export_graph);
}

void AbstractHierarchyIterator::export_graph_clear()
{
  for (ExportGraph::iterator::value_type &it : export_graph) {
    for (HierarchyContext *context : it.second) {
      delete context;
    }
  }
  export_graph.clear();
}

AbstractHierarchyIterator::ExportGraph::mapped_type &AbstractHierarchyIterator::graph_children(
    const HierarchyContext *context)
{
  if (context == nullptr) {
    return export_graph[std::make_pair(nullptr, nullptr)];
  }

  return export_graph[std::make_pair(context->object, context->duplicator)];
}

void AbstractHierarchyIterator::determine_export_paths(const HierarchyContext *parent_context)
{
  const std::string &parent_export_path = parent_context ? parent_context->export_path : "";

  for (HierarchyContext *context : graph_children(parent_context)) {
    context->export_path = path_concatenate(parent_export_path, context->export_name);

    if (context->duplicator == nullptr) {
      /* This is an original object, so we should keep track of where it was exported to, just in
       * case it gets duplicated somewhere. */
      ID *orig_ob = &context->object->id;
      originals_export_paths[orig_ob] = context->export_path;

      if (context->object->data != nullptr) {
        ID *object_data = static_cast<ID *>(context->object->data);
        ID *orig_data = object_data;
        originals_export_paths[orig_data] = get_object_data_path(context);
      }
    }

    determine_export_paths(context);
  }
}

void AbstractHierarchyIterator::determine_duplication_references(
    const HierarchyContext *parent_context, std::string indent)
{
  ExportGraph::mapped_type children = graph_children(parent_context);

  for (HierarchyContext *context : children) {
    if (context->duplicator != nullptr) {
      ID *orig_id = &context->object->id;
      const ExportPathMap::const_iterator &it = originals_export_paths.find(orig_id);

      if (it == originals_export_paths.end()) {
        // The original was not found, so mark this instance as "the original".
        context->mark_as_not_instanced();
        originals_export_paths[orig_id] = context->export_path;
      }
      else {
        context->mark_as_instance_of(it->second);
      }

      if (context->object->data) {
        ID *orig_data_id = (ID *)context->object->data;
        const ExportPathMap::const_iterator &it = originals_export_paths.find(orig_data_id);

        if (it == originals_export_paths.end()) {
          // The original was not found, so mark this instance as "original".
          std::string data_path = get_object_data_path(context);
          context->mark_as_not_instanced();
          originals_export_paths[orig_id] = context->export_path;
          originals_export_paths[orig_data_id] = data_path;
        }
      }
    }

    determine_duplication_references(context, indent + "  ");
  }
}

void AbstractHierarchyIterator::make_writers(const HierarchyContext *parent_context)
{
  AbstractHierarchyWriter *xform_writer = nullptr;
  float parent_matrix_inv_world[4][4];

  if (parent_context) {
    invert_m4_m4(parent_matrix_inv_world, parent_context->matrix_world);
  }
  else {
    unit_m4(parent_matrix_inv_world);
  }

  const std::string &parent_export_path = parent_context ? parent_context->export_path : "";

  for (HierarchyContext *context : graph_children(parent_context)) {
    copy_m4_m4(context->parent_matrix_inv_world, parent_matrix_inv_world);

    // Get or create the transform writer.
    xform_writer = ensure_writer(context, &AbstractHierarchyIterator::create_xform_writer);
    if (xform_writer == nullptr) {
      // Unable to export, so there is nothing to attach any children to; just abort this entire
      // branch of the export hierarchy.
      return;
    }

    BLI_assert(DEG_is_evaluated_object(context->object));
    /* XXX This can lead to too many XForms being written. For example, a camera writer can refuse
     * to write an orthographic camera. By the time that this is known, the XForm has already been
     * written. */
    xform_writer->write(*context);

    if (!context->weak_export) {
      make_writers_particle_systems(context);
      make_writer_object_data(context);
    }

    // Recurse into this object's children.
    make_writers(context);
  }

  // TODO(Sybren): iterate over all unused writers and call unused_during_iteration() or something.
}

void AbstractHierarchyIterator::make_writers_particle_systems(
    const HierarchyContext *xform_context)
{
  Object *object = xform_context->object;
  ParticleSystem *psys = static_cast<ParticleSystem *>(object->particlesystem.first);
  for (; psys; psys = psys->next) {
    if (!psys_check_enabled(object, psys, true)) {
      continue;
    }

    HierarchyContext hair_context = *xform_context;
    hair_context.export_path = path_concatenate(xform_context->export_path,
                                                get_id_name(&psys->part->id));
    hair_context.particle_system = psys;

    AbstractHierarchyWriter *writer = nullptr;
    switch (psys->part->type) {
      case PART_HAIR:
        writer = ensure_writer(&hair_context, &AbstractHierarchyIterator::create_hair_writer);
        break;
      case PART_EMITTER:
        writer = ensure_writer(&hair_context, &AbstractHierarchyIterator::create_particle_writer);
        break;
    }

    if (writer != nullptr) {
      writer->write(hair_context);
    }
  }
}

void AbstractHierarchyIterator::make_writer_object_data(const HierarchyContext *context)
{
  if (context->object->data == nullptr) {
    return;
  }

  HierarchyContext data_context = *context;
  data_context.export_path = get_object_data_path(context);

  /* data_context.original_export_path is just a copy from the context. It points to the object,
   * but needs to point to the object data. */
  if (data_context.is_instance()) {
    ID *object_data = static_cast<ID *>(context->object->data);
    data_context.original_export_path = originals_export_paths[object_data];

    /* If the object is marked as an instance, so should the object data. */
    BLI_assert(data_context.is_instance());
  }

  AbstractHierarchyWriter *data_writer;
  data_writer = ensure_writer(&data_context, &AbstractHierarchyIterator::create_data_writer);
  if (data_writer == nullptr) {
    return;
  }

  data_writer->write(data_context);
}

std::string AbstractHierarchyIterator::get_object_name(const Object *object) const
{
  return get_id_name(&object->id);
}

std::string AbstractHierarchyIterator::get_object_data_name(const Object *object) const
{
  ID *object_data = static_cast<ID *>(object->data);
  return get_id_name(object_data);
}

std::string AbstractHierarchyIterator::get_id_name(const ID *id) const
{
  if (id == nullptr) {
    return "";
  }

  return make_valid_name(std::string(id->name + 2));
}

std::string AbstractHierarchyIterator::make_valid_name(const std::string &name) const
{
  return name;
}

std::string AbstractHierarchyIterator::path_concatenate(const std::string &parent_path,
                                                        const std::string &child_path) const
{
  return parent_path + "/" + child_path;
}

std::string AbstractHierarchyIterator::get_object_data_path(const HierarchyContext *context) const
{
  BLI_assert(!context->export_path.empty());
  BLI_assert(context->object->data);

  return path_concatenate(context->export_path, get_object_data_name(context->object));
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
    HierarchyContext *context, AbstractHierarchyIterator::create_writer_func create_func)
{
  AbstractHierarchyWriter *writer = get_writer(context->export_path);
  if (writer != nullptr) {
    return writer;
  }

  writer = (this->*create_func)(context);
  if (writer == nullptr) {
    return nullptr;
  }

  writers[context->export_path] = writer;

  return writer;
}