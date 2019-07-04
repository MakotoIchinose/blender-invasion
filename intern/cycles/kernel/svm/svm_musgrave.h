/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

CCL_NAMESPACE_BEGIN

/* 1D Musgrave fBm
 *
 * H: fractal increment parameter
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 *
 * from "Texturing and Modelling: A procedural approach"
 */

ccl_device_noinline float noise_musgrave_fBm_1d(float p, float H, float lacunarity, float octaves)
{
  float rmd;
  float value = 0.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value += snoise_1d(p) * pwr;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value += rmd * snoise_1d(p) * pwr;
  }

  return value;
}

/* 1D Musgrave Multifractal
 *
 * H: highest fractal dimension
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 */

ccl_device_noinline float noise_musgrave_multi_fractal_1d(float p,
                                                          float H,
                                                          float lacunarity,
                                                          float octaves)
{
  float rmd;
  float value = 1.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value *= (pwr * snoise_1d(p) + 1.0f);
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value *= (rmd * pwr * snoise_1d(p) + 1.0f); /* correct? */
  }

  return value;
}

/* 1D Musgrave Heterogeneous Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hetero_terrain_1d(
    float p, float H, float lacunarity, float octaves, float offset)
{
  float value, increment, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  /* first unscaled octave of function; later octaves are scaled */
  value = offset + snoise_1d(p);
  p *= lacunarity;

  for (int i = 1; i < float_to_int(octaves); i++) {
    increment = (snoise_1d(p) + offset) * pwr * value;
    value += increment;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    increment = (snoise_1d(p) + offset) * pwr * value;
    value += rmd * increment;
  }

  return value;
}

/* 1D Hybrid Additive/Multiplicative Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hybrid_multi_fractal_1d(
    float p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  result = snoise_1d(p) + offset;
  weight = gain * result;
  p *= lacunarity;

  for (int i = 1; (weight > 0.001f) && (i < float_to_int(octaves)); i++) {
    if (weight > 1.0f) {
      weight = 1.0f;
    }

    signal = (snoise_1d(p) + offset) * pwr;
    pwr *= pwHL;
    result += weight * signal;
    weight *= gain * signal;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    result += rmd * ((snoise_1d(p) + offset) * pwr);
  }

  return result;
}

/* 1D Ridged Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_ridged_multi_fractal_1d(
    float p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  signal = offset - fabsf(snoise_1d(p));
  signal *= signal;
  result = signal;
  weight = 1.0f;

  for (int i = 1; i < float_to_int(octaves); i++) {
    p *= lacunarity;
    weight = saturate(signal * gain);
    signal = offset - fabsf(snoise_1d(p));
    signal *= signal;
    signal *= weight;
    result += signal * pwr;
    pwr *= pwHL;
  }

  return result;
}

/* 2D Musgrave fBm
 *
 * H: fractal increment parameter
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 *
 * from "Texturing and Modelling: A procedural approach"
 */

ccl_device_noinline float noise_musgrave_fBm_2d(float2 p, float H, float lacunarity, float octaves)
{
  float rmd;
  float value = 0.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value += snoise_2d(p) * pwr;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value += rmd * snoise_2d(p) * pwr;
  }

  return value;
}

/* 2D Musgrave Multifractal
 *
 * H: highest fractal dimension
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 */

ccl_device_noinline float noise_musgrave_multi_fractal_2d(float2 p,
                                                          float H,
                                                          float lacunarity,
                                                          float octaves)
{
  float rmd;
  float value = 1.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value *= (pwr * snoise_2d(p) + 1.0f);
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value *= (rmd * pwr * snoise_2d(p) + 1.0f); /* correct? */
  }

  return value;
}

/* 2D Musgrave Heterogeneous Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hetero_terrain_2d(
    float2 p, float H, float lacunarity, float octaves, float offset)
{
  float value, increment, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  /* first unscaled octave of function; later octaves are scaled */
  value = offset + snoise_2d(p);
  p *= lacunarity;

  for (int i = 1; i < float_to_int(octaves); i++) {
    increment = (snoise_2d(p) + offset) * pwr * value;
    value += increment;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    increment = (snoise_2d(p) + offset) * pwr * value;
    value += rmd * increment;
  }

  return value;
}

