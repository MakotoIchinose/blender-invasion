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
 * The Original Code is Copyright (C) 2011 by Nicholas Bishop.
 */

/** \file
 * \ingroup modifiers
 */

#include "MEM_guardedalloc.h"

#include "BLI_utildefines.h"
#include "BLI_listbase.h"
#include "BLI_math_base.h"

#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_mesh_types.h"

#include "MOD_modifiertypes.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_library.h"
#include "BKE_library_query.h"
#include "BKE_object.h"
#include "BKE_remesh.h"

#include "DEG_depsgraph_query.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_MOD_REMESH
#  include "BLI_math_vector.h"

#  include "dualcon.h"
#endif

#ifdef WITH_OPENVDB
	#include "openvdb_capi.h"
#endif

static void initData(ModifierData *md)
{
	RemeshModifierData *rmd = (RemeshModifierData *) md;

	rmd->scale = 0.9;
	rmd->depth = 4;
	rmd->hermite_num = 1;
	rmd->flag = MOD_REMESH_FLOOD_FILL;
	rmd->mode = MOD_REMESH_SHARP_FEATURES;
	rmd->threshold = 1;
	rmd->voxel_size = 0.1f;
	rmd->isovalue = 0.0f;
	rmd->adaptivity = 0.0f;
	rmd->filter_width = 1;
	rmd->filter_bias = OPENVDB_LEVELSET_FIRST_BIAS;
	rmd->filter_type = OPENVDB_LEVELSET_FILTER_NONE;
	rmd->filter_iterations = 1;
	//rmd->shared = MEM_callocN(sizeof(RemeshModifierData_Shared), "remesh shared");
}

#ifdef WITH_MOD_REMESH

static void init_dualcon_mesh(DualConInput *input, Mesh *mesh)
{
	memset(input, 0, sizeof(DualConInput));

	input->co = (void *)mesh->mvert;
	input->co_stride = sizeof(MVert);
	input->totco = mesh->totvert;

	input->mloop = (void *)mesh->mloop;
	input->loop_stride = sizeof(MLoop);

	BKE_mesh_runtime_looptri_ensure(mesh);
	input->looptri = (void *)mesh->runtime.looptris.array;
	input->tri_stride = sizeof(MLoopTri);
	input->tottri = mesh->runtime.looptris.len;

	INIT_MINMAX(input->min, input->max);
	BKE_mesh_minmax(mesh, input->min, input->max);
}

/* simple structure to hold the output: a CDDM and two counters to
 * keep track of the current elements */
typedef struct {
	Mesh *mesh;
	int curvert, curface;
} DualConOutput;

/* allocate and initialize a DualConOutput */
static void *dualcon_alloc_output(int totvert, int totquad)
{
	DualConOutput *output;

	if (!(output = MEM_callocN(sizeof(DualConOutput),
	                           "DualConOutput")))
	{
		return NULL;
	}

	output->mesh = BKE_mesh_new_nomain(totvert, 0, 0, 4 * totquad, totquad);
	return output;
}

static void dualcon_add_vert(void *output_v, const float co[3])
{
	DualConOutput *output = output_v;
	Mesh *mesh = output->mesh;

	assert(output->curvert < mesh->totvert);

	copy_v3_v3(mesh->mvert[output->curvert].co, co);
	output->curvert++;
}

static void dualcon_add_quad(void *output_v, const int vert_indices[4])
{
	DualConOutput *output = output_v;
	Mesh *mesh = output->mesh;
	MLoop *mloop;
	MPoly *cur_poly;
	int i;

	assert(output->curface < mesh->totpoly);

	mloop = mesh->mloop;
	cur_poly = &mesh->mpoly[output->curface];

	cur_poly->loopstart = output->curface * 4;
	cur_poly->totloop = 4;
	for (i = 0; i < 4; i++)
		mloop[output->curface * 4 + i].v = vert_indices[i];

	output->curface++;
}

#if WITH_OPENVDB

