
uniform sampler2DArray shadowCubeTexture;
uniform sampler2DArray shadowCascadeTexture;

#define LAMPS_LIB

layout(std140) uniform shadow_block
{
  ShadowData shadows_data[MAX_SHADOW];
  ShadowCubeData shadows_cube_data[MAX_SHADOW_CUBE];
  ShadowCascadeData shadows_cascade_data[MAX_SHADOW_CASCADE];
};

layout(std140) uniform light_block
{
  LightData lights_data[MAX_LIGHT];
};

/* type */
#define POINT 0.0
#define SUN 1.0
#define SPOT 2.0
#define AREA_RECT 4.0
/* Used to define the area light shape, doesn't directly correspond to a Blender light type. */
#define AREA_ELLIPSE 100.0

#if defined(SHADOW_VSM)
#  define ShadowSample vec2
#  define sample_cube(vec, id) texture_octahedron(shadowCubeTexture, vec4(vec, id)).rg
#  define sample_cascade(vec, id) texture(shadowCascadeTexture, vec3(vec, id)).rg
#elif defined(SHADOW_ESM)
#  define ShadowSample float
#  define sample_cube(vec, id) texture_octahedron(shadowCubeTexture, vec4(vec, id)).r
#  define sample_cascade(vec, id) texture(shadowCascadeTexture, vec3(vec, id)).r
#else
#  define ShadowSample float
#  define sample_cube(vec, id) texture_octahedron(shadowCubeTexture, vec4(vec, id)).r
#  define sample_cascade(vec, id) texture(shadowCascadeTexture, vec3(vec, id)).r
#endif

#if defined(SHADOW_VSM)
#  define get_depth_delta(dist, s) (dist - s.x)
#else
#  define get_depth_delta(dist, s) (dist - s)
#endif

  /* ----------------------------------------------------------- */
  /* ----------------------- Shadow tests ---------------------- */
  /* ----------------------------------------------------------- */

#if defined(SHADOW_VSM)

float shadow_test(ShadowSample moments, float dist, ShadowData sd)
{
  float p = 0.0;

  if (dist <= moments.x) {
    p = 1.0;
  }

  float variance = moments.y - (moments.x * moments.x);
  variance = max(variance, sd.sh_bias / 10.0);

  float d = moments.x - dist;
  float p_max = variance / (variance + d * d);

  /* Now reduce light-bleeding by removing the [0, x] tail and linearly rescaling (x, 1] */
  p_max = clamp((p_max - sd.sh_bleed) / (1.0 - sd.sh_bleed), 0.0, 1.0);

  return max(p, p_max);
}

#elif defined(SHADOW_ESM)

float shadow_test(ShadowSample z, float dist, ShadowData sd)
{
  return saturate(exp(sd.sh_exp * (z - dist + sd.sh_bias)));
}

#else

float shadow_test(ShadowSample z, float dist, ShadowData sd)
{
  return step(0, z - dist + sd.sh_bias);
}

#endif

/* ----------------------------------------------------------- */
/* ----------------------- Shadow types ---------------------- */
/* ----------------------------------------------------------- */

float shadow_cubemap(ShadowData sd, ShadowCubeData scd, float texid, vec3 W)
{
  vec3 cubevec = W - scd.position.xyz;
  float dist = length(cubevec);

  cubevec /= dist;

  ShadowSample s = sample_cube(cubevec, texid);
  return shadow_test(s, dist, sd);
}

float evaluate_cascade(ShadowData sd, mat4 shadowmat, vec3 W, float range, float texid)
{
  vec4 shpos = shadowmat * vec4(W, 1.0);
  float dist = shpos.z * range;

  ShadowSample s = sample_cascade(shpos.xy, texid);
  float vis = shadow_test(s, dist, sd);

  /* If fragment is out of shadowmap range, do not occlude */
  if (shpos.z < 1.0 && shpos.z > 0.0) {
    return vis;
  }
  else {
    return 1.0;
  }
}

