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
 * Copyright 2016, Blender Foundation.
 */

/** \file
 * \ingroup DNA
 */

#include "DRW_render.h"

#include "BLI_bitmap.h"
#include "BLI_dynstr.h"
#include "BLI_rand.h"
#include "BLI_rect.h"

#include "BKE_object.h"

#include "DEG_depsgraph_query.h"

#include "eevee_private.h"

#define SH_CASTER_ALLOC_CHUNK 32

// #define DEBUG_CSM
// #define DEBUG_SHADOW_DISTRIBUTION

static struct {
  struct GPUShader *shadow_sh;
} e_data = {NULL}; /* Engine data */

extern char datatoc_shadow_vert_glsl[];
extern char datatoc_shadow_frag_glsl[];

extern char datatoc_common_view_lib_glsl[];

/* Prototypes */
static void eevee_light_setup(Object *ob, EEVEE_Light *evli);
static void eevee_contact_shadow_setup(const Light *la, EEVEE_Shadow *evsh);
static float light_attenuation_radius_get(Light *la, float light_threshold);

/* *********** FUNCTIONS *********** */

void EEVEE_lights_init(EEVEE_ViewLayerData *sldata)
{
  const uint shadow_ubo_size = sizeof(EEVEE_Shadow) * MAX_SHADOW +
                               sizeof(EEVEE_ShadowCube) * MAX_SHADOW_CUBE +
                               sizeof(EEVEE_ShadowCascade) * MAX_SHADOW_CASCADE;

  const DRWContextState *draw_ctx = DRW_context_state_get();
  const Scene *scene_eval = DEG_get_evaluated_scene(draw_ctx->depsgraph);

  if (!e_data.shadow_sh) {
    e_data.shadow_sh = DRW_shader_create_with_lib(datatoc_shadow_vert_glsl,
                                                  NULL,
                                                  datatoc_shadow_frag_glsl,
                                                  datatoc_common_view_lib_glsl,
                                                  NULL);
  }

  if (!sldata->lights) {
    sldata->lights = MEM_callocN(sizeof(EEVEE_LightsInfo), "EEVEE_LightsInfo");
    sldata->light_ubo = DRW_uniformbuffer_create(sizeof(EEVEE_Light) * MAX_LIGHT, NULL);
    sldata->shadow_ubo = DRW_uniformbuffer_create(shadow_ubo_size, NULL);

    for (int i = 0; i < 2; ++i) {
      sldata->shcasters_buffers[i].bbox = MEM_callocN(
          sizeof(EEVEE_BoundBox) * SH_CASTER_ALLOC_CHUNK, __func__);
      sldata->shcasters_buffers[i].update = BLI_BITMAP_NEW(SH_CASTER_ALLOC_CHUNK, __func__);
      sldata->shcasters_buffers[i].alloc_count = SH_CASTER_ALLOC_CHUNK;
      sldata->shcasters_buffers[i].count = 0;
    }
    sldata->lights->shcaster_frontbuffer = &sldata->shcasters_buffers[0];
    sldata->lights->shcaster_backbuffer = &sldata->shcasters_buffers[1];
  }

  /* Flip buffers */
  SWAP(EEVEE_ShadowCasterBuffer *,
       sldata->lights->shcaster_frontbuffer,
       sldata->lights->shcaster_backbuffer);

  int sh_cube_size = scene_eval->eevee.shadow_cube_size;
  int sh_cascade_size = scene_eval->eevee.shadow_cascade_size;
  const bool sh_high_bitdepth = (scene_eval->eevee.flag & SCE_EEVEE_SHADOW_HIGH_BITDEPTH) != 0;
  sldata->lights->soft_shadows = (scene_eval->eevee.flag & SCE_EEVEE_SHADOW_SOFT) != 0;

  EEVEE_LightsInfo *linfo = sldata->lights;
  if ((linfo->shadow_cube_size != sh_cube_size) ||
      (linfo->shadow_high_bitdepth != sh_high_bitdepth)) {
    BLI_assert((sh_cube_size > 0) && (sh_cube_size <= 4096));
    DRW_TEXTURE_FREE_SAFE(sldata->shadow_cube_pool);
    CLAMP(sh_cube_size, 1, 4096);
  }

  if ((linfo->shadow_cascade_size != sh_cascade_size) ||
      (linfo->shadow_high_bitdepth != sh_high_bitdepth)) {
    BLI_assert((sh_cascade_size > 0) && (sh_cascade_size <= 4096));
    DRW_TEXTURE_FREE_SAFE(sldata->shadow_cascade_pool);
    CLAMP(sh_cascade_size, 1, 4096);
  }

  linfo->shadow_high_bitdepth = sh_high_bitdepth;
  linfo->shadow_cube_size = sh_cube_size;
  linfo->shadow_cascade_size = sh_cascade_size;
}

void EEVEE_lights_cache_init(EEVEE_ViewLayerData *sldata, EEVEE_Data *vedata)
{
  EEVEE_LightsInfo *linfo = sldata->lights;
  EEVEE_StorageList *stl = vedata->stl;
  EEVEE_PassList *psl = vedata->psl;

  EEVEE_ShadowCasterBuffer *backbuffer = linfo->shcaster_backbuffer;
  EEVEE_ShadowCasterBuffer *frontbuffer = linfo->shcaster_frontbuffer;

  frontbuffer->count = 0;
  linfo->num_light = 0;
  linfo->num_cube_layer = 0;
  linfo->num_cascade_layer = 0;
  linfo->cube_len = linfo->cascade_len = linfo->shadow_len = 0;

  /* Shadow Casters: Reset flags. */
  BLI_bitmap_set_all(backbuffer->update, true, backbuffer->alloc_count);
  /* Is this one needed? */
  BLI_bitmap_set_all(frontbuffer->update, false, frontbuffer->alloc_count);

  INIT_MINMAX(linfo->shcaster_aabb.min, linfo->shcaster_aabb.max);

  psl->shadow_cube_store_pass = NULL;
  psl->shadow_cube_store_high_pass = NULL;
  psl->shadow_cascade_store_pass = NULL;
  psl->shadow_cascade_store_high_pass = NULL;

  {
    DRWState state = DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL | DRW_STATE_SHADOW_OFFSET;
    DRW_PASS_CREATE(psl->shadow_pass, state);

    stl->g_data->shadow_shgrp = DRW_shgroup_create(e_data.shadow_sh, psl->shadow_pass);
  }
}

