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

/* Utility functions */

void packPush(PackedBVH *pack, const size_t packIdx, const int object_id, const int prim_id, const int prim_type, const uint visibility, const uint tri_index) {
    pack->prim_index.resize(packIdx + 1);
    pack->prim_type.resize(packIdx + 1);
    pack->prim_object.resize(packIdx + 1);
    pack->prim_visibility.resize(packIdx + 1);
    pack->prim_tri_index.resize(packIdx + 1);

    pack->prim_index[packIdx] = prim_id;
    pack->prim_type[packIdx] = prim_type;
    pack->prim_object[packIdx] = object_id;
    pack->prim_visibility[packIdx] = visibility;
    pack->prim_tri_index[packIdx] = tri_index;

}

void pushVec(PackedBVH *pack, const embree::Vec3f p0, const embree::Vec3f p1, const embree::Vec3f p2) {
    pack->prim_tri_verts.push_back_slow(make_float4(p0.x, p0.y, p0.z, 1));
    pack->prim_tri_verts.push_back_slow(make_float4(p1.x, p1.y, p1.z, 1));
    pack->prim_tri_verts.push_back_slow(make_float4(p2.x, p2.y, p2.z, 1));
}

ccl::BoundBox RTCBoundBoxToCCL(const embree::BBox3fa &bound) {
    return ccl::BoundBox(
                make_float3(bound.lower.x, bound.lower.y, bound.lower.z),
                make_float3(bound.upper.x, bound.upper.y, bound.upper.z));

}

BVHNode *bvh_shrink(BVHNode *root) {
    if(root->is_leaf()) {
        if(root->num_triangles() == 0) // Remove empty leafs
            return nullptr;
        else
            return root;
    }

    InnerNode *node = dynamic_cast<InnerNode*>(root);

    int num_children = 0;
    BVHNode* children[4];

    for(int i = 0; i < node->num_children(); ++i) {
        BVHNode *child = bvh_shrink(node->get_child(i));
        if(child != nullptr)
            children[num_children++] = child;
    }

    if(num_children == 0) {
        delete root;
        return nullptr;
    }

    if(num_children == 1) {
        delete root;
        return children[0];
    }

    // We have 2 node or more, we'll pack them into 2 nodes (to respect BVH2)
    node->num_children_ = 2;

    if(num_children == 2) {
        node->children[0] = children[0];
        node->children[1] = children[1];
        return node;
    }

    node->children[0] = new InnerNode(merge(children[0]->bounds, children[1]->bounds), children[0], children[1]);
    if(num_children == 3) {
        node->children[1] = node->children[2];
    } else {
        node->children[1] = new InnerNode(merge(children[2]->bounds, children[3]->bounds), children[2], children[3]);
    }
    return node;
}

BVHEmbreeConverter::BVHEmbreeConverter(RTCScene scene, std::vector<Object *> objects, const BVHParams &params)
    : s(reinterpret_cast<embree::Scene *>(scene)),
      objects(objects),
      params(params) {}

template<>
std::deque<BVHNode*> BVHEmbreeConverter::handleLeaf<embree::Triangle4i>(const embree::BVH4::NodeRef &node, const BoundBox &bb) {
    size_t from = this->packIdx,
            size = 0;

    size_t nb;
    embree::Triangle4i *prims = reinterpret_cast<embree::Triangle4i *>(node.leaf(nb));

    uint visibility = 0;
    for(size_t i = 0; i < nb; i++) {
        for(size_t j = 0; j < prims[i].size(); j++) {
            size++;
            const auto geom_id = prims[i].geomID(j);
            const auto prim_id = prims[i].primID(j);

            const int object_id = geom_id / 2;
            Object *obj = this->objects.at(object_id);

            int prim_type = obj->mesh->has_motion_blur() ? PRIMITIVE_MOTION_TRIANGLE : PRIMITIVE_TRIANGLE;

            visibility |= obj->visibility;
            packPush(this->pack, this->packIdx++, object_id, prim_id, prim_type, obj->visibility, this->pack->prim_tri_verts.size());
            pushVec(this->pack,
                    prims[i].getVertex(prims[i].v0, j, this->s),
                    prims[i].getVertex(prims[i].v1, j, this->s),
                    prims[i].getVertex(prims[i].v2, j, this->s));
        }
    }

    return {new LeafNode(bb, visibility, from, from + size)};
}

