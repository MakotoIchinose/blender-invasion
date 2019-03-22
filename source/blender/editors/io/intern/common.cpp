extern "C" {
#include "BLI_listbase.h"
#include "BLI_math_matrix.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_modifier_types.h"

#include "BKE_modifier.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "../io_common.h"

#include <stdio.h>
}

#include "common.hpp"

// Anonymous namespace for internal functions
namespace {
	void name_compat(std::string& ob_name, std::string& mesh_name) {
		if(ob_name.compare(mesh_name) != 0) {
			ob_name += "_" + mesh_name;
		}
		size_t start_pos;
		while((start_pos = ob_name.find(" ")) != std::string::npos)
			ob_name.replace(start_pos, 1, "_");
	}
}

namespace common {

	static bool object_is_smoke_sim(const Object * const ob) {
		ModifierData *md = modifiers_findByType((Object *) ob, eModifierType_Smoke);
		if (md) {
			SmokeModifierData *smd = (SmokeModifierData *) md;
			return (smd->type == MOD_SMOKE_TYPE_DOMAIN);
		}
		return false;
	}

	/**
	 * Returns whether this object should be exported into the Alembic file.
	 *
	 * \param settings: export settings, used for options like 'selected only'.
	 * \param ob: the object's base in question.
	 * \param is_duplicated: Normally false; true when the object is instanced
	 * into the scene by a dupli-object (e.g. part of a dupligroup).
	 * This ignores selection and layer visibility,
	 * and assumes that the dupli-object itself (e.g. the group-instantiating empty) is exported.
	 */
	bool should_export_object(const ExportSettings * const settings, const Object * const eob) {

		/* From alembic */

		// TODO someone Currently ignoring all dupli objects; unsure if expected behavior
		if (!(eob->flag & BASE_FROM_DUPLI)) {
			/* !(eob->parent != NULL && eob->parent->transflag & OB_DUPLI) */

			/* These tests only make sense when the object isn't being instanced
			 * into the scene. When it is, its exportability is determined by
			 * its dupli-object and the DupliObject::no_draw property. */
			return  (settings->selected_only && (eob->flag & BASE_SELECTED) != 0) ||
				// FIXME Sybren: handle these cleanly (maybe just remove code),
				// now using active scene layer instead.
				(settings->visible_only && (eob->flag & BASE_VISIBLE) != 0) ||
				(settings->renderable_only && (eob->flag & BASE_ENABLED_RENDER) != 0);
		}

		return should_export_object(settings, eob->parent);
	}


	bool object_type_is_exportable(const Object * const ob) {
		switch (ob->type) {
		case OB_MESH:
			return !object_is_smoke_sim(ob);
			/* case OB_EMPTY: */
			/* case OB_CURVE: */
			/* case OB_SURF: */
			/* case OB_CAMERA: */
			/* return false; */
		default:
			printf("Export for this object type not defined %s\n", ob->id.name);
			return false;
		}
	}

	bool get_final_mesh(const ExportSettings * const settings, const Scene * const escene,
	                    const Object *eob, Mesh **mesh /* out */) {
		/* From abc_mesh.cc */

		/* Temporarily disable subdivs if we shouldn't apply them */
		if (!settings->apply_subdiv)
			for (ModifierData *md = (ModifierData *) eob->modifiers.first; md; md = md->next)
				if (md->type == eModifierType_Subsurf)
					md->mode |= eModifierMode_DisableTemporary;

		float scale_mat[4][4];
		scale_m4_fl(scale_mat, settings->global_scale);
		// TODO someone Unsure if necessary
		// scale_mat[3][3] = m_settings.global_scale;  /* also scale translation */
		mul_m4_m4m4((float (*)[4]) eob->obmat, eob->obmat, scale_mat);
		// yup_mat[3][3] /= m_settings.global_scale;  /* normalise the homogeneous component */

		if (determinant_m4(eob->obmat) < 0.0)
			;               /* TODO someone flip normals */

		/* Object *eob = DEG_get_evaluated_object(settings->depsgraph, ob); */
		*mesh = mesh_get_eval_final(settings->depsgraph, (Scene *) escene, (Object *)eob, CD_MASK_MESH);

		if (!settings->apply_subdiv)
			for (ModifierData *md = (ModifierData *) eob->modifiers.first; md; md = md->next)
				if (md->type == eModifierType_Subsurf)
					md->mode &= ~eModifierMode_DisableTemporary;

		if (settings->triangulate) {
			struct BMeshCreateParams bmcp = {false};
			struct BMeshFromMeshParams bmfmp = {true, false, false, 0};
			BMesh *bm = BKE_mesh_to_bmesh_ex(*mesh, &bmcp, &bmfmp);

			BM_mesh_triangulate(bm, settings->quad_method, settings->ngon_method,
			                    false /* tag_only */, NULL, NULL, NULL);

			Mesh *result = BKE_mesh_from_bmesh_for_eval_nomain(bm, 0);
			BM_mesh_free(bm);

			*mesh = result;
			/* Needs to free? */
			return true;
		}

		/* Needs to free? */
		return false;
	}

	std::string&& get_object_name(const Object *eob, const Mesh *mesh) {
		std::string name{eob->id.name + 2};
		std::string mesh_name{mesh->id.name + 2};
		name_compat(name /* modifies */, mesh_name);
		return std::move(name);
	}

