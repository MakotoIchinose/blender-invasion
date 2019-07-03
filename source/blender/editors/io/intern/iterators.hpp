#ifndef __io_intern_iterators_h__
#define __io_intern_iterators_h__

extern "C" {

#include "BKE_customdata.h"
#include "BKE_global.h"
#include "BKE_library.h"
#include "BKE_material.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_modifier.h"
#include "BKE_scene.h"

#include "DNA_curve_types.h"
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

namespace common {

// Adapt a pointer-size pair as a random access iterator
// This makes use of `boost::iterator_facade` and makes it possible to use
// for each style loops, as well as cleanly hiding how the underlying Blender
// data structures are accessed
template<typename T, typename Tag = std::random_access_iterator_tag> struct pointer_iterator_base {
  using difference_type = ptrdiff_t;
  using value_type = T;
  using pointer = T *;
  using reference = T &;
  using iterator_category = Tag;
  pointer_iterator_base(pointer p, size_t size) : first(p), curr(p), size(size)
  {
  }
  pointer_iterator_base(pointer p, size_t size, pointer first) : first(first), curr(p), size(size)
  {
  }
  pointer_iterator_base(const pointer_iterator_base &pib)
      : first(pib.first), curr(pib.curr), size(pib.size)
  {
  }
  // Conversion to base pointer
  operator pointer() const
  {
    return curr;
  }
  pointer_iterator_base &operator=(const pointer_iterator_base &p)
  {
    // Placement new: construct a new object in the position of `this`
    // Doesn't actually allocate memory
    new (this) pointer_iterator_base(p);
    return *this;
  }
  pointer_iterator_base begin() const
  {
    return {this->first, this->size, this->first};
  }
  pointer_iterator_base end() const
  {
    return {this->first + this->size, this->size, this->first};
  }
  pointer_iterator_base &operator++()
  {
    ++curr;
    return *this;
  }
  pointer_iterator_base &operator--()
  {
    --curr;
    return *this;
  }
  pointer_iterator_base &operator+(difference_type n)
  {
    curr += n;
    return *this;
  }
  difference_type operator-(const pointer_iterator_base &other) const
  {
    return other.curr - curr;
  }
  bool operator==(const pointer_iterator_base &other) const
  {
    return curr == other.curr;
  }
  pointer first;
  pointer curr;
  size_t size;
};

template<typename T>
struct pointer_iterator : pointer_iterator_base<T, std::random_access_iterator_tag> {
  using pointer_iterator_base<T, std::random_access_iterator_tag>::pointer_iterator_base;
  pointer_iterator(const pointer_iterator_base<T, std::random_access_iterator_tag> &pi)
      : pointer_iterator_base<T, std::random_access_iterator_tag>(pi)
  {
  }
  inline const T &operator*() const
  {
    return *this->curr;
  }
};

// An iterator that iterates over doubly linked lists
template<typename SourceT,
         typename ResT = SourceT &,
         typename Tag = std::bidirectional_iterator_tag>
struct list_iterator : pointer_iterator_base<SourceT, Tag> {
  list_iterator(SourceT *p) : pointer_iterator_base<SourceT, Tag>(p, 0)
  {
  }
  list_iterator begin() const
  {
    return {this->first};
  }
  list_iterator end() const
  {
    return {nullptr};
  }
  list_iterator &operator++()
  {
    this->curr = this->curr->next;
    return *this;
  }
  list_iterator &operator--()
  {
    this->curr = this->curr->prev;
    return *this;
  }
  inline const ResT operator*() const
  {
    return *this->curr;
  }
};

// Represents an offset into an array (base for iterators like material_iter)
template<typename T>
struct offset_iterator : pointer_iterator_base<T, std::random_access_iterator_tag> {
  offset_iterator(size_t size) : pointer_iterator_base<T, std::random_access_iterator_tag>(0, size)
  {
  }
  size_t offset() const
  {
    return ((size_t)this->curr) / sizeof(T);
  }
};

// Iterator over the polygons of a mesh
struct poly_iter : pointer_iterator<MPoly> {
  poly_iter(const Mesh *const m) : pointer_iterator(m->mpoly, m->totpoly)
  {
  }
  poly_iter(const pointer_iterator_base<MPoly> &pi) : pointer_iterator(pi)
  {
  }
  poly_iter begin() const
  {
    return {{this->first, this->size, this->first}};
  }
  poly_iter end() const
  {
    return {{this->first + this->size, this->size, this->first}};
  }
};

// Iterator over the vertices of a mesh
struct vert_iter : pointer_iterator<MVert> {
  vert_iter(const Mesh *const m) : pointer_iterator(m->mvert, m->totvert)
  {
  }
};

struct transformed_vertex_iter : pointer_iterator_base<MVert> {
  using Mat = const float (*)[4];  // Must actually be float[4][4]
  transformed_vertex_iter(const Mesh *const m, Mat &mat)
      : pointer_iterator_base(m->mvert, m->totvert), mat(mat)
  {
  }
  transformed_vertex_iter(const pointer_iterator_base<MVert> &pi, Mat &mat)
      : pointer_iterator_base(pi), mat(mat)
  {
  }
  transformed_vertex_iter begin() const
  {
    return {{this->first, this->size, this->first}, mat};
  }
  transformed_vertex_iter end() const
  {
    return {{this->first + this->size, this->size, this->first}, mat};
  }
  const std::array<float, 3> operator*() const
  {
    float co[3];
    mul_v3_m4v3(co, mat, this->curr->co);
    return {co[0], co[1], co[2]};
  }
  Mat &mat;
};

// Iterator over the vertices of a polygon
struct vert_of_poly_iter : pointer_iterator_base<MLoop, std::random_access_iterator_tag> {
  // TODO someone What order are the vertices stored in? Clockwise?
  vert_of_poly_iter(const Mesh *const mesh, const MPoly &mp)
      : pointer_iterator_base(mesh->mloop + mp.loopstart, mp.totloop), mvert(mesh->mvert)
  {
  }
  vert_of_poly_iter(const MVert *const mvert,
                    const pointer_iterator_base<MLoop, std::random_access_iterator_tag> &pi)
      : pointer_iterator_base(pi), mvert(mvert)
  {
  }
  vert_of_poly_iter begin() const
  {
    return {mvert, {this->first, this->size, this->first}};
  }
  vert_of_poly_iter end() const
  {
    return {mvert, {this->first + this->size, this->size, this->first}};
  }
  const MVert &operator*() const
  {
    return mvert[this->curr->v];
  }
  const MVert *const mvert;
};

// Iterator over all the edges of a mesh
struct edge_iter : pointer_iterator<MEdge> {
  edge_iter(const Mesh *const m) : pointer_iterator(m->medge, m->totedge)
  {
  }
  edge_iter(const pointer_iterator<MEdge> &pi) : pointer_iterator(pi)
  {
  }
};

// Iterator over the edges of a mesh which are marked as loose
struct loose_edge_iter : edge_iter {
  loose_edge_iter(const Mesh *const m) : edge_iter(m)
  {
  }
  loose_edge_iter(const pointer_iterator<MEdge> &pi) : edge_iter(pi)
  {
  }
  loose_edge_iter begin() const
  {
    return {{this->first, this->size, this->first}};
  }
  loose_edge_iter end() const
  {
    return {{this->first + this->size, this->size, this->first}};
  }
  loose_edge_iter &operator++()
  {
    do {
      ++this->curr;
    } while (!(this->curr->flag & ME_LOOSEEDGE));
    return *this;
  }
  loose_edge_iter &operator--()
  {
    do {
      --this->curr;
    } while (!(this->curr->flag & ME_LOOSEEDGE));
    return *this;
  }
};

// Iterator over all the objects in a `ViewLayer`
// TODO someone G.is_break
struct object_iter : list_iterator<Base, Object *> {
  object_iter(Base *const b) : list_iterator(b)
  {
  }
  object_iter(const ViewLayer *const vl) : list_iterator((Base *)vl->object_bases.first)
  {
  }
  const Object *operator*()
  {
    return this->curr->object;
  }
};

struct exportable_object_iter : object_iter {
  exportable_object_iter(const ViewLayer *const vl, const ExportSettings *const settings)
      : object_iter(vl), settings(settings)
  {
  }
  exportable_object_iter(Base *base, const ExportSettings *const settings)
      : object_iter(base), settings(settings)
  {
  }
  exportable_object_iter begin() const
  {
    return {this->first, settings};
  }
  exportable_object_iter end() const
  {
    return {(Base *)nullptr, settings};
  }
  exportable_object_iter &operator++()
  {
    do {
      this->curr = this->curr->next;
    } while (this->curr != nullptr && !common::should_export_object(settings, this->curr->object));
    return *this;
  }
  const ExportSettings *const settings;
};

// Iterator over the modifiers of an `Object`
struct modifier_iter : list_iterator<ModifierData> {
  modifier_iter(const Object *const ob) : list_iterator((ModifierData *)ob->modifiers.first)
  {
  }
};

// Iterator over the `MLoop` of a `MPoly` of a mesh
struct loop_of_poly_iter : pointer_iterator<MLoop> {
  loop_of_poly_iter(const Mesh *const mesh, const poly_iter &poly)
      : pointer_iterator(mesh->mloop + (*poly).loopstart, (*poly).totloop)
  {
  }
  loop_of_poly_iter(const Mesh *const mesh, const MPoly &poly)
      : pointer_iterator(mesh->mloop + poly.loopstart, poly.totloop)
  {
  }
  loop_of_poly_iter(const pointer_iterator_base<MLoop> &pi) : pointer_iterator(pi)
  {
  }
  loop_of_poly_iter begin() const
  {
    return loop_of_poly_iter{{this->first, this->size, this->first}};
  }
  loop_of_poly_iter end() const
  {
    return loop_of_poly_iter{{this->first + this->size, this->size, this->first}};
  }
};

struct material_iter : offset_iterator<Material *> {
  material_iter(const Object *const ob)
      : offset_iterator(ob->totcol), ob(ob), mdata(*give_matarar((Object *)ob))
  {
  }
  material_iter begin() const
  {
    return material_iter(ob);
  }
  material_iter end() const
  {
    material_iter mi(ob);
    mi.curr = mi.first + mi.size;
    return mi;
  }
  const Material *operator*()
  {
    const size_t off = offset();
    if (ob->matbits && ob->matbits[off]) {
      // In Object
      return ob->mat[off];
    }
    else {
      // In Data
      return mdata[off];
    }
  }
  const Object *const ob;
  const Material *const *const mdata;
};

struct nurbs_of_curve_iter : list_iterator<Nurb> {
  nurbs_of_curve_iter(const Curve *const curve) : list_iterator((Nurb *)curve->nurb.first)
  {
  }
};

struct points_of_nurbs_iter : pointer_iterator_base<BPoint> {
  points_of_nurbs_iter(const Nurb *const nu)
      : pointer_iterator_base(nu->bp, (nu->pntsv > 0 ? nu->pntsu * nu->pntsv : nu->pntsu))
  {
  }
  points_of_nurbs_iter(const pointer_iterator_base<BPoint> &pi) : pointer_iterator_base(pi)
  {
  }
  points_of_nurbs_iter begin() const
  {
    return {{this->first, this->size, this->first}};
  }
  points_of_nurbs_iter end() const
  {
    return {{this->first + this->size, this->size, this->first}};
  }
  inline const std::array<float, 3> operator*() const
  {
    return {curr->vec[0], curr->vec[1], curr->vec[2]};
  }
};

// Iterator over the UVs of a mesh (as `const std::array<float, 2>`)
struct uv_iter : pointer_iterator_base<MLoopUV> {
  uv_iter(const Mesh *const m) : pointer_iterator_base(m->mloopuv, m->mloopuv ? m->totloop : 0)
  {
  }
  uv_iter(const pointer_iterator_base<MLoopUV> &pi) : pointer_iterator_base(pi)
  {
  }
  uv_iter begin() const
  {
    return {{this->first, this->size, this->first}};
  }
  uv_iter end() const
  {
    return {{this->first + this->size, this->size, this->first}};
  }
  inline const std::array<float, 2> operator*()
  {
    return {this->curr->uv[0], this->curr->uv[1]};
  }
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
  using difference_type = ptrdiff_t;
  using value_type = ResT;
  using pointer = ResT *;
  using reference = ResT &;
  using iterator_category = std::forward_iterator_tag;
  normal_iter(const Mesh *const mesh) : mesh(mesh), mp(mesh->mpoly), ml(mesh->mloop)
  {
    custom_no = static_cast<float(*)[3]>(CustomData_get_layer(&mesh->ldata, CD_NORMAL));
  }
  normal_iter(const Mesh *const mesh, const MPoly *poly, const MLoop *loop)
      : mesh(mesh), mp(poly), ml(loop)
  {
  }
  normal_iter begin() const
  {
    return {mesh};
  }
  normal_iter end() const
  {
    const MPoly *const last_poly = mesh->mpoly + (mesh->totpoly);
    return {mesh, last_poly, mesh->mloop + last_poly->loopstart + (last_poly->totloop - 1)};
  }
  normal_iter &operator++()
  {
    // If not past the last element of the current loop, go to the next one
    if (ml < (mesh->mloop + mp->loopstart + mp->totloop)) {
      ++ml;
    }
    // If past the last (eg after the previous increment)
    if (ml >= (mesh->mloop + mp->loopstart + mp->totloop)) {
      // If not past the last poly
      if (mp < (mesh->mpoly + mesh->totpoly)) {
        // Increment the poly
        ++mp;
        // Get the loop of the current poly
        ml = mesh->mloop + mp->loopstart;
        // If now past the last poly
        if (mp >= (mesh->mpoly + mesh->totpoly)) {
          // Make sure the loop is at it's end, so we compare true with `end()`
          ml += mp->totloop - 1;
        }
      }
    }
    return *this;
  }
  bool operator==(const normal_iter &other) const
  {
    return mp == other.mp && ml == other.ml;
  }
  bool operator!=(const normal_iter &other) const
  {
    return !(*this == other);
  }
  ResT operator*() const
  {
    // TODO someone Should -0 be converted to 0?
    if (custom_no) {
      const float(&no)[3] = custom_no[ml->v];
      return {no[0], no[1], no[2]};
    }
    else {
      float no[3];
      /* Flat shaded, use common normal for all verts. */
      if ((mp->flag & ME_SMOOTH) == 0) {
        BKE_mesh_calc_poly_normal(mp, mesh->mloop + mp->loopstart, mesh->mvert, no);
        return {no[0], no[1], no[2]};
      }
      else {
        /* Smooth shaded, use individual vert normals. */
        normal_short_to_float_v3(no, mesh->mvert[ml->v].no);
        return {no[0], no[1], no[2]};
      }
    }
  }
  const Mesh *const mesh;
  const MPoly *mp;
  const MLoop *ml;
  const float (*custom_no)[3];
};

struct transformed_normal_iter {
  using difference_type = ptrdiff_t;
  using value_type = std::array<float, 3>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::forward_iterator_tag;
  using Mat = const float (*)[4];  // Must actually be float[4][4]
  transformed_normal_iter() = delete;
  transformed_normal_iter(const Mesh *const mesh, Mat mat) : ni(mesh), mat(mat)
  {
  }
  transformed_normal_iter(const normal_iter &ni, Mat mat) : ni(ni), mat(mat)
  {
  }
  transformed_normal_iter &operator++()
  {
    ++ni;
    return *this;
  }
  transformed_normal_iter begin() const
  {
    return {ni.begin(), mat};
  }
  transformed_normal_iter end() const
  {
    return {ni.end(), mat};
  }
  bool operator==(const transformed_normal_iter &other) const
  {
    return ni == other.ni;
  }
  bool operator!=(const transformed_normal_iter &other) const
  {
    return ni != other.ni;
  }
  std::array<float, 3> operator*() const
  {
    std::array<float, 3> no = *ni;
    mul_v3_m4v3(no.data(), mat, no.data());
    return no;
  }
  normal_iter ni;
  Mat mat;
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
  deduplicated_iterator(const Mesh *const mesh,
                        dedup_pair_t<KeyT> &dp,
                        ulong &total,
                        SourceIter it)
      : it(it), mesh(mesh), dedup_pair(dp), total(total)
  {
  }
  deduplicated_iterator(
      const Mesh *const mesh, dedup_pair_t<KeyT> &dp, ulong &total, ulong reserve, SourceIter it)
      : deduplicated_iterator(mesh, dp, total, it)
  {
    // Reserve space so we don't constantly allocate
    // if (dedup_pair.second.size() + reserve > dedup_pair.second.capacity()) {
    dedup_pair.second.reserve(reserve);
    // }
    // Need to insert the first element, because we need to dereference before incrementing
    if (this->it != this->it.end()) {
      auto p = dedup_pair.first.insert(std::make_pair(*this->it, total++));
      dedup_pair.second.push_back(p.first);
      ++this->it;
    }
  }
  deduplicated_iterator begin() const
  {
    return deduplicated_iterator(mesh, dedup_pair, total, it.begin());
  }
  deduplicated_iterator end() const
  {
    return deduplicated_iterator(mesh, dedup_pair, total, it.end());
  }
  deduplicated_iterator &operator++()
  {
    // Handle everything until the next different element, or the end, by...
    while (this->it != this->it.end()) {
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
  bool operator==(const deduplicated_iterator &other)
  {
    return it == other.it;
  }
  bool operator!=(const deduplicated_iterator &other)
  {
    return !(it == other.it);
  }
  ResT operator*() const
  {
    return this->dedup_pair.second.back()->first;
  }
  SourceIter it;
  const Mesh *const mesh;
  dedup_pair_t<KeyT> &dedup_pair;
  ulong &total;
};

// Iterator to deduplicated normals (returns `const std::array<float, 3>`)
struct deduplicated_normal_iter : deduplicated_iterator<no_key_t, transformed_normal_iter> {
  deduplicated_normal_iter(const Mesh *const mesh,
                           ulong &total,
                           dedup_pair_t<no_key_t> &dp,
                           const float (*mat)[4])
      : deduplicated_iterator<no_key_t, transformed_normal_iter>(
            mesh, dp, total, total + mesh->totvert, transformed_normal_iter(mesh, mat))
  {
  }
  // The last element in the mapping vector. Requires we insert the first element
  // in the constructor, otherwise this vector would be empty
  const std::array<float, 3> operator*() const
  {
    return this->dedup_pair.second.back()->first;
  }
};

// Iterator to deduplicated UVs (returns `const std::array<float, 2>`)
struct deduplicated_uv_iter : deduplicated_iterator<uv_key_t, uv_iter> {
  deduplicated_uv_iter(const Mesh *const mesh, ulong &total, dedup_pair_t<uv_key_t> &dp)
      : deduplicated_iterator<uv_key_t, uv_iter>(
            mesh, dp, total, total + mesh->totloop, uv_iter{mesh})
  {
  }
  // The last element in the mapping vector. Requires we insert the first element
  // in the constructor, otherwise this vector would be empty
  const std::array<float, 2> operator*() const
  {
    return this->dedup_pair.second.back()->first;
  }
};

}  // namespace common

#endif  // __io_intern_iterators_h__
