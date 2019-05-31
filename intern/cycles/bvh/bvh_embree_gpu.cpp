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

#ifdef WITH_EMBREE

#include "bvh/bvh_embree_gpu.h"

#include "render/mesh.h"
#include "render/object.h"
#include "util/util_foreach.h"
#include "util/util_logging.h"
#include "util/util_progress.h"

CCL_NAMESPACE_BEGIN

typedef struct {
    BVHEmbreeGPU *bvhBldr;
    Progress *p;
} UserParams;


BVHEmbreeGPU::BVHEmbreeGPU(const BVHParams& params_, const vector<Object*>& objects_)
    : BVH(params_, objects_), stats(nullptr)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    this->rtc_device = rtcNewDevice("verbose=1");

    rtcSetDeviceErrorFunction(this->rtc_device, [](void*, enum RTCError, const char* str) {
        VLOG(1) << str;
    }, nullptr);

    pack.root_index = -1;
}

BVHEmbreeGPU::~BVHEmbreeGPU()
{
    rtcReleaseDevice(this->rtc_device);
}

ccl::BoundBox RTCBoundBoxToCCL(const RTCBounds *bound) {
    return ccl::BoundBox(
                make_float3(bound->lower_x, bound->lower_y, bound->lower_z),
                make_float3(bound->upper_x, bound->upper_y, bound->upper_z));

}
ccl::BoundBox RTCBuildPrimToCCL(const RTCBuildPrimitive &bound) {
    return ccl::BoundBox(
                make_float3(bound.lower_x, bound.lower_y, bound.lower_z),
                make_float3(bound.upper_x, bound.upper_y, bound.upper_z));

}

void CCLBoundBoxToRTC(const ccl::BoundBox &bb, RTCBounds *bound) {
    bound->lower_x = bb.min.x;
    bound->lower_y = bb.min.y;
    bound->lower_z = bb.min.z;

    bound->upper_x = bb.max.x;
    bound->upper_y = bb.max.y;
    bound->upper_z = bb.max.z;
}