template<>
std::deque<BVHNode*> BVHEmbreeConverter::handleLeaf<embree::Triangle4v>(const embree::BVH4::NodeRef &node, const BoundBox &bb) {
    size_t from = this->packIdx,
            size = 0;

    size_t nb;
    embree::Triangle4v *prims = reinterpret_cast<embree::Triangle4v *>(node.leaf(nb));

    uint visibility = 0;
    for(size_t i = 0; i < nb; i++) {
        for(size_t j = 0; j < prims[i].size(); j++) {
            size++;
            const auto geom_id = prims[i].geomID(j);
            const auto prim_id = prims[i].primID(j);

            const int object_id = geom_id / 2;
            Object *obj = this->objects.at(object_id);

            int prim_type = obj->mesh->has_motion_blur() ? PRIMITIVE_MOTION_TRIANGLE : PRIMITIVE_TRIANGLE;

            visibility |= obj->visibility;
            packPush(this->pack, this->packIdx++, object_id, prim_id, prim_type, obj->visibility, this->pack->prim_tri_verts.size());

            this->pack->prim_tri_verts.push_back_slow(make_float4(
                                                          prims[i].v0.x[j],
                                                          prims[i].v0.y[j],
                                                          prims[i].v0.z[j],
                                                          1));
            this->pack->prim_tri_verts.push_back_slow(make_float4(
                                                          prims[i].v1.x[j],
                                                          prims[i].v1.y[j],
                                                          prims[i].v1.z[j],
                                                          1));
            this->pack->prim_tri_verts.push_back_slow(make_float4(
                                                          prims[i].v2.x[j],
                                                          prims[i].v2.y[j],
                                                          prims[i].v2.z[j],
                                                          1));
        }
    }

    return {new LeafNode(bb, visibility, from, from + size)};
}

