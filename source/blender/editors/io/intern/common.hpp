#ifndef __io_intern_common__
#define __io_intern_common__

/* #include "BLI_listbase.h" */

/* #include "DNA_mesh_types.h" */
/* #include "DNA_object_types.h" */

extern "C" {

#include "DNA_layer_types.h"

#include "BLI_math_vector.h"

#include "../io_common.h"

}
#include <utility>
#include <string>
#include <vector>
#include <set>
#include <array>

namespace common {
	using ulong = unsigned long;

	bool should_export_object(const ExportSettings * const settings, const Object * const eob);


	bool object_type_is_exportable(const Object * const ob);

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *eob, Mesh **mesh);

	std::string&& get_object_name(const Object *eob, const Mesh *mesh);

	// template<typename func>
	// void for_each_vertex(const Mesh *mesh, void (*f)());

	// template<typename func>
	// void for_each_uv(Mesh *mesh, func f);

	// template<typename func>
	// void for_each_normal(Mesh *mesh, func f);

	void for_each_vertex(const Mesh *mesh, void (*f)(int, const MVert));
	void for_each_uv(Mesh *mesh, void (*f)(int, const std::array<float, 2>));
	void for_each_normal(Mesh *mesh, void (*f)(int, const std::array<float, 3>));

	template<typename key_t>
	using set_t = std::set<key_t, bool (*)(key_t, key_t)>;

	template<typename key_t>
	using set_mapping_t = std::vector<typename set_t<key_t>::iterator>;

	template<typename key_t>
	using dedup_pair_t = std::pair<set_t<key_t>, set_mapping_t<key_t> >;

	// template<typename key_t>
	// dedup_pair_t<key_t>&& make_deduplicate_set();

	// template<typename dedup_set_mapping_t, typename for_each_t, typename on_insert_t>
	// void deduplicate(ulong &total, ulong reserve, Mesh *mesh,
	//                  dedup_set_mapping_t &p, for_each_t for_each, on_insert_t on_insert);

	using uv_key_t = std::pair<std::array<float, 2>, ulong>;
	using no_key_t = std::pair<std::array<float, 3>, ulong>;

	// template<typename func>
	// void for_each_deduplicated_uv(Mesh *mesh, ulong &total, dedup_pair_t<uv_key_t>& p, func f);

	// template<typename func>
	// void for_each_deduplicated_normal(Mesh *mesh, ulong &total, dedup_pair_t<no_key_t>& p, func f);

	template <class F>
	struct lambda_traits : lambda_traits<decltype(&F::operator())> {};

	template <typename F, typename R, typename... Args>
	struct lambda_traits<R(F::*)(Args...)> : lambda_traits<R(F::*)(Args...) const> {};

	template <class F, class R, class... Args>
	struct lambda_traits<R(F::*)(Args...) const> {
		using pointer = typename std::add_pointer<R(Args...)>::type;

		static pointer stateful_lambda_to_pointer(F&& f) {
			static F fn = std::forward<F>(f);
			return [](Args... args) {
				       return fn(std::forward<Args>(args)...);
			       };
		}
	};

	template <class F>
	inline typename lambda_traits<F>::pointer stateful_lambda_to_pointer(F&& f) {
		return lambda_traits<F>::stateful_lambda_to_pointer(std::forward<F>(f));
	}

	template<typename dedup_set_mapping_t, typename for_each_t, typename on_insert_t>
	void deduplicate(ulong &total, ulong reserve, Mesh *mesh,
	                 dedup_set_mapping_t &p,
	                 for_each_t for_each,
	                 on_insert_t on_insert) {
		auto &set = p.first;
		auto &set_mapping = p.second;

		// Reserve approximate space to reduce allocations inside loop
		set_mapping.reserve(reserve);

		// C++14/17 would help here...
		auto fe_do = [&](ulong i,
		                 typename dedup_set_mapping_t::first_type::key_type::first_type v) {
			             auto p = set.insert(std::make_pair(v, total));
			             set_mapping.push_back(p.first);
			             if (p.second) {
				             on_insert(i, p.first);
				             ++total;
			             }
		             };

		for_each(mesh, stateful_lambda_to_pointer(fe_do));
	}

	// Again, C++14/17 would be really useful here...
	template<typename key_t>
	dedup_pair_t<key_t> make_deduplicate_set() {
		auto cmp = [](key_t lhs, key_t rhs) -> bool { return lhs.first < rhs.first; };
		return {set_t<key_t>{cmp} /* set */, {} /* set_mapping */};
	}

	template<typename func>
	void for_each_deduplicated_uv(Mesh *mesh, ulong &total, dedup_pair_t<uv_key_t>& p, func f) {
		deduplicate(/* modifies  */ total,
		            /* reserve   */ total + mesh->totloop,
		            mesh,
		            /* modifies  */ p,
		            /* for_each  */ for_each_uv,
		            /* on_insert */ f);
	}

	template<typename func>
	void for_each_deduplicated_normal(Mesh *mesh, ulong &total, dedup_pair_t<no_key_t>& p, func f) {
		deduplicate(/* modifies  */ total,
		            /* Assume 4 verts per face, doesn't break anything if not true */
		            total + mesh->totpoly * 4,
		            mesh,
		            /* modifies  */ p,
		            /* for_each  */ for_each_normal,
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

#endif // __io_intern_common__