float shadow_cascade(ShadowData sd, int scd_id, float texid, vec3 W)
{
  vec4 view_z = vec4(dot(W - cameraPos, cameraForward));
  vec4 weights = smoothstep(shadows_cascade_data[scd_id].split_end_distances,
                            shadows_cascade_data[scd_id].split_start_distances.yzwx,
                            view_z);

  weights.yzw -= weights.xyz;

  vec4 vis = vec4(1.0);
  float range = abs(sd.sh_far - sd.sh_near); /* Same factor as in get_cascade_world_distance(). */

  /* Branching using (weights > 0.0) is reaally slooow on intel so avoid it for now. */
  /* TODO OPTI: Only do 2 samples and blend. */
  vis.x = evaluate_cascade(sd, shadows_cascade_data[scd_id].shadowmat[0], W, range, texid + 0);
  vis.y = evaluate_cascade(sd, shadows_cascade_data[scd_id].shadowmat[1], W, range, texid + 1);
  vis.z = evaluate_cascade(sd, shadows_cascade_data[scd_id].shadowmat[2], W, range, texid + 2);
  vis.w = evaluate_cascade(sd, shadows_cascade_data[scd_id].shadowmat[3], W, range, texid + 3);

  float weight_sum = dot(vec4(1.0), weights);
  if (weight_sum > 0.9999) {
    float vis_sum = dot(vec4(1.0), vis * weights);
    return vis_sum / weight_sum;
  }
  else {
    float vis_sum = dot(vec4(1.0), vis * step(0.001, weights));
    return mix(1.0, vis_sum, weight_sum);
  }
}

/* ----------------------------------------------------------- */
/* --------------------- Light Functions --------------------- */
/* ----------------------------------------------------------- */

/* From Frostbite PBR Course
 * Distance based attenuation
 * http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf */
float distance_attenuation(float dist_sqr, float inv_sqr_influence)
{
  float factor = dist_sqr * inv_sqr_influence;
  float fac = saturate(1.0 - factor * factor);
  return fac * fac;
}

float spot_attenuation(LightData ld, vec3 l_vector)
{
  float z = dot(ld.l_forward, l_vector.xyz);
  vec3 lL = l_vector.xyz / z;
  float x = dot(ld.l_right, lL) / ld.l_sizex;
  float y = dot(ld.l_up, lL) / ld.l_sizey;
  float ellipse = inversesqrt(1.0 + x * x + y * y);
  float spotmask = smoothstep(0.0, 1.0, (ellipse - ld.l_spot_size) / ld.l_spot_blend);
  return spotmask;
}

float light_attenuation(LightData ld, vec4 l_vector)
{
  float vis = 1.0;
  if (ld.l_type == SPOT) {
    vis *= spot_attenuation(ld, l_vector.xyz);
  }
  if (ld.l_type >= SPOT) {
    vis *= step(0.0, -dot(l_vector.xyz, ld.l_forward));
  }
  if (ld.l_type != SUN) {
    vis *= distance_attenuation(l_vector.w * l_vector.w, ld.l_influence);
  }
  return vis;
}

float light_visibility(LightData ld,
                       vec3 W,
#ifndef VOLUMETRICS
                       vec3 viewPosition,
                       vec3 vN,
#endif
                       vec4 l_vector)
{
  float vis = light_attenuation(ld, l_vector);

#if !defined(VOLUMETRICS) || defined(VOLUME_SHADOW)
  /* shadowing */
  if (ld.l_shadowid >= 0.0 && vis > 0.001) {
    ShadowData data = shadows_data[int(ld.l_shadowid)];

    if (ld.l_type == SUN) {
      vis *= shadow_cascade(data, int(data.sh_data_start), data.sh_tex_start, W);
    }
    else {
      vis *= shadow_cubemap(
          data, shadows_cube_data[int(data.sh_data_start)], data.sh_tex_start, W);
    }

#  ifndef VOLUMETRICS
    /* Only compute if not already in shadow. */
    if (data.sh_contact_dist > 0.0) {
      vec4 L = (ld.l_type != SUN) ? l_vector : vec4(-ld.l_forward, 1.0);
      float trace_distance = (ld.l_type != SUN) ? min(data.sh_contact_dist, l_vector.w) :
                                                  data.sh_contact_dist;

      vec3 T, B;
      make_orthonormal_basis(L.xyz / L.w, T, B);

      vec4 rand = texelfetch_noise_tex(gl_FragCoord.xy);
      rand.zw *= fast_sqrt(rand.y) * data.sh_contact_spread;

      /* We use the full l_vector.xyz so that the spread is minimize
       * if the shading point is further away from the light source */
      vec3 ray_dir = L.xyz + T * rand.z + B * rand.w;
      ray_dir = transform_direction(ViewMatrix, ray_dir);
      ray_dir = normalize(ray_dir);

      vec3 ray_ori = viewPosition;

      /* Fix translucency shadowed by contact shadows. */
      vN = (gl_FrontFacing) ? vN : -vN;

      if (dot(vN, ray_dir) <= 0.0) {
        return vis;
      }

      float bias = 0.5;                    /* Constant Bias */
      bias += 1.0 - abs(dot(vN, ray_dir)); /* Angle dependent bias */
      bias *= gl_FrontFacing ? data.sh_contact_offset : -data.sh_contact_offset;

      vec3 nor_bias = vN * bias;
      ray_ori += nor_bias;

      ray_dir *= trace_distance;
      ray_dir -= nor_bias;

      vec3 hit_pos = raycast(
          -1, ray_ori, ray_dir, data.sh_contact_thickness, rand.x, 0.1, 0.001, false);

      if (hit_pos.z > 0.0) {
        hit_pos = get_view_space_from_depth(hit_pos.xy, hit_pos.z);
        float hit_dist = distance(viewPosition, hit_pos);
        float dist_ratio = hit_dist / trace_distance;
        return vis * saturate(dist_ratio * dist_ratio * dist_ratio);
      }
    }
#  endif
  }
#endif

  return vis;
}

