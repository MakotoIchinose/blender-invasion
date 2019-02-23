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
 * The Original Code is Copyright (C) 2004 by Blender Foundation
 * All rights reserved.
 */

/** \file
 * \ingroup edobj
 */


#include "MEM_guardedalloc.h"

#include "DNA_object_types.h"
#include "DNA_mesh_types.h"
#include "DNA_material_types.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_string.h"
#include "BLI_fileops.h"
#include "BLI_path_util.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_layer.h"
#include "BKE_library.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_mesh.h"
#include "BKE_modifier.h"
#include "BKE_node.h"
#include "BKE_object.h"
#include "BKE_report.h"
#include "BKE_scene.h"
#include "BKE_screen.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "RE_engine.h"
#include "RE_pipeline.h"

#include "IMB_imbuf_types.h"
#include "IMB_imbuf.h"
#include "IMB_colormanagement.h"

#include "WM_api.h"
#include "WM_types.h"

#include "ED_object.h"
#include "ED_screen.h"
#include "ED_uvedit.h"

#include "GPU_draw.h"

#include "object_intern.h"

typedef struct BakeAPIPass {
	struct BakeAPIPass *next, *prev;

	Object *ob;
	Main *main;
	Scene *scene;
	ViewLayer *view_layer;
	ReportList *reports;

	int margin;

	int save_mode;

	bool do_clear;
	bool is_automatic_name;
	bool is_cage;

	float cage_extrusion;
	int normal_space;
	eBakeNormalSwizzle normal_swizzle[3];

	char uv_layer[MAX_CUSTOMDATA_LAYER_NAME];
	char custom_cage[MAX_NAME];
	char filepath[FILE_MAX];

	struct ImageFormatData im_format;

	int width;
	int height;

	bool ready;

	/* callbacks */
	Render *render;
	float *progress;
	short *do_update;

	/* for redrawing */
	ScrArea *sa;

	BakePass *bp;
} BakeAPIPass;

typedef struct BakeAPIRender {
	ListBase passes;
} BakeAPIRender;

/* callbacks */

static void bake_progress_update(void *bjv, float progress)
{
	BakeAPIPass *bj = bjv;

	if (bj->progress && *bj->progress != progress) {
		*bj->progress = progress;

		/* make jobs timer to send notifier */
		*(bj->do_update) = true;
	}
}

/* catch esc */
static int bake_modal(bContext *C, wmOperator *UNUSED(op), const wmEvent *event)
{
	/* no running blender, remove handler and pass through */
	if (0 == WM_jobs_test(CTX_wm_manager(C), CTX_data_scene(C), WM_JOB_TYPE_OBJECT_BAKE))
		return OPERATOR_FINISHED | OPERATOR_PASS_THROUGH;

	/* running render */
	switch (event->type) {
		case ESCKEY:
		{
			G.is_break = true;
			return OPERATOR_RUNNING_MODAL;
		}
	}
	return OPERATOR_PASS_THROUGH;
}

/* for exec() when there is no render job
 * note: this wont check for the escape key being pressed, but doing so isnt threadsafe */
static int bake_break(void *UNUSED(rjv))
{
	if (G.is_break)
		return 1;
	return 0;
}


static void bake_update_image(ScrArea *sa, Image *image)
{
	if (sa && sa->spacetype == SPACE_IMAGE) { /* in case the user changed while baking */
		SpaceImage *sima = sa->spacedata.first;
		if (sima)
			sima->image = image;
	}
}

