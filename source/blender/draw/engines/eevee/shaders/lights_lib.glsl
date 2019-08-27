
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

float cubeFaceIndexEEVEE(vec3 P)
{
  vec3 aP = abs(P);
  if (all(greaterThan(aP.xx, aP.yz))) {
    return (P.x > 0.0) ? 0.0 : 1.0;
  }
  else if (all(greaterThan(aP.yy, aP.xz))) {
    return (P.y > 0.0) ? 2.0 : 3.0;
  }
  else {
    return (P.z > 0.0) ? 4.0 : 5.0;
  }
}

vec2 cubeFaceCoordEEVEE(vec3 P, float face)
{
  if (face < 2.0) {
    return (P.zy / P.x) * vec2(-0.5, -sign(P.x) * 0.5) + 0.5;
  }
  else if (face < 4.0) {
    return (P.xz / P.y) * vec2(sign(P.y) * 0.5, 0.5) + 0.5;
  }
  else {
    return (P.xy / P.z) * vec2(0.5, -sign(P.z) * 0.5) + 0.5;
  }
}

float cubeFaceDepthEEVEE(vec3 P, float face)
{
  if (face < 2.0) {
    return abs(P.x);
  }
  else if (face < 4.0) {
    return abs(P.y);
  }
  else {
    return abs(P.z);
  }
}

vec4 sample_cube(sampler2DArray tex, vec3 cubevec, float cube)
{
  /* Manual Shadow Cube Layer indexing. */
  /* TODO Shadow Cube Array. */
  float face = cubeFaceIndexEEVEE(cubevec);
  vec3 coord = vec3(cubeFaceCoordEEVEE(cubevec, face), cube * 6.0 + face);
  return texture(tex, coord);
}

vec4 sample_cascade(sampler2DArray tex, vec2 co, float cascade_id)
{
  return texture(tex, vec3(co, cascade_id));
}

/** Convert screenspace derivatives to texture space derivatives.
 * As described by John R. Isidoro in GDC 2006 presentation "Shadow Mapping: GPU-based Tips and
 *Techniques". https://developer.amd.com/wordpress/media/2012/10/Isidoro-ShadowMapping.pdf
 **/
vec2 texture_space_derivatives(vec3 duvfdx, vec3 duvfdy)
{
  /* Invert texture Jacobian and use chain rule to compute ddist/du and ddist/dv
   *  |ddist/du| = |du/dx  du/dy|-T  * |ddist/dx|
   *  |ddist/dv|   |dv/dx  dv/dy|      |ddist/dy| */

  /* // Multiply ddist/dx and ddist/dy by inverse transpose of Jacobian
   * float invDet = 1 / ((duvdist_dx.x * duvdist_dy.y) - (duvdist_dx.y * duvdist_dy.x));
   * // Top row of 2x2
   * ddist_duv.x = duvdist_dy.y * duvdist_dx.w;  // invJtrans[0][0] * ddist_dx
   * ddist_duv.x -= duvdist_dx.y * duvdist_dy.w; // invJtrans[0][1] * ddist_dy
   * // Bottom row of 2x2
   * ddist_duv.y = duvdist_dx.x * duvdist_dy.w;  // invJtrans[1][1] * ddist_dy
   * ddist_duv.y -= duvdist_dy.x * duvdist_dx.w; // invJtrans[1][0] * ddist_dx
   * ddist_duv *= invDet;
   **/

  /* Optimized version. */
  vec2 a = duvfdx.xy * duvfdy.yx;
  vec4 b = duvfdy.yzzx * duvfdx.zyxz;
  return (b.xz - b.yw) / (a.x - a.y);
}

/** Returns Receiver Plane Depth Bias
 * As described by John R. Isidoro in GDC 2006 presentation "Shadow Mapping: GPU-based Tips and
 * Techniques". https://developer.amd.com/wordpress/media/2012/10/Isidoro-ShadowMapping.pdf
 */