void BVHEmbreeGPU::build(Progress& progress, Stats *stats_)
{
    this->stats = stats_;
    rtcSetDeviceMemoryMonitorFunction(this->rtc_device, [](void* userPtr, const ssize_t bytes, const bool) -> bool {
        Stats *stats = static_cast<Stats*>(userPtr);
        if(stats == NULL) return true;

        if(bytes > 0) {
            stats->mem_alloc(static_cast<size_t>(bytes));
        }
        else {
            stats->mem_free(static_cast<size_t>(-bytes));
        }
        return true;
    }, stats);

    progress.set_substatus("Building BVH");


    struct RTCBuildArguments args = rtcDefaultBuildArguments();
    args.byteSize = sizeof(args);

    const bool dynamic = params.bvh_type == SceneParams::BVH_DYNAMIC;

    args.buildFlags = (dynamic ? RTC_BUILD_FLAG_DYNAMIC : RTC_BUILD_FLAG_NONE);
    args.buildQuality = dynamic ? RTC_BUILD_QUALITY_LOW :
                                  (params.use_spatial_split ? RTC_BUILD_QUALITY_HIGH : RTC_BUILD_QUALITY_MEDIUM);

    /* Count triangles first so we can reserve arrays once. */
    size_t prim_count = 0;

    foreach(Object *ob, objects) {
        prim_count += ob->mesh->num_triangles();
    }

    pack.prim_object.reserve(prim_count);
    pack.prim_type.reserve(prim_count);
    pack.prim_index.reserve(prim_count);
    pack.prim_tri_index.reserve(prim_count);

    this->offset.resize(objects.size());
    unsigned int i = 0;

    pack.object_node.clear();

    vector<RTCBuildPrimitive> prims;
    prims.reserve(objects.size() * 3);
    foreach(Object *ob, objects) {
        add_object(ob, i);

        const float3 *mesh_verts = ob->mesh->verts.data();
        for(size_t tri = 0; tri < ob->mesh->num_triangles(); ++tri) {
            BoundBox bb = BoundBox::empty;
            ob->mesh->get_triangle(tri).bounds_grow(mesh_verts, bb);
            RTCBuildPrimitive prim;
            prim.lower_x = bb.min.x;
            prim.lower_y = bb.min.y;
            prim.lower_z = bb.min.z;
            prim.upper_x = bb.max.x;
            prim.upper_y = bb.max.y;
            prim.upper_z = bb.max.z;
            prim.geomID = i;
            prim.primID = tri;

            prims.push_back(prim);
        }

        ++i;
        if(progress.get_cancel()) return;
    }

    if(progress.get_cancel()) {
        stats = nullptr;
        return;
    }

    args.bvh = rtcNewBVH(this->rtc_device);
    args.maxBranchingFactor = 2;

    args.primitives = prims.data();
    args.primitiveCount = prims.size();
    args.primitiveArrayCapacity = prims.capacity();

    args.sahBlockSize = 1;
    args.maxDepth = BVHParams::MAX_DEPTH;
    args.traversalCost = this->params.sah_node_cost;
    // 2 is a corrective factor for Embree (may depend on the scene for optimal results)
    args.intersectionCost = this->params.sah_primitive_cost * 2;

    args.createNode = [](RTCThreadLocalAllocator alloc, unsigned int numChildren, void*) -> void* {
        CHECK_EQ(numChildren, 2) << "Should only have two children";
        void* ptr = rtcThreadLocalAlloc(alloc,sizeof(InnerNode),16);
        return new (ptr) InnerNode(BoundBox::empty);
    };
    args.setNodeBounds = [](void* nodePtr, const RTCBounds** bounds, unsigned int numChildren, void*) {
        InnerNode *node = static_cast<InnerNode*>(nodePtr);
        node->num_children_ = static_cast<int>(numChildren);
        for (size_t i=0; i < numChildren; i++) {
            node->bounds.grow(RTCBoundBoxToCCL(bounds[i]));
        }
    };
    args.setNodeChildren = [](void* nodePtr, void** childPtr, unsigned int numChildren, void*) {
        InnerNode *node = static_cast<InnerNode*>(nodePtr);
        node->num_children_ = static_cast<int>(numChildren);
        for (size_t i=0; i < numChildren; i++) {
            node->children[i] = static_cast<BVHNode*>(childPtr[i]);
        }
    };
    args.createLeaf = [](RTCThreadLocalAllocator alloc, const RTCBuildPrimitive* prims, size_t numPrims, void *user_ptr) -> void* {
        UserParams *userParams = static_cast<UserParams*>(user_ptr);
        void* ptr = rtcThreadLocalAlloc(alloc, sizeof(LeafNode), 16);

        int min = 999999,
                max = 0;
        uint visibility = 0;
        BoundBox bounds = BoundBox::empty;

        for(size_t i = 0; i < numPrims; i++) {
            const Object *ob = userParams->bvhBldr->objects.at(prims[i].geomID);
            const int idx = userParams->bvhBldr->offset[prims[i].geomID] + prims[i].primID;

            if(idx < min) min = idx;
            if(idx > max) max = idx;

            visibility |= ob->visibility;
            bounds.grow(RTCBuildPrimToCCL(prims[i]));
        }

        return new (ptr) LeafNode(bounds,
                                  visibility,
                                  min,
                                  max + 1);
    };

    args.splitPrimitive = [](const struct RTCBuildPrimitive* prims, unsigned int dim, float pos, RTCBounds *l, RTCBounds *r, void *user_ptr) {
        CHECK_LE(dim, 2) << "Dimension should be lower than 3";

        UserParams *userParams = static_cast<UserParams*>(user_ptr);

        const Object *ob = userParams->bvhBldr->objects.at(prims->geomID);
        const float3 *mesh_verts = ob->mesh->verts.data();
        const float3 *mesh_vert_steps = NULL;
        size_t motion_steps = 1;

        if (ob->mesh->has_motion_blur()) {
            const Attribute *attr_motion_vertex = ob->mesh->attributes.find(ATTR_STD_MOTION_VERTEX_POSITION);
            if (attr_motion_vertex) {
                mesh_vert_steps = attr_motion_vertex->data_float3();
                motion_steps = ob->mesh->motion_steps;
            }
        }

        Mesh::Triangle t = ob->mesh->get_triangle(prims->primID);

        BoundBox left_bounds = BoundBox::empty;
        BoundBox right_bounds = BoundBox::empty;

        for (uint step = 0; step < motion_steps; ++step) {
            float3 verts[3];
            t.verts_for_step(mesh_verts, mesh_vert_steps, ob->mesh->num_triangles(), motion_steps, step, verts);

            float3 v1 = verts[2];

            for (int i = 0; i < 3; i++) {
                float3 v0 = v1;
                v1 = verts[i];
                float v0p = v0[dim];
                float v1p = v1[dim];

                /* insert vertex to the boxes it belongs to. */
                if (v0p <= pos)
                    left_bounds.grow(v0);

                if (v0p >= pos)
                    right_bounds.grow(v0);

                /* edge intersects the plane => insert intersection to both boxes. */
                if ((v0p < pos && v1p > pos) || (v0p > pos && v1p < pos)) {
                    float3 t = lerp(v0, v1, clamp((pos - v0p) / (v1p - v0p), 0.0f, 1.0f));
                    left_bounds.grow(t);
                    right_bounds.grow(t);
                }
            }
        }

        CCLBoundBoxToRTC(left_bounds, l);
        CCLBoundBoxToRTC(right_bounds, r);
    };
    args.buildProgress = [](void* user_ptr, const double n) -> bool {
        UserParams *userParams = static_cast<UserParams*>(user_ptr);

        if(time_dt() - userParams->bvhBldr->progress_start_time < 0.25) {
            return true;
        }

        string msg = string_printf("Building BVH (embree) %.0f%%", n * 100.0);
        userParams->p->set_substatus(msg);
        userParams->bvhBldr->progress_start_time = time_dt();

        return !userParams->p->get_cancel();
    };

    UserParams p;
    p.bvhBldr = this;
    p.p = &progress;
    args.userPtr = &p;

    BVHNode* root = static_cast<BVHNode*>(rtcBuildBVH(&args));
    root->update_time();
    root->update_visibility();

    std::cout << "Subtree SAH cost " << root->computeSubtreeSAHCost(this->params) << std::endl;

    pack_primitives();

    progress.set_substatus("Packing geometry");
    pack_nodes(root);

    rtcReleaseBVH(args.bvh);
    stats = nullptr;
}

