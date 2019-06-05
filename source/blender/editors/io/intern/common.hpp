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

	bool should_export_object(const ExportSettings * const settings, const Object * const eob);

	bool object_type_is_exportable(const Object * const ob);

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
	                 typeof(export_start) start, typeof(export_end) end);

	const std::array<float, 3> calculate_normal(const Mesh * const mesh,
	                                            const MPoly &mp);

	// --- TEMPLATES ---

	// Adapt a pointer-size pair as a random access iterator
	template<typename SourceT, typename Tag = std::random_access_iterator_tag>
	struct pointer_iterator :
		public boost::iterator_facade<pointer_iterator<SourceT, Tag>, SourceT &, Tag> {
		pointer_iterator() = default;
		pointer_iterator(const pointer_iterator<SourceT, Tag> &) = default;
		pointer_iterator(pointer_iterator<SourceT, Tag> &&) = default;
		explicit pointer_iterator(SourceT *p) : it(p), first(p), size(0) {}
		explicit pointer_iterator(SourceT *p, size_t size) : it(p), first(p), size(size) {}
		operator SourceT *() const { return it; }
		pointer_iterator operator=(const pointer_iterator<SourceT, Tag> &p) {
			return pointer_iterator<SourceT, Tag>(p);
		}
		pointer_iterator operator=(pointer_iterator<SourceT, Tag> &&p) {
			return pointer_iterator<SourceT, Tag>(p);
		}
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

	// TODO soemeone Document CTRP
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
		dereference_iterator       begin()  const { return {this->base().begin(), crtp}; }
		const dereference_iterator cbegin() const { return {this->base().cbegin(), crtp}; }
		dereference_iterator       end()    const { return {this->base().end(), crtp}; }
		const dereference_iterator cend()   const { return {this->base().cend(), crtp}; }
		friend class boost::iterator_core_access;
		ResT dereference() const { return crtp->dereference(this->base()); }
		CRTP *crtp;
	};

	template<typename SourceT, typename ResT = SourceT &,
	         typename Tag = std::bidirectional_iterator_tag>
	struct list_iterator : public boost::iterator_adaptor<list_iterator<SourceT, ResT, Tag>,
	                                                      pointer_iterator<SourceT, Tag>, ResT, Tag, ResT> {
		list_iterator() : list_iterator::iterator_adaptor_() {}
		list_iterator(const pointer_iterator<SourceT, Tag> & other)
			: list_iterator::iterator_adaptor_(other), first(other.first) {}
		explicit list_iterator(SourceT *first)
			: list_iterator::iterator_adaptor_(pointer_iterator<SourceT, Tag>{first}),
			  first(first) {}
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

	struct poly_iter : pointer_iterator<MPoly> {
		poly_iter(const Mesh * const m) : pointer_iterator(m->mpoly, m->totpoly) {}
		poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
		poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
	};

	struct vert_iter : pointer_iterator<MVert> {
		vert_iter(const Mesh * const m) : pointer_iterator(m->mvert, m->totvert) {}
	};

	struct vert_of_poly_iter : public dereference_iterator<MLoop, MVert &, vert_of_poly_iter> {
		// TODO someone What order are the vertices stored in? Clockwise?
		explicit vert_of_poly_iter(const Mesh * const mesh, const MPoly &mp)
			: dereference_iterator(mesh->mloop + mp.loopstart, mp.totloop, this), mesh(mesh) {}
		inline MVert & dereference(const pointer_iterator<MLoop> &b) { return mesh->mvert[b->v]; }
		const Mesh * const mesh;
	};

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

	struct edge_iter : pointer_iterator<MEdge> {
		edge_iter(const Mesh * const m) : pointer_iterator(m->medge, m->totedge) {}
		edge_iter(const pointer_iterator<MEdge> &pi) : pointer_iterator(pi) {}
		edge_iter(pointer_iterator<MEdge> &&pi) : pointer_iterator(pi) {}
	};

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

	// TODO someone G.is_break
	struct object_iter : dereference_iterator<Base, Object *, object_iter, list_iterator<Base>> {
		explicit object_iter(const ViewLayer * const vl)
			: dereference_iterator((Base *) vl->object_bases.first, this) {}
		inline static Object * dereference(const list_iterator<Base> &b) { return b->object; }
	};

	struct modifier_iter : list_iterator<ModifierData> {
		explicit modifier_iter(const Object * const ob)
			: list_iterator((ModifierData *) ob->modifiers.first) {}
	};

	struct loop_of_poly_iter : pointer_iterator<MLoop> {
		explicit loop_of_poly_iter(const Mesh * const mesh, const poly_iter &poly)
			: pointer_iterator(mesh->mloop + poly->loopstart, poly->totloop) {}
		loop_of_poly_iter(const pointer_iterator &p) : pointer_iterator(p) {}
		loop_of_poly_iter(pointer_iterator &&p) : pointer_iterator(std::move(p)) {}
	};

	struct normal_iter :
		public boost::iterator_facade<normal_iter, std::array<float, 3>,
		                              std::bidirectional_iterator_tag, const std::array<float, 3>> {
		using ResT = const std::array<float, 3>;
		normal_iter() = default;
		normal_iter(const normal_iter &) = default;
		normal_iter(normal_iter &&) = default;
		explicit normal_iter(const Mesh * const mesh, const poly_iter poly, const loop_of_poly_iter loop)
			: mesh(mesh), poly(poly), loop(loop) {
			loop_no = static_cast<float(*)[3]>(CustomData_get_layer(&mesh->ldata, CD_NORMAL));
		}
		explicit normal_iter(const Mesh * const mesh)
			: normal_iter(mesh, poly_iter(mesh), loop_of_poly_iter(mesh, poly_iter(mesh))) {}
		normal_iter begin() const { return normal_iter{mesh}; }
		normal_iter end()   const { return normal_iter(mesh, poly.end(), loop.end()); }
		friend class boost::iterator_core_access;
		void increment() {
			// If not in the last loop and face is smooth shaded, each vertex has it's
			// own normal, so increment loop, otherwise go to the next poly
			if ((loop != loop.end()) && ((poly->flag & ME_SMOOTH) == 0)) {
				++loop;
			} else if (poly != poly.end()) {
				++poly;
				loop = loop_of_poly_iter(mesh, poly);
			}
		}
		void decrement() {
			if ((loop != loop.begin()) && ((poly->flag & ME_SMOOTH) == 0)) {
				--loop;
			} else if (poly != poly.begin()) {
				--poly;
				loop = loop_of_poly_iter(mesh, poly);
			}
		}
		bool equal(const normal_iter &other) const { return poly == other.poly && loop == other.loop; }
		ResT dereference() const {
			if (loop_no) {
				const float (&no)[3] = loop_no[loop->v];
				return {no[0], no[1], no[2]};
			} else {
				float no[3];
				if ((poly->flag & ME_SMOOTH) == 0)
					BKE_mesh_calc_poly_normal(&(*poly), &(*loop), mesh->mvert, no);
				else
					normal_short_to_float_v3(no, mesh->mvert[loop->v].no);
				return {no[0], no[1], no[2]};
			}
		}
		const Mesh * const mesh;
		poly_iter poly;
		loop_of_poly_iter loop;
		const float (*loop_no)[3];
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

	struct threshold_comparator {
		threshold_comparator(const float th) : th(th) {}
		template<typename key_t>
		bool operator()(const key_t &lhs, const key_t &rhs) const {
			return // too_similar(lhs.first, rhs.first, th) &&
				lhs.first < rhs.first;
		}
		const float th;
	};

	template<typename key_t>
	using set_t = std::set<key_t, threshold_comparator>;

	template<typename key_t>
	using set_mapping_t = std::vector<typename set_t<key_t>::iterator>;

	template<typename key_t>
	using dedup_pair_t = std::pair<set_t<key_t>, set_mapping_t<key_t> >;

	using uv_key_t = std::pair<std::array<float, 2>, ulong>; // ulong is the original index
	using no_key_t = std::pair<std::array<float, 3>, ulong>;

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
			// Handle everything until the next different element
			while(this->base() != this->base().end()) {
				auto p = dedup_pair.first.insert(std::make_pair(*this->base(), total));
				++this->base_reference();
				dedup_pair.second.push_back(p.first);
				if (p.second) {
					++total;
					return;
				}
			}
		}
		const ResT dereference() const { return dedup_pair.second.back()->first; }
		const Mesh * const mesh;
		dedup_pair_t<KeyT> &dedup_pair;
		ulong &total;
	};

	struct deduplicated_normal_iter : deduplicated_iterator<no_key_t, normal_iter> {
		deduplicated_normal_iter(const Mesh * const mesh, ulong &total, dedup_pair_t<no_key_t> &dp)
			: deduplicated_iterator<no_key_t, normal_iter>
			(mesh, dp, total, total + mesh->totvert) {}
	};

	struct deduplicated_uv_iter : deduplicated_iterator<uv_key_t, uv_iter> {
		deduplicated_uv_iter(const Mesh * const mesh, ulong &total, dedup_pair_t<uv_key_t> &dp)
			: deduplicated_iterator<uv_key_t, uv_iter>(mesh, dp, total, total + mesh->totloop) {}
	};

	// C++14/17 would be useful here...
	template<typename key_t>
	dedup_pair_t<key_t> make_deduplicate_set(const float threshold) {
		return {set_t<key_t>{threshold_comparator(threshold)} /* set */, {} /* set_mapping */};
	}

}

#endif // __io_intern_common_
