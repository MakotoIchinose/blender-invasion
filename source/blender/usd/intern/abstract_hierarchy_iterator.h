/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Blender Foundation.
 * All rights reserved.
 */

/*
 * This file contains the AbstractHierarchyIterator. It is intended for exporters for file
 * formats that concern an entire hierarchy of objects (rather than, for example, an OBJ file that
 * contains only a single mesh). Examples are Universal Scene Description (USD) and Alembic.
 * AbstractHierarchyIterator is intended to be subclassed to support concrete file formats.
 *
 * The AbstractHierarchyIterator makes a distinction between the actual object hierarchy and the
 * export hierarchy. The former is the parent/child structure in Blender, which can have multiple
 * parent-like objects. For example, a duplicated object can have both a duplicator and a parent,
 * both determining the final transform. The export hierarchy is the hierarchy as written to the
 * file, and every object has only one export-parent.
 *
 * Currently the AbstractHierarchyIterator does not make any decisions about *what* to export.
 * Selections like "selected only" or "no hair systems" are left to concrete subclasses.
 */

#ifndef __USD__ABSTRACT_HIERARCHY_ITERATOR_H__
#define __USD__ABSTRACT_HIERARCHY_ITERATOR_H__

#include <map>
#include <string>
#include <set>

struct Base;
struct Depsgraph;
struct DupliObject;
struct ID;
struct Object;
struct ParticleSystem;
struct ViewLayer;

class AbstractHierarchyWriter;

struct HierarchyContext {
  /* Determined during hierarchy iteration: */
  Object *object;
  Object *export_parent;
  Object *duplicator;
  float matrix_world[4][4];
  std::string export_name;

  /* When true, the object will be exported only as transform, and only if is an ancestor of a
   * non-weak child: */
  bool weak_export;

  /* When true, this object should check its parents for animation data when determining whether
   * it's animated. */
  bool animation_check_include_parent;

  /* Determined during writer creation: */
  float parent_matrix_inv_world[4][4]; /* Inverse of the parent's world matrix. */
  std::string export_path;  // Hierarchical path, such as "/grandparent/parent/objectname".
  ParticleSystem *particle_system;         // Only set for particle/hair writers.

  // For making the struct insertable into a std::set<>.
  bool operator<(const HierarchyContext &other) const;

  /* Return a HierarchyContext representing the root of the export hierarchy. */
  static const HierarchyContext *root();
};

class AbstractHierarchyWriter {
 public:
  virtual void write(HierarchyContext &context) = 0;
  // TODO: add function like unused_during_iteration() that's called when a writer was previously
  // created, but wasn't used this iteration.
};

class AbstractHierarchyIterator {
 public:
  typedef std::map<std::string, AbstractHierarchyWriter *> WriterMap;
  // Mapping from <object, duplicator> to the object's export-children.
  typedef std::map<std::pair<Object *, Object *>, std::set<HierarchyContext *>> ExportGraph;

 protected:
  ExportGraph export_graph;
  Depsgraph *depsgraph;
  WriterMap writers;

 public:
  explicit AbstractHierarchyIterator(Depsgraph *depsgraph);
  virtual ~AbstractHierarchyIterator();

  void iterate();
  const WriterMap &writer_map() const;
  void release_writers();

  virtual std::string get_id_name(const ID *id) const;
  virtual std::string make_valid_name(const std::string &name) const;

 private:
  void debug_print_export_graph() const;

  void export_graph_construct();
  void export_graph_prune();
  void export_graph_clear();

  void visit_object(Object *object, Object *export_parent, bool weak_export);
  void visit_dupli_object(DupliObject *dupli_object,
                          Object *duplicator,
                          const std::set<Object *> &dupli_set);

  ExportGraph::mapped_type &graph_children(const HierarchyContext *parent_context);

  void make_writers(const HierarchyContext *parent_context);
  void make_writer_object_data(const HierarchyContext *context);
  void make_writers_particle_systems(const HierarchyContext *context);

  std::string get_object_name(const Object *object) const;

  AbstractHierarchyWriter *get_writer(const std::string &name);

  typedef AbstractHierarchyWriter *(AbstractHierarchyIterator::*create_writer_func)(
      const HierarchyContext *);
  AbstractHierarchyWriter *ensure_writer(HierarchyContext *context,
                                         create_writer_func create_func);

 protected:
  virtual bool should_visit_duplilink(const DupliObject *link) const;
  virtual bool should_export_object(const Object *object) const;

  virtual AbstractHierarchyWriter *create_xform_writer(const HierarchyContext *context) = 0;
  virtual AbstractHierarchyWriter *create_data_writer(const HierarchyContext *context) = 0;
  virtual AbstractHierarchyWriter *create_hair_writer(const HierarchyContext *context) = 0;
  virtual AbstractHierarchyWriter *create_particle_writer(const HierarchyContext *context) = 0;

  virtual void delete_object_writer(AbstractHierarchyWriter *writer) = 0;

  virtual std::string path_concatenate(const std::string &parent_path,
                                       const std::string &child_path) const;
};

#endif /* __USD__ABSTRACT_HIERARCHY_ITERATOR_H__ */