/* 2D Hybrid Additive/Multiplicative Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hybrid_multi_fractal_2d(
    float2 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  result = snoise_2d(p) + offset;
  weight = gain * result;
  p *= lacunarity;

  for (int i = 1; (weight > 0.001f) && (i < float_to_int(octaves)); i++) {
    if (weight > 1.0f) {
      weight = 1.0f;
    }

    signal = (snoise_2d(p) + offset) * pwr;
    pwr *= pwHL;
    result += weight * signal;
    weight *= gain * signal;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    result += rmd * ((snoise_2d(p) + offset) * pwr);
  }

  return result;
}

/* 2D Ridged Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_ridged_multi_fractal_2d(
    float2 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  signal = offset - fabsf(snoise_2d(p));
  signal *= signal;
  result = signal;
  weight = 1.0f;

  for (int i = 1; i < float_to_int(octaves); i++) {
    p *= lacunarity;
    weight = saturate(signal * gain);
    signal = offset - fabsf(snoise_2d(p));
    signal *= signal;
    signal *= weight;
    result += signal * pwr;
    pwr *= pwHL;
  }

  return result;
}

/* 3D Musgrave fBm
 *
 * H: fractal increment parameter
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 *
 * from "Texturing and Modelling: A procedural approach"
 */

ccl_device_noinline float noise_musgrave_fBm_3d(float3 p, float H, float lacunarity, float octaves)
{
  float rmd;
  float value = 0.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value += snoise_3d(p) * pwr;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value += rmd * snoise_3d(p) * pwr;
  }

  return value;
}

/* 3D Musgrave Multifractal
 *
 * H: highest fractal dimension
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 */

ccl_device_noinline float noise_musgrave_multi_fractal_3d(float3 p,
                                                          float H,
                                                          float lacunarity,
                                                          float octaves)
{
  float rmd;
  float value = 1.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value *= (pwr * snoise_3d(p) + 1.0f);
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value *= (rmd * pwr * snoise_3d(p) + 1.0f); /* correct? */
  }

  return value;
}

/* 3D Musgrave Heterogeneous Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hetero_terrain_3d(
    float3 p, float H, float lacunarity, float octaves, float offset)
{
  float value, increment, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  /* first unscaled octave of function; later octaves are scaled */
  value = offset + snoise_3d(p);
  p *= lacunarity;

  for (int i = 1; i < float_to_int(octaves); i++) {
    increment = (snoise_3d(p) + offset) * pwr * value;
    value += increment;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    increment = (snoise_3d(p) + offset) * pwr * value;
    value += rmd * increment;
  }

  return value;
}

/* 3D Hybrid Additive/Multiplicative Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hybrid_multi_fractal_3d(
    float3 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  result = snoise_3d(p) + offset;
  weight = gain * result;
  p *= lacunarity;

  for (int i = 1; (weight > 0.001f) && (i < float_to_int(octaves)); i++) {
    if (weight > 1.0f) {
      weight = 1.0f;
    }

    signal = (snoise_3d(p) + offset) * pwr;
    pwr *= pwHL;
    result += weight * signal;
    weight *= gain * signal;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    result += rmd * ((snoise_3d(p) + offset) * pwr);
  }

  return result;
}

/* 3D Ridged Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_ridged_multi_fractal_3d(
    float3 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  signal = offset - fabsf(snoise_3d(p));
  signal *= signal;
  result = signal;
  weight = 1.0f;

  for (int i = 1; i < float_to_int(octaves); i++) {
    p *= lacunarity;
    weight = saturate(signal * gain);
    signal = offset - fabsf(snoise_3d(p));
    signal *= signal;
    signal *= weight;
    result += signal * pwr;
    pwr *= pwHL;
  }

  return result;
}

/* 4D Musgrave fBm
 *
 * H: fractal increment parameter
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 *
 * from "Texturing and Modelling: A procedural approach"
 */