#ifdef USE_LTC
float light_diffuse(LightData ld, vec3 N, vec3 V, vec4 l_vector)
{
  if (ld.l_type == AREA_RECT) {
    vec3 corners[4];
    corners[0] = normalize((l_vector.xyz + ld.l_right * -ld.l_sizex) + ld.l_up * ld.l_sizey);
    corners[1] = normalize((l_vector.xyz + ld.l_right * -ld.l_sizex) + ld.l_up * -ld.l_sizey);
    corners[2] = normalize((l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * -ld.l_sizey);
    corners[3] = normalize((l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * ld.l_sizey);

    return ltc_evaluate_quad(corners, N);
  }
  else if (ld.l_type == AREA_ELLIPSE) {
    vec3 points[3];
    points[0] = (l_vector.xyz + ld.l_right * -ld.l_sizex) + ld.l_up * -ld.l_sizey;
    points[1] = (l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * -ld.l_sizey;
    points[2] = (l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * ld.l_sizey;

    return ltc_evaluate_disk(N, V, mat3(1.0), points);
  }
  else {
    float radius = ld.l_radius;
    radius /= (ld.l_type == SUN) ? 1.0 : l_vector.w;
    vec3 L = (ld.l_type == SUN) ? -ld.l_forward : (l_vector.xyz / l_vector.w);

    return ltc_evaluate_disk_simple(radius, dot(N, L));
  }
}

float light_specular(LightData ld, vec4 ltc_mat, vec3 N, vec3 V, vec4 l_vector)
{
  if (ld.l_type == AREA_RECT) {
    vec3 corners[4];
    corners[0] = (l_vector.xyz + ld.l_right * -ld.l_sizex) + ld.l_up * ld.l_sizey;
    corners[1] = (l_vector.xyz + ld.l_right * -ld.l_sizex) + ld.l_up * -ld.l_sizey;
    corners[2] = (l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * -ld.l_sizey;
    corners[3] = (l_vector.xyz + ld.l_right * ld.l_sizex) + ld.l_up * ld.l_sizey;

    ltc_transform_quad(N, V, ltc_matrix(ltc_mat), corners);

    return ltc_evaluate_quad(corners, vec3(0.0, 0.0, 1.0));
  }
  else {
    bool is_ellipse = (ld.l_type == AREA_ELLIPSE);
    float radius_x = is_ellipse ? ld.l_sizex : ld.l_radius;
    float radius_y = is_ellipse ? ld.l_sizey : ld.l_radius;

    vec3 L = (ld.l_type == SUN) ? -ld.l_forward : l_vector.xyz;
    vec3 Px = ld.l_right;
    vec3 Py = ld.l_up;

    if (ld.l_type == SPOT || ld.l_type == POINT) {
      make_orthonormal_basis(l_vector.xyz / l_vector.w, Px, Py);
    }

    vec3 points[3];
    points[0] = (L + Px * -radius_x) + Py * -radius_y;
    points[1] = (L + Px * radius_x) + Py * -radius_y;
    points[2] = (L + Px * radius_x) + Py * radius_y;

    return ltc_evaluate_disk(N, V, ltc_matrix(ltc_mat), points);
  }
}
#endif
