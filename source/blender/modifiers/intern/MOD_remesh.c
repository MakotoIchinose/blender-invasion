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

#include "BLI_math_base.h"

#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_mesh_types.h"

#include "MOD_modifiertypes.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_library_query.h"
#include "BKE_bvhutils.h"
#include "BKE_object.h"

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

#if 1
static struct OpenVDBLevelSet *mesh_to_levelset(Mesh* mesh, struct OpenVDBTransform *xform)
{
	float* verts;
	unsigned int *faces;
	int totfaces, totverts;

	BKE_mesh_runtime_looptri_recalc(mesh);
	const MLoopTri *looptri = BKE_mesh_runtime_looptri_ensure(mesh);
	MVertTri *verttri = MEM_callocN(sizeof(*verttri) * BKE_mesh_runtime_looptri_len(mesh), "remesh_looptri");
	BKE_mesh_runtime_verttri_from_looptri(verttri, mesh->mloop, looptri, BKE_mesh_runtime_looptri_len(mesh));

	totfaces = BKE_mesh_runtime_looptri_len(mesh);
	totverts = mesh->totvert;
	verts = (float *)MEM_calloc_arrayN(totverts * 3, sizeof(float), "remesh_input_verts");
	faces = (unsigned int *)MEM_calloc_arrayN(totfaces * 3, sizeof(unsigned int), "remesh_intput_faces");

	for(int i = 0; i < totverts; i++) {
		MVert mvert = mesh->mvert[i];
		verts[i * 3] = mvert.co[0];
		verts[i * 3 + 1] = mvert.co[1];
		verts[i * 3 + 2] = mvert.co[2];
	}

	for(int i = 0; i < totfaces; i++) {
		MVertTri vt = verttri[i];
		faces[i * 3] = vt.tri[0];
		faces[i * 3 + 1] = vt.tri[1];
		faces[i * 3 + 2] = vt.tri[2];
	}

	struct OpenVDBLevelSet *level_set = OpenVDBLevelSet_create(false, NULL);
	OpenVDBLevelSet_mesh_to_level_set(level_set, verts, faces, totverts, totfaces, xform);

	MEM_freeN(verts);
	MEM_freeN(faces);
	MEM_freeN(verttri);

	return level_set;
}