void BVHEmbreeGPU::add_object(const Object *ob, const unsigned int i)
{
    const auto offset = pack.prim_index.size();
    this->offset[i] = offset;

    Mesh *mesh = ob->mesh;

    const size_t num_triangles = mesh->num_triangles();

    size_t prim_object_size = pack.prim_object.size();
    pack.prim_object.resize(prim_object_size + num_triangles);

    size_t prim_type_size = pack.prim_type.size();
    pack.prim_type.resize(prim_type_size + num_triangles);

    size_t prim_index_size = pack.prim_index.size();
    pack.prim_index.resize(prim_index_size + num_triangles);
    pack.prim_tri_index.resize(prim_index_size + num_triangles);

    for(size_t j = 0; j < num_triangles; ++j) {
        pack.prim_object[prim_object_size + j] = static_cast<int>(i);
        pack.prim_type[prim_type_size + j] = PRIMITIVE_TRIANGLE;
        pack.prim_index[prim_index_size + j] = static_cast<int>(j);
        pack.prim_tri_index[prim_index_size + j] = static_cast<unsigned int>(j);
    }
}

void BVHEmbreeGPU::pack_leaf(size_t idx, const LeafNode *leaf)
{
    float4 data;
    memset(&data, 0, sizeof(data));

    data.x = __int_as_float(leaf->lo);
    data.y = __int_as_float(leaf->hi);
    data.z = __uint_as_float(leaf->visibility);
    if(leaf->num_triangles() != 0) {
        data.w = __uint_as_float(static_cast<uint>(pack.prim_type[static_cast<size_t>(leaf->lo)]));
    }

    memcpy(&pack.leaf_nodes[idx], &data, sizeof(float4));
}