float shadowmap_bias(vec2 co, vec2 tap_co, vec2 dwduv)
{
  return dot(tap_co - co, dwduv);
}

/**
 * Parameters:
 * shadow_tx : shadowmap to sample.
 * co        : the original sample position.
 * ref       : the original depth to test (Depth of the surface in shadowmap space).
 * co_ofs    : the actual offseted sample position.
 * dwduv     : result of texture_space_derivatives().
 **/
float filtered_and_biased_shadow_test(sampler2DArray shadow_tx,
                                      vec2 co,
                                      float layer,
                                      float ref,
                                      float max_bias,
                                      vec2 co_ofs,
                                      vec2 dwduv)
{
  vec4 biases, depths;
  vec2 texture_size = vec2(textureSize(shadow_tx, 0).xy);
  vec3 texel_size = vec3(1.0 / texture_size, 0.0);
  /* Center texel coordinate to match hardware filtering */
  vec2 texel_uv = co_ofs * texture_size.xy - 0.5;
  vec2 texel_fract = fract(texel_uv);
  vec2 texel_floor = (floor(texel_uv) + 0.5) * texel_size.xy;
  /* Calc bias for each sample */
  biases.x = shadowmap_bias(co, texel_floor + texel_size.zy, dwduv);
  biases.y = shadowmap_bias(co, texel_floor + texel_size.xy, dwduv);
  biases.z = shadowmap_bias(co, texel_floor + texel_size.xz, dwduv);
  biases.w = shadowmap_bias(co, texel_floor + texel_size.zz, dwduv);
  /* Gather samples */
#ifdef GPU_ARB_texture_gather
  /* Note: To make sure that we get the exact same result we use texel center.
   * This is because Anisotropic filtering is offsetting the coordinate. */
  depths = textureGather(shadow_tx, vec3(texel_floor + texel_size.xy * 0.5, layer));
#else
  depths.x = texture(shadow_tx, vec3(texel_floor + texel_size.zy, layer)).x;
  depths.y = texture(shadow_tx, vec3(texel_floor + texel_size.xy, layer)).x;
  depths.z = texture(shadow_tx, vec3(texel_floor + texel_size.xz, layer)).x;
  depths.w = texture(shadow_tx, vec3(texel_floor + texel_size.zz, layer)).x;
#endif
  /* Take absolute of bias to avoid overshadowing in proximity area.
   * Also limit the bias in case the geom is almost perpendicular to the shadow source
   * to avoid light leaking. */
  vec4 refs = saturate(ref - min(vec4(max_bias), abs(biases)));
  /* Bilinear PCF. */
  vec4 tests = step(refs, depths);
  tests.xy = mix(tests.wz, tests.xy, texel_fract.y);
  return saturate(mix(tests.x, tests.y, texel_fract.x));
}