static bool write_internal_bake_pixels(
        Image *image, BakePixel pixel_array[], float *buffer,
        const int width, const int height, const int margin,
        const bool do_clear, const bool is_noncolor)
{
	ImBuf *ibuf;
	void *lock;
	bool is_float;
	char *mask_buffer = NULL;
	const size_t num_pixels = (size_t)width * (size_t)height;

	ibuf = BKE_image_acquire_ibuf(image, NULL, &lock);

	if (!ibuf)
		return false;

	if (margin > 0 || !do_clear) {
		mask_buffer = MEM_callocN(sizeof(char) * num_pixels, "Bake Mask");
		RE_bake_mask_fill(pixel_array, num_pixels, mask_buffer);
	}

	is_float = (ibuf->flags & IB_rectfloat);

	/* colormanagement conversions */
	if (!is_noncolor) {
		const char *from_colorspace;
		const char *to_colorspace;

		from_colorspace = IMB_colormanagement_role_colorspace_name_get(COLOR_ROLE_SCENE_LINEAR);

		if (is_float)
			to_colorspace = IMB_colormanagement_get_float_colorspace(ibuf);
		else
			to_colorspace = IMB_colormanagement_get_rect_colorspace(ibuf);

		if (from_colorspace != to_colorspace)
			IMB_colormanagement_transform(buffer, ibuf->x, ibuf->y, ibuf->channels, from_colorspace, to_colorspace, false);
	}

	/* populates the ImBuf */
	if (do_clear) {
		if (is_float) {
			IMB_buffer_float_from_float(
			        ibuf->rect_float, buffer, ibuf->channels,
			        IB_PROFILE_LINEAR_RGB, IB_PROFILE_LINEAR_RGB, false,
			        ibuf->x, ibuf->y, ibuf->x, ibuf->x);
		}
		else {
			IMB_buffer_byte_from_float(
			        (unsigned char *) ibuf->rect, buffer, ibuf->channels, ibuf->dither,
			        IB_PROFILE_SRGB, IB_PROFILE_SRGB,
			        false, ibuf->x, ibuf->y, ibuf->x, ibuf->x);
		}
	}
	else {
		if (is_float) {
			IMB_buffer_float_from_float_mask(
			        ibuf->rect_float, buffer, ibuf->channels,
			        ibuf->x, ibuf->y, ibuf->x, ibuf->x, mask_buffer);
		}
		else {
			IMB_buffer_byte_from_float_mask(
			        (unsigned char *) ibuf->rect, buffer, ibuf->channels, ibuf->dither,
			        false, ibuf->x, ibuf->y, ibuf->x, ibuf->x, mask_buffer);
		}
	}

	/* margins */
	if (margin > 0)
		RE_bake_margin(ibuf, mask_buffer, margin);

	ibuf->userflags |= IB_DISPLAY_BUFFER_INVALID | IB_BITMAPDIRTY;

	if (ibuf->rect_float)
		ibuf->userflags |= IB_RECT_INVALID;

	/* force mipmap recalc */
	if (ibuf->mipmap[0]) {
		ibuf->userflags |= IB_MIPMAP_INVALID;
		imb_freemipmapImBuf(ibuf);
	}

	BKE_image_release_ibuf(image, ibuf, NULL);

	if (mask_buffer)
		MEM_freeN(mask_buffer);

	return true;
}

/* force OpenGL reload */
static void refresh_images(BakeImage *bake_image)
{
	Image *ima = bake_image->image;
	if (ima->ok == IMA_OK_LOADED) {
		GPU_free_image(ima);
		DEG_id_tag_update(&ima->id, 0);
	}
}

static bool write_external_bake_pixels(
        const char *filepath, BakePixel pixel_array[], float *buffer,
        const int width, const int height, const int margin,
        ImageFormatData *im_format, const bool is_noncolor)
{
	ImBuf *ibuf = NULL;
	bool ok = false;
	bool is_float;

	is_float = im_format->depth > 8;

	/* create a new ImBuf */
	ibuf = IMB_allocImBuf(width, height, im_format->planes, (is_float ? IB_rectfloat : IB_rect));

	if (!ibuf)
		return false;

	/* populates the ImBuf */
	if (is_float) {
		IMB_buffer_float_from_float(
		        ibuf->rect_float, buffer, ibuf->channels,
		        IB_PROFILE_LINEAR_RGB, IB_PROFILE_LINEAR_RGB, false,
		        ibuf->x, ibuf->y, ibuf->x, ibuf->x);
	}
	else {
		if (!is_noncolor) {
			const char *from_colorspace = IMB_colormanagement_role_colorspace_name_get(COLOR_ROLE_SCENE_LINEAR);
			const char *to_colorspace = IMB_colormanagement_get_rect_colorspace(ibuf);
			IMB_colormanagement_transform(buffer, ibuf->x, ibuf->y, ibuf->channels, from_colorspace, to_colorspace, false);
		}

		IMB_buffer_byte_from_float(
		        (unsigned char *) ibuf->rect, buffer, ibuf->channels, ibuf->dither,
		        IB_PROFILE_SRGB, IB_PROFILE_SRGB,
		        false, ibuf->x, ibuf->y, ibuf->x, ibuf->x);
	}

	/* margins */
	if (margin > 0) {
		char *mask_buffer = NULL;
		const size_t num_pixels = (size_t)width * (size_t)height;

		mask_buffer = MEM_callocN(sizeof(char) * num_pixels, "Bake Mask");
		RE_bake_mask_fill(pixel_array, num_pixels, mask_buffer);
		RE_bake_margin(ibuf, mask_buffer, margin);

		if (mask_buffer)
			MEM_freeN(mask_buffer);
	}

	if ((ok = BKE_imbuf_write(ibuf, filepath, im_format))) {
#ifndef WIN32
		chmod(filepath, S_IRUSR | S_IWUSR);
#endif
		//printf("%s saving bake map: '%s'\n", __func__, filepath);
	}

	/* garbage collection */
	IMB_freeImBuf(ibuf);

	return ok;
}