void EEVEE_lights_cache_add(EEVEE_ViewLayerData *sldata, Object *ob)
{
  EEVEE_LightsInfo *linfo = sldata->lights;

  /* Step 1 find all lights in the scene and setup them */
  if (linfo->num_light >= MAX_LIGHT) {
    printf("Too many lights in the scene !!!\n");
  }
  else {
    const Light *la = (Light *)ob->data;

    /* Early out if light has no power. */
    if (la->energy == 0.0f || is_zero_v3(&la->r)) {
      return;
    }

    EEVEE_Light *evli = linfo->light_data + linfo->num_light;
    eevee_light_setup(ob, evli);

    if (la->mode & LA_SHADOW) {
      if (la->type == LA_SUN) {
        if (linfo->cascade_len < MAX_SHADOW_CASCADE) {
          EEVEE_Shadow *sh_data = linfo->shadow_data + linfo->shadow_len;
          EEVEE_ShadowCascade *csm_data = linfo->shadow_cascade_data + linfo->cascade_len;
          EEVEE_ShadowCascadeRender *csm_render = linfo->shadow_cascade_render +
                                                  linfo->cascade_len;

          eevee_contact_shadow_setup(la, sh_data);

          linfo->shadow_cascade_light_indices[linfo->cascade_len] = linfo->num_light;
          evli->shadow_id = linfo->shadow_len++;
          sh_data->type_data_id = linfo->cascade_len++;
          csm_data->tex_id = linfo->num_cascade_layer;
          csm_render->cascade_fade = la->cascade_fade;
          csm_render->cascade_count = la->cascade_count;
          csm_render->cascade_exponent = la->cascade_exponent;
          csm_render->cascade_max_dist = la->cascade_max_dist;

          linfo->num_cascade_layer += la->cascade_count;
        }
      }
      else if (ELEM(la->type, LA_SPOT, LA_LOCAL, LA_AREA)) {
        if (linfo->cube_len < MAX_SHADOW_CUBE) {
          EEVEE_Shadow *sh_data = linfo->shadow_data + linfo->shadow_len;

          /* Always update dupli lights as EEVEE_LightEngineData is not saved.
           * Same issue with dupli shadow casters. */
          bool update = (ob->base_flag & BASE_FROM_DUPLI) != 0;
          if (!update) {
            EEVEE_LightEngineData *led = EEVEE_light_data_ensure(ob);
            if (led->need_update) {
              update = true;
              led->need_update = false;
            }
          }

          if (update) {
            BLI_BITMAP_ENABLE(&linfo->sh_cube_update[0], linfo->cube_len);
          }

          sh_data->near = max_ff(la->clipsta, 1e-8f);
          eevee_contact_shadow_setup(la, sh_data);

          /* Saving light bounds for later. */
          BoundSphere *cube_bound = linfo->shadow_bounds + linfo->cube_len;
          copy_v3_v3(cube_bound->center, evli->position);
          cube_bound->radius = sqrt(1.0f / evli->invsqrdist);

          linfo->shadow_cube_light_indices[linfo->cube_len] = linfo->num_light;
          evli->shadow_id = linfo->shadow_len++;
          sh_data->type_data_id = linfo->cube_len++;

          /* Same as linfo->cube_len, no need to save. */
          linfo->num_cube_layer++;
        }
      }
    }

    linfo->num_light++;
  }
}

/* Add a shadow caster to the shadowpasses */
void EEVEE_lights_cache_shcaster_add(EEVEE_ViewLayerData *UNUSED(sldata),
                                     EEVEE_StorageList *stl,
                                     struct GPUBatch *geom,
                                     Object *ob)
{
  DRW_shgroup_call(stl->g_data->shadow_shgrp, geom, ob);
}

void EEVEE_lights_cache_shcaster_material_add(EEVEE_ViewLayerData *sldata,
                                              EEVEE_PassList *psl,
                                              struct GPUMaterial *gpumat,
                                              struct GPUBatch *geom,
                                              struct Object *ob,
                                              const float *alpha_threshold)
{
  /* TODO / PERF : reuse the same shading group for objects with the same material */
  DRWShadingGroup *grp = DRW_shgroup_material_create(gpumat, psl->shadow_pass);

  if (grp == NULL) {
    return;
  }

  /* Grrr needed for correctness but not 99% of the time not needed.
   * TODO detect when needed? */
  DRW_shgroup_uniform_block(grp, "probe_block", sldata->probe_ubo);
  DRW_shgroup_uniform_block(grp, "grid_block", sldata->grid_ubo);
  DRW_shgroup_uniform_block(grp, "planar_block", sldata->planar_ubo);
  DRW_shgroup_uniform_block(grp, "light_block", sldata->light_ubo);
  DRW_shgroup_uniform_block(grp, "shadow_block", sldata->shadow_ubo);
  DRW_shgroup_uniform_block(grp, "common_block", sldata->common_ubo);

  if (alpha_threshold != NULL) {
    DRW_shgroup_uniform_float(grp, "alphaThreshold", alpha_threshold, 1);
  }

  DRW_shgroup_call(grp, geom, ob);
}