static Mesh* voxel_remesh(Mesh* mesh, Mesh* csg_operand, char operation, float voxel_size, float voxel_size_csg,
                          bool smooth_normals, float isovalue, float adaptivity, bool relax_triangles, int filter_type,
                          int filter_bias, int filter_width, int filter_iterations, bool reproject_vertex_paint,
                          float *csg_transform)
{
	int f = 0;
	BVHTreeFromMesh bvhtree = {NULL};
	MLoopCol *oldMesh_col = CustomData_get_layer(&mesh->ldata, CD_MLOOPCOL);
	MLoopCol *oldMesh_color = NULL;

	if (reproject_vertex_paint && oldMesh_col)
	{
		int i = 0;
		BKE_bvhtree_from_mesh_get(&bvhtree, mesh, BVHTREE_FROM_VERTS, 2);
		oldMesh_color = MEM_calloc_arrayN(mesh->totvert, sizeof(MLoopCol), "oldcolor");
		for (i = 0; i < mesh->totloop; i++) {
			MLoopCol c = oldMesh_col[i];
			//printf("COLOR %d %d %d %d %d \n", mesh->mloop[i].v, c.r, c.g, c.b, c.a);
			oldMesh_color[mesh->mloop[i].v].r = c.r;
			oldMesh_color[mesh->mloop[i].v].g = c.g;
			oldMesh_color[mesh->mloop[i].v].b = c.b;
			oldMesh_color[mesh->mloop[i].v].a = c.a;
		}
	}

	struct OpenVDBVolumeToMeshData output_mesh;
	struct OpenVDBLevelSet *level_set;
	struct OpenVDBLevelSet *level_setA;
	struct OpenVDBLevelSet *level_setB;
	struct OpenVDBTransform *xform = OpenVDBTransform_create();
	OpenVDBTransform_create_linear_transform(xform, voxel_size);

	if (csg_operand) {
		level_setA = mesh_to_levelset(mesh, xform);
		level_setB = mesh_to_levelset(csg_operand, xform);
		level_set = OpenVDBLevelSet_create(true, xform);
		OpenVDBLevelSet_CSG_operation(level_set, level_setA, level_setB, (OpenVDBLevelSet_CSGOperation)operation );

		OpenVDBLevelSet_free(level_setA);
		OpenVDBLevelSet_free(level_setB);
	}
	else {
		level_set = mesh_to_levelset(mesh, xform);
	}

	OpenVDBLevelSet_filter(level_set, filter_type, filter_width, filter_iterations, filter_bias);
	OpenVDBLevelSet_volume_to_mesh(level_set, &output_mesh, isovalue, adaptivity, relax_triangles);

	Mesh *newMesh = BKE_mesh_new_nomain(output_mesh.totvertices, 0, output_mesh.totquads + output_mesh.tottriangles, 0, 0);

	for(int i = 0; i < output_mesh.totvertices; i++) {
		float vco[3] = { output_mesh.vertices[i * 3], output_mesh.vertices[i * 3 + 1], output_mesh.vertices[i * 3 + 2]};
		copy_v3_v3(newMesh->mvert[i].co, vco);

	}
	for(int i = 0; i < output_mesh.totquads; i++) {
		newMesh->mface[i].v4 = output_mesh.quads[i * 4];
		newMesh->mface[i].v3 = output_mesh.quads[i * 4 + 1];
		newMesh->mface[i].v2 = output_mesh.quads[i * 4 + 2];
		newMesh->mface[i].v1 = output_mesh.quads[i * 4 + 3];
	}

	f = output_mesh.totquads;
	for(int i = 0; i < output_mesh.tottriangles; i++) {
		newMesh->mface[i+f].v4 = 0; //output_mesh.triangles[i * 3];
		newMesh->mface[i+f].v3 = output_mesh.triangles[i * 3];
		newMesh->mface[i+f].v2 = output_mesh.triangles[i * 3 + 1];
		newMesh->mface[i+f].v1 = output_mesh.triangles[i * 3 + 2];
	}

	OpenVDBLevelSet_free(level_set);
	OpenVDBTransform_free(xform);

	BKE_mesh_calc_edges_tessface(newMesh);
	BKE_mesh_convert_mfaces_to_mpolys(newMesh);
	BKE_mesh_calc_normals(newMesh);

	if (reproject_vertex_paint && oldMesh_color) {
		MVert *newMesh_verts =  newMesh->mvert;
		MLoop* loops = newMesh->mloop;
		MLoopCol *newMesh_color = CustomData_add_layer(&newMesh->ldata, CD_MLOOPCOL, CD_CALLOC, NULL, newMesh->totloop);

		for(int i = 0; i < newMesh->totloop; i++) {
			float from_co[3];
			BVHTreeNearest nearest;
			nearest.index = -1;
			nearest.dist_sq = FLT_MAX;
			copy_v3_v3(from_co, newMesh_verts[loops[i].v].co);
			BLI_bvhtree_find_nearest(bvhtree.tree, from_co, &nearest, bvhtree.nearest_callback, &bvhtree);

			if (nearest.index != -1) {
				MLoopCol c = oldMesh_color[nearest.index];
				//printf("MAPPED %d %d %d %d %d %d \n", i, nearest.index, c.r, c.g, c.b, c.a);
				newMesh_color[i].r = c.r;
				newMesh_color[i].g = c.g;
				newMesh_color[i].b = c.b;
				newMesh_color[i].a = c.a;
			}
			else {
				newMesh_color[i].r = 255;
				newMesh_color[i].g = 255;
				newMesh_color[i].b = 255;
				newMesh_color[i].a = 255;
			}
		}

		MEM_freeN(oldMesh_color);
		BKE_mesh_update_customdata_pointers(newMesh, false);
	}

	if (smooth_normals) {
		MPoly *mpoly = newMesh->mpoly;
		int i, totpoly = newMesh->totpoly;

		/* Apply smooth shading to output faces */
		for (i = 0; i < totpoly; i++) {
			mpoly[i].flag |= ME_SMOOTH;
		}
	}

	MEM_freeN(output_mesh.quads);
	MEM_freeN(output_mesh.vertices);
	if (output_mesh.tottriangles > 0) {
		MEM_freeN(output_mesh.triangles);
	}

	return newMesh;
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
		//invoke an operator ? nah, better make a function
		if (rmd->voxel_size > 0.0f) {
			if (rmd->object)
			{
				Mesh *me = BKE_modifier_get_evaluated_mesh_from_evaluated_object(rmd->object, false);
				Mesh *res = NULL;

				float imat[4][4];
				float omat[4][4];
				float* transform = MEM_calloc_arrayN(16, sizeof(float), "transform");
				int k = 0;

				invert_m4_m4(imat, ctx->object->obmat);
				mul_m4_m4m4(omat, imat, rmd->object->obmat);

				for (int i = 0; i < me->totvert; i++)
				{
					mul_m4_v3(omat, me->mvert[i].co);
				}

				for (int i = 0; i < 4; i++) {
					for (int j = 0; j < 4; j++) {
						transform[k] = omat[i][j];
						k++;
					}
				}

				res = voxel_remesh(mesh, me, rmd->operation, rmd->voxel_size, rmd->voxel_size,
									rmd->flag & MOD_REMESH_SMOOTH_NORMALS,
									rmd->isovalue, rmd->adaptivity, rmd->flag & MOD_REMESH_RELAX_TRIANGLES,
									rmd->filter_type, rmd->filter_bias, rmd->filter_width, rmd->filter_iterations,
									rmd->flag & MOD_REMESH_REPROJECT_VPAINT, transform);

				MEM_freeN(transform);
				return res;
			}
			else {
				return voxel_remesh(mesh, NULL, rmd->operation, rmd->voxel_size, rmd->voxel_size,
									rmd->flag & MOD_REMESH_SMOOTH_NORMALS,
									rmd->isovalue, rmd->adaptivity, rmd->flag & MOD_REMESH_RELAX_TRIANGLES,
									rmd->filter_type, rmd->filter_bias, rmd->filter_width, rmd->filter_iterations,
									rmd->flag & MOD_REMESH_REPROJECT_VPAINT, NULL);
			}
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

	walk(userData, ob, &rmd->object, IDWALK_CB_NOP);
}

static void updateDepsgraph(ModifierData *md, const ModifierUpdateDepsgraphContext *ctx)
{
	RemeshModifierData *rmd = (RemeshModifierData *)md;
	if (rmd->object != NULL) {
		DEG_add_object_relation(ctx->node, rmd->object, DEG_OB_COMP_TRANSFORM, "Remesh Modifier");
		DEG_add_object_relation(ctx->node, rmd->object, DEG_OB_COMP_GEOMETRY, "Remesh Modifier");
	}
	/* We need own transformation as well. */
	DEG_add_modifier_to_transform_relation(ctx->node, "Remesh Modifier");
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

	/* copyData */          modifier_copyData_generic,

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
