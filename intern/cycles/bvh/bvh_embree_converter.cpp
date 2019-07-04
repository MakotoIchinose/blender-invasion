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

/* This class implements a converter from Embree internal data structure to
 * Blender's internal structure
 */

#ifdef WITH_EMBREE

#include "bvh_embree_converter.h"

#include "bvh_node.h"
#include "render/mesh.h"

CCL_NAMESPACE_BEGIN

struct RangeInput {
    RangeInput(int id, uint visibility, BoundBox bb)
        : id(id),
          visibility(visibility),
          bb(bb) {}

    int id;
    uint visibility;
    BoundBox bb;
};

std::stack<LeafNode*> groupByRange(std::vector<RangeInput> &ids) {
    std::sort(ids.begin(), ids.end(), [](const RangeInput &lhs, const RangeInput &rhs) -> bool {
        return lhs.id > rhs.id;
    });
    std::stack<LeafNode*> groups;

    for(const RangeInput &r : ids) {
        if(!groups.empty() && groups.top()->hi == r.id) {
            LeafNode *ref = groups.top();

            ref->hi = r.id + 1;
            ref->visibility |= r.visibility;
            ref->bounds.grow(r.bb);
        } else {
            groups.push(new LeafNode(r.bb, r.visibility, r.id, r.id + 1));
        }
    }

    return groups;
}


ccl::BoundBox RTCBoundBoxToCCL(const embree::BBox3fa &bound) {
    return ccl::BoundBox(
                make_float3(bound.lower.x, bound.lower.y, bound.lower.z),
                make_float3(bound.upper.x, bound.upper.y, bound.upper.z));

}

BVHNode *bvh_shrink(BVHNode *root) {
    if(root->is_leaf()) return root;

    InnerNode *node = dynamic_cast<InnerNode*>(root);

    if(node->num_children() == 1) return bvh_shrink(root->get_child(0));
    if(node->num_children() <= 2) {
        node->children[0] = bvh_shrink(node->children[0]);
        node->children[1] = bvh_shrink(node->children[1]);
        return node;
    }

    node->children[0] = new InnerNode(merge(node->children[0]->bounds, node->children[1]->bounds), bvh_shrink(node->children[0]), bvh_shrink(node->children[1]));
    if(root->num_children() == 3) {
        node->children[1] = bvh_shrink(node->children[2]);
    } else {
	node->children[1] = new InnerNode(merge(node->children[2]->bounds, node->children[3]->bounds), bvh_shrink(node->children[2]), bvh_shrink(node->children[3]));
    }
    node->num_children_ = 2;
    return node;
}

BVHEmbreeConverter::BVHEmbreeConverter(RTCScene scene, std::vector<Object *> objects, const BVHParams &params)
    : s(reinterpret_cast<embree::Scene *>(scene)),
      objects(objects),
      params(params) {}


template<typename Primitive>
std::deque<BVHNode*> BVHEmbreeConverter::handleLeaf(const embree::BVH4::NodeRef &node, const BoundBox &) {
    size_t nb;
    Primitive *prims = reinterpret_cast<Primitive *>(node.leaf(nb));
    std::vector<RangeInput> ids; ids.reserve(nb * 4);

    for(size_t i = 0; i < nb; i++) {
        for(size_t j = 0; j < prims[i].size(); j++) {
            const auto geom_id = prims[i].geomID(j);
            const auto prim_id = prims[i].primID(j);

            embree::Geometry *g = s->get(geom_id);

            size_t prim_offset = reinterpret_cast<size_t>(g->getUserData());

            Object *obj = this->objects.at(geom_id / 2);
            Mesh *m = obj->mesh;
            BoundBox bb = BoundBox::empty;

            const Mesh::Triangle t = m->get_triangle(prim_id);
            const float3 *mesh_verts = m->verts.data();
            const float3 *mesh_vert_steps = nullptr;
            size_t motion_steps = 1;

            if (m->has_motion_blur()) {
                const Attribute *attr_motion_vertex = m->attributes.find(ATTR_STD_MOTION_VERTEX_POSITION);
                if (attr_motion_vertex) {
                    mesh_vert_steps = attr_motion_vertex->data_float3();
                    motion_steps = m->motion_steps;
                }
            }

            for (uint step = 0; step < motion_steps; ++step) {
                float3 verts[3];
                t.verts_for_step(mesh_verts, mesh_vert_steps, m->num_triangles(), motion_steps, step, verts);


              for (int i = 0; i < 3; i++) {
                  bb.grow(verts[i]);
              }
            }

            ids.push_back(RangeInput(prim_offset + prim_id, obj->visibility, bb));
        }
    }

    std::stack<LeafNode *> leafs = groupByRange(ids);
    std::deque<BVHNode *> nodes;

    while(!leafs.empty()) {
        nodes.push_back(leafs.top());
        leafs.pop();
    }

    return nodes;
}

template<>
std::deque<BVHNode*> BVHEmbreeConverter::handleLeaf<embree::InstancePrimitive>(const embree::BVH4::NodeRef &node, const BoundBox &) {
    size_t nb;
    embree::InstancePrimitive *prims = reinterpret_cast<embree::InstancePrimitive *>(node.leaf(nb));

    std::stack<LeafNode *> leafs;

    for(size_t i = 0; i < nb; i++) {
        uint id = prims[i].instance->geomID / 2;
        Object *obj = objects.at(id);
        // Better solution, but crash -> RTCBoundBoxToCCL(prims[i].instance->bounds(0));
        LeafNode *leafNode = new LeafNode(obj->bounds, obj->visibility, obj->pack_index, obj->pack_index + 1);
        leafs.push(leafNode);
    }

    std::deque<BVHNode *> nodes;
    while(!leafs.empty()) {
        nodes.push_back(leafs.top());
        leafs.pop();
    }

    return nodes;
}