/* Make that object update shadow casting lights inside its influence bounding box. */
void EEVEE_lights_cache_shcaster_object_add(EEVEE_ViewLayerData *sldata, Object *ob)
{
  EEVEE_LightsInfo *linfo = sldata->lights;
  EEVEE_ShadowCasterBuffer *backbuffer = linfo->shcaster_backbuffer;
  EEVEE_ShadowCasterBuffer *frontbuffer = linfo->shcaster_frontbuffer;
  bool update = true;
  int id = frontbuffer->count;

  /* Make sure shadow_casters is big enough. */
  if (id + 1 >= frontbuffer->alloc_count) {
    frontbuffer->alloc_count += SH_CASTER_ALLOC_CHUNK;
    frontbuffer->bbox = MEM_reallocN(frontbuffer->bbox,
                                     sizeof(EEVEE_BoundBox) * frontbuffer->alloc_count);
    BLI_BITMAP_RESIZE(frontbuffer->update, frontbuffer->alloc_count);
  }

  if (ob->base_flag & BASE_FROM_DUPLI) {
    /* Duplis will always refresh the shadowmaps as if they were deleted each frame. */
    /* TODO(fclem) fix this. */
    update = true;
  }
  else {
    EEVEE_ObjectEngineData *oedata = EEVEE_object_data_ensure(ob);
    int past_id = oedata->shadow_caster_id;
    oedata->shadow_caster_id = id;
    /* Update flags in backbuffer. */
    if (past_id > -1 && past_id < backbuffer->count) {
      BLI_BITMAP_SET(backbuffer->update, past_id, oedata->need_update);
    }
    update = oedata->need_update;
    oedata->need_update = false;
  }

  if (update) {
    BLI_BITMAP_ENABLE(frontbuffer->update, id);
  }

  /* Update World AABB in frontbuffer. */
  BoundBox *bb = BKE_object_boundbox_get(ob);
  float min[3], max[3];
  INIT_MINMAX(min, max);
  for (int i = 0; i < 8; ++i) {
    float vec[3];
    copy_v3_v3(vec, bb->vec[i]);
    mul_m4_v3(ob->obmat, vec);
    minmax_v3v3_v3(min, max, vec);
  }

  EEVEE_BoundBox *aabb = &frontbuffer->bbox[id];
  add_v3_v3v3(aabb->center, min, max);
  mul_v3_fl(aabb->center, 0.5f);
  sub_v3_v3v3(aabb->halfdim, aabb->center, max);

  aabb->halfdim[0] = fabsf(aabb->halfdim[0]);
  aabb->halfdim[1] = fabsf(aabb->halfdim[1]);
  aabb->halfdim[2] = fabsf(aabb->halfdim[2]);

  minmax_v3v3_v3(linfo->shcaster_aabb.min, linfo->shcaster_aabb.max, min);
  minmax_v3v3_v3(linfo->shcaster_aabb.min, linfo->shcaster_aabb.max, max);

  frontbuffer->count++;
}

void EEVEE_lights_cache_finish(EEVEE_ViewLayerData *sldata, EEVEE_Data *vedata)
{
  EEVEE_LightsInfo *linfo = sldata->lights;
  eGPUTextureFormat shadow_pool_format = (linfo->shadow_high_bitdepth) ? GPU_DEPTH_COMPONENT24 :
                                                                         GPU_DEPTH_COMPONENT16;

  sldata->common_data.la_num_light = linfo->num_light;

  /* Setup enough layers. */
  /* Free textures if number mismatch. */
  if (linfo->num_cube_layer != linfo->cache_num_cube_layer) {
    DRW_TEXTURE_FREE_SAFE(sldata->shadow_cube_pool);
    linfo->cache_num_cube_layer = linfo->num_cube_layer;
    linfo->update_flag |= LIGHT_UPDATE_SHADOW_CUBE;
  }

  if (linfo->num_cascade_layer != linfo->cache_num_cascade_layer) {
    DRW_TEXTURE_FREE_SAFE(sldata->shadow_cascade_pool);
    linfo->cache_num_cascade_layer = linfo->num_cascade_layer;
  }

  if (!sldata->shadow_cube_pool) {
    /* TODO shadowcube array. */
    int cube_size = linfo->shadow_cube_size + ((true) ? 2 : 0);
    sldata->shadow_cube_pool = DRW_texture_create_2d_array(cube_size,
                                                           cube_size,
                                                           max_ii(1, linfo->num_cube_layer * 6),
                                                           shadow_pool_format,
                                                           DRW_TEX_FILTER | DRW_TEX_COMPARE,
                                                           NULL);
  }

  if (!sldata->shadow_cascade_pool) {
    sldata->shadow_cascade_pool = DRW_texture_create_2d_array(linfo->shadow_cascade_size,
                                                              linfo->shadow_cascade_size,
                                                              max_ii(1, linfo->num_cascade_layer),
                                                              shadow_pool_format,
                                                              DRW_TEX_FILTER | DRW_TEX_COMPARE,
                                                              NULL);
  }

  if (sldata->shadow_fb == NULL) {
    sldata->shadow_fb = GPU_framebuffer_create();
  }

  /* Update Lights UBOs. */
  EEVEE_lights_update(sldata, vedata);
}

void eevee_contact_shadow_setup(const Light *la, EEVEE_Shadow *evsh)
{
  evsh->contact_dist = (la->mode & LA_SHAD_CONTACT) ? la->contact_dist : 0.0f;
  evsh->contact_bias = 0.05f * la->contact_bias;
  evsh->contact_spread = la->contact_spread;
  evsh->contact_thickness = la->contact_thickness;
}

float light_attenuation_radius_get(Light *la, float light_threshold)
{
  if (la->mode & LA_CUSTOM_ATTENUATION) {
    return la->att_dist;
  }

  /* Compute max light power. */
  float power = max_fff(la->r, la->g, la->b);
  power *= fabsf(la->energy / 100.0f);
  power *= max_ff(1.0f, la->spec_fac);
  /* Compute the distance (using the inverse square law)
   * at which the light power reaches the light_threshold. */
  float distance = sqrtf(max_ff(1e-16, power / max_ff(1e-16, light_threshold)));
  return distance;
}

static void light_shape_parameters_set(EEVEE_Light *evli, const Light *la, float scale[3])
{
  if (la->type == LA_SPOT) {
    /* Spot size & blend */
    evli->sizex = scale[0] / scale[2];
    evli->sizey = scale[1] / scale[2];
    evli->spotsize = cosf(la->spotsize * 0.5f);
    evli->spotblend = (1.0f - evli->spotsize) * la->spotblend;
    evli->radius = max_ff(0.001f, la->area_size);
  }
  else if (la->type == LA_AREA) {
    evli->sizex = max_ff(0.003f, la->area_size * scale[0] * 0.5f);
    if (ELEM(la->area_shape, LA_AREA_RECT, LA_AREA_ELLIPSE)) {
      evli->sizey = max_ff(0.003f, la->area_sizey * scale[1] * 0.5f);
    }
    else {
      evli->sizey = max_ff(0.003f, la->area_size * scale[1] * 0.5f);
    }
  }
  else if (la->type == LA_SUN) {
    evli->radius = max_ff(0.001f, tanf(la->sun_angle / 2.0f));
  }
  else {
    evli->radius = max_ff(0.001f, la->area_size);
  }
}