/* if all is good tag image and return true */
static bool bake_object_check(BakeAPIPass *bkp, Object *ob)
{
	ReportList *reports = bkp->reports;
	Base *base = BKE_view_layer_base_find(bkp->view_layer, ob);

	if (base == NULL) {
		BKE_reportf(reports, RPT_ERROR, "Object \"%s\" is not in view layer", ob->id.name + 2);
		return false;
	}

	if (!(base->flag & BASE_ENABLED_RENDER)) {
		BKE_reportf(reports, RPT_ERROR, "Object \"%s\" is not enabled for rendering", ob->id.name + 2);
		return false;
	}


	if (ob->type != OB_MESH) {
		BKE_reportf(reports, RPT_ERROR, "Object \"%s\" is not a mesh", ob->id.name + 2);
		return false;
	}
	else {
		Mesh *me = (Mesh *)ob->data;

		if (CustomData_get_active_layer_index(&me->ldata, CD_MLOOPUV) == -1) {
			BKE_reportf(reports, RPT_ERROR,
			            "No active UV layer found in the object \"%s\"", ob->id.name + 2);
			return false;
		}
	}

	Material *mat_filter = bkp->bp->material;

	if (mat_filter) {
		bool has_material = false;
		for (int i = 0; i < ob->totcol; i++) {
			if (mat_filter == give_current_material(ob, i + 1)) {
				has_material = true;
				break;
			}
		}
		if (!has_material) {
			BKE_reportf(bkp->reports, RPT_ERROR, "Object \"%s\" doesn't contain the selected material \"%s\"", ob->id.name + 2, mat_filter->id.name + 2);
			return false;
		}
	}

	return true;
}

/* before even getting in the bake function we check for some basic errors */
static bool bake_objects_check(BakeAPIPass *bkp)
{
	ReportList *reports = bkp->reports;

	if (!bkp->ob) {
		BKE_report(reports, RPT_ERROR, "No object selected");
		return false;
	}

	if (!bkp->bp->image) {
		BKE_report(reports, RPT_ERROR, "The bake pass has no target image");
		return false;
	}

	if (!bake_object_check(bkp, bkp->ob))
		return false;

	if (bkp->bp->bake_from_collection) {
		int tot_objects = 0;

		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_BEGIN(bkp->bp->bake_from_collection, object, DAG_EVAL_RENDER)
		{
			if (object == bkp->ob)
				continue;

			if (ELEM(object->type, OB_MESH, OB_FONT, OB_CURVE, OB_SURF, OB_MBALL) == false) {
				BKE_reportf(reports, RPT_ERROR, "Object \"%s\" is not a mesh or can't be converted to a mesh (Curve, Text, Surface or Metaball)", object->id.name + 2);
				return false;
			}
			tot_objects += 1;
		}
		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_END;

		if (tot_objects == 0) {
			BKE_report(reports, RPT_ERROR, "No valid objects in collection");
			return false;
		}
	}

	return true;
}

/*
 * returns the total number of pixels
 */
static size_t initialize_internal_image(BakeImage *bake_image, ReportList *reports)
{
	ImBuf *ibuf;
	void *lock;

	ibuf = BKE_image_acquire_ibuf(bake_image->image, NULL, &lock);

	if (ibuf) {
		bake_image->width = ibuf->x;
		bake_image->height = ibuf->y;

		BKE_image_release_ibuf(bake_image->image, ibuf, lock);
		return (size_t)ibuf->x * (size_t)ibuf->y;
	}
	else {
		BKE_image_release_ibuf(bake_image->image, ibuf, lock);
		BKE_reportf(reports, RPT_ERROR, "Uninitialized image %s", bake_image->image->id.name + 2);
		return 0;
	}
}

/* create new mesh with edit mode changes and modifiers applied */
static Mesh *bake_mesh_new_from_object(Depsgraph *depsgraph, Main *bmain, Scene *scene, Object *ob)
{
	bool apply_modifiers = (ob->type != OB_MESH);
	Mesh *me = BKE_mesh_new_from_object(depsgraph, bmain, scene, ob, apply_modifiers, false);

	if (me->flag & ME_AUTOSMOOTH) {
		BKE_mesh_split_faces(me, true);
	}

	return me;
}

