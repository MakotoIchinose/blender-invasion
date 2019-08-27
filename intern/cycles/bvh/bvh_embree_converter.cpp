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

#include "bvh_embree_converter.h"

#include "bvh_node.h"
#include "render/mesh.h"

CCL_NAMESPACE_BEGIN

/* Utility functions */
void packPush(PackedBVH *pack, const size_t packIdx,
              const int object_id, const int prim_id,
              const int prim_type, const uint visibility,
              const uint tri_index)
{
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

BoundBox RTCBoundBoxToCCL(const RTCBounds &bound)
{
  return BoundBox(make_float3(bound.lower_x, bound.lower_y, bound.lower_z),
                  make_float3(bound.upper_x, bound.upper_y, bound.upper_z));

}

Transform transformSpaceFromBound(const BoundBox &bounds) {
  Transform space = transform_identity();

  space.x.w -= bounds.min.x;
  space.y.w -= bounds.min.y;
  space.z.w -= bounds.min.z;
  float3 dim = bounds.max - bounds.min;

  return transform_scale(1.0f / max(1e-18f, dim.x),
                         1.0f / max(1e-18f, dim.y),
                         1.0f / max(1e-18f, dim.z)) * space;
}


BVHNode *merge(BVHNode *oldRoot, BVHNode *n0, BVHNode *n1) {
  BoundBox childBB = merge(n0->bounds, n1->bounds);
  BVHNode *root = new InnerNode(childBB, n0, n1);

  /* If one of the child has linear bound, take the parents bound */
  if(n0->deltaBounds != nullptr || n1->deltaBounds != nullptr) {
    root->bounds = oldRoot->bounds;
    if(oldRoot->deltaBounds != nullptr) root->deltaBounds = new BoundBox(*oldRoot->deltaBounds);
  }

  return root;
}

BVHNode *makeBVHTreeFromList(std::deque<BVHNode *> nodes)
{
  BVHNode *ret = nullptr;
  while(!nodes.empty()) {
    if(ret == nullptr) {
      ret = nodes.front();
      nodes.pop_front();
      continue;
    }

    /* If it's a leaf or a full node -> create a new parrent */
    if(ret->is_leaf() || ret->num_children() == 4) {
      ret = new InnerNode(ret->bounds, &ret, 1);
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
}

BVHNode *bvh_shrink(BVHNode *root)
{
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

  node->children[0] = merge(root, children[0], children[1]);

  if(num_children == 3) {
    node->children[1] = children[2];
  } else {
    node->children[1] = merge(root, children[2], children[3]);
  }

  return node;
}

BVHEmbreeConverter::BVHEmbreeConverter(RTCScene scene,
                                       std::vector<Object *> objects,
                                       const BVHParams &params)
  : s(scene),
    objects(objects),
    params(params)
{}

BVHNode* BVHEmbreeConverter::createLeaf(unsigned int nbPrim,
                                        const BVHPrimitive prims[])
{
  const unsigned int from = this->packIdx;

  uint visibility = 0;
  for(unsigned int i = 0; i < nbPrim; i++) {
    const unsigned int geom_id = prims[i].geomID;
    const unsigned int prim_id = prims[i].primID;

    const unsigned int object_id = geom_id / 2;
    Object *obj = this->objects.at(object_id);
    Mesh::Triangle tri = obj->mesh->get_triangle(prim_id);

    int prim_type = obj->mesh->has_motion_blur()
                    ? PRIMITIVE_MOTION_TRIANGLE
                    : PRIMITIVE_TRIANGLE;

    visibility |= obj->visibility;
    packPush(this->pack, this->packIdx++,
             object_id, prim_id,
             prim_type, obj->visibility,
             this->pack->prim_tri_verts.size());

    array<float4> *prim_tri_verts = &this->pack->prim_tri_verts;
    prim_tri_verts->reserve(this->pack->prim_tri_verts.size());
    float3 *verts = obj->mesh->verts.data();
    prim_tri_verts->reserve(this->pack->prim_tri_verts.size() + 3);
    for(int i = 0; i < 3; i++)
      prim_tri_verts->push_back_reserved(float3_to_float4(verts[tri.v[i]]));
  }

  return new LeafNode(BoundBox::empty, visibility, from, from + nbPrim);
}

BVHNode* BVHEmbreeConverter::createInstance(unsigned int nbPrim,
                                            const unsigned int geomID[])
{
  std::deque<BVHNode *> nodes;

  for(size_t i = 0; i < nbPrim; i++) {
    uint id = geomID[i] / 2;
    Object *obj = this->objects.at(id);
    LeafNode *leafNode = new LeafNode(obj->bounds, obj->visibility,
                                      this->packIdx, this->packIdx + 1);

    packPush(this->pack, this->packIdx++,
             id, -1,
             PRIMITIVE_NONE, obj->visibility,
             -1);

    nodes.push_back(leafNode);
  }

  return makeBVHTreeFromList(nodes);
}

BVHNode* BVHEmbreeConverter::createCurve(unsigned int nbPrim,
                     const BVHPrimitive prims[])
{
  std::deque<BVHNode *> nodes;

  for(unsigned int i = 0; i < nbPrim; i++) {
    const auto geom_id = prims[i].geomID;
    const auto prim_id = prims[i].primID;

    const unsigned int object_id = geom_id / 2;
    Object *obj = this->objects.at(object_id);

    unsigned int curve_id = 0;
    int segment_id = prim_id;
    Mesh::Curve curve = obj->mesh->get_curve(0);
    while(segment_id >= curve.num_segments()) {
      curve = obj->mesh->get_curve(++curve_id);
      segment_id -= curve.num_segments();
    }

    int prim_type = PRIMITIVE_PACK_SEGMENT(obj->mesh->has_motion_blur()
                                           ? PRIMITIVE_MOTION_CURVE
                                           : PRIMITIVE_CURVE, segment_id);

    LeafNode *leafNode = new LeafNode(BoundBox::empty, obj->visibility,
                                      this->packIdx, this->packIdx + 1);
    curve.bounds_grow(segment_id,
                      obj->mesh->curve_keys.data(),
                      obj->mesh->curve_radius.data(),
                      leafNode->bounds);

    packPush(this->pack, this->packIdx++,
             object_id, curve_id,
             prim_type, obj->visibility, -1);

    nodes.push_back(leafNode);
  }

  return makeBVHTreeFromList(nodes);
}


BVHNode *BVHEmbreeConverter::createInnerNode(unsigned int nbChild,
                                             BVHNode *children[])
{
  return new InnerNode(BoundBox::empty, children, nbChild);
}

void BVHEmbreeConverter::setAlignedBounds(BVHNode *node, BoundBox bounds)
{
  node->bounds = bounds;
}

void BVHEmbreeConverter::setLinearBounds(BVHNode *node, BoundBox bounds, BoundBox deltaBounds)
{
    node->bounds = bounds;
    node->deltaBounds = new BoundBox(deltaBounds);
}

void BVHEmbreeConverter::setUnalignedBounds(BVHNode *node, Transform affSpace)
{
    Transform inv = transform_inverse(affSpace);

    BoundBox bb(transform_point(&inv, make_float3(0)));
    bb.grow(transform_point(&inv, make_float3(1)));

    node->bounds = bb;
    node->set_aligned_space(affSpace);
}

void BVHEmbreeConverter::set_time_bounds(BVHNode *node, float2 time_range)
{
  node->time_from = time_range.x;
  node->time_to   = time_range.y;
}

#define SET_THIS \
  BVHEmbreeConverter* This = reinterpret_cast<BVHEmbreeConverter*>(userData)
#define SET_NODE \
  BVHNode* Node = reinterpret_cast<BVHNode*>(node)

BVHNode* BVHEmbreeConverter::getBVH4()
{
  RTCBVHExtractFunction param;
  param.createLeaf = [](unsigned int nbPrim, const BVHPrimitive prims[], void* userData) -> void* {
    SET_THIS;
    return This->createLeaf(nbPrim, prims);
  };

  param.createInstance = [](unsigned int nbPrim, const unsigned int geomID[], void* userData) -> void* {
    SET_THIS;
    return This->createInstance(nbPrim, geomID);
  };

  param.createCurve = [](unsigned int nbPrim, const BVHPrimitive prims[], void* userData) -> void* {
    SET_THIS;
    return This->createCurve(nbPrim, prims);
  };

  param.createInnerNode = [](unsigned int nbChild, void* children[], void* userData) -> void* {
    SET_THIS;
    return This->createInnerNode(nbChild, reinterpret_cast<BVHNode**>(children));
  };

  param.setAlignedBounds = [](void* node, const RTCBounds &bounds, void* userData) {
    SET_THIS; SET_NODE;
    This->setAlignedBounds(Node, RTCBoundBoxToCCL(bounds));
  };

  param.setLinearBounds = [](void* node, const RTCLinearBounds &lbounds, void* userData) {
    SET_THIS; SET_NODE;
    This->setLinearBounds(Node,
                          RTCBoundBoxToCCL(lbounds.bounds0),
                          RTCBoundBoxToCCL(lbounds.bounds1));

    This->set_time_bounds(Node, make_float2(lbounds.bounds0.align0,
                                            lbounds.bounds0.align1));
  };

  param.setUnalignedBounds = [](void* node, const RTCAffineSpace &affSpace, void* userData) {
    SET_THIS; SET_NODE;
    Transform tr;
    tr.x.x = affSpace.linear[0];
    tr.x.y = affSpace.linear[1];
    tr.x.z = affSpace.linear[2];
    tr.y.x = affSpace.linear[3];
    tr.y.y = affSpace.linear[4];
    tr.y.z = affSpace.linear[5];
    tr.z.x = affSpace.linear[6];
    tr.z.y = affSpace.linear[7];
    tr.z.z = affSpace.linear[8];

    tr.x.w = affSpace.affine[0];
    tr.y.w = affSpace.affine[1];
    tr.z.w = affSpace.affine[2];

    This->setUnalignedBounds(Node, tr);
  };

  return reinterpret_cast<BVHNode *>(rtcExtractBVH(this->s, param, this));
}

BVHNode* BVHEmbreeConverter::getBVH2()
{
  BVHNode *root = this->getBVH4();
  if(root == nullptr) return nullptr;
  return bvh_shrink(root);
}

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
      for (size_t i = 0, j = 0; i < leaf_nodes_offset_size; i += BVH_NODE_SIZE_LEAF, j++) {
        int4 data = leaf_nodes_offset[i];
        data.x += prim_offset;
        data.y += prim_offset;
        pack_leaf_nodes[pack_leaf_nodes_offset] = data;
        for (int j = 1; j < BVH_NODE_SIZE_LEAF; ++j) {
          pack_leaf_nodes[pack_leaf_nodes_offset + j] = leaf_nodes_offset[i + j];
        }
        pack_leaf_nodes_offset += BVH_NODE_SIZE_LEAF;
      }
    }

    if (bvh->pack.nodes.size()) {
      int4 *bvh_nodes = &bvh->pack.nodes[0];
      size_t bvh_nodes_size = bvh->pack.nodes.size();

      for (size_t i = 0, j = 0; i < bvh_nodes_size; j++) {
        size_t nsize = BVH_NODE_SIZE_BASE;
        if (bvh_nodes[i].x & PATH_RAY_NODE_4D) nsize += BVH_NODE_SIZE_4D;
        if (bvh_nodes[i].x & PATH_RAY_NODE_MB) nsize += BVH_NODE_SIZE_MOTION_BLUR;
        if (bvh_nodes[i].x & PATH_RAY_NODE_UNALIGNED) nsize += BVH_NODE_SIZE_UNALIGNED;

        /* Modify offsets into arrays */
        int4 data = bvh_nodes[i];
        data.z += (data.z < 0) ? -noffset_leaf : noffset;
        data.w += (data.w < 0) ? -noffset_leaf : noffset;
        pack_nodes[pack_nodes_offset] = data;

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

void pack_leaf(const BVHStackEntry &e, const LeafNode *leaf, PackedBVH &pack)
{
  assert(e.idx + BVH_NODE_SIZE_LEAF <= pack.leaf_nodes.size());
  float4 data[BVH_NODE_SIZE_LEAF];
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
  data[0].w = __uint_as_float(pack.prim_type[leaf->lo]);

  memcpy(&pack.leaf_nodes[e.idx], data, sizeof(float4) * BVH_NODE_SIZE_LEAF);
}

void pack_base_node(const int idx, const BVHStackEntry &c0, const BVHStackEntry &c1, uint visibilityFlag, PackedBVH &pack)
{
  assert(idx + BVH_NODE_SIZE_BASE <= pack.nodes.size());

  // Clear visibility flags used for node type, and apply relevant ones
  uint visibility0 = (c0.node->visibility & ~PATH_RAY_NODE_CLEAR) | visibilityFlag;
  uint visibility1 = (c1.node->visibility & ~PATH_RAY_NODE_CLEAR) | visibilityFlag;

  int4 data[BVH_NODE_SIZE_BASE] = {
    make_int4(visibility0, visibility1, c0.encodeIdx(), c1.encodeIdx()),
    make_int4(__float_as_int(c0.node->bounds.min.x),
    __float_as_int(c1.node->bounds.min.x),
    __float_as_int(c0.node->bounds.max.x),
    __float_as_int(c1.node->bounds.max.x)),
    make_int4(__float_as_int(c0.node->bounds.min.y),
    __float_as_int(c1.node->bounds.min.y),
    __float_as_int(c0.node->bounds.max.y),
    __float_as_int(c1.node->bounds.max.y)),
    make_int4(__float_as_int(c0.node->bounds.min.z),
    __float_as_int(c1.node->bounds.min.z),
    __float_as_int(c0.node->bounds.max.z),
    __float_as_int(c1.node->bounds.max.z)),
  };

  memcpy(&pack.nodes[idx], data, sizeof(int4) * BVH_NODE_SIZE_BASE);
}

void pack_unaligned_node(const int idx, const BVHStackEntry &c0, const BVHStackEntry &c1, uint visibilityFlag, PackedBVH &pack) {
  assert(idx + BVH_NODE_SIZE_BASE + BVH_NODE_SIZE_UNALIGNED <= pack.nodes.size());

  // Clear visibility flags used for node type, and apply relevant ones
  uint visibility0 = (c0.node->visibility & ~PATH_RAY_NODE_CLEAR) | visibilityFlag;
  uint visibility1 = (c1.node->visibility & ~PATH_RAY_NODE_CLEAR) | visibilityFlag;

  float4 data[BVH_NODE_SIZE_BASE + BVH_NODE_SIZE_UNALIGNED];
  data[0] = make_float4(__int_as_float(visibility0),
                        __int_as_float(visibility1),
                        __int_as_float(c0.encodeIdx()),
                        __int_as_float(c1.encodeIdx()));

  Transform bb0 = c0.node->is_unaligned ? *c0.node->aligned_space
                                        : transformSpaceFromBound(c0.node->bounds);
  Transform bb1 = c1.node->is_unaligned ? *c1.node->aligned_space
                                        : transformSpaceFromBound(c1.node->bounds);

  data[1] = bb0.x;
  data[2] = bb0.y;
  data[3] = bb0.z;
  data[4] = bb1.x;
  data[5] = bb1.y;
  data[6] = bb1.z;

  memcpy(&pack.nodes[idx], data, sizeof(float4) * (BVH_NODE_SIZE_BASE + BVH_NODE_SIZE_UNALIGNED));
}

void pack_4D_node(const int idx, const BVHNode *c0, const BVHNode *c1, PackedBVH &pack)
{
  assert(idx + BVH_NODE_SIZE_4D <= pack.nodes.size());
  assert(BVH_NODE_SIZE_4D == 1);

  pack.nodes[idx] = make_int4(__float_as_int(c0->time_from),
                              __float_as_int(c1->time_from),
                              __float_as_int(c0->time_to),
                              __float_as_int(c1->time_to));
};

void pack_MB_node(const int idx, const BoundBox *bb0, const BoundBox *bb1, PackedBVH &pack)
{
  assert(idx + BVH_NODE_SIZE_MOTION_BLUR <= pack.nodes.size());

  if(bb0 == nullptr) bb0 = new BoundBox(make_float3(0));
  if(bb1 == nullptr) bb1 = new BoundBox(make_float3(0));

  int4 data[BVH_NODE_SIZE_MOTION_BLUR] = {
    make_int4(__float_as_int(bb0->min.x),
    __float_as_int(bb1->min.x),
    __float_as_int(bb0->max.x),
    __float_as_int(bb1->max.x)),
    make_int4(__float_as_int(bb0->min.y),
    __float_as_int(bb1->min.y),
    __float_as_int(bb0->max.y),
    __float_as_int(bb1->max.y)),
    make_int4(__float_as_int(bb0->min.z),
    __float_as_int(bb1->min.z),
    __float_as_int(bb0->max.z),
    __float_as_int(bb1->max.z)),
  };

  memcpy(&pack.nodes[idx], data, sizeof(int4) * BVH_NODE_SIZE_MOTION_BLUR);
}

int getSize(const BVHNode *e) {
  int                       size  = BVH_NODE_SIZE_BASE;
  if(e->has_time_limited()) size += BVH_NODE_SIZE_4D;
  if(e->has_motion_blur())  size += BVH_NODE_SIZE_MOTION_BLUR;
  if(e->has_unaligned())    size += BVH_NODE_SIZE_UNALIGNED;
  return size;
}

void pack_inner(const BVHStackEntry &e, const BVHStackEntry &c0, const BVHStackEntry &c1, PackedBVH &pack)
{
  uint visibility = 0;

  if(e.node->has_time_limited())
    visibility |= PATH_RAY_NODE_4D;

  if(e.node->has_motion_blur())
    visibility |= PATH_RAY_NODE_MB;

  if(e.node->has_unaligned())
    visibility |= PATH_RAY_NODE_UNALIGNED;

  int idx = e.idx;
  if(e.node->has_unaligned()) {
    pack_unaligned_node(idx, c0, c1, visibility, pack);
    idx += BVH_NODE_SIZE_BASE + BVH_NODE_SIZE_UNALIGNED;
  } else {
    pack_base_node(idx, c0, c1, visibility, pack);
    idx += BVH_NODE_SIZE_BASE;
  }

  if(visibility & PATH_RAY_NODE_MB) {
    pack_MB_node(idx, c0.node->deltaBounds, c1.node->deltaBounds, pack);
    idx += BVH_NODE_SIZE_MOTION_BLUR;
  }

  if(visibility & PATH_RAY_NODE_4D) {
    pack_4D_node(idx, c0.node, c1.node, pack);
    idx += BVH_NODE_SIZE_4D;
  }

  assert(e.idx + getSize(e.node) == idx);
}

void BVHEmbreeConverter::fillPack(PackedBVH &pack) {
  size_t num_prim = 0;
  size_t num_tri = 0;

  foreach (Object *ob, objects) {
    if (params.top_level) {
      if (!ob->is_traceable()) {
        continue;
      }
      if (!ob->mesh->is_instanced()) {
        if (params.primitive_mask & PRIMITIVE_ALL_TRIANGLE) {
          num_prim += ob->mesh->num_triangles();
          num_tri += ob->mesh->num_triangles();
        }
        if (params.primitive_mask & PRIMITIVE_ALL_CURVE) {
          for (size_t j = 0; j < ob->mesh->num_curves(); ++j) {
            num_prim += ob->mesh->get_curve(j).num_segments();
          }
        }
      }
      else {
        ++num_prim;
      }
    }
    else {
      if (params.primitive_mask & PRIMITIVE_ALL_TRIANGLE && ob->mesh->num_triangles() > 0) {
        num_prim += ob->mesh->num_triangles();
        num_tri += ob->mesh->num_triangles();
      }
      if (params.primitive_mask & PRIMITIVE_ALL_CURVE) {
        for (size_t j = 0; j < ob->mesh->num_curves(); ++j) {
          num_prim += ob->mesh->get_curve(j).num_segments();
        }
      }
    }
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
  pack.prim_tri_index.reserve(3 * num_tri);

  this->pack = &pack;

  BVHNode *root = this->getBVH2();
  if(root == nullptr) {
    pack.root_index = 0;
    pack.nodes.clear();
    pack.leaf_nodes.clear();
    return;
  }

  const size_t num_nodes = root->getSubtreeSize(BVH_STAT_NODE_COUNT);
  const size_t num_leaf_nodes = root->getSubtreeSize(BVH_STAT_LEAF_COUNT);
  assert(num_leaf_nodes <= num_nodes);
  const size_t num_inner_nodes = num_nodes - num_leaf_nodes;
  size_t node_size = num_inner_nodes * BVH_NODE_SIZE_BASE;

  // Additional size for unaligned nodes
  if (params.use_unaligned_nodes) {
    const size_t num_unaligned_nodes = root->getSubtreeSize(BVH_STAT_UNALIGNED_INNER_COUNT);
    node_size += num_unaligned_nodes * BVH_NODE_SIZE_UNALIGNED;
  }

  // Additional size for linear bound
  node_size += root->getSubtreeSize(BVH_STAT_MOTION_BLURED_NODE_COUNT) * BVH_NODE_SIZE_MOTION_BLUR;

  // Additional size for time limits
  node_size += root->getSubtreeSize(BVH_STAT_4D_NODE_COUNT) * BVH_NODE_SIZE_4D;

  /* Resize arrays */
  pack.nodes.clear();
  pack.leaf_nodes.clear();
  /* For top level BVH, first merge existing BVH's so we know the offsets. */
  if (params.top_level) {
    pack_instances(node_size, num_leaf_nodes * BVH_NODE_SIZE_LEAF, pack);
  }
  else {
    pack.nodes.resize(node_size);
    pack.leaf_nodes.resize(num_leaf_nodes * BVH_NODE_SIZE_LEAF);
  }

  int nextNodeIdx = 0, nextLeafNodeIdx = 0;

  vector<BVHStackEntry> stack;
  stack.reserve(BVHParams::MAX_DEPTH * 2);
  if (root->is_leaf()) {
    stack.push_back(BVHStackEntry(root, nextLeafNodeIdx++));
  }
  else {
    stack.push_back(BVHStackEntry(root, nextNodeIdx));
    nextNodeIdx += getSize(root);
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
          nextNodeIdx += getSize(e.node->get_child(i));
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