static float light_shape_power_get(const Light *la, const EEVEE_Light *evli)
{
  float power;
  /* Make illumination power constant */
  if (la->type == LA_AREA) {
    power = 1.0f / (evli->sizex * evli->sizey * 4.0f * M_PI) * /* 1/(w*h*Pi) */
            0.8f; /* XXX : Empirical, Fit cycles power */
    if (ELEM(la->area_shape, LA_AREA_DISK, LA_AREA_ELLIPSE)) {
      /* Scale power to account for the lower area of the ellipse compared to the surrounding
       * rectangle. */
      power *= 4.0f / M_PI;
    }
  }
  else if (la->type == LA_SPOT || la->type == LA_LOCAL) {
    power = 1.0f / (4.0f * evli->radius * evli->radius * M_PI * M_PI); /* 1/(4*r²*Pi²) */

    /* for point lights (a.k.a radius == 0.0) */
    // power = M_PI * M_PI * 0.78; /* XXX : Empirical, Fit cycles power */
  }
  else {
    power = 1.0f / (evli->radius * evli->radius * M_PI); /* 1/(r²*Pi) */
    /* Make illumination power closer to cycles for bigger radii. Cycles uses a cos^3 term that we
     * cannot reproduce so we account for that by scaling the light power. This function is the
     * result of a rough manual fitting. */
    power += 1.0f / (2.0f * M_PI); /* power *= 1 + r²/2 */
  }
  return power;
}

/* Update buffer with light data */
static void eevee_light_setup(Object *ob, EEVEE_Light *evli)
{
  Light *la = (Light *)ob->data;
  float mat[4][4], scale[3], power, att_radius;

  const DRWContextState *draw_ctx = DRW_context_state_get();
  const float light_threshold = draw_ctx->scene->eevee.light_threshold;

  /* Position */
  copy_v3_v3(evli->position, ob->obmat[3]);

  /* Color */
  copy_v3_v3(evli->color, &la->r);

  evli->spec = la->spec_fac;

  /* Influence Radius */
  att_radius = light_attenuation_radius_get(la, light_threshold);
  /* Take the inverse square of this distance. */
  evli->invsqrdist = 1.0 / max_ff(1e-4f, att_radius * att_radius);

  /* Vectors */
  normalize_m4_m4_ex(mat, ob->obmat, scale);
  copy_v3_v3(evli->forwardvec, mat[2]);
  normalize_v3(evli->forwardvec);
  negate_v3(evli->forwardvec);

  copy_v3_v3(evli->rightvec, mat[0]);
  normalize_v3(evli->rightvec);

  copy_v3_v3(evli->upvec, mat[1]);
  normalize_v3(evli->upvec);

  /* Make sure we have a consistent Right Hand coord frame.
   * (in case of negatively scaled Z axis) */
  float cross[3];
  cross_v3_v3v3(cross, evli->rightvec, evli->forwardvec);
  if (dot_v3v3(cross, evli->upvec) < 0.0) {
    negate_v3(evli->upvec);
  }

  light_shape_parameters_set(evli, la, scale);

  /* Light Type */
  evli->light_type = (float)la->type;
  if ((la->type == LA_AREA) && ELEM(la->area_shape, LA_AREA_DISK, LA_AREA_ELLIPSE)) {
    evli->light_type = LAMPTYPE_AREA_ELLIPSE;
  }

  power = light_shape_power_get(la, evli);
  mul_v3_fl(evli->color, power * la->energy);

  /* No shadow by default */
  evli->shadow_id = -1.0f;
}

/**
 * Special ball distribution:
 * Point are distributed in a way that when they are orthogonally
 * projected into any plane, the resulting distribution is (close to)
 * a uniform disc distribution.
 */
static void sample_ball(int sample_ofs, float radius, float rsample[3])
{
  double ht_point[3];
  double ht_offset[3] = {0.0, 0.0, 0.0};
  uint ht_primes[3] = {2, 3, 7};

  BLI_halton_3d(ht_primes, ht_offset, sample_ofs, ht_point);

  float omega = ht_point[1] * 2.0f * M_PI;

  rsample[2] = ht_point[0] * 2.0f - 1.0f; /* cos theta */

  float r = sqrtf(fmaxf(0.0f, 1.0f - rsample[2] * rsample[2])); /* sin theta */

  rsample[0] = r * cosf(omega);
  rsample[1] = r * sinf(omega);

  radius *= sqrt(sqrt(ht_point[2]));
  mul_v3_fl(rsample, radius);
}

static void sample_rectangle(
    int sample_ofs, float x_axis[3], float y_axis[3], float size_x, float size_y, float rsample[3])
{
  double ht_point[2];
  double ht_offset[2] = {0.0, 0.0};
  uint ht_primes[2] = {2, 3};

  BLI_halton_2d(ht_primes, ht_offset, sample_ofs, ht_point);

  /* Change ditribution center to be 0,0 */
  ht_point[0] = (ht_point[0] > 0.5f) ? ht_point[0] - 1.0f : ht_point[0];
  ht_point[1] = (ht_point[1] > 0.5f) ? ht_point[1] - 1.0f : ht_point[1];

  zero_v3(rsample);
  madd_v3_v3fl(rsample, x_axis, (ht_point[0] * 2.0f) * size_x);
  madd_v3_v3fl(rsample, y_axis, (ht_point[1] * 2.0f) * size_y);
}

static void sample_ellipse(
    int sample_ofs, float x_axis[3], float y_axis[3], float size_x, float size_y, float rsample[3])
{
  double ht_point[2];
  double ht_offset[2] = {0.0, 0.0};
  uint ht_primes[2] = {2, 3};

  BLI_halton_2d(ht_primes, ht_offset, sample_ofs, ht_point);

  /* Uniform disc sampling. */
  float omega = ht_point[1] * 2.0f * M_PI;
  float r = sqrtf(ht_point[0]);
  ht_point[0] = r * cosf(omega) * size_x;
  ht_point[1] = r * sinf(omega) * size_y;

  zero_v3(rsample);
  madd_v3_v3fl(rsample, x_axis, ht_point[0]);
  madd_v3_v3fl(rsample, y_axis, ht_point[1]);
}

static void shadow_cube_random_position_set(EEVEE_Light *evli,
                                            int sample_ofs,
                                            float ws_sample_pos[3])
{
  float jitter[3];

#ifndef DEBUG_SHADOW_DISTRIBUTION
  int i = sample_ofs;
#else
  for (int i = 0; i <= sample_ofs; ++i) {
#endif
  switch ((int)evli->light_type) {
    case LA_AREA:
      sample_rectangle(i, evli->rightvec, evli->upvec, evli->sizex, evli->sizey, jitter);
      break;
    case (int)LAMPTYPE_AREA_ELLIPSE:
      sample_ellipse(i, evli->rightvec, evli->upvec, evli->sizex, evli->sizey, jitter);
      break;
    default:
      sample_ball(i, evli->radius, jitter);
  }
#ifdef DEBUG_SHADOW_DISTRIBUTION
  float p[3];
  add_v3_v3v3(p, jitter, ws_sample_pos);
  DRW_debug_sphere(p, 0.01f, (float[4]){1.0f, (sample_ofs == i) ? 1.0f : 0.0f, 0.0f, 1.0f});
}
#endif
add_v3_v3(ws_sample_pos, jitter);
}

