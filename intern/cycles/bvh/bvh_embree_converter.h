/*
 * Copyright 2018, Blender Foundation.
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

#ifndef __BVH_EMBREE_CONVERTER_H__
#define __BVH_EMBREE_CONVERTER_H__

#ifdef WITH_EMBREE

#include <deque>
#include <stack>

#include "render/object.h"
#include <embree3/rtcore_builder.h>

#include "bvh_node.h"

#include "bvh.h"
CCL_NAMESPACE_BEGIN

#define BVH_NODE_SIZE_LEAF 1
#define BVH_NODE_SIZE_BASE 4
#define BVH_NODE_SIZE_4D 1
#define BVH_NODE_SIZE_MOTION_BLUR 3
#define BVH_NODE_SIZE_UNALIGNED 3

class BVHEmbreeConverter
{
public:
    BVHEmbreeConverter(RTCScene scene, std::vector<Object *> objects, const BVHParams &params);
    BVHNode *getBVH4();
    BVHNode *getBVH2();
    void fillPack(PackedBVH &pack, vector<Object *> objects);
private:
    RTCScene s;
    std::vector<Object *> objects;
    const BVHParams &params;

private:
    BVHNode* print_bvhInfo(RTCScene scene);

    void pack_instances(size_t nodes_size, size_t leaf_nodes_size, PackedBVH &pack);
    PackedBVH *pack;
    size_t packIdx;
};

CCL_NAMESPACE_END

#endif /* WITH_EMBREE */

#endif /* __BVH_EMBREE_CONVERTER_H__ */
