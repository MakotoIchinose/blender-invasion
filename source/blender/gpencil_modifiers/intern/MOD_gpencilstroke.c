/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
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
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017, Blender Foundation
 * This is a new part of Blender
 *
 * Contributor(s): Antonio Vazquez, Joshua Leung
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

/** \file blender/gpencil_modifiers/intern/MOD_gpencilarray.c
 *  \ingroup modifiers
 */

#include <stdio.h>

#include "MEM_guardedalloc.h"

#include "DNA_scene_types.h"
#include "DNA_object_types.h"
#include "DNA_gpencil_types.h"
#include "DNA_gpencil_modifier_types.h"

#include "BLI_blenlib.h"
#include "BLI_rand.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "BLI_linklist.h"
#include "BLI_alloca.h"

#include "BKE_gpencil.h"
#include "BKE_gpencil_modifier.h"
#include "BKE_modifier.h"
#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_object.h"
#include "BKE_main.h"
#include "BKE_scene.h"
#include "BKE_layer.h"
#include "BKE_library_query.h"
#include "BKE_collection.h"
#include "BKE_mesh.h"
#include "BKE_mesh_mapping.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "MOD_gpencil_util.h"
#include "MOD_gpencil_modifiertypes.h"

static void initData(GpencilModifierData *md)
{
	StrokeGpencilModifierData *gpmd = (StrokeGpencilModifierData *)md;
	gpmd->object = NULL;
}

static void copyData(const GpencilModifierData *md, GpencilModifierData *target)
{
	BKE_gpencil_modifier_copyData_generic(md, target);
}

static void generate_geometry(
        GpencilModifierData *md, Depsgraph *UNUSED(depsgraph),
        Object *ob, bGPDlayer *gpl, bGPDframe *gpf)
{
	StrokeGpencilModifierData *gpmd = (StrokeGpencilModifierData *)md;

	if( gpmd->object == NULL ){
		printf("NULL object!\n");
    	return;
	}

	int color_idx = 0;
	int tot_points = 0;
	short thickness = 1;

	float mat[4][4];

	unit_m4(mat);

	BMesh *bm;

	bm = BKE_mesh_to_bmesh_ex(
			gpmd->object->data,
			&(struct BMeshCreateParams){0},
			&(struct BMeshFromMeshParams){
			.calc_face_normal = true,
			.cd_mask_extra = CD_MASK_ORIGINDEX,
			});

 	BMVert *vert;
	BMIter iter;

	BM_ITER_MESH (vert, &iter, bm, BM_VERTS_OF_MESH) {

        //Have we already used this vert?
		if(!BM_elem_flag_test(vert, BM_ELEM_SELECT)){
        	continue;
		}

		BMVert *prepend_vert = NULL;
		BMVert *next_vert = vert;
		//Chain together the C verts and export them as GP strokes (chain in object space)
		BMVert *edge_vert;
		BMEdge *e;
		BMIter iter_e;

		LinkNodePair chain = {NULL, NULL};

		int connected_c_verts;

		while( next_vert != NULL ){

			connected_c_verts = 0;
			vert = next_vert;

			BLI_linklist_append(&chain, vert);

			BM_elem_flag_disable(vert, BM_ELEM_SELECT);

			BM_ITER_ELEM (e, &iter_e, vert, BM_EDGES_OF_VERT) {
				edge_vert = BM_edge_other_vert(e, vert);

				if(BM_elem_flag_test(edge_vert, BM_ELEM_SELECT)){
					if( connected_c_verts == 0 ){
                    	next_vert = edge_vert;
					} else if( connected_c_verts == 1 && prepend_vert == NULL ){
                    	prepend_vert = edge_vert;
					} else {
                    	printf("C verts not connected in a simple line!\n");
					}
					connected_c_verts++;
				}

			}

			if( connected_c_verts == 0 ){
            	next_vert = NULL;
			}

		}

		LinkNode *pre_list = chain.list;

		while( prepend_vert != NULL ) {

			connected_c_verts = 0;
            vert = prepend_vert;

			BLI_linklist_prepend(&pre_list, vert);

			BM_elem_flag_disable(vert, BM_ELEM_SELECT);

			BM_ITER_ELEM (e, &iter_e, vert, BM_EDGES_OF_VERT) {
				edge_vert = BM_edge_other_vert(e, vert);

				if(BM_elem_flag_test(edge_vert, BM_ELEM_SELECT)){
					if( connected_c_verts == 0 ){
                    	prepend_vert = edge_vert;
					} else {
                    	printf("C verts not connected in a simple line!\n");
					}
					connected_c_verts++;
				}

			}

			if( connected_c_verts == 0 ){
            	prepend_vert = NULL;
			}
		}

		tot_points = BLI_linklist_count(pre_list);

		printf("Tot points: %d\n", tot_points);

		if( tot_points <= 1 ){
			//Don't draw a stroke, chain too short.
			printf("Chain to short\n");
        	continue;
		}

		float *stroke_data = BLI_array_alloca(stroke_data, tot_points * GP_PRIM_DATABUF_SIZE);

		int array_idx = 0;

		for (LinkNode *entry = pre_list; entry; entry = entry->next) {
			vert = entry->link;
			stroke_data[array_idx] = vert->co[0];
			stroke_data[array_idx + 1] = vert->co[1];
			stroke_data[array_idx + 2] = vert->co[2];

			stroke_data[array_idx + 3] = 1.0f; //thickness
			stroke_data[array_idx + 4] = 1.0f; //hardness?

			array_idx += 5;
		}

		/* generate stroke */
		bGPDstroke *gps;
		gps = BKE_gpencil_add_stroke(gpf, color_idx, tot_points, thickness);
		BKE_gpencil_stroke_add_points(gps, stroke_data, tot_points, mat);

		BLI_linklist_free(pre_list, NULL);
	}

	BM_mesh_free(bm);
}