static int bake(BakeAPIPass *bkp)
{
	Scene *scene = bkp->scene;
	Main *bmain = bkp->main;
	ReportList *reports = bkp->reports;
	Render *re = bkp->render;
	ViewLayer *view_layer = bkp->view_layer;
	char *uv_layer = bkp->uv_layer;
	bool is_cage = bkp->is_cage;
	char *custom_cage = bkp->custom_cage;

	if (!bake_objects_check(bkp)) {
		return OPERATOR_CANCELLED;
	}

	RE_SetReports(re, reports);

	/* We build a depsgraph for the baking,
	 * so we don't need to change the original data to adjust visibility and modifiers. */
	Depsgraph *depsgraph = DEG_graph_new(scene, view_layer, DAG_EVAL_RENDER);
	DEG_graph_build_from_view_layer(depsgraph, bmain, scene, view_layer);

	int op_result = OPERATOR_CANCELLED;
	bool ok = false;

	Object *ob_cage = NULL;
	Object *ob_cage_eval = NULL;
	Object *ob_low_eval = NULL;

	BakeHighPolyData *highpoly = NULL;
	int tot_highpoly = 0;

	Mesh *me_low = NULL;
	Mesh *me_cage = NULL;

	MultiresModifierData *mmd_low = NULL;
	int mmd_flags_low = 0;

	BakeResult result = {0};

	BakePixel *pixel_array_low = NULL;
	BakePixel *pixel_array_high = NULL;

	const bool is_save_internal = (bkp->save_mode == R_BAKE_SAVE_INTERNAL);

	BakeImage bake_image = {0};

	size_t num_pixels;

	RE_bake_engine_set_engine_parameters(re, bmain, scene);

	if (!RE_bake_has_engine(re)) {
		BKE_report(reports, RPT_ERROR, "Current render engine does not support baking");
		goto cleanup;
	}

	if (uv_layer && uv_layer[0] != '\0') {
		Mesh *me = (Mesh *)bkp->ob->data;
		if (CustomData_get_named_layer(&me->ldata, CD_MLOOPUV, uv_layer) == -1) {
			BKE_reportf(reports, RPT_ERROR,
			            "No UV layer named \"%s\" found in the object \"%s\"", uv_layer, bkp->ob->id.name + 2);
			goto cleanup;
		}
	}

	bake_image.image = bkp->bp->image;
	bake_image.mat_mask_length = bkp->ob->totcol;
	bake_image.mat_mask = MEM_mallocN(sizeof(bool) * bake_image.mat_mask_length, "bake material mask");

	Material *mat_filter = bkp->bp->material;
	for (int i = 0; i < bake_image.mat_mask_length; i++) {
		bake_image.mat_mask[i] = (mat_filter == NULL) || (mat_filter == give_current_material(bkp->ob, i + 1));
	}

	if (is_save_internal) {
		num_pixels = initialize_internal_image(&bake_image, reports);

		if (num_pixels == 0) {
			goto cleanup;
		}
	}
	else {
		/* when saving externally always use the size specified in the UI */

		num_pixels = (size_t)bkp->width * (size_t)bkp->height;

		bake_image.width = bkp->width;
		bake_image.height = bkp->height;
		bake_image.image = NULL;
	}

	if (bkp->bp->bake_from_collection) {
		tot_highpoly = 0;

		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_BEGIN(bkp->bp->bake_from_collection, object, DAG_EVAL_RENDER)
		{
			if (object == bkp->ob)
				continue;

			tot_highpoly ++;
		}
		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_END;

		if (is_cage && custom_cage[0] != '\0') {
			ob_cage = BLI_findstring(&bmain->object, custom_cage, offsetof(ID, name) + 2);

			if (ob_cage == NULL || ob_cage->type != OB_MESH) {
				BKE_report(reports, RPT_ERROR, "No valid cage object");
				goto cleanup;
			}
			else {
				ob_cage_eval = DEG_get_evaluated_object(depsgraph, ob_cage);
				ob_cage_eval->restrictflag |= OB_RESTRICT_RENDER;
				ob_cage_eval->base_flag &= ~(BASE_VISIBLE | BASE_ENABLED_RENDER);
			}
		}
	}

	pixel_array_low = MEM_mallocN(sizeof(BakePixel) * num_pixels, "bake pixels low poly");

	/* for multires bake, use linear UV subdivision to match low res UVs */
	if (bkp->normal_space == R_BAKE_SPACE_TANGENT && !bkp->bp->bake_from_collection) {
		mmd_low = (MultiresModifierData *) modifiers_findByType(bkp->ob, eModifierType_Multires);
		if (mmd_low) {
			mmd_flags_low = mmd_low->flags;
			mmd_low->uv_smooth = SUBSURF_UV_SMOOTH_NONE;
		}
	}

	result.num_pixels = num_pixels;

	/* Make sure depsgraph is up to date. */
	BKE_scene_graph_update_tagged(depsgraph, bmain);
	ob_low_eval = DEG_get_evaluated_object(depsgraph, bkp->ob);

	/* get the mesh as it arrives in the renderer */
	me_low = bake_mesh_new_from_object(depsgraph, bmain, scene, ob_low_eval);

	/* populate the pixel array with the face data */
	if ((bkp->bp->bake_from_collection && (ob_cage == NULL) && is_cage) == false)
		RE_bake_pixels_populate(me_low, pixel_array_low, num_pixels, &bake_image, uv_layer);
	/* else populate the pixel array with the 'cage' mesh (the smooth version of the mesh)  */

	if (bkp->bp->bake_from_collection) {
		int i = 0;

		/* prepare cage mesh */
		if (ob_cage) {
			me_cage = bake_mesh_new_from_object(depsgraph, bmain, scene, ob_cage_eval);
			if ((me_low->totpoly != me_cage->totpoly) || (me_low->totloop != me_cage->totloop)) {
				BKE_report(reports, RPT_ERROR,
				           "Invalid cage object, the cage mesh must have the same number "
				           "of faces as the active object");
				goto cleanup;
			}
		}
		else if (is_cage) {
			BKE_object_eval_reset(ob_low_eval);

			ModifierData *md = ob_low_eval->modifiers.first;
			while (md) {
				ModifierData *md_next = md->next;

				/* Edge Split cannot be applied in the cage,
				 * the cage is supposed to have interpolated normals
				 * between the faces unless the geometry is physically
				 * split. So we create a copy of the low poly mesh without
				 * the eventual edge split.*/

				if (md->type == eModifierType_EdgeSplit) {
					BLI_remlink(&ob_low_eval->modifiers, md);
					modifier_free(md);
				}
				md = md_next;
			}

			me_cage = bake_mesh_new_from_object(depsgraph, bmain, scene, ob_low_eval);
			RE_bake_pixels_populate(me_cage, pixel_array_low, num_pixels, &bake_image, uv_layer);
		}

		highpoly = MEM_callocN(sizeof(BakeHighPolyData) * tot_highpoly, "bake high poly objects");

		/* populate highpoly array */
		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_BEGIN(bkp->bp->bake_from_collection, object, DAG_EVAL_RENDER)
		{
			if (object == bkp->ob)
				continue;

			/* initialize highpoly_data */
			highpoly[i].ob = object;
			highpoly[i].ob_eval = DEG_get_evaluated_object(depsgraph, object);
			highpoly[i].ob_eval->restrictflag &= ~OB_RESTRICT_RENDER;
			highpoly[i].ob_eval->base_flag |= (BASE_VISIBLE | BASE_ENABLED_RENDER);
			highpoly[i].me = bake_mesh_new_from_object(depsgraph, bmain, scene, highpoly[i].ob_eval);

			/* lowpoly to highpoly transformation matrix */
			copy_m4_m4(highpoly[i].obmat, highpoly[i].ob->obmat);
			invert_m4_m4(highpoly[i].imat, highpoly[i].obmat);

			highpoly[i].is_flip_object = is_negative_m4(highpoly[i].ob->obmat);

			i++;
		}
		FOREACH_COLLECTION_VISIBLE_OBJECT_RECURSIVE_END;

		BLI_assert(i == tot_highpoly);


		if (ob_cage != NULL) {
			ob_cage_eval->restrictflag |= OB_RESTRICT_RENDER;
			ob_cage_eval->base_flag &= ~(BASE_VISIBLE | BASE_ENABLED_RENDER);
		}
		ob_low_eval->restrictflag |= OB_RESTRICT_RENDER;
		ob_low_eval->base_flag &= ~(BASE_VISIBLE | BASE_ENABLED_RENDER);

		pixel_array_high = MEM_mallocN(sizeof(BakePixel) * num_pixels, "bake pixels high poly");

		/* populate the pixel arrays with the corresponding face data for each high poly object */
		if (!RE_bake_pixels_populate_from_objects(
		            me_low, pixel_array_low, pixel_array_high, highpoly, tot_highpoly, num_pixels, ob_cage != NULL,
		            bkp->cage_extrusion, ob_low_eval->obmat, (ob_cage ? ob_cage->obmat : ob_low_eval->obmat), me_cage))
		{
			BKE_report(reports, RPT_ERROR, "Error handling selected objects");
			goto cleanup;
		}

		/* the baking itself */
		for (i = 0; i < tot_highpoly; i++) {
			ok = RE_bake_engine(re, depsgraph, bkp->bp, highpoly[i].ob, i, pixel_array_high, &result);
			if (!ok) {
				BKE_reportf(reports, RPT_ERROR, "Error baking from object \"%s\"", highpoly[i].ob->id.name + 2);
				goto cleanup;
			}
		}
	}
	else {
		/* If low poly is not renderable it should have failed long ago. */
		BLI_assert((ob_low_eval->restrictflag & OB_RESTRICT_RENDER) == 0);

		if (RE_bake_has_engine(re)) {
			ok = RE_bake_engine(re, depsgraph, bkp->bp, ob_low_eval, 0, pixel_array_low, &result);
		}
		else {
			BKE_report(reports, RPT_ERROR, "Current render engine does not support baking");
			goto cleanup;
		}
	}

	/* normal space conversion
	 * the normals are expected to be in world space, +X +Y +Z */
	if (ok && result.is_normal) {
		const eBakeNormalSwizzle *normal_swizzle = bkp->normal_swizzle;
		switch (bkp->normal_space) {
			case R_BAKE_SPACE_WORLD:
			{
				/* Cycles internal format */
				if ((normal_swizzle[0] == R_BAKE_POSX) &&
				    (normal_swizzle[1] == R_BAKE_POSY) &&
				    (normal_swizzle[2] == R_BAKE_POSZ))
				{
					break;
				}
				else {
					RE_bake_normal_world_to_world(pixel_array_low, num_pixels, result.depth, result.pixels, normal_swizzle);
				}
				break;
			}
			case R_BAKE_SPACE_OBJECT:
			{
				RE_bake_normal_world_to_object(pixel_array_low, num_pixels, result.depth, result.pixels, ob_low_eval, normal_swizzle);
				break;
			}
			case R_BAKE_SPACE_TANGENT:
			{
				if (bkp->bp->bake_from_collection) {
					RE_bake_normal_world_to_tangent(pixel_array_low, num_pixels, result.depth, result.pixels, me_low, normal_swizzle, ob_low_eval->obmat);
				}
				else {
					/* from multiresolution */
					Mesh *me_nores = NULL;
					ModifierData *md = NULL;
					int mode;

					BKE_object_eval_reset(ob_low_eval);
					md = modifiers_findByType(ob_low_eval, eModifierType_Multires);

					if (md) {
						mode = md->mode;
						md->mode &= ~eModifierMode_Render;
					}

					/* Evaluate modifiers again. */
					me_nores = BKE_mesh_new_from_object(depsgraph, bmain, scene, ob_low_eval, true, false);
					RE_bake_pixels_populate(me_nores, pixel_array_low, num_pixels, &bake_image, uv_layer);

					RE_bake_normal_world_to_tangent(pixel_array_low, num_pixels, result.depth, result.pixels, me_nores, normal_swizzle, ob_low_eval->obmat);
					BKE_id_free(bmain, me_nores);

					if (md)
						md->mode = mode;
				}
				break;
			}
			default:
				break;
		}
	}

	if (bkp->do_clear) {
		RE_bake_ibuf_clear(bake_image.image, result.fill_color);
	}

	if (!ok) {
		BKE_reportf(reports, RPT_ERROR, "Problem baking object \"%s\"", bkp->ob->id.name + 2);
		op_result = OPERATOR_CANCELLED;
	}
	else {
		/* save the results */
		if (is_save_internal) {
			ok = write_internal_bake_pixels(
						bake_image.image,
						pixel_array_low,
						result.pixels,
						bake_image.width, bake_image.height,
						bkp->margin, bkp->do_clear, !result.is_color);

			/* might be read by UI to set active image for display */
			bake_update_image(bkp->sa, bake_image.image);

			if (!ok) {
				BKE_reportf(reports, RPT_ERROR,
							"Problem saving the bake map internally for object \"%s\"", bkp->ob->id.name + 2);
				op_result = OPERATOR_CANCELLED;
			}
			else {
				BKE_report(reports, RPT_INFO,
							"Baking map saved to internal image, save it externally or pack it");
				op_result = OPERATOR_FINISHED;
			}
		}
		/* save externally */
		else {
			char name[FILE_MAX];

			BKE_image_path_from_imtype(name, bkp->filepath, BKE_main_blendfile_path(bmain),
			                           0, bkp->bp->im_format.imtype, true, false, NULL);

			if (bkp->is_automatic_name) {
				BLI_path_suffix(name, FILE_MAX, bkp->ob->id.name + 2, "_");
				BLI_path_suffix(name, FILE_MAX, result.identifier, "_");
			}

			/* save it externally */
			ok = write_external_bake_pixels(
					name,
					pixel_array_low,
					result.pixels,
					bake_image.width, bake_image.height,
					bkp->margin, &bkp->bp->im_format, !result.is_color);

			if (!ok) {
				BKE_reportf(reports, RPT_ERROR, "Problem saving baked map in \"%s\"", name);
				op_result = OPERATOR_CANCELLED;
			}
			else {
				BKE_reportf(reports, RPT_INFO, "Baking map written to \"%s\"", name);
				op_result = OPERATOR_FINISHED;
			}
		}
	}

	if (is_save_internal)
		refresh_images(&bake_image);

cleanup:

	if (highpoly) {
		int i;
		for (i = 0; i < tot_highpoly; i++) {
			if (highpoly[i].me)
				BKE_id_free(bmain, highpoly[i].me);
		}
		MEM_freeN(highpoly);
	}

	if (mmd_low)
		mmd_low->flags = mmd_flags_low;

	if (pixel_array_low)
		MEM_freeN(pixel_array_low);

	if (pixel_array_high)
		MEM_freeN(pixel_array_high);

	if (result.pixels)
		MEM_freeN(result.pixels);

	if (bake_image.mat_mask)
		MEM_freeN(bake_image.mat_mask);

	if (me_low)
		BKE_id_free(bmain, me_low);

	if (me_cage)
		BKE_id_free(bmain, me_cage);

	DEG_graph_free(depsgraph);

	RE_SetReports(re, NULL);

	return op_result;
}

