/*
 * Copyright 2011-2014 Blender Foundation
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

ccl_device void svm_mapping(
    float3 *outVec, NodeMappingType vecType, float3 vecIn, float3 loc, float3 rot, float3 size)
{
  Transform rot_t;
  if (vecType == NODE_MAPPING_TYPE_TEXTURE) {
    rot_t = euler_to_transform(-rot);
  }
  else {
    rot_t = euler_to_transform(rot);
  }

  switch (vecType) {
    case NODE_MAPPING_TYPE_POINT:
      *outVec = transform_direction(&rot_t, (vecIn * size)) + loc;
      break;
    case NODE_MAPPING_TYPE_TEXTURE:
      *outVec = safe_divide_float3_float3(transform_direction(&rot_t, (vecIn - loc)), size);
      break;
    case NODE_MAPPING_TYPE_VECTOR:
      *outVec = transform_direction(&rot_t, (vecIn * size));
      break;
    case NODE_MAPPING_TYPE_NORMAL:
      *outVec = safe_normalize(
          transform_direction(&rot_t, safe_divide_float3_float3(vecIn, size)));
      break;
    default:
      *outVec = make_float3(0.0f, 0.0f, 0.0f);
  }
}

CCL_NAMESPACE_END