        	// template<typename func>
	void for_each_vertex(const Mesh *mesh, void (*f)(int, const MVert)) {
		// TODO someone Should this use iterators? Unsure if those are only for BMesh
		for (int i = 0, e = mesh->totvert; i < e; ++i)
			f(i, mesh->mvert[i]);
	}

	// template<typename func>
	void for_each_uv(Mesh *mesh, void (*f)(int, const std::array<float, 2>)) {
		for (int i = 0, e = mesh->totloop; i < e; ++i) {
			const float (&uv)[2] = mesh->mloopuv[i].uv;
			f(i, std::array<float, 2>{uv[0], uv[1]});
		}
	}

	// template<typename func>
	void for_each_normal(Mesh *mesh, void (*f)(int, const std::array<float, 3>)) {
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


	// template<typename func>
	// void for_each_vertex(const Mesh *mesh, func f) {
	// 	// TODO someone Should this use iterators? Unsure if those are only for BMesh
	// 	for (int i = 0, e = mesh->totvert; i < e; ++i)
	// 		f(i, mesh->mvert[i]);
	// }

	// template<typename func>
	// void for_each_uv(Mesh *mesh, func f) {
	// 	for (int i = 0, e = mesh->totloop; i < e; ++i) {
	// 		f(i, mesh->mloopuv[i].uv);
	// 	}
	// }

	// template<typename func>
	// void for_each_normal(Mesh *mesh, func f) {
	// 	const float (*loop_no)[3] = static_cast<float(*)[3]>
	// 		(CustomData_get_layer(&mesh->ldata,
	// 		                      CD_NORMAL));
	// 	unsigned loop_index = 0;
	// 	MVert *verts = mesh->mvert;
	// 	MPoly *mp = mesh->mpoly;
	// 	MLoop *mloop = mesh->mloop;
	// 	MLoop *ml = mloop;

	// 	// TODO someone Should -0 be converted to 0?
	// 	if (loop_no) {
	// 		for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
	// 			ml = mesh->mloop + mp->loopstart + (mp->totloop - 1);
	// 			for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
	// 				f(i, loop_no[ml->v]);
	// 			}
	// 		}
	// 	} else {
	// 		float no[3];
	// 		for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
	// 			ml = mloop + mp->loopstart + (mp->totloop - 1);

	// 			/* Flat shaded, use common normal for all verts. */
	// 			if ((mp->flag & ME_SMOOTH) == 0) {
	// 				BKE_mesh_calc_poly_normal(mp, ml - (mp->totloop - 1),
	// 				                          verts, no);
	// 				f(i, no);
	// 				ml -= mp->totloop;
	// 				loop_index += mp->totloop;
	// 			} else {
	// 				/* Smooth shaded, use individual vert normals. */
	// 				for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
	// 					normal_short_to_float_v3(no, verts[ml->v].no);
	// 					f(i, no);
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// template<typename dedup_set_mapping_t, typename for_each_t, typename for_each_do_t, typename on_insert_t>
	// void deduplicate(ulong &total, ulong reserve, Mesh *mesh,
	//                  dedup_set_mapping_t &p, for_each_t for_each, for_each_do_t for_each_do,
	//                  on_insert_t on_insert) {
	// 	auto &set = p.first;
	// 	auto &set_mapping = p.second;

	// 	// Reserve approximate space to reduce allocations inside loop
	// 	set_mapping.reserve(reserve);

	// 	// C++14/17 would help here...
	// 	for_each(mesh, [&](ulong i,
	// 	                   typename dedup_set_mapping_t::first_type::first_type v) {
	// 		               auto p = set.insert({v, total});
	// 		               set_mapping.push_back(p.first);
	// 		               if (p.second) {
	// 			               on_insert(i, p.first);
	// 			               ++total;
	// 		               }
	// 	               });
	// }

	// // Again, C++14/17 would be really useful here...
	// template<typename key_t>
	// dedup_pair_t<key_t>&& make_deduplicate_set() {
	// 	auto cmp = [](key_t lhs, key_t rhs) -> bool { return lhs.first < rhs.first; };
	// 	return std::move({{cmp} /* set */, {} /* set_mapping */});
	// }

	// template<typename func>
	// void for_each_deduplicated_uv(Mesh *mesh, ulong &total, dedup_pair_t<uv_key_t>& p, func f) {
	// 	deduplicate(/* modifies  */ total,
	// 	            /* reserve   */ total + mesh->totloop,
	// 	            /* modifies  */ p,
	// 	            /* for_each  */ &for_each_uv,
	// 	            /* on_insert */ f);
	// }

	// template<typename func>
	// void for_each_deduplicated_normal(Mesh *mesh, ulong &total, dedup_pair_t<no_key_t>& p, func f) {
	// 	deduplicate(/* modifies  */ total,
	// 	            /* Assume 4 verts per face, doesn't break anything if not true */
	// 	            total + mesh->totpoly * 4,
	// 	            /* modifies  */ p,
	// 	            /* for_each  */ &for_each_normal,
	// 	            /* on_insert */
	// 	            [&p, &f, &mesh](ulong i, typename set_t<no_key_t>::iterator it){
	// 		            // Call callee function
	// 		            f(i, it);

	// 		            // If the face is flat shaded, f is only invoked once, but we need to
	// 		            // add the value to the map an additional totloop - 1 times
	// 		            MPoly *mp = mesh->mpoly[i];
	// 		            if ((mp->flag & ME_SMOOTH) == 0)
	// 			            for (int j = 0; j < (mp->totloop - 1); ++j)
	// 				            p.second.push_back(it);
	// 	            });
	// }
}