template<typename Primitive>
BVHNode* BVHEmbreeConverter::nodeEmbreeToCcl(embree::BVH4::NodeRef node, ccl::BoundBox bb) {
    if(node.isLeaf()) {
        BVHNode *ret = nullptr;
        std::deque<BVHNode *> nodes = this->handleLeaf<Primitive>(node, bb);

        while(!nodes.empty()) {
            if(ret == nullptr) {
                ret = nodes.front();
                nodes.pop_front();
                continue;
            }

            if(ret->is_leaf() || ret->num_children()) {
                ret = new InnerNode(bb, &ret, 1);
            }

            InnerNode *innerNode = dynamic_cast<InnerNode*>(ret);
            innerNode->children[innerNode->num_children_++] = nodes.front();
            nodes.pop_front();

            if(ret->num_children() == 4) {
                nodes.push_back(ret);
                ret = nullptr;
            }
        }

        return ret;
    } else {
        InnerNode *ret = nullptr;

        if(node.isAlignedNode()) {
            embree::BVH4::AlignedNode *anode = node.alignedNode();

            int nb = 0;
            BVHNode *children[4];
            for(uint i = 0; i < 4; i++) {
                BVHNode *child = this->nodeEmbreeToCcl<Primitive>(anode->children[i], RTCBoundBoxToCCL(anode->bounds(i)));
                if(child != nullptr)
                    children[nb++] = child;
            }

            ret = new InnerNode(
                        bb,
                        children,
                        nb);
        } else if(node.isAlignedNodeMB()) {
            embree::BVH4::AlignedNodeMB *anode = node.alignedNodeMB();

            int nb = 0;
            BVHNode *children[4];
            for(uint i = 0; i < 4; i++) {
                BVHNode *child = this->nodeEmbreeToCcl<Primitive>(anode->children[i], RTCBoundBoxToCCL(anode->bounds(i)));
                if(child != nullptr) {
                    children[nb++] = child;
                }
            }

            ret = new InnerNode(
                        bb,
                        children,
                        nb);
        } else if (node.isUnalignedNode()) {
            std::cout << "[EMBREE - BVH] Node type is unaligned" << std::endl;
        } else if (node.isUnalignedNodeMB()) {
            std::cout << "[EMBREE - BVH] Node type is unaligned MB" << std::endl;
        } else if (node.isBarrier()) {
            std::cout << "[EMBREE - BVH] Node type is barrier ??" << std::endl;
        } else if (node.isQuantizedNode()) {
            std::cout << "[EMBREE - BVH] Node type is Quantized node !" << std::endl;
        } else if (node.isAlignedNodeMB4D()) {
            // He is an aligned node MB
            embree::BVH4::AlignedNodeMB4D *anode = node.alignedNodeMB4D();

            int nb = 0;
            BVHNode *children[4];
            for(uint i = 0; i < 4; i++) {
                BVHNode *child = this->nodeEmbreeToCcl<Primitive>(anode->children[i], RTCBoundBoxToCCL(anode->bounds(i)));
                if(child != nullptr) {
                    embree::BBox1f timeRange = anode->timeRange(i);
                    child->time_from = timeRange.lower;
                    child->time_to = timeRange.upper;
                    children[nb++] = child;
                }
            }

            ret = new InnerNode(
                        bb,
                        children,
                        nb);
        } else {
            std::cout << "[EMBREE - BVH] Node type is unknown -> " << node.type() << std::endl;
        }

        return ret;
    }
}

BVHNode* BVHEmbreeConverter::getBVH4() {
    std::vector<BVHNode *> nodes;
    BoundBox bb = BoundBox::empty;

    for (embree::Accel *a : this->s->accels) {
        std::cout << "Accel " << a->intersectors.intersector1.name << std::endl;
        embree::AccelData *ad = a->intersectors.ptr;
        switch (ad->type) {
        case embree::AccelData::TY_BVH4: {
            embree::BVH4 *bvh = dynamic_cast<embree::BVHN<4> *>(ad);
            std::cout << "Prim type -> " << bvh->primTy->name() << std::endl;

            embree::BVH4::NodeRef root = bvh->root;
            BVHNode *rootNode = nullptr;
            if(bvh->primTy == &embree::Triangle4v::type) {
                rootNode = this->nodeEmbreeToCcl<embree::Triangle4v>(root, RTCBoundBoxToCCL(bvh->bounds.bounds()));
            } else if(bvh->primTy == &embree::InstancePrimitive::type) {
                rootNode = nodeEmbreeToCcl<embree::InstancePrimitive>(root, RTCBoundBoxToCCL(bvh->bounds.bounds()));
            } else if(bvh->primTy == &embree::Triangle4i::type) {
                rootNode = nodeEmbreeToCcl<embree::Triangle4i>(root, RTCBoundBoxToCCL(bvh->bounds.bounds()));
            } else {
                std::cout << "[EMBREE - BVH] Unknown primitive type " << bvh->primTy->name() << std::endl;
            }

            if(rootNode != nullptr) {
                bb.grow(rootNode->bounds);
                nodes.push_back(rootNode);
            }
        } break;
        default:
            std::cout << "[EMBREE - BVH] Unknown acceleration type " << ad->type << std::endl;
            break;
        }
    }
    std::cout << "[DONE]" << std::endl;

    if(nodes.size() == 1)
        return nodes.front();

    return new InnerNode(bb, nodes.data(), nodes.size());
}

BVHNode* BVHEmbreeConverter::getBVH2() {
    BVHNode *root = this->getBVH4();
    std::cout << "BVH4 SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    root = bvh_shrink(root);
    std::cout << "BVH2 SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    return root;
}

CCL_NAMESPACE_END

#endif /* WITH_EMBREE */