float shadow_cubemap(ShadowData sd, ShadowCubeData scd, float texid, vec3 W)
{
  vec3 cubevec = W - scd.position.xyz;

#ifndef VOLUMETRICS
  vec3 rand = texelfetch_noise_tex(gl_FragCoord.xy).zwy;
  float ofs_len = fast_sqrt(rand.z) * sd.sh_blur * 0.1;
  vec3 cubevec_nor = normalize(cubevec);
  vec3 T, B;
  make_orthonormal_basis(cubevec_nor, T, B);
  vec3 cubevec_ofs = cubevec_nor + (T * rand.x + B * rand.y) * ofs_len;
  /* Use face of the offseted cubevec. */
  float face_id = cubeFaceIndexEEVEE(cubevec_ofs);
  vec2 uv_ofs = cubeFaceCoordEEVEE(cubevec_ofs, face_id);

  /* To avoid derivatives discontinuity, we compute derivatives
   * on a smooth linear function (unormalized cubevec),
   * and manually do the derivative of the uv coords. */
  vec3 duvwdx, duvwdy;
  vec3 dPdx = cubevec + dFdx(cubevec);
  vec3 dPdy = cubevec + dFdy(cubevec);
  vec2 uv = cubeFaceCoordEEVEE(cubevec, face_id);
  duvwdx.xy = cubeFaceCoordEEVEE(dPdx, face_id) - uv;
  duvwdy.xy = cubeFaceCoordEEVEE(dPdy, face_id) - uv;

  float dist = cubeFaceDepthEEVEE(cubevec, face_id);
  duvwdx.z = cubeFaceDepthEEVEE(dPdx, face_id);
  duvwdy.z = cubeFaceDepthEEVEE(dPdy, face_id);

  dist = buffer_depth(true, dist, sd.sh_far, sd.sh_near);
  duvwdx.z = buffer_depth(true, duvwdx.z, sd.sh_far, sd.sh_near) - dist;
  duvwdy.z = buffer_depth(true, duvwdy.z, sd.sh_far, sd.sh_near) - dist;

  vec2 dwduv = texture_space_derivatives(duvwdx, duvwdy);

  float layer = texid * 6.0 + face_id;
  return filtered_and_biased_shadow_test(
      shadowCubeTexture, uv, layer, dist, sd.sh_bias * 0.15, uv_ofs, dwduv);
#else
  float dist = max_v3(abs(cubevec));
  dist = buffer_depth(true, dist, sd.sh_far, sd.sh_near);
  float face_id = cubeFaceIndexEEVEE(cubevec);
  vec2 uv = cubeFaceCoordEEVEE(cubevec, face_id);

  float layer = texid * 6.0 + face_id;
  return filtered_and_biased_shadow_test(shadowCubeTexture, uv, layer, dist, 0.0, uv, vec2(0.0));
#endif
}

float evaluate_cascade(ShadowData sd, mat4 shadowmat, mat4 shadowmat0, vec3 W, float texid)
{
  vec4 shpos = shadowmat * vec4(W, 1.0);

#ifndef VOLUMETRICS
  float fac = length(shadowmat0[0].xyz) / length(shadowmat[0].xyz);
  vec3 rand = texelfetch_noise_tex(gl_FragCoord.xy).zwy;
  float ofs_len = fast_sqrt(rand.z) * sd.sh_blur * 0.05 / fac;
  /* This assumes that shadowmap is a square (w == h)
   * and that all cascades are the same dimensions. */
  vec2 uv_ofs = shpos.xy + rand.xy * ofs_len;

  vec2 dwduv = texture_space_derivatives(dFdx(shpos.xyz), dFdy(shpos.xyz));

  float vis = filtered_and_biased_shadow_test(
      shadowCascadeTexture, shpos.xy, texid, shpos.z, sd.sh_bias * 0.75, uv_ofs, dwduv);
#else
  float vis = filtered_and_biased_shadow_test(
      shadowCascadeTexture, shpos.xy, texid, shpos.z, 0.0, shpos.xy, vec2(0.0));
#endif

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

  /* Branching using (weights > 0.0) is reaally slooow on intel so avoid it for now. */
  /* TODO OPTI: Only do 2 samples and blend. */
  vis.x = evaluate_cascade(sd,
                           shadows_cascade_data[scd_id].shadowmat[0],
                           shadows_cascade_data[scd_id].shadowmat[0],
                           W,
                           texid + 0);
  vis.y = evaluate_cascade(sd,
                           shadows_cascade_data[scd_id].shadowmat[1],
                           shadows_cascade_data[scd_id].shadowmat[0],
                           W,
                           texid + 1);
  vis.z = evaluate_cascade(sd,
                           shadows_cascade_data[scd_id].shadowmat[2],
                           shadows_cascade_data[scd_id].shadowmat[0],
                           W,
                           texid + 2);
  vis.w = evaluate_cascade(sd,
                           shadows_cascade_data[scd_id].shadowmat[3],
                           shadows_cascade_data[scd_id].shadowmat[0],
                           W,
                           texid + 3);

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