template<>
std::deque<BVHNode*> BVHEmbreeConverter::handleLeaf<embree::InstancePrimitive>(const embree::BVH4::NodeRef &node, const BoundBox &) {
    size_t nb;
    embree::InstancePrimitive *prims = reinterpret_cast<embree::InstancePrimitive *>(node.leaf(nb));

    std::deque<BVHNode *> leafs;

    for(size_t i = 0; i < nb; i++) {
        uint id = prims[i].instance->geomID / 2;
        Object *obj = objects.at(id);
        /* TODO Better solution, but crash
         * BoundBox bb = RTCBoundBoxToCCL(prims[i].instance->bounds(0));
         */
        LeafNode *leafNode = new LeafNode(obj->bounds, obj->visibility, this->packIdx, this->packIdx + 1);
        leafs.push_back(leafNode);

        packPush(this->pack, this->packIdx++, id, -1, PRIMITIVE_NONE, obj->visibility, -1);
    }

    return leafs;
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

            /* If it's a leaf or a full node -> create a new parrent */
            if(ret->is_leaf() || ret->num_children() == 4) {
                ret = new InnerNode(bb, &ret, 1);
            }

            InnerNode *innerNode = dynamic_cast<InnerNode*>(ret);
            innerNode->children[innerNode->num_children_++] = nodes.front();
            innerNode->bounds.grow(nodes.front()->bounds);
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
                rootNode = nodeEmbreeToCcl<embree::Triangle4v>(root, RTCBoundBoxToCCL(bvh->bounds.bounds()));
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

BoundBox bvh_tighten(BVHNode *root) {
    if(root->is_leaf())
        return root->bounds;

    assert(root->num_children() == 2);
    BoundBox bb = BoundBox::empty;
    for(int i = 0; i < root->num_children(); i++) {
        bb.grow(bvh_tighten(root->get_child(i)));
    }

    if(std::abs(root->bounds.area() - bb.area()) > .05f) {
        std::cout << "Area " << root->bounds.area() << "\t" << bb.area() << std::endl;
    }
    root->bounds.intersect(bb);
    return root->bounds;
}

BVHNode* BVHEmbreeConverter::getBVH2() {
    BVHNode *root = this->getBVH4();
    std::cout << root->getSubtreeSize(BVH_STAT_TIMELIMIT_NODE) << " times nodes" << std::endl;
    std::cout << "BVH4 SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    root = bvh_shrink(root);
    std::cout << root->getSubtreeSize(BVH_STAT_TIMELIMIT_NODE) << " times nodes" << std::endl;
    std::cout << "BVH2 SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    bvh_tighten(root);
    std::cout << "BVH² SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    return root;
}

#define BVH_NODE_SIZE (4+1)
#define BVH_NODE_LEAF_SIZE 1
#define BVH_UNALIGNED_NODE_SIZE 7



void BVHEmbreeConverter::pack_instances(size_t nodes_size, size_t leaf_nodes_size, PackedBVH &pack)
{
  /* Adjust primitive index to point to the triangle in the global array, for
   * meshes with transform applied and already in the top level BVH.
   */
  for (size_t i = 0; i < pack.prim_index.size(); i++)
    if (pack.prim_index[i] != -1) {
      if (pack.prim_type[i] & PRIMITIVE_ALL_CURVE)
        pack.prim_index[i] += objects[pack.prim_object[i]]->mesh->curve_offset;
      else
        pack.prim_index[i] += objects[pack.prim_object[i]]->mesh->tri_offset;
    }

  /* track offsets of instanced BVH data in global array */
  size_t prim_offset = pack.prim_index.size();
  size_t nodes_offset = nodes_size;
  size_t nodes_leaf_offset = leaf_nodes_size;

  /* clear array that gives the node indexes for instanced objects */
  pack.object_node.clear();

  /* reserve */
  size_t prim_index_size = pack.prim_index.size();
  size_t prim_tri_verts_size = pack.prim_tri_verts.size();

  size_t pack_prim_index_offset = prim_index_size;
  size_t pack_prim_tri_verts_offset = prim_tri_verts_size;
  size_t pack_nodes_offset = nodes_size;
  size_t pack_leaf_nodes_offset = leaf_nodes_size;
  size_t object_offset = 0;

  map<Mesh *, int> mesh_map;

  foreach (Object *ob, objects) {
    Mesh *mesh = ob->mesh;
    BVH *bvh = mesh->bvh;

    if (mesh->need_build_bvh()) {
      if (mesh_map.find(mesh) == mesh_map.end()) {
        prim_index_size += bvh->pack.prim_index.size();
        prim_tri_verts_size += bvh->pack.prim_tri_verts.size();
        nodes_size += bvh->pack.nodes.size();
        leaf_nodes_size += bvh->pack.leaf_nodes.size();

        mesh_map[mesh] = 1;
      }
    }
  }

  mesh_map.clear();

  pack.prim_index.resize(prim_index_size);
  pack.prim_type.resize(prim_index_size);
  pack.prim_object.resize(prim_index_size);
  pack.prim_visibility.resize(prim_index_size);
  pack.prim_tri_verts.resize(prim_tri_verts_size);
  pack.prim_tri_index.resize(prim_index_size);
  pack.nodes.resize(nodes_size);
  pack.leaf_nodes.resize(leaf_nodes_size);
  pack.object_node.resize(objects.size());

  if (params.num_motion_curve_steps > 0 || params.num_motion_triangle_steps > 0) {
    pack.prim_time.resize(prim_index_size);
  }

  int *pack_prim_index = (pack.prim_index.size()) ? &pack.prim_index[0] : nullptr;
  int *pack_prim_type = (pack.prim_type.size()) ? &pack.prim_type[0] : nullptr;
  int *pack_prim_object = (pack.prim_object.size()) ? &pack.prim_object[0] : nullptr;
  uint *pack_prim_visibility = (pack.prim_visibility.size()) ? &pack.prim_visibility[0] : nullptr;
  float4 *pack_prim_tri_verts = (pack.prim_tri_verts.size()) ? &pack.prim_tri_verts[0] : nullptr;
  uint *pack_prim_tri_index = (pack.prim_tri_index.size()) ? &pack.prim_tri_index[0] : nullptr;
  int4 *pack_nodes = (pack.nodes.size()) ? &pack.nodes[0] : nullptr;
  int4 *pack_leaf_nodes = (pack.leaf_nodes.size()) ? &pack.leaf_nodes[0] : nullptr;
  float2 *pack_prim_time = (pack.prim_time.size()) ? &pack.prim_time[0] : nullptr;

  /* merge */
  foreach (Object *ob, objects) {
    Mesh *mesh = ob->mesh;

    /* We assume that if mesh doesn't need own BVH it was already included
     * into a top-level BVH and no packing here is needed.
     */
    if (!mesh->need_build_bvh()) {
      pack.object_node[object_offset++] = 0;
      continue;
    }

    /* if mesh already added once, don't add it again, but used set
     * node offset for this object */
    map<Mesh *, int>::iterator it = mesh_map.find(mesh);

    if (mesh_map.find(mesh) != mesh_map.end()) {
      int noffset = it->second;
      pack.object_node[object_offset++] = noffset;
      continue;
    }

    BVH *bvh = mesh->bvh;

    int noffset = nodes_offset;
    int noffset_leaf = nodes_leaf_offset;
    int mesh_tri_offset = mesh->tri_offset;
    int mesh_curve_offset = mesh->curve_offset;

    /* fill in node indexes for instances */
    if (bvh->pack.root_index == -1)
      pack.object_node[object_offset++] = -noffset_leaf - 1;
    else
      pack.object_node[object_offset++] = noffset;

    mesh_map[mesh] = pack.object_node[object_offset - 1];

    /* merge primitive, object and triangle indexes */
    if (bvh->pack.prim_index.size()) {
      size_t bvh_prim_index_size = bvh->pack.prim_index.size();
      int *bvh_prim_index = &bvh->pack.prim_index[0];
      int *bvh_prim_type = &bvh->pack.prim_type[0];
      uint *bvh_prim_visibility = &bvh->pack.prim_visibility[0];
      uint *bvh_prim_tri_index = &bvh->pack.prim_tri_index[0];
      float2 *bvh_prim_time = bvh->pack.prim_time.size() ? &bvh->pack.prim_time[0] : nullptr;

      for (size_t i = 0; i < bvh_prim_index_size; i++) {
        if (bvh->pack.prim_type[i] & PRIMITIVE_ALL_CURVE) {
          pack_prim_index[pack_prim_index_offset] = bvh_prim_index[i] + mesh_curve_offset;
          pack_prim_tri_index[pack_prim_index_offset] = -1;
        }
        else {
          pack_prim_index[pack_prim_index_offset] = bvh_prim_index[i] + mesh_tri_offset;
          pack_prim_tri_index[pack_prim_index_offset] = bvh_prim_tri_index[i] +
                                                        pack_prim_tri_verts_offset;
        }

        pack_prim_type[pack_prim_index_offset] = bvh_prim_type[i];
        pack_prim_visibility[pack_prim_index_offset] = bvh_prim_visibility[i];
        pack_prim_object[pack_prim_index_offset] = 0;  // unused for instances
        if (bvh_prim_time != nullptr) {
          pack_prim_time[pack_prim_index_offset] = bvh_prim_time[i];
        }
        pack_prim_index_offset++;
      }
    }

    /* Merge triangle vertices data. */
    if (bvh->pack.prim_tri_verts.size()) {
      const size_t prim_tri_size = bvh->pack.prim_tri_verts.size();
      memcpy(pack_prim_tri_verts + pack_prim_tri_verts_offset,
             &bvh->pack.prim_tri_verts[0],
             prim_tri_size * sizeof(float4));
      pack_prim_tri_verts_offset += prim_tri_size;
    }

    /* merge nodes */
    if (bvh->pack.leaf_nodes.size()) {
      int4 *leaf_nodes_offset = &bvh->pack.leaf_nodes[0];
      size_t leaf_nodes_offset_size = bvh->pack.leaf_nodes.size();
      for (size_t i = 0, j = 0; i < leaf_nodes_offset_size; i += BVH_NODE_LEAF_SIZE, j++) {
        int4 data = leaf_nodes_offset[i];
        data.x += prim_offset;
        data.y += prim_offset;
        pack_leaf_nodes[pack_leaf_nodes_offset] = data;
        for (int j = 1; j < BVH_NODE_LEAF_SIZE; ++j) {
          pack_leaf_nodes[pack_leaf_nodes_offset + j] = leaf_nodes_offset[i + j];
        }
        pack_leaf_nodes_offset += BVH_NODE_LEAF_SIZE;
      }
    }

    if (bvh->pack.nodes.size()) {
      int4 *bvh_nodes = &bvh->pack.nodes[0];
      size_t bvh_nodes_size = bvh->pack.nodes.size();

      for (size_t i = 0, j = 0; i < bvh_nodes_size; j++) {
        size_t nsize = (bvh_nodes[i].x & PATH_RAY_NODE_UNALIGNED) ? BVH_UNALIGNED_NODE_SIZE : BVH_NODE_SIZE;

        /* Modify offsets into arrays */
        int4 data = bvh_nodes[i];
        int4 data1 = bvh_nodes[i - 1];
          data.z += (data.z < 0) ? -noffset_leaf : noffset;
          data.w += (data.w < 0) ? -noffset_leaf : noffset;
        pack_nodes[pack_nodes_offset] = data;

        /* Usually this copies nothing, but we better
         * be prepared for possible node size extension.
         */
        memcpy(&pack_nodes[pack_nodes_offset + 1],
               &bvh_nodes[i + 1],
               sizeof(int4) * (nsize - 1));

        pack_nodes_offset += nsize;
        i += nsize;
      }
    }

    nodes_offset += bvh->pack.nodes.size();
    nodes_leaf_offset += bvh->pack.leaf_nodes.size();
    prim_offset += bvh->pack.prim_index.size();
  }
}

void pack_leaf(const BVHStackEntry &e, const LeafNode *leaf, PackedBVH &pack) {
    assert(e.idx + BVH_NODE_LEAF_SIZE <= pack.leaf_nodes.size());
    float4 data[BVH_NODE_LEAF_SIZE];
    memset(data, 0, sizeof(data));
    if (leaf->num_triangles() == 1 && pack.prim_index[leaf->lo] == -1) {
        /* object */
        data[0].x = __int_as_float(~(leaf->lo));
        data[0].y = __int_as_float(0);
    }
    else {
        /* triangle */
        data[0].x = __int_as_float(leaf->lo);
        data[0].y = __int_as_float(leaf->hi);
    }
    data[0].z = __uint_as_float(leaf->visibility);
    if (leaf->num_triangles() != 0) {
        data[0].w = __uint_as_float(pack.prim_type[leaf->lo]);
    } else {
        printf("A leaf can be empty\n");
    }

    memcpy(&pack.leaf_nodes[e.idx], data, sizeof(float4) * BVH_NODE_LEAF_SIZE);
}

void pack_aligned_node(int idx,
                       const BoundBox &b0,
                       const BoundBox &b1,
                       int c0,
                       int c1,
                       float2 time0,
                       float2 time1,
                       uint visibility0,
                       uint visibility1,
                       PackedBVH &pack)
{
  assert(idx + BVH_NODE_SIZE <= pack.nodes.size());
  assert(c0 < 0 || c0 < pack.nodes.size());
  assert(c1 < 0 || c1 < pack.nodes.size());

  if(time0.x > 0 || time0.y < 1)
      visibility0 |= PATH_RAY_NODE_4D;
  else
      visibility0 &= ~PATH_RAY_NODE_4D;

  if(time1.x > 0 || time1.y < 1)
      visibility1 |= PATH_RAY_NODE_4D;
  else
      visibility1 &= ~PATH_RAY_NODE_4D;

  int4 data[BVH_NODE_SIZE] = {
      make_int4(
          visibility0 & ~PATH_RAY_NODE_UNALIGNED, visibility1 & ~PATH_RAY_NODE_UNALIGNED, c0, c1),
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
      make_int4(__float_as_int(time0.x),
                __float_as_int(time1.x),
                __float_as_int(time0.y),
                __float_as_int(time1.y)),
  };

  memcpy(&pack.nodes[idx], data, sizeof(int4) * BVH_NODE_SIZE);
}


void pack_inner(const BVHStackEntry &e, const BVHStackEntry &c0, const BVHStackEntry &c1, PackedBVH &pack) {
    pack_aligned_node(e.idx,
                      c0.node->bounds,
                      c1.node->bounds,
                      c0.encodeIdx(),
                      c1.encodeIdx(),
                      make_float2(c0.node->time_from, c0.node->time_to),
                      make_float2(c1.node->time_from, c1.node->time_to),
                      c0.node->visibility,
                      c1.node->visibility,
                      pack);
}

void BVHEmbreeConverter::fillPack(PackedBVH &pack, vector<Object *> objects) {
    int num_prim = 0;

    for (size_t i = 0; i < this->s->size(); i++) {
	const auto tree = this->s->get(i);
        if(tree != nullptr)
            num_prim += 4 * tree->size();
    }

    pack.prim_visibility.clear();
    pack.prim_visibility.reserve(num_prim);
    pack.prim_object.clear();
    pack.prim_object.reserve(num_prim);
    pack.prim_type.clear();
    pack.prim_type.reserve(num_prim);
    pack.prim_index.clear();
    pack.prim_index.reserve(num_prim);
    pack.prim_tri_index.clear();
    pack.prim_tri_index.reserve(num_prim);

    pack.prim_tri_verts.clear();

    this->pack = &pack;
    this->packIdx = 0;


    BVHNode *root = this->getBVH2();
    std::cout << "BVH2 SAH is " << root->computeSubtreeSAHCost(this->params) << std::endl;
    const size_t num_nodes = root->getSubtreeSize(BVH_STAT_NODE_COUNT);
    const size_t num_leaf_nodes = root->getSubtreeSize(BVH_STAT_LEAF_COUNT);
    assert(num_leaf_nodes <= num_nodes);
    const size_t num_inner_nodes = num_nodes - num_leaf_nodes;
    size_t node_size;
    if (params.use_unaligned_nodes) {
        const size_t num_unaligned_nodes = root->getSubtreeSize(BVH_STAT_UNALIGNED_INNER_COUNT);
        node_size = (num_unaligned_nodes * BVH_UNALIGNED_NODE_SIZE) +
                (num_inner_nodes - num_unaligned_nodes) * BVH_NODE_SIZE;
    }
    else {
        node_size = num_inner_nodes * BVH_NODE_SIZE;
    }
    /* Resize arrays */
    pack.nodes.clear();
    pack.leaf_nodes.clear();
    /* For top level BVH, first merge existing BVH's so we know the offsets. */
    if (params.top_level) {
        pack_instances(node_size, num_leaf_nodes * BVH_NODE_LEAF_SIZE, pack);
    }
    else {
        pack.nodes.resize(node_size);
        pack.leaf_nodes.resize(num_leaf_nodes * BVH_NODE_LEAF_SIZE);
    }

    int nextNodeIdx = 0, nextLeafNodeIdx = 0;

    vector<BVHStackEntry> stack;
    stack.reserve(BVHParams::MAX_DEPTH * 2);
    if (root->is_leaf()) {
        stack.push_back(BVHStackEntry(root, nextLeafNodeIdx++));
    }
    else {
        stack.push_back(BVHStackEntry(root, nextNodeIdx));
        nextNodeIdx += root->has_unaligned() ? BVH_UNALIGNED_NODE_SIZE : BVH_NODE_SIZE;
    }

    while (stack.size()) {
        BVHStackEntry e = stack.back();
        stack.pop_back();

        if (e.node->is_leaf()) {
            /* leaf node */
            const LeafNode *leaf = reinterpret_cast<const LeafNode *>(e.node);
            pack_leaf(e, leaf, pack);
        }
        else {
            assert(e.node->num_children() == 2);
            /* inner node */
            int idx[2];
            for (int i = 0; i < 2; ++i) {
                if (e.node->get_child(i)->is_leaf()) {
                    idx[i] = nextLeafNodeIdx++;
                }
                else {
                    idx[i] = nextNodeIdx;
                    nextNodeIdx += e.node->get_child(i)->has_unaligned() ? BVH_UNALIGNED_NODE_SIZE :
                                                                           BVH_NODE_SIZE;
                }
            }

            stack.push_back(BVHStackEntry(e.node->get_child(0), idx[0]));
            stack.push_back(BVHStackEntry(e.node->get_child(1), idx[1]));

            pack_inner(e, stack[stack.size() - 2], stack[stack.size() - 1], pack);
        }
    }
    assert(node_size == nextNodeIdx);
    /* root index to start traversal at, to handle case of single leaf node */
    pack.root_index = (root->is_leaf()) ? -1 : 0;

    root->deleteSubtree();
}

CCL_NAMESPACE_END

#endif /* WITH_EMBREE */
