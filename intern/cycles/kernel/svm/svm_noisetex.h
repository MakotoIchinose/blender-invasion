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

/* To compute the color output of the noise, we either swizzle the
 * components, add a random offset {75, 125, 150}, or do both.
 */
ccl_device void tex_noise_1d(
    float p, float detail, float distortion, bool color_is_needed, float *value, float3 *color)
{
  if (distortion != 0.0f) {
    p += noise_1d(p + 13.5f) * distortion;
  }

  *value = noise_turbulence_1d(p, detail);
  if (color_is_needed) {
    *color = make_float3(
        *value, noise_turbulence_1d(p + 75.0f, detail), noise_turbulence_1d(p + 125.0f, detail));
  }
}

ccl_device void tex_noise_2d(
    float2 p, float detail, float distortion, bool color_is_needed, float *value, float3 *color)
{
  if (distortion != 0.0f) {
    float2 r;
    r.x = noise_2d(p + make_float2(13.5f, 13.5f)) * distortion;
    r.y = noise_2d(p) * distortion;
    p += r;
  }

  *value = noise_turbulence_2d(p, detail);
  if (color_is_needed) {
    *color = make_float3(*value,
                         noise_turbulence_2d(p + make_float2(150.0f, 125.0f), detail),
                         noise_turbulence_2d(p + make_float2(75.0f, 125.0f), detail));
  }
}

ccl_device void tex_noise_3d(
    float3 p, float detail, float distortion, bool color_is_needed, float *value, float3 *color)
{
  if (distortion != 0.0f) {
    float3 r, offset = make_float3(13.5f, 13.5f, 13.5f);
    r.x = noise_3d(p + offset) * distortion;
    r.y = noise_3d(p) * distortion;
    r.z = noise_3d(p - offset) * distortion;
    p += r;
  }

  *value = noise_turbulence_3d(p, detail);
  if (color_is_needed) {
    *color = make_float3(*value,
                         noise_turbulence_3d(make_float3(p.y, p.x, p.z), detail),
                         noise_turbulence_3d(make_float3(p.y, p.z, p.x), detail));
  }
}

ccl_device void tex_noise_4d(
    float4 p, float detail, float distortion, bool color_is_needed, float *value, float3 *color)
{
  if (distortion != 0.0f) {
    float4 r, offset = make_float4(13.5, 13.5, 13.5, 13.5);
    r.x = noise_4d(p + offset) * distortion;
    r.y = noise_4d(p) * distortion;
    r.z = noise_4d(p - offset) * distortion;
    r.w = noise_4d(make_float4(p.w, p.y, p.z, p.x) + offset) * distortion;
    p += r;
  }

  *value = noise_turbulence_4d(p, detail);
  if (color_is_needed) {
    *color = make_float3(*value,
                         noise_turbulence_4d(make_float4(p.y, p.w, p.z, p.x), detail),
                         noise_turbulence_4d(make_float4(p.y, p.z, p.w, p.x), detail));
  }
}

ccl_device void svm_node_tex_noise(KernelGlobals *kg,
                                   ShaderData *sd,
                                   float *stack,
                                   uint dimensions,
                                   uint offsets1,
                                   uint offsets2,
                                   int *offset)
{
  uint vector_offset, w_offset, scale_offset, detail_offset, distortion_offset, value_offset,
      color_offset;

  decode_node_uchar4(offsets1, &vector_offset, &w_offset, &scale_offset, &detail_offset);
  decode_node_uchar4(offsets2, &distortion_offset, &value_offset, &color_offset, NULL);

  uint4 node1 = read_node(kg, offset);

  float3 vector = stack_load_float3(stack, vector_offset);
  float w = stack_load_float_default(stack, w_offset, node1.x);
  float scale = stack_load_float_default(stack, scale_offset, node1.y);
  float detail = stack_load_float_default(stack, detail_offset, node1.z);
  float distortion = stack_load_float_default(stack, distortion_offset, node1.w);

  vector *= scale;
  w *= scale;

  float value;
  float3 color;

  switch (dimensions) {
    case 1:
      tex_noise_1d(w, detail, distortion, stack_valid(color_offset), &value, &color);
      break;
    case 2:
      tex_noise_2d(make_float2(vector.x, vector.y),
                   detail,
                   distortion,
                   stack_valid(color_offset),
                   &value,
                   &color);
      break;
    case 3:
      tex_noise_3d(vector, detail, distortion, stack_valid(color_offset), &value, &color);
      break;
    case 4:
      tex_noise_4d(make_float4(vector.x, vector.y, vector.z, w),
                   detail,
                   distortion,
                   stack_valid(color_offset),
                   &value,
                   &color);
      break;
    default:
      kernel_assert(0);
  }

  if (stack_valid(value_offset)) {
    stack_store_float(stack, value_offset, value);
  }
  if (stack_valid(color_offset)) {
    stack_store_float3(stack, color_offset, color);
  }
}

CCL_NAMESPACE_END
