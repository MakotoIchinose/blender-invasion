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

ccl_device void svm_node_tex_white_noise(KernelGlobals *kg,
                                         ShaderData *sd,
                                         float *stack,
                                         uint dimensions,
                                         uint vec_offset,
                                         uint w_offset,
                                         int *offset)
{
  uint4 node1 = read_node(kg, offset);

  float3 vec = stack_load_float3(stack, vec_offset);
  float w = stack_load_float(stack, w_offset);

  float r;
  switch (dimensions) {
    case 1:
      r = hash_float_01(w);
      break;
    case 2:
      r = hash_float2_01(vec.x, vec.y);
      break;
    case 3:
      r = hash_float3_01(vec.x, vec.y, vec.z);
      break;
    case 4:
      r = hash_float4_01(vec.x, vec.y, vec.z, w);
      break;
    default:
      r = 0.0f;
  }
  stack_store_float(stack, node1.y, r);
}

CCL_NAMESPACE_END