void BVHEmbreeGPU::pack_inner(size_t idx, const BVHNode *e0, int idx0, const BVHNode *e1, int idx1)
{
    const BoundBox b0 = e0->bounds,
            b1 = e1->bounds;
    int visibility0 = static_cast<int>(e0->visibility),
            visibility1 = static_cast<int>(e1->visibility);

    int4 data[4] = {
        make_int4(visibility0 & ~PATH_RAY_NODE_UNALIGNED,
        visibility1 & ~PATH_RAY_NODE_UNALIGNED,
        idx0,
        idx1),

        make_int4(__float_as_int(b0.min.x),
        __float_as_int(b1.min.x),
        __float_as_int(b0.max.x),
        __float_as_int(b1.max.x)),

        make_int4(__float_as_int(b0.min.y),
        __float_as_int(b1.min.y),
        __float_as_int(b0.max.y),
        __float_as_int(b1.max.y)),

        make_int4(__float_as_int(b0.min.z),
        __float_as_int(b1.min.z),
        __float_as_int(b0.max.z),
        __float_as_int(b1.max.z)),
    };

    memcpy(&pack.nodes[idx], data, sizeof(int4)*4);
}

void BVHEmbreeGPU::pack_nodes(const BVHNode *root)
{
    const size_t num_nodes = static_cast<size_t>(root->getSubtreeSize(BVH_STAT_NODE_COUNT));
    const size_t num_leaf_nodes = static_cast<size_t>(root->getSubtreeSize(BVH_STAT_LEAF_COUNT));

    CHECK_LE(num_leaf_nodes, num_nodes);
    const size_t num_inner_nodes = num_nodes - num_leaf_nodes;
    size_t node_size = num_inner_nodes * 4;
    /* Resize arrays */
    pack.nodes.clear();
    pack.leaf_nodes.clear();

    pack_instances(node_size, num_leaf_nodes);

    size_t nextNodeIdx = 0, nextLeafNodeIdx = 0;

    vector<BVHStackEntry> stack;
    stack.reserve(BVHParams::MAX_DEPTH*2);
    if(root->is_leaf()) {
        stack.push_back(BVHStackEntry(root, static_cast<int>(nextLeafNodeIdx++)));
    }
    else {
        stack.push_back(BVHStackEntry(root, static_cast<int>(nextNodeIdx)));
        nextNodeIdx += 4;
    }

    while(stack.size()) {
        BVHStackEntry e = stack.back();
        stack.pop_back();

        if(e.node->is_leaf()) {
            /* leaf node */
            const LeafNode *leaf = reinterpret_cast<const LeafNode*>(e.node);
            CHECK_NOTNULL(leaf);
            CHECK_LE(e.idx + 1, static_cast<int>(pack.leaf_nodes.size()));

            pack_leaf(static_cast<size_t>(e.idx), leaf);
        }
        else {
            /* inner node */
            size_t idx[2];
            for(int i = 0; i < 2; ++i) {
                if(e.node->get_child(i)->is_leaf()) {
                    idx[i] = nextLeafNodeIdx++;
                }
                else {
                    idx[i] = nextNodeIdx;
                    nextNodeIdx += 4;
                }
            }

            stack.push_back(BVHStackEntry(e.node->get_child(0), static_cast<int>(idx[0])));
            stack.push_back(BVHStackEntry(e.node->get_child(1), static_cast<int>(idx[1])));

            BVHStackEntry *e0 = &stack[stack.size()-2],
                    *e1 = &stack[stack.size()-1];

            int c0 = e0->encodeIdx(),
                    c1 = e1->encodeIdx();

            CHECK_LE(e.idx + 4, pack.nodes.size());
            CHECK(c0 < 0 || c0 < static_cast<int>(pack.nodes.size()));
            CHECK(c1 < 0 || c1 < static_cast<int>(pack.nodes.size()));

            pack_inner(static_cast<size_t>(e.idx), e0->node, c0, e1->node, c1);
        }
    }
    CHECK_EQ(node_size, nextNodeIdx) << "Should have handled all node";
    /* root index to start traversal at, to handle case of single leaf node */
    pack.root_index = (root->is_leaf())? -1: 0;
}

BVHNode *BVHEmbreeGPU::widen_children_nodes(const BVHNode * /*root*/)
{
    DLOG(FATAL) << "Must not be called";
    return NULL;
}

void BVHEmbreeGPU::refit_nodes()
{
    DLOG(FATAL) << "Must not be called";
}

CCL_NAMESPACE_END

#endif  /* WITH_EMBREE */
