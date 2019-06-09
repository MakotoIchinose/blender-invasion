#ifndef __io_intern_common__
#define __io_intern_common__

/* #include "BLI_listbase.h" */

/* #include "DNA_mesh_types.h" */
/* #include "DNA_object_types.h" */

extern "C" {

#include "BKE_global.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_modifier.h"
#include "BKE_library.h"
#include "BKE_customdata.h"
#include "BKE_scene.h"

#include "MEM_guardedalloc.h"

/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#define new extern_new
#include "BKE_screen.h"
#undef new

#include "BLI_listbase.h"
#include "BLI_math_matrix.h"
#include "BLI_math_vector.h"
#include "BLI_utildefines.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "DNA_layer_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"

#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "../io_common.h"

}

#include <utility>
#include <string>
#include <vector>
#include <set>
#include <array>
#include <typeinfo>
#include <iterator>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

namespace common {
	using ulong = unsigned long;

	// --- PROTOTYPES ---

	bool object_is_smoke_sim(const Object * const ob);

	bool object_type_is_exportable(const Object * const ob);

	bool should_export_object(const ExportSettings * const settings, const Object * const ob);

	void change_orientation(float (&mat)[4][4], int forward, int up);

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *ob, Mesh **mesh /* out */);

	void free_mesh(Mesh *mesh, bool needs_free);

	std::string get_object_name(const Object * const ob, const Mesh * const mesh);
	std::string get_version_string();

	void export_start(bContext *C, ExportSettings * const settings);
	bool export_end(bContext *C, ExportSettings * const settings);

	// Execute `start` and `end` and time it. Those functions should be
	// specific to each exportter, but have the same signature as the two above
	bool time_export(bContext *C, ExportSettings * const settings,
	                 void (*start)(bContext *C, ExportSettings * const settings),
	                 void (*end)(bContext *C, ExportSettings * const settings));

	const std::array<float, 3> calculate_normal(const Mesh * const mesh,
	                                            const MPoly &mp);

	// --- TEMPLATES ---

	// Adapt a pointer-size pair as a random access iterator
	// This makes use of `boost::iterator_facade` and makes it possible to use
	// for each style loops, as well as cleanly hiding how the underlying Blender
	// data structures are accessed
	template<typename SourceT, typename Tag = std::random_access_iterator_tag>
	struct pointer_iterator :
		public boost::iterator_facade<pointer_iterator<SourceT, Tag>, SourceT &, Tag> {
		pointer_iterator() : first(nullptr) {}
		pointer_iterator(const pointer_iterator<SourceT, Tag> &) = default;
		pointer_iterator(pointer_iterator<SourceT, Tag> &&) = default;
		explicit pointer_iterator(SourceT *p) : it(p), first(p), size(0) {}
		explicit pointer_iterator(SourceT *p, size_t size) : it(p), first(p), size(size) {}
		operator SourceT *() const { return it; }
		pointer_iterator & operator=(const pointer_iterator<SourceT, Tag> &p) {
			// Placement new: construct a new object in the position of `this`
			// Doesn't actually allocate memory
			new (this) pointer_iterator(p);
			return *this;
		}
		// pointer_iterator & operator=(pointer_iterator<SourceT, Tag> &&p) = default;//  {
		// 	return pointer_iterator<SourceT, Tag>(p);
		// }
		pointer_iterator       begin()  const { return pointer_iterator{first, size}; }
		const pointer_iterator cbegin() const { return pointer_iterator{first, size}; }
		pointer_iterator       end()    const { return pointer_iterator{first + size, size}; }
		const pointer_iterator cend()   const { return pointer_iterator{first + size, size}; }
		friend class boost::iterator_core_access;
		void increment() { ++it; }
		void decrement() { --it; }
		void advance(ptrdiff_t n) { it += n; }
		ptrdiff_t distance_to(const pointer_iterator &other) { return other.it - it; }
		bool equal(const pointer_iterator &other) const { return it == other.it; }
		SourceT & dereference() const { return *this->it; }
		SourceT *it;
		SourceT * const first;
		size_t size;
	};

	// This is another iterator, whcih makes use of `boost::iterator_adaptor` to change  how the underlying
	// iterator behaves. Specifically, it uses the derived class' `dereference` method.
	// This makes use of CRTP, the curiously recurring template pattern
	// This means that the base class has, as a template paramater, the type of the derived class
	// which means that it can hold a pointer to said derived class and call it's `dereference` method.
	template<typename SourceT, typename ResT, typename CRTP,
	         typename Base = pointer_iterator<SourceT>,
	         typename Tag = typename std::iterator_traits<Base>::iterator_category>
	struct dereference_iterator
		: public boost::iterator_adaptor<dereference_iterator<SourceT, ResT, CRTP, Base>,
		                                 Base, ResT, Tag, ResT> {
		using dereference_iterator_ = dereference_iterator<SourceT, ResT, CRTP, Base>;
		dereference_iterator() : dereference_iterator::iterator_adaptor_(), crtp(nullptr) {}
		dereference_iterator(const dereference_iterator &di, CRTP *crtp)
			: dereference_iterator::iterator_adaptor_(di.base()), crtp(crtp) {}
		dereference_iterator(dereference_iterator &&di, CRTP *crtp)
			: dereference_iterator::iterator_adaptor_(di.base()), crtp(crtp) {}
		dereference_iterator(Base const & other, CRTP *crtp)
			: dereference_iterator::iterator_adaptor_(other), crtp(crtp) {}
		template<typename Size = size_t>
		explicit dereference_iterator(SourceT *p, Size size, CRTP *crtp)
			: dereference_iterator(Base{p, (size_t) size}, crtp) {}
		explicit dereference_iterator(SourceT *p, CRTP *crtp) // For list_iterator
			: dereference_iterator(Base{p}, crtp) {}
		dereference_iterator       begin()  const { return {this->base().begin(),  crtp}; }
		const dereference_iterator cbegin() const { return {this->base().cbegin(), crtp}; }
		dereference_iterator       end()    const { return {this->base().end(),    crtp}; }
		const dereference_iterator cend()   const { return {this->base().cend(),   crtp}; }
		friend class boost::iterator_core_access;
		ResT dereference() const { return crtp->dereference(this->base()); }
		CRTP *crtp;
	};

	// Another iterator that iterates over doubly linked lists
	template<typename SourceT, typename ResT = SourceT &,
	         typename Tag = std::bidirectional_iterator_tag>
	struct list_iterator : public boost::iterator_adaptor<list_iterator<SourceT, ResT, Tag>,
	                                                      pointer_iterator<SourceT, Tag>, ResT, Tag, ResT> {
		list_iterator() : list_iterator::iterator_adaptor_(), first(nullptr) {}
		list_iterator(const pointer_iterator<SourceT, Tag> &other)
			: list_iterator::iterator_adaptor_(other), first(other.first) {}
		explicit list_iterator(SourceT *first)
			: list_iterator::iterator_adaptor_(pointer_iterator<SourceT, Tag>{first}), first(first) {}
		list_iterator       begin()  const { return list_iterator{first}; }
		list_iterator       end()    const { return list_iterator{nullptr}; }
		const list_iterator cbegin() const { return list_iterator{first}; }
		const list_iterator cend()   const { return list_iterator{nullptr}; }
		friend class boost::iterator_core_access;
		void increment() { this->base_reference().it = this->base_reference()->next; }
		void decrement() { this->base_reference().it = this->base_reference()->prev; }
		ResT dereference() const { return *this->base(); }
		SourceT * const first;
	};

	// Iterator over the polygons of a mesh
	struct poly_iter : pointer_iterator<MPoly> {
		poly_iter(const Mesh * const m) : pointer_iterator(m->mpoly, m->totpoly) {}
		poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
		poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
	};

	// Iterator over the vertices of a mesh
	struct vert_iter : pointer_iterator<MVert> {
		vert_iter(const Mesh * const m) : pointer_iterator(m->mvert, m->totvert) {}
	};

	// Iterator over the vertices of a polygon
	struct vert_of_poly_iter : public dereference_iterator<MLoop, MVert &, vert_of_poly_iter> {
		// TODO someone What order are the vertices stored in? Clockwise?
		explicit vert_of_poly_iter(const Mesh * const mesh, const MPoly &mp)
			: dereference_iterator(mesh->mloop + mp.loopstart, mp.totloop, this), mesh(mesh) {}
		inline MVert & dereference(const pointer_iterator<MLoop> &b) { return mesh->mvert[b->v]; }
		const Mesh * const mesh;
	};

	// Iterator over all the edges of a mesh
	struct edge_iter : pointer_iterator<MEdge> {
		edge_iter(const Mesh * const m) : pointer_iterator(m->medge, m->totedge) {}
		edge_iter(const pointer_iterator<MEdge> &pi) : pointer_iterator(pi) {}
		edge_iter(pointer_iterator<MEdge> &&pi) : pointer_iterator(pi) {}
	};

	// Iterator over the edges of a mesh whcih are marked as loose
	struct loose_edge_iter : public boost::iterator_adaptor<loose_edge_iter, edge_iter,
	                                                        MEdge, std::bidirectional_iterator_tag> {
		explicit loose_edge_iter(const Mesh * const m, const edge_iter &e)
			: loose_edge_iter::iterator_adaptor_(e), mesh(m) {}
		explicit loose_edge_iter(const Mesh * const m) : loose_edge_iter(m, edge_iter(m)) {}
		loose_edge_iter begin() const { return loose_edge_iter(mesh); }
		loose_edge_iter end()   const { return loose_edge_iter(mesh, this->base().end()); }
		void increment() {
			do {
				++this->base_reference();
			} while(!(this->base()->flag & ME_LOOSEEDGE));
		}
		void decrement() {
			do {
				--this->base_reference();
			} while(!(this->base()->flag & ME_LOOSEEDGE));
		}
		const Mesh * const mesh;
	};

	// Iterator over all the objects in a `ViewLayer`
	// TODO someone G.is_break
	struct object_iter : dereference_iterator<Base, Object *, object_iter, list_iterator<Base>> {
		object_iter(const ViewLayer * const vl)
			: dereference_iterator((Base *) vl->object_bases.first, this) {}
		object_iter(Base *b)
			: dereference_iterator(b, this) {}
		inline static Object * dereference(const list_iterator<Base> &b) { return b->object; }
	};

	struct exportable_object_iter
		: public boost::iterator_adaptor<exportable_object_iter, object_iter> {
		explicit exportable_object_iter(const ExportSettings * const settings)
			: exportable_object_iter::iterator_adaptor_(settings->view_layer),
			  settings(settings) {}
		exportable_object_iter(const ExportSettings * const settings, Base *base)
			: exportable_object_iter::iterator_adaptor_(base),
			  settings(settings) {}
		friend class boost::iterator_core_access;
		exportable_object_iter begin() const { return { settings, (Base *) settings->view_layer->
		                                                                   object_bases.first}; }
		exportable_object_iter end()   const { return { settings, (Base *) nullptr}; }
		void increment() {
			do {
				++this->base_reference();
			} while (this->base() != this->base().end() &&
			         !should_export_object(settings, *this->base()));
		}
		void decrement() {
			do {
				--this->base_reference();
			} while (this->base() != this->base().begin() &&
			         !should_export_object(settings, *this->base()));
		}
		const ExportSettings * const settings;
	};

	// Iterator over the modifiers of an `Object`
	struct modifier_iter : list_iterator<ModifierData> {
		explicit modifier_iter(const Object * const ob)
			: list_iterator((ModifierData *) ob->modifiers.first) {}
	};

	// Iterator over the `MLoop` of a `MPoly` of a mesh
	struct loop_of_poly_iter : pointer_iterator<MLoop> {
		explicit loop_of_poly_iter(const Mesh * const mesh, const poly_iter &poly)
			: pointer_iterator(mesh->mloop + poly->loopstart, poly->totloop - 1) {}
		explicit loop_of_poly_iter(const Mesh * const mesh, const MPoly &poly)
			: pointer_iterator(mesh->mloop + poly.loopstart, poly.totloop - 1) {}
		loop_of_poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
		loop_of_poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
	};

	// Iterator over the UVs of a mesh (as `const std::array<float, 2>`)
	struct uv_iter : public dereference_iterator<MLoopUV, const std::array<float, 2>, uv_iter> {
		explicit uv_iter(const Mesh * const m)
			: dereference_iterator(m->mloopuv, m->totloop, this) {}
		uv_iter(const dereference_iterator_ &di)
			: dereference_iterator(di, this) {}
		uv_iter(dereference_iterator_ &&di)
			: dereference_iterator(di, this) {}
		inline const std::array<float, 2> dereference(const pointer_iterator<MLoopUV> &b) {
			return {b->uv[0], b->uv[1]};
		}
	};

	// Iterator over the normals of mesh
	// This is a more complex one, because there are three possible sources for the normals of a mesh:
	//   - Custom normals
	//   - Face normals, when the face is not smooth shaded
	//   - Vertex normals, when the face is smooth shaded
	// This is a completely separate iterator because it needs to override pretty much all behaviours
	// It's only a bidirectional iterator, because it is not continuous
	struct normal_iter :
		public boost::iterator_facade<normal_iter, std::array<float, 3>,
		                              std::bidirectional_iterator_tag, const std::array<float, 3>> {
		using ResT = const std::array<float, 3>;
		normal_iter() = default;
		normal_iter(const normal_iter &) = default;
		normal_iter(normal_iter &&) = default;
		explicit normal_iter(const Mesh * const mesh, const poly_iter poly, const loop_of_poly_iter loop)
			: mesh(mesh), poly(poly), loop(loop) {
			custom_no = static_cast<float(*)[3]>(CustomData_get_layer(&mesh->ldata, CD_NORMAL));
		}
		explicit normal_iter(const Mesh * const mesh)
			: normal_iter(mesh, poly_iter(mesh), loop_of_poly_iter(mesh, poly_iter(mesh))) {}
		normal_iter begin() const { return *this; }
		normal_iter end()   const { return normal_iter(mesh, poly.end(), loop.end()); }
		friend class boost::iterator_core_access;
		void increment() {
			// If we're not at the end of the loop, we increment it
			if (loop != loop.end()) {
				++loop;
			} else if (poly != poly.end()) {
				// if we're not at the end of the poly, we increment it
				++poly;
				if (poly != poly.end())
					// if incrementing the poly didn't put us past the end,
					// use the new poly to generate the new loop iterator
					loop = loop_of_poly_iter{mesh, poly};
			}
		}
		void decrement() {
			if (loop != loop.begin()) {
				--loop;
			} else if (poly != poly.begin()) {
				--poly;
				loop = loop_of_poly_iter{mesh, poly};
			}
		}
		bool equal(const normal_iter &other) const {
			// Equal if the poly iterator is the same
			return poly == other.poly &&
				// And either the face is not smooth shaded, in which case we
				// don't care about the loop, or if the loop is the same
				((poly->flag & ME_SMOOTH) == 0 || loop == other.loop);
		}
		ResT dereference() const {
			// If we have custom normals, read from there
			if (custom_no) {
				const float (&no)[3] = custom_no[loop->v];
				return {no[0], no[1], no[2]};
			} else {
				float no[3];
				// If the face is not smooth shaded, calculate the normal of the face
				if ((poly->flag & ME_SMOOTH) == 0)
					// Note the `loop.first`. This is because the function expects
					// a pointer to the first element of the loop
					BKE_mesh_calc_poly_normal(poly, loop.first, mesh->mvert, no);
				else
					// Otherwise, the normal is stored alongside the vertex,
					// as a short, so we retrieve it
					normal_short_to_float_v3(no, mesh->mvert[loop->v].no);
				return {no[0], no[1], no[2]};
			}
		}
		const Mesh * const mesh;
		poly_iter poly;
		loop_of_poly_iter loop;
		const float (*custom_no)[3];
	};

	template<size_t N>
	inline float len_sq(const std::array<float, N> &lhs, const std::array<float, N> &rhs) {
		float f = 0.0;
		for(int i = 0; i < N; ++i)
			f += std::pow(lhs[i] - rhs[i], 2);
		return f;
	}

	template<size_t N>
	inline float too_similar(const std::array<float, N> &lhs, const std::array<float, N> &rhs,
	                         const float th) {
		bool b = true;
		for(int i = 0; i < N; ++i)
			b &= (lhs[i] - rhs[i]) < th;
		return b;
	}

	// TODO someone How to properly compare similarity of vectors?
	struct threshold_comparator {
		threshold_comparator(const float th) : th(th) {}
		template<typename key_t>
		bool operator()(const key_t &lhs, const key_t &rhs) const {
			return // too_similar(lhs.first, rhs.first, th) &&
				lhs.first < rhs.first;
		}
		const float th;
	};

	// --- Deduplication ---
	// The set used to deduplicate UVs and normals
	template<typename key_t>
	using set_t = std::set<key_t, threshold_comparator>;
	// Provides the mapping from the deduplicated to the original index
	template<typename key_t>
	using set_mapping_t = std::vector<typename set_t<key_t>::iterator>;
	// A pair of the two above, to tie them together
	template<typename key_t>
	using dedup_pair_t = std::pair<set_t<key_t>, set_mapping_t<key_t> >;
	// The set key for UV and normal. ulong is the original index
	using uv_key_t = std::pair<std::array<float, 2>, ulong>;
	using no_key_t = std::pair<std::array<float, 3>, ulong>;

	// Iterator which performs a deduplication of the values coming out of the SourceIter
	template<typename KeyT, typename SourceIter, typename ResT = const typename KeyT::first_type,
	         typename Tag = std::forward_iterator_tag>
	struct deduplicated_iterator
		: public boost::iterator_adaptor<deduplicated_iterator<KeyT, SourceIter, ResT, Tag>,
		                                 SourceIter, ResT, Tag, ResT> {
		deduplicated_iterator() = default;
		explicit deduplicated_iterator(const Mesh * const mesh, dedup_pair_t<KeyT> &dp,
		                               ulong &total, SourceIter it)
			: deduplicated_iterator::iterator_adaptor_(it), mesh(mesh),
			  dedup_pair(dp), total(total) {}
		explicit deduplicated_iterator(const Mesh * const mesh, dedup_pair_t<KeyT> &dp,
		                               ulong &total, ulong reserve)
			: deduplicated_iterator(mesh, dp, total, SourceIter{mesh}) {
			// Reserve space so we don't constantly allocate
			dedup_pair.second.reserve(reserve);
			// Need to insert the first element, because we need to dereference before incrementing
			auto p = dedup_pair.first.insert(std::make_pair(*this->base(), total++));
			dedup_pair.second.push_back(p.first);
			++this->base_reference();
		}
		deduplicated_iterator begin() const { return *this; }
		deduplicated_iterator end() const {
			return deduplicated_iterator(mesh, dedup_pair, total, this->base().end()); }
		friend class boost::iterator_core_access;
		void increment() {
			// Handle everything until the next different element, or the end, by...
			while(this->base() != this->base().end()) {
				// tring to insert it into the set
				auto p = dedup_pair.first.insert(std::make_pair(*this->base(), total));
				// going to the next element of the `SourceIter`
				++this->base_reference();
				// push the set::iterator onto the back of the mapping vector
				dedup_pair.second.push_back(p.first);
				// If we actually inserted in the set
				if (p.second) {
					// There's a new element, so stop
					++total;
					return;
				}
			}
		}
		// The last element in the mapping vector. Requires we insert the first element
		// in the constructor, otherwise this vector would be empty
		const ResT dereference() const { return dedup_pair.second.back()->first; }
		const Mesh * const mesh;
		dedup_pair_t<KeyT> &dedup_pair;
		ulong &total;
	};

	// Iterator to deduplicated normals (returns `const std::array<float, 3>`)
	struct deduplicated_normal_iter : deduplicated_iterator<no_key_t, normal_iter> {
		deduplicated_normal_iter(const Mesh * const mesh, ulong &total, dedup_pair_t<no_key_t> &dp)
			: deduplicated_iterator<no_key_t, normal_iter>
			(mesh, dp, total, total + mesh->totvert) {}
	};

	// Iterator to deduplicated UVs (returns `const std::array<float, 2>`)
	struct deduplicated_uv_iter : deduplicated_iterator<uv_key_t, uv_iter> {
		deduplicated_uv_iter(const Mesh * const mesh, ulong &total, dedup_pair_t<uv_key_t> &dp)
			: deduplicated_iterator<uv_key_t, uv_iter>(mesh, dp, total, total + mesh->totloop) {}
	};

	// Construct a new deduplicated set for either normals or UVs, with the given similarity threshold
	// C++14/17 would be useful here...
	template<typename key_t>
	dedup_pair_t<key_t> make_deduplicate_set(const float threshold) {
		return {set_t<key_t>{threshold_comparator(threshold)} /* set */, {} /* set_mapping */};
	}
}

#endif // __io_intern_common_
