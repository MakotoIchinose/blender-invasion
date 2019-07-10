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

#define TASKING_INTERNAL
#define RTC_NAMESPACE_BEGIN
#define RTC_NAMESPACE_OPEN
#define RTC_NAMESPACE_END

#include "embree/kernels/common/scene.h"
#include "embree/kernels/bvh/bvh.h"
#include "embree/kernels/geometry/trianglev.h"
#include "embree/kernels/geometry/trianglei.h"
#include "embree/kernels/geometry/instance.h"
#include "embree/kernels/geometry/curveNi.h"
#include "bvh_node.h"

CCL_NAMESPACE_BEGIN

class BVHEmbreeConverter
{
public:
    BVHEmbreeConverter(RTCScene scene, std::vector<Object *> objects, const BVHParams &params);
    BVHNode *getBVH4();
    BVHNode *getBVH2();
private:
    embree::Scene *s;
    std::vector<Object *> objects;
    const BVHParams &params;

private:
    template<typename Primitive>
    std::deque<BVHNode*> handleLeaf(const embree::BVH4::NodeRef &node, const BoundBox &bb);

    template<typename Primitive>
    BVHNode* nodeEmbreeToCcl(embree::BVH4::NodeRef node, ccl::BoundBox bb);

    BVHNode* print_bvhInfo(RTCScene scene);
};

CCL_NAMESPACE_END

#endif /* WITH_EMBREE */

#endif /* __BVH_EMBREE_CONVERTER_H__ */