static void bake_init_api_data(wmOperator *op, bContext *C, BakePass *bp, BakeAPIPass *bkp)
{
	bScreen *sc = CTX_wm_screen(C);

	bkp->ob = CTX_data_active_object(C);
	bkp->main = CTX_data_main(C);
	bkp->view_layer = CTX_data_view_layer(C);
	bkp->scene = CTX_data_scene(C);
	bkp->sa = sc ? BKE_screen_find_big_area(sc, SPACE_IMAGE, 10) : NULL;

#define RNA_ISDEF(name) (RNA_property_is_set(op->ptr, RNA_struct_find_property(op->ptr, name)))

	if (RNA_ISDEF("filepath"))
		RNA_string_get(op->ptr, "filepath", bkp->filepath);
	else BLI_strncpy(bkp->filepath, bp->filepath, sizeof(bkp->filepath));

	if (RNA_ISDEF("width"))
		bkp->width = RNA_int_get(op->ptr, "width");
	else bkp->width = bp->width;

	if (RNA_ISDEF("height"))
		bkp->height = RNA_int_get(op->ptr, "height");
	else bkp->height = bp->width;

	if (RNA_ISDEF("margin"))
		bkp->margin = RNA_int_get(op->ptr, "margin");
	else bkp->margin = bp->margin;

	if (RNA_ISDEF("cage_extrusion"))
		bkp->cage_extrusion = RNA_float_get(op->ptr, "cage_extrusion");
	else bkp->cage_extrusion = bp->cage_extrusion;

	if (RNA_ISDEF("cage_object"))
		RNA_string_get(op->ptr, "cage_object", bkp->custom_cage);
	else if(bp->cage_object)
		BLI_strncpy(bkp->custom_cage, bp->cage_object->id.name + 2, sizeof(bkp->custom_cage));

	if (RNA_ISDEF("normal_space"))
		bkp->normal_space = RNA_enum_get(op->ptr, "normal_space");
	else bkp->normal_space = bp->normal_space;

	if (RNA_ISDEF("normal_r"))
		bkp->normal_swizzle[0] = RNA_enum_get(op->ptr, "normal_r");
	else bkp->normal_swizzle[0] = bp->normal_swizzle[0];

	if (RNA_ISDEF("normal_g"))
		bkp->normal_swizzle[1] = RNA_enum_get(op->ptr, "normal_g");
	else bkp->normal_swizzle[1] = bp->normal_swizzle[1];

	if (RNA_ISDEF("normal_b"))
		bkp->normal_swizzle[2] = RNA_enum_get(op->ptr, "normal_b");
	else bkp->normal_swizzle[2] = bp->normal_swizzle[2];

	if (RNA_ISDEF("save_mode"))
		bkp->save_mode = RNA_enum_get(op->ptr, "save_mode");
	else bkp->save_mode = bp->save_mode;

	if (RNA_ISDEF("use_clear"))
		bkp->do_clear = RNA_boolean_get(op->ptr, "use_clear");
	else bkp->do_clear = (bp->flag & R_BAKE_CLEAR) != 0;

	if (RNA_ISDEF("use_cage"))
		bkp->is_cage = RNA_boolean_get(op->ptr, "use_cage");
	else bkp->is_cage = (bp->flag & R_BAKE_CAGE) != 0;

	if (RNA_ISDEF("use_automatic_name"))
		bkp->is_automatic_name = RNA_boolean_get(op->ptr, "use_automatic_name");
	else bkp->is_automatic_name = (bp->flag & R_BAKE_AUTO_NAME) != 0;

	if (RNA_ISDEF("uv_layer"))
		RNA_string_get(op->ptr, "uv_layer", bkp->uv_layer);
	else BLI_strncpy(bkp->uv_layer, bp->uvlayer_name, sizeof(bkp->uv_layer));

	bkp->bp = bp;

	bkp->reports = op->reports;

	bkp->render = RE_NewSceneRender(bkp->scene);

	/* XXX hack to force saving to always be internal. Whether (and how) to support
	 * external saving will be addressed later */
	bkp->save_mode = R_BAKE_SAVE_INTERNAL;
}