static void bakeModifier(
        Main *UNUSED(bmain), Depsgraph *depsgraph,
        GpencilModifierData *md, Object *ob)
{

	bGPdata *gpd = ob->data;

	for (bGPDlayer *gpl = gpd->layers.first; gpl; gpl = gpl->next) {
		for (bGPDframe *gpf = gpl->frames.first; gpf; gpf = gpf->next) {
			generate_geometry(md, depsgraph, ob, gpl, gpf);
			return;
		}
	}
}

/* -------------------------------- */

/* Generic "generateStrokes" callback */
static void generateStrokes(
        GpencilModifierData *md, Depsgraph *depsgraph,
        Object *ob, bGPDlayer *gpl, bGPDframe *gpf)
{
	generate_geometry(md, depsgraph, ob, gpl, gpf);
}

static void updateDepsgraph(GpencilModifierData *md, const ModifierUpdateDepsgraphContext *ctx)
{
	StrokeGpencilModifierData *lmd = (StrokeGpencilModifierData *)md;
	if (lmd->object != NULL) {
		DEG_add_object_relation(ctx->node, lmd->object, DEG_OB_COMP_GEOMETRY, "Stroke Modifier");
		DEG_add_object_relation(ctx->node, lmd->object, DEG_OB_COMP_TRANSFORM, "Stroke Modifier");
	}
	DEG_add_object_relation(ctx->node, ctx->object, DEG_OB_COMP_TRANSFORM, "Stroke Modifier");
}

static void foreachObjectLink(
	GpencilModifierData *md, Object *ob,
	ObjectWalkFunc walk, void *userData)
{
	StrokeGpencilModifierData *mmd = (StrokeGpencilModifierData *)md;

	walk(userData, ob, &mmd->object, IDWALK_CB_NOP);
}


GpencilModifierTypeInfo modifierType_Gpencil_Stroke = {
	/* name */              "Stroke",
	/* structName */        "StrokeGpencilModifierData",
	/* structSize */        sizeof(StrokeGpencilModifierData),
	/* type */              eGpencilModifierTypeType_Gpencil,
	/* flags */             0,

	/* copyData */          copyData,

	/* deformStroke */      NULL,
	/* generateStrokes */   generateStrokes,
	/* bakeModifier */      bakeModifier,
	/* remapTime */         NULL,

	/* initData */          initData,
	/* freeData */          NULL,
	/* isDisabled */        NULL,
	/* updateDepsgraph */   updateDepsgraph,
	/* dependsOnTime */     NULL,
	/* foreachObjectLink */ foreachObjectLink,
	/* foreachIDLink */     NULL,
	/* foreachTexLink */    NULL,
};
