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

namespace common {
	using ulong = unsigned long;

	bool object_is_smoke_sim(const Object * const ob);

	bool should_export_object(const ExportSettings * const settings, const Object * const eob);

	bool object_type_is_exportable(const Object * const ob);

	void change_orientation(float (&mat)[4][4], int forward, int up);

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *eob, Mesh **mesh);

	std::string get_object_name(const Object * const eob, const Mesh * const mesh);

	void export_start(bContext *C, const ExportSettings * const settings);
	bool export_end(bContext *C, ExportSettings * const settings);

	template<typename func>
	void for_each_modifier(const Object * const ob, func f) {
		for (ModifierData *md = (ModifierData *) ob->modifiers.first;
		     md; md = md->next)
			f(md);
	}

	template<typename func>
	void for_each_base(ViewLayer * const view_layer, func f) {
		for (Base *base = static_cast<Base *>(view_layer->object_bases.first);
		     base; base = base->next)
			if (!G.is_break)
				f(base);
		G.is_break = false;
	}

	template<typename func>
	void for_each_vertex(const Mesh * const mesh, func f) {
		// TODO someone Should this use iterators? Unsure if those are only for BMesh
		for (int i = 0, e = mesh->totvert; i < e; ++i)
			f(i, mesh->mvert[i]);
	}

	template<typename func>
	void for_each_edge(const Mesh * const mesh, func f) {
		for (int i = 0, e = mesh->totedge; i < e; ++i)
			f(i, mesh->medge[i]);
	}

	// Meeded for the templated call operator, for deduping
	struct for_each_uv_t {
		template<typename func>
		void operator()(const Mesh * const mesh, func f) {
			for (int i = 0, e = mesh->totloop; i < e; ++i) {
				const float (&uv)[2] = mesh->mloopuv[i].uv;
				f(i, std::array<float, 2>{uv[0], uv[1]});
			}
		}
	};

	template<typename func>
	void for_each_uv(const Mesh * const mesh, func f) {
		for_each_uv_t{}(mesh, f);
	}

	struct for_each_normal_t {
		template<typename func>
		void operator()(const Mesh * const mesh, func f) {
			const float (*loop_no)[3] = static_cast<float(*)[3]>
				(CustomData_get_layer(&mesh->ldata,
				                      CD_NORMAL));
			unsigned loop_index = 0;
			MVert *verts = mesh->mvert;
			MPoly *mp = mesh->mpoly;
			MLoop *mloop = mesh->mloop;
			MLoop *ml = mloop;

			// TODO someone Should -0 be converted to 0?
			if (loop_no) {
				for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
					ml = mesh->mloop + mp->loopstart + (mp->totloop - 1);
					for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
						const float (&no)[3] = loop_no[ml->v];
						f(i, std::array<float, 3>{no[0], no[1], no[2]});
					}
				}
			} else {
				float no[3];
				for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
					ml = mloop + mp->loopstart + (mp->totloop - 1);

					/* Flat shaded, use common normal for all verts. */
					if ((mp->flag & ME_SMOOTH) == 0) {
						BKE_mesh_calc_poly_normal(mp, ml - (mp->totloop - 1),
						                          verts, no);
						f(i, std::array<float, 3>{no[0], no[1], no[2]});
						ml -= mp->totloop;
						loop_index += mp->totloop;
					} else {
						/* Smooth shaded, use individual vert normals. */
						for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
							normal_short_to_float_v3(no, verts[ml->v].no);
							f(i, std::array<float, 3>{no[0], no[1], no[2]});
						}
					}
				}
			}
		}
	};

	template<typename func>
	void for_each_normal(const Mesh * const mesh, func f) {
		for_each_normal_t{}(mesh, f);
	}

	template<typename key_t>
	using set_t = std::set<key_t, bool (*)(const key_t &, const key_t &)>;

	template<typename key_t>
	using set_mapping_t = std::vector<typename set_t<key_t>::iterator>;

	template<typename key_t>
	using dedup_pair_t = std::pair<set_t<key_t>, set_mapping_t<key_t> >;

	using uv_key_t = std::pair<std::array<float, 2>, ulong>;
	using no_key_t = std::pair<std::array<float, 3>, ulong>;

	// TODO someone Benchmark the performance wth iterators and normal deduplication
	template<typename for_each_t, typename dedup_set_mapping_t, typename on_insert_t>
	void deduplicate(ulong &total, ulong reserve, const Mesh * const mesh,
	                 dedup_set_mapping_t &p,
	                 for_each_t for_each,
	                 on_insert_t on_insert) {
		auto &set = p.first;
		auto &set_mapping = p.second;

		// Reserve approximate space to reduce allocations inside loop
		set_mapping.reserve(reserve);

		// C++14/17 would help here...
		for_each(mesh, [&](ulong i, // key_t
		                   typename dedup_set_mapping_t::first_type::
		                   key_type::first_type v) {
			               auto p = set.insert(std::make_pair(v, total));
			               set_mapping.push_back(p.first);
			               if (p.second) {
				               on_insert(i, p.first);
				               ++total;
			               }
		               });
	}

	// Again, C++14/17 would be really useful here...rmal
	template<typename key_t>
	dedup_pair_t<key_t> make_deduplicate_set() {
		auto cmp = [](const key_t &lhs, const key_t &rhs) -> bool
		           { return lhs.first < rhs.first; };
		return {set_t<key_t>{cmp} /* set */, {} /* set_mapping */};
	}

	template<typename func>
	void for_each_deduplicated_uv(const Mesh *mesh, ulong &total,
	                              dedup_pair_t<uv_key_t>& p, func f) {
		deduplicate(/* modifies  */ total,
		            /* reserve   */ total + mesh->totloop,
		            mesh,
		            /* modifies  */ p,
		            /* for_each */  for_each_uv_t{},
		            /* on_insert */ f);
	}

	template<typename func>
	void for_each_deduplicated_normal(const Mesh *mesh, ulong &total,
	                                  dedup_pair_t<no_key_t>& p, func f) {
		deduplicate(/* modifies  */ total,
		            total + mesh->totvert,
		            mesh,
		            /* modifies  */ p,
		            /* for_each  */ for_each_normal_t{},
		            /* on_insert */
		            [&p, &f, &mesh](ulong i, typename set_t<no_key_t>::iterator it){
			            // Call callee function
			            f(i, it);

			            // If the face is flat shaded, f is only invoked once, but we need to
			            // add the value to the map an additional totloop - 1 times
			            MPoly *mp = mesh->mpoly + i;
			            if ((mp->flag & ME_SMOOTH) == 0)
				            for (int j = 0; j < (mp->totloop - 1); ++j)
					            p.second.push_back(it);
		            });
	}
}

#endif // __io_intern_common_