/* Return true if sample has changed and light needs to be updated. */
static bool eevee_shadow_cube_setup(EEVEE_LightsInfo *linfo, EEVEE_Light *evli, int sample_ofs)
{
  EEVEE_Shadow *shdw_data = linfo->shadow_data + (int)evli->shadow_id;
  EEVEE_ShadowCube *cube_data = linfo->shadow_cube_data + (int)shdw_data->type_data_id;

  copy_v3_v3(cube_data->shadowmat[0], evli->rightvec);
  copy_v3_v3(cube_data->shadowmat[1], evli->upvec);
  negate_v3_v3(cube_data->shadowmat[2], evli->forwardvec);
  copy_v3_v3(cube_data->shadowmat[3], evli->position);
  cube_data->shadowmat[3][3] = 1.0f;

  bool update = false;

  if (linfo->soft_shadows) {
    shadow_cube_random_position_set(evli, sample_ofs, cube_data->shadowmat[3]);
    /* Update if position changes (avoid infinite update if soft shadows does not move).
     * Other changes are caught by depsgraph tagging. This one is for update between samples. */
    update = !compare_v3v3(cube_data->shadowmat[3], cube_data->position, 1e-10f);
  }

  copy_v3_v3(cube_data->position, cube_data->shadowmat[3]);
  invert_m4(cube_data->shadowmat);

  shdw_data->far = max_ff(sqrt(1.0f / evli->invsqrdist), 3e-4);
  shdw_data->near = min_ff(shdw_data->near, shdw_data->far - 1e-4);

  return update;
}

static void shadow_cascade_random_matrix_set(float mat[4][4], float radius, int sample_ofs)
{
  float jitter[3];

#ifndef DEBUG_SHADOW_DISTRIBUTION
  int i = sample_ofs;
#else
    for (int i = 0; i <= sample_ofs; ++i) {
#endif
  sample_ellipse(i, mat[0], mat[1], radius, radius, jitter);
#ifdef DEBUG_SHADOW_DISTRIBUTION
  float p[3];
  add_v3_v3v3(p, jitter, mat[2]);
  DRW_debug_sphere(p, 0.01f, (float[4]){1.0f, (sample_ofs == i) ? 1.0f : 0.0f, 0.0f, 1.0f});
}
#endif

add_v3_v3(mat[2], jitter);
orthogonalize_m4(mat, 2);
}

#define LERP(t, a, b) ((a) + (t) * ((b) - (a)))

static double round_to_digits(double value, int digits)
{
  double factor = pow(10.0, digits - ceil(log10(fabs(value))));
  return round(value * factor) / factor;
}

static void frustum_min_bounding_sphere(const float corners[8][3],
                                        float r_center[3],
                                        float *r_radius)
{
#if 0 /* Simple solution but waste too much space. */
  float minvec[3], maxvec[3];

  /* compute the bounding box */
  INIT_MINMAX(minvec, maxvec);
  for (int i = 0; i < 8; ++i) {
    minmax_v3v3_v3(minvec, maxvec, corners[i]);
  }

  /* compute the bounding sphere of this box */
  r_radius = len_v3v3(minvec, maxvec) * 0.5f;
  add_v3_v3v3(r_center, minvec, maxvec);
  mul_v3_fl(r_center, 0.5f);
#else
      /* Find averaged center. */
      zero_v3(r_center);
      for (int i = 0; i < 8; ++i) {
        add_v3_v3(r_center, corners[i]);
      }
      mul_v3_fl(r_center, 1.0f / 8.0f);

      /* Search the largest distance from the sphere center. */
      *r_radius = 0.0f;
      for (int i = 0; i < 8; ++i) {
        float rad = len_squared_v3v3(corners[i], r_center);
        if (rad > *r_radius) {
          *r_radius = rad;
        }
      }

      /* TODO try to reduce the radius further by moving the center.
       * Remember we need a __stable__ solution! */

      /* Try to reduce float imprecision leading to shimmering. */
      *r_radius = (float)round_to_digits(sqrtf(*r_radius), 3);
#endif
}