static int bake_exec(bContext *C, wmOperator *op)
{
	int result = OPERATOR_CANCELLED;
	Object *ob = CTX_data_active_object(C);

	G.is_break = false;
	G.is_rendering = true;

	for (BakePass *bp = ob->bake_passes.first; bp; bp = bp->next) {
		if (!(bp->flag & R_BAKE_ENABLED))
			continue;

		BakeAPIPass bkp = {NULL};

		bake_init_api_data(op, C, bp, &bkp);
		Render *re = bkp.render;

		/* setup new render */
		RE_test_break_cb(re, NULL, bake_break);

		result = bake(&bkp);

		if (result == OPERATOR_CANCELLED) {
			break;
		}
	}

	G.is_rendering = false;

	return result;
}

static void bake_startjob(void *bkv, short *UNUSED(stop), short *do_update, float *progress)
{
	BakeAPIRender *bkr = (BakeAPIRender *)bkv;

	for(BakeAPIPass *bkp = bkr->passes.first; bkp; bkp = bkp->next) {
		/* setup new render */
		bkp->do_update = do_update;
		bkp->progress = progress;

		int result = bake(bkp);
		if (result == OPERATOR_CANCELLED) {
			break;
		}
	}
}

static void bake_freejob(void *bkv)
{
	BakeAPIRender *bkr = bkv;

	BLI_freelistN(&bkr->passes);
	MEM_freeN(bkr);

	G.is_rendering = false;
}

