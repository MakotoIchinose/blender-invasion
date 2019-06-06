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

/* Clamp Node */

ccl_device void svm_node_clamp(KernelGlobals *kg,
                               ShaderData *sd,
                               float *stack,
                               uint value_offset,
                               uint min_offset,
                               uint max_offset,
                               int *offset)
{
  uint4 node1 = read_node(kg, offset);

  float value = stack_load_float(stack, value_offset);
  float min = stack_load_float(stack, min_offset);
  float max = stack_load_float(stack, max_offset);

  stack_store_float(stack, node1.y, clamp(value, min, max));
}

CCL_NAMESPACE_END