static Mesh* voxel_remesh(RemeshModifierData *rmd, Mesh* mesh, struct OpenVDBLevelSet *level_set)
{
	MLoopCol** remap = NULL;
	Mesh* target = NULL;

	if (rmd->flag & MOD_REMESH_REPROJECT_VPAINT)
	{
		CSGVolume_Object *vcob;
		int i = 1;
		remap = MEM_calloc_arrayN(BLI_listbase_count(&rmd->csg_operands) + 1, sizeof(MLoopCol*), "remap");
		remap[0] = BKE_remesh_remap_loop_vertex_color_layer(mesh);
		for (vcob = rmd->csg_operands.first; vcob; vcob = vcob->next) {
			if (vcob->object && vcob->flag & MOD_REMESH_CSG_OBJECT_ENABLED)
			{
				Mesh* me = BKE_object_get_final_mesh(vcob->object);
				remap[i] = BKE_remesh_remap_loop_vertex_color_layer(me);
				i++;
			}
		}
	}

	OpenVDBLevelSet_filter(level_set, rmd->filter_type, rmd->filter_width, rmd->filter_iterations, rmd->filter_bias);
	target = BKE_remesh_voxel_ovdb_volume_to_mesh_nomain(level_set, rmd->isovalue, rmd->adaptivity,
	                                                     rmd->flag & MOD_REMESH_RELAX_TRIANGLES);
	OpenVDBLevelSet_free(level_set);

	if (rmd->flag & MOD_REMESH_REPROJECT_VPAINT)
	{
		int i = 1;
		CSGVolume_Object *vcob;

		if (remap[0]) {
			BKE_remesh_voxel_reproject_remapped_vertex_paint(target, mesh, remap[0]);
			MEM_freeN(remap[0]);
		}

		for (vcob = rmd->csg_operands.first; vcob; vcob = vcob->next) {
			if (vcob->object && vcob->flag & MOD_REMESH_CSG_OBJECT_ENABLED && remap && remap[i])
			{
				Mesh* me = BKE_object_get_final_mesh(vcob->object);
				BKE_remesh_voxel_reproject_remapped_vertex_paint(target, me, remap[i]);
				if (remap[i])
					MEM_freeN(remap[i]);
				i++;
			}
		}

		MEM_freeN(remap);
		BKE_mesh_update_customdata_pointers(target, false);
	}

	if (rmd->flag & MOD_REMESH_SMOOTH_NORMALS) {
		MPoly *mpoly = target->mpoly;
		int i, totpoly = target->totpoly;

		/* Apply smooth shading to output faces */
		for (i = 0; i < totpoly; i++) {
			mpoly[i].flag |= ME_SMOOTH;
		}
	}

	return target;
}

static struct OpenVDBLevelSet* csgOperation(struct OpenVDBLevelSet* level_set, CSGVolume_Object* vcob, Object* ob)
{
	Mesh *me = BKE_object_get_final_mesh(vcob->object);
	float imat[4][4];
	float omat[4][4];

	invert_m4_m4(imat, ob->obmat);
	mul_m4_m4m4(omat, imat, vcob->object->obmat);

	for (int i = 0; i < me->totvert; i++)
	{
		mul_m4_v3(omat, me->mvert[i].co);
	}

	struct OpenVDBTransform *xform = OpenVDBTransform_create();
	OpenVDBTransform_create_linear_transform(xform, vcob->voxel_size);

	struct OpenVDBLevelSet *level_setB = BKE_remesh_voxel_ovdb_mesh_to_level_set_create(me, xform);
	OpenVDBLevelSet_CSG_operation(level_set, level_set, level_setB, (OpenVDBLevelSet_CSGOperation)vcob->operation );

	OpenVDBLevelSet_free(level_setB);
	OpenVDBTransform_free(xform);

	//restore transform
	invert_m4_m4(imat, omat);
	for (int i = 0; i < me->totvert; i++)
	{
		mul_m4_v3(imat, me->mvert[i].co);
	}

	return level_set;
}
#endif

static Mesh *applyModifier(
        ModifierData *md,
        const ModifierEvalContext *ctx,
        Mesh *mesh)
{
	RemeshModifierData *rmd;
	DualConOutput *output;
	DualConInput input;
	Mesh *result;
	DualConFlags flags = 0;
	DualConMode mode = 0;

	rmd = (RemeshModifierData *)md;

	if (rmd->mode == MOD_REMESH_VOXEL)
	{
#if WITH_OPENVDB
		CSGVolume_Object *vcob;
		struct OpenVDBLevelSet* level_set;

		if (rmd->voxel_size > 0.0f) {
			struct OpenVDBTransform *xform = OpenVDBTransform_create();
			OpenVDBTransform_create_linear_transform(xform, rmd->voxel_size);
			level_set = BKE_remesh_voxel_ovdb_mesh_to_level_set_create(mesh, xform);
			OpenVDBTransform_free(xform);

			for (vcob = rmd->csg_operands.first; vcob; vcob = vcob->next)
			{
				if (vcob->object && (vcob->flag & MOD_REMESH_CSG_OBJECT_ENABLED))
				{
					level_set = csgOperation(level_set, vcob, ctx->object);
				}
			}

			return voxel_remesh(rmd, mesh, level_set);
		}
		else {
			return mesh;
		}
#else
		modifier_setError((ModifierData*)rmd, "Built without OpenVDB support, cant execute voxel remesh");
		return mesh;
#endif
	}

	init_dualcon_mesh(&input, mesh);

	if (rmd->flag & MOD_REMESH_FLOOD_FILL)
		flags |= DUALCON_FLOOD_FILL;

	switch (rmd->mode) {
		case MOD_REMESH_CENTROID:
			mode = DUALCON_CENTROID;
			break;
		case MOD_REMESH_MASS_POINT:
			mode = DUALCON_MASS_POINT;
			break;
		case MOD_REMESH_SHARP_FEATURES:
			mode = DUALCON_SHARP_FEATURES;
			break;
	}

	output = dualcon(&input,
	                 dualcon_alloc_output,
	                 dualcon_add_vert,
	                 dualcon_add_quad,
	                 flags,
	                 mode,
	                 rmd->threshold,
	                 rmd->hermite_num,
	                 rmd->scale,
	                 rmd->depth);
	result = output->mesh;
	MEM_freeN(output);

	if (rmd->flag & MOD_REMESH_SMOOTH_SHADING) {
		MPoly *mpoly = result->mpoly;
		int i, totpoly = result->totpoly;

		/* Apply smooth shading to output faces */
		for (i = 0; i < totpoly; i++) {
			mpoly[i].flag |= ME_SMOOTH;
		}
	}

	BKE_mesh_calc_edges(result, true, false);
	result->runtime.cd_dirty_vert |= CD_MASK_NORMAL;
	return result;
}