static int bake_invoke(bContext *C, wmOperator *op, const wmEvent *UNUSED(event))
{
	wmJob *wm_job;
	Scene *scene = CTX_data_scene(C);
	Object *ob = CTX_data_active_object(C);

	/* only one render job at a time */
	if (WM_jobs_test(CTX_wm_manager(C), scene, WM_JOB_TYPE_OBJECT_BAKE))
		return OPERATOR_CANCELLED;

	BakeAPIRender *bkr;
	bkr = MEM_callocN(sizeof(BakeAPIRender), "render bake");

	for (BakePass *bp = ob->bake_passes.first; bp; bp = bp->next) {
		if (!(bp->flag & R_BAKE_ENABLED))
			continue;

		BakeAPIPass *bkp = MEM_callocN(sizeof(BakeAPIPass), "render bake pass");
		bake_init_api_data(op, C, bp, bkp);

		/* setup new render */
		RE_test_break_cb(bkp->render, NULL, bake_break);
		RE_progress_cb(bkp->render, bkp, bake_progress_update);

		BLI_addtail(&bkr->passes, bkp);
	}

	/* setup job */
	wm_job = WM_jobs_get(CTX_wm_manager(C), CTX_wm_window(C), scene, "Texture Bake",
	                     WM_JOB_EXCL_RENDER | WM_JOB_PRIORITY | WM_JOB_PROGRESS, WM_JOB_TYPE_OBJECT_BAKE);
	WM_jobs_customdata_set(wm_job, bkr, bake_freejob);
	WM_jobs_timer(wm_job, 0.5, NC_IMAGE, 0); /* TODO - only draw bake image, can we enforce this */
	WM_jobs_callbacks(wm_job, bake_startjob, NULL, NULL, NULL);

	G.is_break = false;
	G.is_rendering = true;

	WM_jobs_start(CTX_wm_manager(C), wm_job);

	WM_cursor_wait(0);

	/* add modal handler for ESC */
	WM_event_add_modal_handler(C, op);

	WM_event_add_notifier(C, NC_SCENE | ND_RENDER_RESULT, scene);
	return OPERATOR_RUNNING_MODAL;
}