ccl_device_noinline float noise_musgrave_fBm_4d(float4 p, float H, float lacunarity, float octaves)
{
  float rmd;
  float value = 0.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value += snoise_4d(p) * pwr;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value += rmd * snoise_4d(p) * pwr;
  }

  return value;
}

/* 4D Musgrave Multifractal
 *
 * H: highest fractal dimension
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 */

ccl_device_noinline float noise_musgrave_multi_fractal_4d(float4 p,
                                                          float H,
                                                          float lacunarity,
                                                          float octaves)
{
  float rmd;
  float value = 1.0f;
  float pwr = 1.0f;
  float pwHL = powf(lacunarity, -H);

  for (int i = 0; i < float_to_int(octaves); i++) {
    value *= (pwr * snoise_4d(p) + 1.0f);
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    value *= (rmd * pwr * snoise_4d(p) + 1.0f); /* correct? */
  }

  return value;
}

/* 4D Musgrave Heterogeneous Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hetero_terrain_4d(
    float4 p, float H, float lacunarity, float octaves, float offset)
{
  float value, increment, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  /* first unscaled octave of function; later octaves are scaled */
  value = offset + snoise_4d(p);
  p *= lacunarity;

  for (int i = 1; i < float_to_int(octaves); i++) {
    increment = (snoise_4d(p) + offset) * pwr * value;
    value += increment;
    pwr *= pwHL;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    increment = (snoise_4d(p) + offset) * pwr * value;
    value += rmd * increment;
  }

  return value;
}

/* 4D Hybrid Additive/Multiplicative Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_hybrid_multi_fractal_4d(
    float4 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight, rmd;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  result = snoise_4d(p) + offset;
  weight = gain * result;
  p *= lacunarity;

  for (int i = 1; (weight > 0.001f) && (i < float_to_int(octaves)); i++) {
    if (weight > 1.0f) {
      weight = 1.0f;
    }

    signal = (snoise_4d(p) + offset) * pwr;
    pwr *= pwHL;
    result += weight * signal;
    weight *= gain * signal;
    p *= lacunarity;
  }

  rmd = octaves - floorf(octaves);
  if (rmd != 0.0f) {
    result += rmd * ((snoise_4d(p) + offset) * pwr);
  }

  return result;
}

/* 4D Ridged Multifractal Terrain
 *
 * H: fractal dimension of the roughest area
 * lacunarity: gap between successive frequencies
 * octaves: number of frequencies in the fBm
 * offset: raises the terrain from `sea level'
 */

ccl_device_noinline float noise_musgrave_ridged_multi_fractal_4d(
    float4 p, float H, float lacunarity, float octaves, float offset, float gain)
{
  float result, signal, weight;
  float pwHL = powf(lacunarity, -H);
  float pwr = pwHL;

  signal = offset - fabsf(snoise_4d(p));
  signal *= signal;
  result = signal;
  weight = 1.0f;

  for (int i = 1; i < float_to_int(octaves); i++) {
    p *= lacunarity;
    weight = saturate(signal * gain);
    signal = offset - fabsf(snoise_4d(p));
    signal *= signal;
    signal *= weight;
    result += signal * pwr;
    pwr *= pwHL;
  }

  return result;
}

