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
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017, Blender Foundation
 * This is a new part of Blender
 */

/** \file
 * \ingroup modifiers
 */

#include <stdio.h>

#include "BLI_utildefines.h"

#include "BLI_math.h"

#include "DNA_meshdata_types.h"
#include "DNA_scene_types.h"
#include "DNA_object_types.h"
#include "DNA_gpencil_types.h"
#include "DNA_gpencil_modifier_types.h"
#include "DNA_modifier_types.h"

#include "BKE_action.h"
#include "BKE_colorband.h"
#include "BKE_colortools.h"
#include "BKE_deform.h"
#include "BKE_gpencil.h"
#include "BKE_gpencil_modifier.h"
#include "BKE_modifier.h"
#include "BKE_library_query.h"
#include "BKE_scene.h"
#include "BKE_main.h"
#include "BKE_layer.h"

#include "MEM_guardedalloc.h"

#include "MOD_gpencil_util.h"
#include "MOD_gpencil_modifiertypes.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

static void initData(GpencilModifierData *md)
{
  VertexcolorGpencilModifierData *gpmd = (VertexcolorGpencilModifierData *)md;
  gpmd->pass_index = 0;
  gpmd->layername[0] = '\0';
  gpmd->materialname[0] = '\0';
  gpmd->vgname[0] = '\0';
  gpmd->object = NULL;
  gpmd->radius = 1.0f;
  gpmd->factor = 1.0f;

  /* Add default color ramp. */
  gpmd->colorband = BKE_colorband_add(false);
  if (gpmd->colorband) {
    BKE_colorband_init(gpmd->colorband, true);
    CBData *ramp = gpmd->colorband->data;
    ramp[0].r = ramp[0].g = ramp[0].b = ramp[0].a = 1.0f;
    ramp[0].pos = 0.0f;
    ramp[1].r = ramp[1].g = ramp[1].b = 0.0f;
    ramp[1].a = 1.0f;
    ramp[1].pos = 1.0f;

    gpmd->colorband->tot = 2;
  }
}

static void copyData(const GpencilModifierData *md, GpencilModifierData *target)
{
  VertexcolorGpencilModifierData *gmd = (VertexcolorGpencilModifierData *)md;
  VertexcolorGpencilModifierData *tgmd = (VertexcolorGpencilModifierData *)target;

  MEM_SAFE_FREE(tgmd->colorband);

  BKE_gpencil_modifier_copyData_generic(md, target);
  if (gmd->colorband) {
    tgmd->colorband = MEM_dupallocN(gmd->colorband);
  }
}

/* deform stroke */
static void deformStroke(GpencilModifierData *md,
                         Depsgraph *UNUSED(depsgraph),
                         Object *ob,
                         bGPDlayer *gpl,
                         bGPDframe *UNUSED(gpf),
                         bGPDstroke *gps)
{
  VertexcolorGpencilModifierData *mmd = (VertexcolorGpencilModifierData *)md;
  if (!mmd->object) {
    return;
  }

  const int def_nr = defgroup_name_index(ob, mmd->vgname);

  if (!is_stroke_affected_by_modifier(ob,
                                      mmd->layername,
                                      mmd->materialname,
                                      mmd->pass_index,
                                      mmd->layer_pass,
                                      1,
                                      gpl,
                                      gps,
                                      mmd->flag & GP_HOOK_INVERT_LAYER,
                                      mmd->flag & GP_HOOK_INVERT_PASS,
                                      mmd->flag & GP_HOOK_INVERT_LAYERPASS,
                                      mmd->flag & GP_HOOK_INVERT_MATERIAL)) {
    return;
  }

  float radius_sqr = mmd->radius * mmd->radius;
  float coba_res[4];

  /* loop points and apply deform */
  float target_loc[3];
  copy_v3_v3(target_loc, mmd->object->loc);

  bool doit = false;
  for (int i = 0; i < gps->totpoints; i++) {
    bGPDspoint *pt = &gps->points[i];
    MDeformVert *dvert = gps->dvert != NULL ? &gps->dvert[i] : NULL;

    /* Calc world position of point. */
    float pt_loc[3];
    mul_v3_m4v3(pt_loc, ob->obmat, &pt->x);

    /* Cal distance to point (squared) */
    float dist_sqr = len_squared_v3v3(pt_loc, target_loc);

    /* Only points in the radius. */
    if (dist_sqr > radius_sqr) {
      continue;
    }

    if (!doit) {
      /* Apply to fill. */
      if (mmd->mode != GPPAINT_MODE_STROKE) {
        BKE_colorband_evaluate(mmd->colorband, 1.0f, coba_res);
        interp_v3_v3v3(gps->mix_color_fill, gps->mix_color_fill, coba_res, mmd->factor);
        gps->mix_color_fill[3] = mmd->factor;
      }
      /* If no stroke, cancel loop. */
      if (mmd->mode != GPPAINT_MODE_BOTH) {
        break;
      }

      doit = true;
    }

    /* Verify vertex group. */
    if (mmd->mode != GPPAINT_MODE_FILL) {
      const float weight = get_modifier_point_weight(
          dvert, (mmd->flag & GP_HOOK_INVERT_VGROUP) != 0, def_nr);
      if (weight < 0.0f) {
        continue;
      }
      /* Calc the factor using the distance and get mix color. */
      float mix_factor = dist_sqr / radius_sqr;
      BKE_colorband_evaluate(mmd->colorband, mix_factor, coba_res);

      interp_v3_v3v3(pt->mix_color, pt->mix_color, coba_res, mmd->factor * weight);
      pt->mix_color[3] = mmd->factor * (1.0f - mix_factor);
    }
  }
}