static void eevee_shadow_cascade_setup(EEVEE_LightsInfo *linfo,
                                       EEVEE_Light *evli,
                                       DRWView *view,
                                       float view_near,
                                       float view_far,
                                       int sample_ofs)
{
  EEVEE_Shadow *shdw_data = linfo->shadow_data + (int)evli->shadow_id;
  EEVEE_ShadowCascade *csm_data = linfo->shadow_cascade_data + (int)shdw_data->type_data_id;
  EEVEE_ShadowCascadeRender *csm_render = linfo->shadow_cascade_render +
                                          (int)shdw_data->type_data_id;
  int cascade_nbr = csm_render->cascade_count;
  float cascade_fade = csm_render->cascade_fade;
  float cascade_max_dist = csm_render->cascade_max_dist;
  float cascade_exponent = csm_render->cascade_exponent;

  /* Camera Matrices */
  float persinv[4][4], vp_projmat[4][4];
  DRW_view_persmat_get(view, persinv, true);
  DRW_view_winmat_get(view, vp_projmat, false);
  bool is_persp = DRW_view_is_persp_get(view);

  /* obmat = Object Space > World Space */
  /* viewmat = World Space > View Space */
  float(*viewmat)[4] = csm_render->viewmat;
#if 0 /* done at culling time */
  normalize_m4_m4(viewmat, ob->obmat);
#endif

  if (linfo->soft_shadows) {
    shadow_cascade_random_matrix_set(viewmat, evli->radius, sample_ofs);
  }

  copy_m4_m4(csm_render->viewinv, viewmat);
  invert_m4(viewmat);

  copy_v3_v3(csm_data->shadow_vec, csm_render->viewinv[2]);

  /* Compute near and far value based on all shadow casters cumulated AABBs. */
  float sh_near = -1.0e30f, sh_far = 1.0e30f;
  BoundBox shcaster_bounds;
  BKE_boundbox_init_from_minmax(
      &shcaster_bounds, linfo->shcaster_aabb.min, linfo->shcaster_aabb.max);
#ifdef DEBUG_CSM
  float dbg_col1[4] = {1.0f, 0.5f, 0.6f, 1.0f};
  DRW_debug_bbox(&shcaster_bounds, dbg_col1);
#endif
  for (int i = 0; i < 8; i++) {
    mul_m4_v3(viewmat, shcaster_bounds.vec[i]);
    sh_near = max_ff(sh_near, shcaster_bounds.vec[i][2]);
    sh_far = min_ff(sh_far, shcaster_bounds.vec[i][2]);
  }
#ifdef DEBUG_CSM
  float dbg_col2[4] = {0.5f, 1.0f, 0.6f, 1.0f};
  float pts[2][3] = {{0.0, 0.0, sh_near}, {0.0, 0.0, sh_far}};
  mul_m4_v3(csm_render->viewinv, pts[0]);
  mul_m4_v3(csm_render->viewinv, pts[1]);
  DRW_debug_sphere(pts[0], 1.0f, dbg_col1);
  DRW_debug_sphere(pts[1], 1.0f, dbg_col2);
#endif
  /* The rest of the function is assuming inverted Z. */
  /* Add a little bias to avoid invalid matrices. */
  sh_far = -(sh_far - 1e-3);
  sh_near = -sh_near;

  /* The technique consists into splitting
   * the view frustum into several sub-frustum
   * that are individually receiving one shadow map */

  float csm_start, csm_end;

  if (is_persp) {
    csm_start = view_near;
    csm_end = max_ff(view_far, -cascade_max_dist);
    /* Avoid artifacts */
    csm_end = min_ff(view_near, csm_end);
  }
  else {
    csm_start = -view_far;
    csm_end = view_far;
  }

  /* init near/far */
  for (int c = 0; c < MAX_CASCADE_NUM; ++c) {
    csm_data->split_start[c] = csm_end;
    csm_data->split_end[c] = csm_end;
  }

  /* Compute split planes */
  float splits_start_ndc[MAX_CASCADE_NUM];
  float splits_end_ndc[MAX_CASCADE_NUM];

  {
    /* Nearest plane */
    float p[4] = {1.0f, 1.0f, csm_start, 1.0f};
    /* TODO: we don't need full m4 multiply here */
    mul_m4_v4(vp_projmat, p);
    splits_start_ndc[0] = p[2];
    if (is_persp) {
      splits_start_ndc[0] /= p[3];
    }
  }

  {
    /* Farthest plane */
    float p[4] = {1.0f, 1.0f, csm_end, 1.0f};
    /* TODO: we don't need full m4 multiply here */
    mul_m4_v4(vp_projmat, p);
    splits_end_ndc[cascade_nbr - 1] = p[2];
    if (is_persp) {
      splits_end_ndc[cascade_nbr - 1] /= p[3];
    }
  }

  csm_data->split_start[0] = csm_start;
  csm_data->split_end[cascade_nbr - 1] = csm_end;

  for (int c = 1; c < cascade_nbr; ++c) {
    /* View Space */
    float linear_split = LERP(((float)(c) / (float)cascade_nbr), csm_start, csm_end);
    float exp_split = csm_start * powf(csm_end / csm_start, (float)(c) / (float)cascade_nbr);

    if (is_persp) {
      csm_data->split_start[c] = LERP(cascade_exponent, linear_split, exp_split);
    }
    else {
      csm_data->split_start[c] = linear_split;
    }
    csm_data->split_end[c - 1] = csm_data->split_start[c];

    /* Add some overlap for smooth transition */
    csm_data->split_start[c] = LERP(cascade_fade,
                                    csm_data->split_end[c - 1],
                                    (c > 1) ? csm_data->split_end[c - 2] :
                                              csm_data->split_start[0]);

    /* NDC Space */
    {
      float p[4] = {1.0f, 1.0f, csm_data->split_start[c], 1.0f};
      /* TODO: we don't need full m4 multiply here */
      mul_m4_v4(vp_projmat, p);
      splits_start_ndc[c] = p[2];

      if (is_persp) {
        splits_start_ndc[c] /= p[3];
      }
    }

    {
      float p[4] = {1.0f, 1.0f, csm_data->split_end[c - 1], 1.0f};
      /* TODO: we don't need full m4 multiply here */
      mul_m4_v4(vp_projmat, p);
      splits_end_ndc[c - 1] = p[2];

      if (is_persp) {
        splits_end_ndc[c - 1] /= p[3];
      }
    }
  }

  /* Set last cascade split fade distance into the first split_start. */
  float prev_split = (cascade_nbr > 1) ? csm_data->split_end[cascade_nbr - 2] :
                                         csm_data->split_start[0];
  csm_data->split_start[0] = LERP(cascade_fade, csm_data->split_end[cascade_nbr - 1], prev_split);

  /* For each cascade */
  for (int c = 0; c < cascade_nbr; ++c) {
    float(*projmat)[4] = csm_render->projmat[c];
    /* Given 8 frustum corners */
    float corners[8][3] = {
        /* Near Cap */
        {1.0f, -1.0f, splits_start_ndc[c]},
        {-1.0f, -1.0f, splits_start_ndc[c]},
        {-1.0f, 1.0f, splits_start_ndc[c]},
        {1.0f, 1.0f, splits_start_ndc[c]},
        /* Far Cap */
        {1.0f, -1.0f, splits_end_ndc[c]},
        {-1.0f, -1.0f, splits_end_ndc[c]},
        {-1.0f, 1.0f, splits_end_ndc[c]},
        {1.0f, 1.0f, splits_end_ndc[c]},
    };

    /* Transform them into world space */
    for (int i = 0; i < 8; ++i) {
      mul_project_m4_v3(persinv, corners[i]);
    }

    float center[3];
    frustum_min_bounding_sphere(corners, center, &(csm_render->radius[c]));

#ifdef DEBUG_CSM
    float dbg_col[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (c < 3) {
      dbg_col[c] = 1.0f;
    }
    DRW_debug_bbox((BoundBox *)&corners, dbg_col);
    DRW_debug_sphere(center, csm_render->radius[c], dbg_col);
#endif

    /* Project into lightspace */
    mul_m4_v3(viewmat, center);

    /* Snap projection center to nearest texel to cancel shimmering. */
    float shadow_origin[2], shadow_texco[2];
    /* Light to texture space. */
    mul_v2_v2fl(
        shadow_origin, center, linfo->shadow_cascade_size / (2.0f * csm_render->radius[c]));

    /* Find the nearest texel. */
    shadow_texco[0] = roundf(shadow_origin[0]);
    shadow_texco[1] = roundf(shadow_origin[1]);

    /* Compute offset. */
    sub_v2_v2(shadow_texco, shadow_origin);
    /* Texture to light space. */
    mul_v2_fl(shadow_texco, (2.0f * csm_render->radius[c]) / linfo->shadow_cascade_size);

    /* Apply offset. */
    add_v2_v2(center, shadow_texco);

    /* Expand the projection to cover frustum range */
    rctf rect_cascade;
    BLI_rctf_init_pt_radius(&rect_cascade, center, csm_render->radius[c]);
    orthographic_m4(projmat,
                    rect_cascade.xmin,
                    rect_cascade.xmax,
                    rect_cascade.ymin,
                    rect_cascade.ymax,
                    sh_near,
                    sh_far);

    float viewprojmat[4][4];
    mul_m4_m4m4(viewprojmat, projmat, viewmat);
    mul_m4_m4m4(csm_data->shadowmat[c], texcomat, viewprojmat);

#ifdef DEBUG_CSM
    DRW_debug_m4_as_bbox(viewprojmat, dbg_col, true);
#endif
  }

  shdw_data->near = sh_near;
  shdw_data->far = sh_far;
}

/* Used for checking if object is inside the shadow volume. */
static bool sphere_bbox_intersect(const BoundSphere *bs, const EEVEE_BoundBox *bb)
{
  /* We are testing using a rougher AABB vs AABB test instead of full AABB vs Sphere. */
  /* TODO test speed with AABB vs Sphere. */
  bool x = fabsf(bb->center[0] - bs->center[0]) <= (bb->halfdim[0] + bs->radius);
  bool y = fabsf(bb->center[1] - bs->center[1]) <= (bb->halfdim[1] + bs->radius);
  bool z = fabsf(bb->center[2] - bs->center[2]) <= (bb->halfdim[2] + bs->radius);

  return x && y && z;
}

void EEVEE_lights_update(EEVEE_ViewLayerData *sldata, EEVEE_Data *vedata)
{
  EEVEE_StorageList *stl = vedata->stl;
  EEVEE_EffectsInfo *effects = stl->effects;
  EEVEE_LightsInfo *linfo = sldata->lights;
  EEVEE_ShadowCasterBuffer *backbuffer = linfo->shcaster_backbuffer;
  EEVEE_ShadowCasterBuffer *frontbuffer = linfo->shcaster_frontbuffer;

  /* Gather all light own update bits. to avoid costly intersection check.  */
  for (int j = 0; j < linfo->cube_len; j++) {
    EEVEE_Light *evli = linfo->light_data + linfo->shadow_cube_light_indices[j];
    /* Setup shadow cube in UBO and tag for update if necessary. */
    if (eevee_shadow_cube_setup(linfo, evli, effects->taa_current_sample - 1)) {
      BLI_BITMAP_ENABLE(&linfo->sh_cube_update[0], j);
    }
  }

  if ((linfo->update_flag & LIGHT_UPDATE_SHADOW_CUBE) != 0) {
    /* Update all lights. */
    BLI_bitmap_set_all(&linfo->sh_cube_update[0], true, MAX_LIGHT);
  }
  else {
    /* TODO(fclem) This part can be slow, optimize it. */
    EEVEE_BoundBox *bbox = backbuffer->bbox;
    BoundSphere *bsphere = linfo->shadow_bounds;
    /* Search for deleted shadow casters or if shcaster WAS in shadow radius. */
    for (int i = 0; i < backbuffer->count; ++i) {
      /* If the shadowcaster has been deleted or updated. */
      if (BLI_BITMAP_TEST(backbuffer->update, i)) {
        for (int j = 0; j < linfo->cube_len; j++) {
          if (!BLI_BITMAP_TEST(&linfo->sh_cube_update[0], j)) {
            if (sphere_bbox_intersect(&bsphere[j], &bbox[i])) {
              BLI_BITMAP_ENABLE(&linfo->sh_cube_update[0], j);
            }
          }
        }
      }
    }
    /* Search for updates in current shadow casters. */
    bbox = frontbuffer->bbox;
    for (int i = 0; i < frontbuffer->count; i++) {
      /* If the shadowcaster has been deleted or updated. */
      if (BLI_BITMAP_TEST(frontbuffer->update, i)) {
        for (int j = 0; j < linfo->cube_len; j++) {
          if (!BLI_BITMAP_TEST(&linfo->sh_cube_update[0], j)) {
            if (sphere_bbox_intersect(&bsphere[j], &bbox[i])) {
              BLI_BITMAP_ENABLE(&linfo->sh_cube_update[0], j);
            }
          }
        }
      }
    }
  }

  /* Resize shcasters buffers if too big. */
  if (frontbuffer->alloc_count - frontbuffer->count > SH_CASTER_ALLOC_CHUNK) {
    frontbuffer->alloc_count = (frontbuffer->count / SH_CASTER_ALLOC_CHUNK) *
                               SH_CASTER_ALLOC_CHUNK;
    frontbuffer->alloc_count += (frontbuffer->count % SH_CASTER_ALLOC_CHUNK != 0) ?
                                    SH_CASTER_ALLOC_CHUNK :
                                    0;
    frontbuffer->bbox = MEM_reallocN(frontbuffer->bbox,
                                     sizeof(EEVEE_BoundBox) * frontbuffer->alloc_count);
    BLI_BITMAP_RESIZE(frontbuffer->update, frontbuffer->alloc_count);
  }
}

static void eevee_ensure_cube_views(
    float near, float far, int cube_res, const float viewmat[4][4], DRWView *view[6])
{
  float winmat[4][4];
  float side = near;

  /* TODO shadowcube array. */
  if (true) {
    /* This half texel offset is used to ensure correct filtering between faces. */
    /* FIXME: This exhibit float precision issue with lower cube_res.
     * But it seems to be caused by the perspective_m4. */
    side *= ((float)cube_res + 1.0f) / (float)(cube_res);
  }

  perspective_m4(winmat, -side, side, -side, side, near, far);

  for (int i = 0; i < 6; i++) {
    float tmp[4][4];
    mul_m4_m4m4(tmp, cubefacemat[i], viewmat);

    if (view[i] == NULL) {
      view[i] = DRW_view_create(tmp, winmat, NULL, NULL, NULL);
    }
    else {
      DRW_view_update(view[i], tmp, winmat, NULL, NULL);
    }
  }
}

static void eevee_ensure_cascade_views(EEVEE_ShadowCascadeRender *csm_render,
                                       DRWView *view[MAX_CASCADE_NUM])
{
  for (int i = 0; i < csm_render->cascade_count; i++) {
    if (view[i] == NULL) {
      view[i] = DRW_view_create(csm_render->viewmat, csm_render->projmat[i], NULL, NULL, NULL);
    }
    else {
      DRW_view_update(view[i], csm_render->viewmat, csm_render->projmat[i], NULL, NULL);
    }
  }
}

/* this refresh lights shadow buffers */
void EEVEE_draw_shadows(EEVEE_ViewLayerData *sldata, EEVEE_Data *vedata, DRWView *view)
{
  EEVEE_PassList *psl = vedata->psl;
  EEVEE_StorageList *stl = vedata->stl;
  EEVEE_EffectsInfo *effects = stl->effects;
  EEVEE_PrivateData *g_data = stl->g_data;
  EEVEE_LightsInfo *linfo = sldata->lights;

  int saved_ray_type = sldata->common_data.ray_type;

  /* TODO: make it optional if we don't draw shadows. */
  sldata->common_data.ray_type = EEVEE_RAY_SHADOW;
  DRW_uniformbuffer_update(sldata->common_ubo, &sldata->common_data);

  /* Precompute all shadow/view test before rendering and trashing the culling cache. */
  bool cube_visible[MAX_SHADOW_CUBE];
  for (int cube = 0; cube < linfo->cube_len; cube++) {
    cube_visible[cube] = DRW_culling_sphere_test(view, linfo->shadow_bounds + cube);
  }

  /* Cube Shadow Maps */
  DRW_stats_group_start("Cube Shadow Maps");
  /* Render each shadow to one layer of the array */
  for (int cube = 0; cube < linfo->cube_len; cube++) {
    if (!cube_visible[cube] || !BLI_BITMAP_TEST(linfo->sh_cube_update, cube)) {
      continue;
    }

    EEVEE_Light *evli = linfo->light_data + linfo->shadow_cube_light_indices[cube];
    EEVEE_Shadow *shdw_data = linfo->shadow_data + (int)evli->shadow_id;
    EEVEE_ShadowCube *cube_data = linfo->shadow_cube_data + (int)shdw_data->type_data_id;

    DRW_debug_sphere(evli->position, shdw_data->far, (const float[4]){1.0f, 0.0f, 0.0f, 1.0f});

    eevee_ensure_cube_views(shdw_data->near,
                            shdw_data->far,
                            linfo->shadow_cube_size,
                            cube_data->shadowmat,
                            g_data->cube_views);

    /* Render shadow cube */
    /* Render 6 faces separately: seems to be faster for the general case.
     * The only time it's more beneficial is when the CPU culling overhead
     * outweigh the instancing overhead. which is rarely the case. */
    for (int j = 0; j < 6; j++) {
      /* Optimization: Only render the needed faces. */
      /* Skip all but -Z face. */
      if (evli->light_type == LA_SPOT && j != 5 && evli->spotsize < cosf(DEG2RADF(90.0f) * 0.5f))
        continue;
      /* Skip +Z face. */
      if (evli->light_type != LA_LOCAL && j == 4)
        continue;
      /* TODO(fclem) some cube sides can be invisible in the main views. Cull them. */
      // if (frustum_intersect(g_data->cube_views[j], main_view))
      //   continue;

      DRW_view_set_active(g_data->cube_views[j]);
      int layer = cube * 6 + j;
      GPU_framebuffer_texture_layer_attach(
          sldata->shadow_fb, sldata->shadow_cube_pool, 0, layer, 0);
      GPU_framebuffer_bind(sldata->shadow_fb);
      GPU_framebuffer_clear_depth(sldata->shadow_fb, 1.0f);
      DRW_draw_pass(psl->shadow_pass);
    }

    BLI_BITMAP_SET(&linfo->sh_cube_update[0], cube, false);
  }
  linfo->update_flag &= ~LIGHT_UPDATE_SHADOW_CUBE;
  DRW_stats_group_end();

  float near = DRW_view_near_distance_get(view);
  float far = DRW_view_far_distance_get(view);

  /* Cascaded Shadow Maps */
  DRW_stats_group_start("Cascaded Shadow Maps");
  for (int i = 0; i < linfo->cascade_len; i++) {
    EEVEE_Light *evli = linfo->light_data + linfo->shadow_cascade_light_indices[i];
    EEVEE_Shadow *shdw_data = linfo->shadow_data + (int)evli->shadow_id;
    EEVEE_ShadowCascade *csm_data = linfo->shadow_cascade_data + (int)shdw_data->type_data_id;
    EEVEE_ShadowCascadeRender *csm_render = linfo->shadow_cascade_render +
                                            (int)shdw_data->type_data_id;

    eevee_shadow_cascade_setup(linfo, evli, view, near, far, effects->taa_current_sample - 1);

    /* Meh, Reusing the cube views. */
    BLI_assert(MAX_CASCADE_NUM <= 6);
    eevee_ensure_cascade_views(csm_render, g_data->cube_views);

    /* Render shadow cascades */
    /* Render cascade separately: seems to be faster for the general case.
     * The only time it's more beneficial is when the CPU culling overhead
     * outweigh the instancing overhead. which is rarely the case. */
    for (int j = 0; j < csm_render->cascade_count; j++) {
      DRW_view_set_active(g_data->cube_views[j]);
      int layer = csm_data->tex_id + j;
      GPU_framebuffer_texture_layer_attach(
          sldata->shadow_fb, sldata->shadow_cascade_pool, 0, layer, 0);
      GPU_framebuffer_bind(sldata->shadow_fb);
      GPU_framebuffer_clear_depth(sldata->shadow_fb, 1.0f);
      DRW_draw_pass(psl->shadow_pass);
    }
  }

  DRW_stats_group_end();

  DRW_view_set_active(view);

  DRW_uniformbuffer_update(sldata->light_ubo, &linfo->light_data);
  DRW_uniformbuffer_update(sldata->shadow_ubo, &linfo->shadow_data); /* Update all data at once */

  sldata->common_data.ray_type = saved_ray_type;
  DRW_uniformbuffer_update(sldata->common_ubo, &sldata->common_data);
}

void EEVEE_lights_free(void)
{
  DRW_SHADER_FREE_SAFE(e_data.shadow_sh);
}