void OBJECT_OT_bake(wmOperatorType *ot)
{
	/* identifiers */
	ot->name = "Bake";
	ot->description = "Bake image textures of selected objects";
	ot->idname = "OBJECT_OT_bake";

	/* api callbacks */
	ot->exec = bake_exec;
	ot->modal = bake_modal;
	ot->invoke = bake_invoke;
	ot->poll = ED_operator_object_active_editable_mesh;

	RNA_def_string_file_path(ot->srna, "filepath", NULL, FILE_MAX, "File Path",
	                         "Image filepath to use when saving externally");
	RNA_def_int(ot->srna, "width", 512, 1, INT_MAX, "Width",
	            "Horizontal dimension of the baking map (external only)", 64, 4096);
	RNA_def_int(ot->srna, "height", 512, 1, INT_MAX, "Height",
	            "Vertical dimension of the baking map (external only)", 64, 4096);
	RNA_def_int(ot->srna, "margin", 16, 0, INT_MAX, "Margin",
	            "Extends the baked result as a post process filter", 0, 64);
	RNA_def_boolean(ot->srna, "use_selected_to_active", false, "Selected to Active",
	                "Bake shading on the surface of selected objects to the active object");
	RNA_def_float(ot->srna, "cage_extrusion", 0.0f, 0.0f, FLT_MAX, "Cage Extrusion",
	              "Distance to use for the inward ray cast when using selected to active", 0.0f, 1.0f);
	RNA_def_string(ot->srna, "cage_object", NULL, MAX_NAME, "Cage Object",
	               "Object to use as cage, instead of calculating the cage from the active object with cage extrusion");
	RNA_def_enum(ot->srna, "normal_space", rna_enum_normal_space_items, R_BAKE_SPACE_TANGENT, "Normal Space",
	             "Choose normal space for baking");
	RNA_def_enum(ot->srna, "normal_r", rna_enum_normal_swizzle_items, R_BAKE_POSX, "R", "Axis to bake in red channel");
	RNA_def_enum(ot->srna, "normal_g", rna_enum_normal_swizzle_items, R_BAKE_POSY, "G", "Axis to bake in green channel");
	RNA_def_enum(ot->srna, "normal_b", rna_enum_normal_swizzle_items, R_BAKE_POSZ, "B", "Axis to bake in blue channel");
	RNA_def_enum(ot->srna, "save_mode", rna_enum_bake_save_mode_items, R_BAKE_SAVE_INTERNAL, "Save Mode",
	             "Choose how to save the baking map");
	RNA_def_boolean(ot->srna, "use_clear", false, "Clear",
	                "Clear Images before baking (only for internal saving)");
	RNA_def_boolean(ot->srna, "use_cage", false, "Cage",
	                "Cast rays to active object from a cage");
	RNA_def_boolean(ot->srna, "use_automatic_name", false, "Automatic Name",
	                "Automatically name the output file with the pass type");
	RNA_def_string(ot->srna, "uv_layer", NULL, MAX_CUSTOMDATA_LAYER_NAME, "UV Layer", "UV layer to override active");
}