ccl_device void svm_node_tex_musgrave(KernelGlobals *kg,
                                      ShaderData *sd,
                                      float *stack,
                                      uint offsets1,
                                      uint offsets2,
                                      uint offsets3,
                                      int *offset)
{
  uint type, dimensions, co_offset, w_offset;
  uint scale_offset, detail_offset, dimension_offset, lacunarity_offset;
  uint offset_offset, gain_offset, fac_offset;

  decode_node_uchar4(offsets1, &type, &dimensions, &co_offset, &w_offset);
  decode_node_uchar4(
      offsets2, &scale_offset, &detail_offset, &dimension_offset, &lacunarity_offset);
  decode_node_uchar4(offsets3, &offset_offset, &gain_offset, &fac_offset, NULL);

  uint4 node1 = read_node(kg, offset);
  uint4 node2 = read_node(kg, offset);

  float3 co = stack_load_float3(stack, co_offset);
  float w = stack_load_float_default(stack, w_offset, node1.x);
  float scale = stack_load_float_default(stack, scale_offset, node1.y);
  float detail = stack_load_float_default(stack, detail_offset, node1.z);
  float dimension = stack_load_float_default(stack, dimension_offset, node1.w);
  float lacunarity = stack_load_float_default(stack, lacunarity_offset, node2.x);
  float foffset = stack_load_float_default(stack, offset_offset, node2.y);
  float gain = stack_load_float_default(stack, gain_offset, node2.z);

  dimension = fmaxf(dimension, 1e-5f);
  detail = clamp(detail, 0.0f, 16.0f);
  lacunarity = fmaxf(lacunarity, 1e-5f);

  float fac;

  switch (dimensions) {
    case 1: {
      float p = w * scale;
      switch ((NodeMusgraveType)type) {
        case NODE_MUSGRAVE_MULTIFRACTAL:
          fac = noise_musgrave_multi_fractal_1d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_FBM:
          fac = noise_musgrave_fBm_1d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_HYBRID_MULTIFRACTAL:
          fac = noise_musgrave_hybrid_multi_fractal_1d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_RIDGED_MULTIFRACTAL:
          fac = noise_musgrave_ridged_multi_fractal_1d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_HETERO_TERRAIN:
          fac = noise_musgrave_hetero_terrain_1d(p, dimension, lacunarity, detail, foffset);
          break;
        default:
          fac = 0.0f;
      }
      break;
    }
    case 2: {
      float2 p = make_float2(co.x, co.y) * scale;
      switch ((NodeMusgraveType)type) {
        case NODE_MUSGRAVE_MULTIFRACTAL:
          fac = noise_musgrave_multi_fractal_2d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_FBM:
          fac = noise_musgrave_fBm_2d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_HYBRID_MULTIFRACTAL:
          fac = noise_musgrave_hybrid_multi_fractal_2d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_RIDGED_MULTIFRACTAL:
          fac = noise_musgrave_ridged_multi_fractal_2d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_HETERO_TERRAIN:
          fac = noise_musgrave_hetero_terrain_2d(p, dimension, lacunarity, detail, foffset);
          break;
        default:
          fac = 0.0f;
      }
      break;
    }
    case 3: {
      float3 p = co * scale;
      switch ((NodeMusgraveType)type) {
        case NODE_MUSGRAVE_MULTIFRACTAL:
          fac = noise_musgrave_multi_fractal_3d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_FBM:
          fac = noise_musgrave_fBm_3d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_HYBRID_MULTIFRACTAL:
          fac = noise_musgrave_hybrid_multi_fractal_3d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_RIDGED_MULTIFRACTAL:
          fac = noise_musgrave_ridged_multi_fractal_3d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_HETERO_TERRAIN:
          fac = noise_musgrave_hetero_terrain_3d(p, dimension, lacunarity, detail, foffset);
          break;
        default:
          fac = 0.0f;
      }
      break;
    }
    case 4: {
      float4 p = make_float4(co.x, co.y, co.z, w) * scale;
      switch ((NodeMusgraveType)type) {
        case NODE_MUSGRAVE_MULTIFRACTAL:
          fac = noise_musgrave_multi_fractal_4d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_FBM:
          fac = noise_musgrave_fBm_4d(p, dimension, lacunarity, detail);
          break;
        case NODE_MUSGRAVE_HYBRID_MULTIFRACTAL:
          fac = noise_musgrave_hybrid_multi_fractal_4d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_RIDGED_MULTIFRACTAL:
          fac = noise_musgrave_ridged_multi_fractal_4d(
              p, dimension, lacunarity, detail, foffset, gain);
          break;
        case NODE_MUSGRAVE_HETERO_TERRAIN:
          fac = noise_musgrave_hetero_terrain_4d(p, dimension, lacunarity, detail, foffset);
          break;
        default:
          fac = 0.0f;
      }
      break;
    }
    default:
      fac = 0.0f;
  }

  if (stack_valid(fac_offset))
    stack_store_float(stack, fac_offset, fac);
}

CCL_NAMESPACE_END