/* FIXME: Ideally we be doing this on a copy of the main depsgraph
 * (i.e. one where we don't have to worry about restoring state)
 */
static void bakeModifier(Main *bmain, Depsgraph *depsgraph, GpencilModifierData *md, Object *ob)
{
  VertexcolorGpencilModifierData *mmd = (VertexcolorGpencilModifierData *)md;
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  bGPdata *gpd = ob->data;
  int oldframe = (int)DEG_get_ctime(depsgraph);

  if (mmd->object == NULL) {
    return;
  }

  for (bGPDlayer *gpl = gpd->layers.first; gpl; gpl = gpl->next) {
    for (bGPDframe *gpf = gpl->frames.first; gpf; gpf = gpf->next) {
      /* apply effects on this frame
       * NOTE: this assumes that we don't want animation on non-keyframed frames
       */
      CFRA = gpf->framenum;
      BKE_scene_graph_update_for_newframe(depsgraph, bmain);

      /* compute effects on this frame */
      for (bGPDstroke *gps = gpf->strokes.first; gps; gps = gps->next) {
        deformStroke(md, depsgraph, ob, gpl, gpf, gps);
      }
    }
  }

  /* return frame state and DB to original state */
  CFRA = oldframe;
  BKE_scene_graph_update_for_newframe(depsgraph, bmain);
}

static void freeData(GpencilModifierData *md)
{
  VertexcolorGpencilModifierData *mmd = (VertexcolorGpencilModifierData *)md;
  if (mmd->colorband) {
    MEM_freeN(mmd->colorband);
    mmd->colorband = NULL;
  }
}

static bool isDisabled(GpencilModifierData *md, int UNUSED(userRenderParams))
{
  VertexcolorGpencilModifierData *mmd = (VertexcolorGpencilModifierData *)md;

  return !mmd->object;
}

static void updateDepsgraph(GpencilModifierData *md, const ModifierUpdateDepsgraphContext *ctx)
{
  VertexcolorGpencilModifierData *lmd = (VertexcolorGpencilModifierData *)md;
  if (lmd->object != NULL) {
    DEG_add_object_relation(ctx->node, lmd->object, DEG_OB_COMP_GEOMETRY, "Vertexcolor Modifier");
    DEG_add_object_relation(ctx->node, lmd->object, DEG_OB_COMP_TRANSFORM, "Vertexcolor Modifier");
  }
  DEG_add_object_relation(ctx->node, ctx->object, DEG_OB_COMP_TRANSFORM, "Vertexcolor Modifier");
}

static void foreachObjectLink(GpencilModifierData *md,
                              Object *ob,
                              ObjectWalkFunc walk,
                              void *userData)
{
  VertexcolorGpencilModifierData *mmd = (VertexcolorGpencilModifierData *)md;

  walk(userData, ob, &mmd->object, IDWALK_CB_NOP);
}

GpencilModifierTypeInfo modifierType_Gpencil_Vertexcolor = {
    /* name */ "Vertexcolor",
    /* structName */ "VertexcolorGpencilModifierData",
    /* structSize */ sizeof(VertexcolorGpencilModifierData),
    /* type */ eGpencilModifierTypeType_Gpencil,
    /* flags */ eGpencilModifierTypeFlag_SupportsEditmode,

    /* copyData */ copyData,

    /* deformStroke */ deformStroke,
    /* generateStrokes */ NULL,
    /* bakeModifier */ bakeModifier,
    /* remapTime */ NULL,

    /* initData */ initData,
    /* freeData */ freeData,
    /* isDisabled */ isDisabled,
    /* updateDepsgraph */ updateDepsgraph,
    /* dependsOnTime */ NULL,
    /* foreachObjectLink */ foreachObjectLink,
    /* foreachIDLink */ NULL,
    /* foreachTexLink */ NULL,
};