#else /* !WITH_MOD_REMESH */

static Mesh *applyModifier(
        ModifierData *UNUSED(md),
        const ModifierEvalContext *UNUSED(ctx),
        Mesh *mesh)
{
	return mesh;
}

#endif /* !WITH_MOD_REMESH */

static void requiredDataMask(Object *UNUSED(ob), ModifierData *md, CustomData_MeshMasks *r_cddata_masks)
{
	RemeshModifierData *rmd = (RemeshModifierData *)md;

	/* ask for vertexcolors if we need them */
	if (rmd->mode == MOD_REMESH_VOXEL) {
		r_cddata_masks->lmask |= CD_MASK_MLOOPCOL;
	}
}

static void foreachObjectLink(
        ModifierData *md, Object *ob,
        ObjectWalkFunc walk, void *userData)
{
	RemeshModifierData *rmd = (RemeshModifierData *)md;
	CSGVolume_Object *vcob;

	for (vcob = rmd->csg_operands.first; vcob; vcob = vcob->next)
	{
		if (vcob->object)
		{
			walk(userData, ob, &vcob->object, IDWALK_CB_NOP);
		}
	}
}

static void updateDepsgraph(ModifierData *md, const ModifierUpdateDepsgraphContext *ctx)
{
	RemeshModifierData *rmd = (RemeshModifierData *)md;
	CSGVolume_Object *vcob;

	for (vcob = rmd->csg_operands.first; vcob; vcob = vcob->next)
	{
		if (vcob->object != NULL) {
			DEG_add_object_relation(ctx->node, vcob->object, DEG_OB_COMP_TRANSFORM, "Remesh Modifier");
			DEG_add_object_relation(ctx->node, vcob->object, DEG_OB_COMP_GEOMETRY, "Remesh Modifier");
		}
	}
	/* We need own transformation as well. */
	DEG_add_modifier_to_transform_relation(ctx->node, "Remesh Modifier");
}

static void copyData(const ModifierData *md_src, ModifierData *md_dst, const int flag)
{
	RemeshModifierData *rmd_src = (RemeshModifierData*)md_src;
	RemeshModifierData *rmd_dst = (RemeshModifierData*)md_dst;

	modifier_copyData_generic(md_src, md_dst, flag);
	BLI_duplicatelist(&rmd_dst->csg_operands, &rmd_src->csg_operands);
}

ModifierTypeInfo modifierType_Remesh = {
	/* name */              "Remesh",
	/* structName */        "RemeshModifierData",
	/* structSize */        sizeof(RemeshModifierData),
	/* type */              eModifierTypeType_Constructive,
	/* flags */             eModifierTypeFlag_AcceptsMesh |
	                        eModifierTypeFlag_AcceptsCVs |
	                        eModifierTypeFlag_SupportsEditmode |
	                        eModifierTypeFlag_SupportsMapping,

	/* copyData */          copyData,

	/* deformVerts */       NULL,
	/* deformMatrices */    NULL,
	/* deformVertsEM */     NULL,
	/* deformMatricesEM */  NULL,
	/* applyModifier */     applyModifier,

	/* initData */          initData,
	/* requiredDataMask */  requiredDataMask,
	/* freeData */          NULL,
	/* isDisabled */        NULL,
	/* updateDepsgraph */   updateDepsgraph,
	/* dependsOnTime */     NULL,
	/* dependsOnNormals */	NULL,
	/* foreachObjectLink */ foreachObjectLink,
	/* foreachIDLink */     NULL,
	/* freeRuntimeData */   NULL,
};
