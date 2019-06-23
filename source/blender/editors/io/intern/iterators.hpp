#ifndef __io_intern_iterators_h__
#define __io_intern_iterators_h__

extern "C" {
/* clang-format off */
#include "BKE_global.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_modifier.h"
#include "BKE_library.h"
#include "BKE_customdata.h"
#include "BKE_scene.h"
#include "BKE_material.h"

#include "DNA_layer_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"

#include "../io_common.h"
}

#include "common.hpp"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

/* clang-format on */

namespace common {

/* clang-format off */
// Adapt a pointer-size pair as a random access iterator
// This makes use of `boost::iterator_facade` and makes it possible to use
// for each style loops, as well as cleanly hiding how the underlying Blender
// data structures are accessed
template<typename T, typename Tag = std::random_access_iterator_tag>
struct pointer_iterator_base {
  using difference_type = ptrdiff_t;
  using value_type = T;
  using pointer    = T *;
  using reference  = T &;
  using iterator_category = Tag;
  pointer_iterator_base(pointer p, size_t size) : first(p), curr(p), size(size) {}
  pointer_iterator_base(const pointer_iterator_base &pib) : first(pib.first), curr(pib.curr), size(pib.size) {}
  // Conversion to base pointer
  operator pointer() const { return curr; }
  pointer_iterator_base &operator=(const pointer_iterator_base &p) {
    // Placement new: construct a new object in the position of `this`
    // Doesn't actually allocate memory
    new (this) pointer_iterator_base(p);
    return *this;
  }
  pointer_iterator_base begin() const { return {this->first, this->size}; }
  pointer_iterator_base end()   const { return {this->first + this->size, this->size}; }
  pointer_iterator_base & operator++() { ++curr; return *this; }
  pointer_iterator_base & operator--() { --curr; return *this; }
  pointer_iterator_base & operator+(difference_type n) { curr += n; return *this; }
  difference_type operator-(const pointer_iterator_base &other) const { return other.curr - curr; }
  bool operator==(const pointer_iterator_base &other) const { return curr == other.curr; }
  pointer first;
  pointer curr;
  size_t size;
};

template<typename T>
struct pointer_iterator : pointer_iterator_base<T, std::random_access_iterator_tag> {
  using pointer_iterator_base<T, std::random_access_iterator_tag>
      ::pointer_iterator_base;
  inline const T & operator*() const { return *this->curr; }
};

// An iterator that iterates over doubly linked lists
template<typename SourceT,
         typename ResT = SourceT &,
         typename Tag = std::bidirectional_iterator_tag>
struct list_iterator : pointer_iterator_base<SourceT, Tag> {
  list_iterator(SourceT *p) : pointer_iterator_base<SourceT, Tag>(p, 0) {}
  list_iterator begin() const { return {this->first}; }
  list_iterator end()   const { return {nullptr}; }
  list_iterator & operator++() { this->curr = this->curr->next; return *this; }
  list_iterator & operator--() { this->curr = this->curr->prev; return *this; }
  inline const ResT operator*() const { return *this->curr; }
};

// Represents an offset into an array (base for iterators like material_iter)
template<typename T>
struct offset_iterator : pointer_iterator_base<T, std::random_access_iterator_tag> {
  offset_iterator(size_t size)
    : pointer_iterator_base<T, std::random_access_iterator_tag>(0, size) {}
  size_t offset() const { return ((size_t) this->curr) / sizeof(T); }
};

// Iterator over the polygons of a mesh
struct poly_iter : pointer_iterator<MPoly> {
  poly_iter(const Mesh *const m) : pointer_iterator(m->mpoly, m->totpoly) {}
  poly_iter(MPoly * const poly, size_t size) : pointer_iterator(poly, size) {}
  poly_iter begin() const { return {this->first, this->size}; }
  poly_iter end()   const { return {this->first + this->size, this->size}; }
  // poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
  // poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
};

// Iterator over the vertices of a mesh
struct vert_iter : pointer_iterator<MVert> {
  vert_iter(const Mesh *const m) : pointer_iterator(m->mvert, m->totvert) {}
};

// Iterator over the vertices of a polygon
struct vert_of_poly_iter : pointer_iterator_base<MLoop, std::random_access_iterator_tag> {
  // TODO someone What order are the vertices stored in? Clockwise?
  vert_of_poly_iter(const Mesh *const mesh, const MPoly &mp)
    : pointer_iterator_base(mesh->mloop + mp.loopstart, mp.totloop), mvert(mesh->mvert) {}
  vert_of_poly_iter(const MVert * const mvert, MLoop *poly, size_t size)
    : pointer_iterator_base(poly, size), mvert(mvert) {}
  vert_of_poly_iter begin() const { return {mvert, this->first, this->size}; }
  vert_of_poly_iter end()   const { return {mvert, this->first + this->size, this->size}; }
  const MVert & operator*() const { return mvert[this->curr->v]; }
  const MVert * const mvert;
};

// Iterator over all the edges of a mesh
struct edge_iter : pointer_iterator<MEdge> {
  edge_iter(const Mesh *const m) : pointer_iterator(m->medge, m->totedge) {}
  edge_iter(MEdge * const e, size_t s) : pointer_iterator(e, s) {}
  // edge_iter(const pointer_iterator<MEdge> &pi) : pointer_iterator(pi) {}
  // edge_iter(pointer_iterator<MEdge> &&pi) : pointer_iterator(pi) {}
};

// Iterator over the edges of a mesh which are marked as loose
struct loose_edge_iter : edge_iter {
  loose_edge_iter(const Mesh *const m) : edge_iter(m) {}
  loose_edge_iter(MEdge * const e, size_t s) : edge_iter(e, s) {}
  loose_edge_iter begin() const { return {this->first, this->size}; }
  loose_edge_iter end()   const { return {this->first + this->size, this->size}; }
  loose_edge_iter & operator++() {
    do {
      ++this->curr;
    } while (!(this->curr->flag & ME_LOOSEEDGE));
    return *this;
  }
  loose_edge_iter & operator--() {
    do {
      --this->curr;
    } while (!(this->curr->flag & ME_LOOSEEDGE));
    return *this;
  }
};

// Iterator over all the objects in a `ViewLayer`
// TODO someone G.is_break
struct object_iter : list_iterator<Base, Object *> {
  object_iter(Base * const b) : list_iterator(b) {}
  object_iter(const ViewLayer *const vl)
      : list_iterator((Base *)vl->object_bases.first) {}
  const Object * operator*() {
    return this->curr->object;
  }
};

struct exportable_object_iter : object_iter {
  exportable_object_iter(const ViewLayer *const vl,
                         const ExportSettings *const settings)
      : object_iter(vl), settings(settings) {}
  exportable_object_iter(Base *base, const ExportSettings *const settings)
      : object_iter(base), settings(settings) {}
  exportable_object_iter begin() const {
    return {this->first, settings};
  }
  exportable_object_iter end() const {
    return {(Base *)nullptr, settings};
  }
  exportable_object_iter & operator++() {
    do {
      this->curr = this->curr->next;
    } while (this->curr != nullptr &&
             !common::should_export_object(settings, this->curr->object));
    return *this;
  }
  const ExportSettings *const settings;
};

// Iterator over the modifiers of an `Object`
struct modifier_iter : list_iterator<ModifierData> {
  modifier_iter(const Object *const ob)
      : list_iterator((ModifierData *)ob->modifiers.first) {}
};

// Iterator over the `MLoop` of a `MPoly` of a mesh
struct loop_of_poly_iter : pointer_iterator<MLoop> {
  loop_of_poly_iter(const Mesh *const mesh, const poly_iter &poly)
    : pointer_iterator(mesh->mloop + (*poly).loopstart, (*poly).totloop) {} // XXX DEBUG THIS HERE
  loop_of_poly_iter(const Mesh *const mesh, const MPoly &poly)
      : pointer_iterator(mesh->mloop + poly.loopstart, poly.totloop) {}
  loop_of_poly_iter(MLoop * const loop, size_t size)
    : pointer_iterator(loop, size) {}
  loop_of_poly_iter begin() const { return loop_of_poly_iter{this->first, this->size}; }
  loop_of_poly_iter end()   const { return loop_of_poly_iter{this->first + this->size, this->size}; }
  // loop_of_poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
  // loop_of_poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
};

struct material_iter : offset_iterator<Material *> {
  material_iter(const Object * const ob)
    : offset_iterator(ob->totcol), ob(ob), mdata(*give_matarar((Object *) ob)) {}
  material_iter begin() const { return material_iter(ob); }
  material_iter end()   const { material_iter mi(ob); mi.curr = mi.first + mi.size; return mi; }
  const Material * operator*() {
    const size_t off = offset();
    if (ob->matbits && ob->matbits[off]) {
      // In Object
      return ob->mat[off];
    } else {
      // In Data
      return mdata[off];
    }
  }
  const Object * const ob;
  const Material * const * const mdata;
};

// Iterator over the UVs of a mesh (as `const std::array<float, 2>`)
struct uv_iter : pointer_iterator_base<MLoopUV> {
  uv_iter(const Mesh *const m) : pointer_iterator_base(m->mloopuv, m->totloop) {}
  uv_iter(MLoopUV * const uv, size_t size) : pointer_iterator_base(uv, size) {}
  uv_iter begin() const { return {this->first, this->size}; }
  uv_iter end()   const { return {this->first + this->size, this->size}; }
  inline const std::array<float, 2> operator*() {
    return {this->curr->uv[0], this->curr->uv[1]};
  }
  // uv_iter(const dereference_iterator_ &di) : dereference_iterator(di, this) {}
  // uv_iter(dereference_iterator_ &&di) : dereference_iterator(di, this) {}
};

// Iterator over the normals of mesh
// This is a more complex one, because there are three possible sources for the normals of a mesh:
//   - Custom normals
//   - Face normals, when the face is not smooth shaded
//   - Vertex normals, when the face is smooth shaded
// This is a completely separate iterator because it needs to override pretty much all behaviours
// It's only a bidirectional iterator, because it is not continuous
struct normal_iter {
  using ResT = const std::array<float, 3>;
  // normal_iter() = default;
  // normal_iter(const normal_iter &) = default;
  // normal_iter(normal_iter &&) = default;
  using difference_type = ptrdiff_t;
  using value_type = ResT;
  using pointer    = ResT *;
  using reference  = ResT &;
  using iterator_category = std::bidirectional_iterator_tag;
  normal_iter(const Mesh *const mesh, const poly_iter &poly,
              const loop_of_poly_iter &loop)
    : mesh(mesh), poly(poly), loop(loop) {
    custom_no = static_cast<float(*)[3]>(CustomData_get_layer(&mesh->ldata, CD_NORMAL));
  }
  normal_iter(const Mesh *const mesh)
    : normal_iter(mesh, poly_iter(mesh), loop_of_poly_iter(mesh, poly_iter(mesh))) {}
  normal_iter begin() const {
    return {mesh};
  }
  normal_iter end() const {
    return {mesh, poly.end(), loop.end()};
  }
  normal_iter & operator++() {
    // If not flat shaded
    // const bool flat_shaded = (((*poly).flag & ME_SMOOTH) == 1);
    // if (flat_shaded)
    ++loop;
    if (loop == loop.end()) {
      ++poly;
      // if incrementing the poly didn't put us past the end,
      if (poly != poly.end())
        // use the new poly to generate the new loop iterator
        loop = loop_of_poly_iter{mesh, poly};
      else
        // otherwise make sure loop is at the end, so we can stop iterating
        loop = loop.end();
    }
    return *this;
  }
  normal_iter & operator--() {
    if (loop != loop.begin()) {
      --loop;
    }
    else if (poly != poly.begin()) {
      --poly;
      loop = loop_of_poly_iter{mesh, poly};
    }
    return *this;
  }
  bool operator==(const normal_iter &other) const {
    // Equal if the poly iterator is the same
    return poly == other.poly &&
      // And either the face is not smooth shaded, in which case we
      // don't care about the loop, or if the loop is the same
      (((*poly).flag & ME_SMOOTH) == 0 || loop == other.loop);
  }
  bool operator!=(const normal_iter &other) const {
    return !(*this == other);
  }
  ResT operator*() const {
    // If we have custom normals, read from there
    if (custom_no) {
      const float(&no)[3] = custom_no[(*loop).v];
      return {no[0], no[1], no[2]};
    } else {
      float no[3];
      // If the face is not smooth shaded, calculate the normal of the face
      if (((*poly).flag & ME_SMOOTH) == 0) {
        // Note the `loop.first`. This is because the function expects
        // a pointer to the first element of the loop
        BKE_mesh_calc_poly_normal(poly, loop.first, mesh->mvert, no);
      } else {
        // Otherwise, the normal is stored alongside the vertex,
        // as a short, so we retrieve it
        normal_short_to_float_v3(no, mesh->mvert[(*loop).v].no);
      }
      return {no[0], no[1], no[2]};
    }
  }
  const Mesh *const mesh;
  poly_iter poly;
  loop_of_poly_iter loop;
  const float (*custom_no)[3];
};

// --- Deduplication ---

// Iterator which performs a deduplication of the values coming out of the SourceIter
template<typename KeyT,
         typename SourceIter,
         typename ResT = const typename KeyT::first_type,
         typename Tag = std::forward_iterator_tag>
struct deduplicated_iterator {
  using difference_type = ptrdiff_t;
  using value_type = ResT;
  using pointer = ResT *;
  using reference = ResT &;
  using iterator_category = Tag;
  deduplicated_iterator(const Mesh *const mesh, dedup_pair_t<KeyT> &dp,
                        ulong &total, SourceIter it)
    : it(it), mesh(mesh), dedup_pair(dp), total(total) {}
  deduplicated_iterator(const Mesh *const mesh,
                                 dedup_pair_t<KeyT> &dp,
                                 ulong &total, ulong reserve)
      : deduplicated_iterator(mesh, dp, total, SourceIter{mesh}) {
    // Reserve space so we don't constantly allocate
    dedup_pair.second.reserve(reserve);
    // Need to insert the first element, because we need to dereference before incrementing
    auto p = dedup_pair.first.insert(std::make_pair(*this->it, total++));
    dedup_pair.second.push_back(p.first);
    ++this->it;
  }
  deduplicated_iterator begin() const {
    return deduplicated_iterator(mesh, dedup_pair, total, it.begin());
  }
  deduplicated_iterator end() const {
    return deduplicated_iterator(mesh, dedup_pair, total, it.end());
  }
  deduplicated_iterator & operator++() {
    // Handle everything until the next different element, or the end, by...
    while (true) {
      // going to the next element of the `SourceIter`
      ++this->it;
      // if at the end, we're done
      if (this->it == this->it.end())
        return *this;
      // try to insert it into the set
      auto p = dedup_pair.first.insert(std::make_pair(*this->it, total));
      // push the set::iterator onto the back of the mapping vector
      dedup_pair.second.push_back(p.first);
      // If we actually inserted in the set
      if (p.second) {
        // There's a new element, so increment the total and stop
        ++total;
        return *this;
      }
    }
    // Should be unreachable
    return *this;
  }
  bool operator==(const deduplicated_iterator &other) {
    return it == other.it;
  }
  bool operator!=(const deduplicated_iterator &other) {
    return !(it == other.it);
  }
  ResT operator*() const {
    return this->dedup_pair.second.back()->first;
  }
  SourceIter it;
  const Mesh *const mesh;
  dedup_pair_t<KeyT> &dedup_pair;
  ulong &total;
};

// Iterator to deduplicated normals (returns `const std::array<float, 3>`)
struct deduplicated_normal_iter : deduplicated_iterator<no_key_t, normal_iter> {
  deduplicated_normal_iter(const Mesh *const mesh, ulong &total, dedup_pair_t<no_key_t> &dp)
      : deduplicated_iterator<no_key_t, normal_iter>(mesh, dp, total, total + mesh->totvert) {}
  // The last element in the mapping vector. Requires we insert the first element
  // in the constructor, otherwise this vector would be empty
  const std::array<float, 3> operator*() const {
    return this->dedup_pair.second.back()->first;
  }
};

// Iterator to deduplicated UVs (returns `const std::array<float, 2>`)
struct deduplicated_uv_iter : deduplicated_iterator<uv_key_t, uv_iter> {
  deduplicated_uv_iter(const Mesh *const mesh, ulong &total, dedup_pair_t<uv_key_t> &dp)
      : deduplicated_iterator<uv_key_t, uv_iter>(mesh, dp, total, total + mesh->totloop) {}
  // The last element in the mapping vector. Requires we insert the first element
  // in the constructor, otherwise this vector would be empty
  const std::array<float, 2> operator*() const {
    return this->dedup_pair.second.back()->first;
  }
};

}  // namespace common

/* clang-format on */
#endif  // __io_intern_iterators_h__
