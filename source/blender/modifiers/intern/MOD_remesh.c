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
#include "BKE_bvhutils.h"

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
	rmd->filter_bias = VOXEL_BIAS_FIRST;
	rmd->filter_type = VOXEL_FILTER_NONE;
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
static Mesh* voxel_remesh(Mesh* mesh, float voxel_size, bool smooth_normals, float isovalue,
                          float adaptivity, bool relax_triangles, int filter_type,
                          int filter_bias, int filter_width, bool reproject_vertex_paint)
{
	struct OpenVDBRemeshData rmd;
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

	BKE_mesh_runtime_looptri_recalc(mesh);
	const MLoopTri *looptri = BKE_mesh_runtime_looptri_ensure(mesh);
	MVertTri *verttri = MEM_callocN(sizeof(*verttri) * BKE_mesh_runtime_looptri_len(mesh), "remesh_looptri");
	BKE_mesh_runtime_verttri_from_looptri(verttri, mesh->mloop, looptri, BKE_mesh_runtime_looptri_len(mesh));

	rmd.totfaces = BKE_mesh_runtime_looptri_len(mesh);
	rmd.totverts = mesh->totvert;
	rmd.verts = (float *)MEM_calloc_arrayN(rmd.totverts * 3, sizeof(float), "remesh_input_verts");
	rmd.faces = (unsigned int *)MEM_calloc_arrayN(rmd.totfaces * 3, sizeof(unsigned int), "remesh_intput_faces");
	rmd.voxel_size = voxel_size;
	rmd.isovalue = isovalue;
	rmd.adaptivity = adaptivity;
	rmd.relax_disoriented_triangles = relax_triangles;
	rmd.filter_type = filter_type;
	rmd.filter_bias = filter_bias;
	rmd.filter_width = filter_width;

	for(int i = 0; i < mesh->totvert; i++) {
		MVert mvert = mesh->mvert[i];
		rmd.verts[i * 3] = mvert.co[0];
		rmd.verts[i * 3 + 1] = mvert.co[1];
		rmd.verts[i * 3 + 2] = mvert.co[2];
	}

	for(int i = 0; i < rmd.totfaces; i++) {
		MVertTri vt = verttri[i];
		rmd.faces[i * 3] = vt.tri[0];
		rmd.faces[i * 3 + 1] = vt.tri[1];
		rmd.faces[i * 3 + 2] = vt.tri[2];
	}

	OpenVDB_voxel_remesh(&rmd);

	Mesh *newMesh = BKE_mesh_new_nomain(rmd.out_totverts, 0, rmd.out_totfaces + rmd.out_tottris, 0, 0);

	for(int i = 0; i < rmd.out_totverts; i++) {
		float vco[3] = { rmd.out_verts[i * 3], rmd.out_verts[i * 3 + 1], rmd.out_verts[i * 3 + 2]};
		copy_v3_v3(newMesh->mvert[i].co, vco);

	}
	for(int i = 0; i < rmd.out_totfaces; i++) {
		newMesh->mface[i].v4 = rmd.out_faces[i * 4];
		newMesh->mface[i].v3 = rmd.out_faces[i * 4 + 1];
		newMesh->mface[i].v2 = rmd.out_faces[i * 4 + 2];
		newMesh->mface[i].v1 = rmd.out_faces[i * 4 + 3];
	}

	f = rmd.out_totfaces;
	for(int i = 0; i < rmd.out_tottris; i++) {
		newMesh->mface[i+f].v4 = rmd.out_tris[i * 3];
		newMesh->mface[i+f].v3 = rmd.out_tris[i * 3];
		newMesh->mface[i+f].v2 = rmd.out_tris[i * 3 + 1];
		newMesh->mface[i+f].v1 = rmd.out_tris[i * 3 + 2];
	}

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

	MEM_freeN(rmd.faces);
	MEM_freeN(rmd.verts);
	MEM_freeN(verttri);
	MEM_freeN(rmd.out_verts);
	MEM_freeN(rmd.out_faces);
	if (rmd.out_tottris > 0) {
		MEM_freeN(rmd.out_tris);
	}

	return newMesh;
}
#endif

static Mesh *applyModifier(
        ModifierData *md,
        const ModifierEvalContext *UNUSED(ctx),
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
			return voxel_remesh(mesh, rmd->voxel_size, rmd->flag & MOD_REMESH_SMOOTH_NORMALS,
			                    rmd->isovalue, rmd->adaptivity, rmd->flag & MOD_REMESH_RELAX_TRIANGLES,
			                    rmd->filter_type, rmd->filter_bias, rmd->filter_width,
			                    rmd->flag & MOD_REMESH_REPROJECT_VPAINT);
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
	/* updateDepsgraph */   NULL,
	/* dependsOnTime */     NULL,
	/* dependsOnNormals */	NULL,
	/* foreachObjectLink */ NULL,
	/* foreachIDLink */     NULL,
	/* freeRuntimeData */   NULL,
};
