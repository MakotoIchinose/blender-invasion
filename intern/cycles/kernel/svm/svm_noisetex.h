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

/* Noise */

ccl_device void svm_node_tex_noise(KernelGlobals *kg,
                                   ShaderData *sd,
                                   float *stack,
                                   uint dimensions,
                                   uint offsets1,
                                   uint offsets2,
                                   int *offset)
{
  uint co_offset, w_offset, scale_offset, detail_offset, distortion_offset, fac_offset,
      color_offset;

  decode_node_uchar4(offsets1, &co_offset, &w_offset, &scale_offset, &detail_offset);
  decode_node_uchar4(offsets2, &distortion_offset, &color_offset, &fac_offset, NULL);

  uint4 node1 = read_node(kg, offset);

  float3 p = stack_load_float3(stack, co_offset);
  float w = stack_load_float_default(stack, scale_offset, node1.x);
  float scale = stack_load_float_default(stack, scale_offset, node1.y);
  float detail = stack_load_float_default(stack, detail_offset, node1.z);
  float distortion = stack_load_float_default(stack, distortion_offset, node1.w);

  p *= scale;
  w *= scale;

  if (distortion != 0.0f) {
    float3 r, offset = make_float3(13.5f, 13.5f, 13.5f);

    r.x = noise(p + offset) * distortion;
    r.y = noise(p) * distortion;
    r.z = noise(p - offset) * distortion;

    p += r;
  }
  int hard = 0;
  float f = noise_turbulence(p, detail, hard);

  if (stack_valid(fac_offset)) {
    stack_store_float(stack, fac_offset, f);
  }
  if (stack_valid(color_offset)) {
    float3 color = make_float3(f,
                               noise_turbulence(make_float3(p.y, p.x, p.z), detail, hard),
                               noise_turbulence(make_float3(p.y, p.z, p.x), detail, hard));
    stack_store_float3(stack, color_offset, color);
  }
}

CCL_NAMESPACE_END
