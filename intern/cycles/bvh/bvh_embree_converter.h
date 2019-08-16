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

/**
 * @brief This class implements a converter from Embree internal data structure
 * to Blender's internal structure
 */
class BVHEmbreeConverter
{
public:
  BVHEmbreeConverter(RTCScene scene,
                     std::vector<Object *> objects,
                     const BVHParams &params);

  BVHNode *getBVH4();
  BVHNode *getBVH2();

  void fillPack(PackedBVH &pack);
private:
  RTCScene s;
  std::vector<Object *> objects;
  const BVHParams &params;
  PackedBVH *pack;
  unsigned int packIdx = 0;

private:
  BVHNode* createLeaf(unsigned int nbPrim, const BVHPrimitive prims[]);
  BVHNode* createInstance(unsigned int nbPrim, const unsigned int geomID[]);
  BVHNode *createCurve(unsigned int nbPrim, const BVHPrimitive prims[]);
  BVHNode *createInnerNode(unsigned int nbChild, BVHNode *children[]);
  void setAlignedBounds(BVHNode* node, BoundBox bounds);
  void setLinearBounds(BVHNode* node, BoundBox bounds, BoundBox deltaBounds);
  void setUnalignedBounds(BVHNode* node, Transform affSpace);
  void set_time_bounds(BVHNode* node, float2 time_range);

  void pack_instances(size_t nodes_size,
                      size_t leaf_nodes_size,
                      PackedBVH &pack);
};


/**
 * @brief Merge two BVHNode into a tree that contain both of them
 * @param oldRoot A common ancestor of the two nodes
 * @param n0 First child
 * @param n1 Second child
 * @return A node that contains both child
 */
BVHNode *merge(BVHNode *oldRoot, BVHNode *n0, BVHNode *n1);

/**
 * @brief Insert a new primitive in packed data
 * @param pack Pack structure
 * @param packIdx Index to insert into
 * @param object_id Object ID
 * @param prim_id Primitive ID
 * @param prim_type Primitive type
 * @param visibility Visibility flag of the primitive
 * @param tri_index Index for the array containing triangle information
 */
void packPush(PackedBVH *pack, const size_t packIdx,
        const int object_id, const int prim_id,
        const int prim_type, const uint visibility, const uint tri_index);

/**
 * @brief Convert embree Bound Box to cycle Bound Box
 * @param bound Embree Bound Box
 * @return Cycle Bound Box
 */
BoundBox RTCBoundBoxToCCL(const RTCBounds &bound);

/**
 * @brief Create unaligned space from aligned bounds
 * @param bounds Aligne bounds box
 * @return A space in which bounds are [0; 1] on all axis
 */
Transform transformSpaceFromBound(const BoundBox &bounds);

/**
 * @brief Convert a list of BVHNode to a BVH2
 * Create new nodes if required
 * @param nodes A list of nodes to insert in the tree
 * @return A BVH2 containing all provided nodes
 */
BVHNode *makeBVHTreeFromList(std::deque<BVHNode *> nodes);

/**
 * @brief Convert a tree (up to BVH4) to a BVH2
 * Do not copy node, but update child list: thus every pointer to anywhere in
 * this tree remain valid
 * @param root The source BVH tree
 * @return A BVH2 containg at least all node from the source
 */
BVHNode *bvh_shrink(BVHNode *root);

/**
 * @brief Copute the size for a node
 * @param e The node to get the size
 * @return The size used in memory by the node
 */
int getSize(const BVHNode *e);

/**
 * @brief Pack a leaf node
 * @param e Node entry
 * @param leaf Leaf node to insert
 * @param pack Pack structure to fill
 */
void pack_leaf(const BVHStackEntry &e,
               const LeafNode *leaf,
               PackedBVH &pack);

/**
 * @brief Pack a generic aligned node
 * @param idx Index of the node to pack
 * @param c0 First child of the node
 * @param c1 Second child of the node
 * @param visibilityFlag Visibility mask to apply to both node
 * @param pack Pack structure to fill
 *
 * @see pack_unaligned_node
 */
void pack_base_node(const int idx,
                    const BVHStackEntry &c0,
                    const BVHStackEntry &c1,
                    uint visibilityFlag,
                    PackedBVH &pack);

/**
 * @brief Pack an unaligned node
 * @param idx Index of the node to pack
 * @param c0 First child of the node
 * @param c1 Second child of the node
 * @param visibilityFlag Visibility mask to apply to both node
 * @param pack Pack structure to fill
 *
 * @see pack_unaligned_node
 */
void pack_unaligned_node(const int idx,
                         const BVHStackEntry &c0,
                         const BVHStackEntry &c1,
                         uint visibilityFlag,
                         PackedBVH &pack);

/**
 * @brief Pack additional data for 4D node
 * @param idx Index to insert data to (should be baseIdx + BVH_NODE_SIZE_BASE)
 * @param c0 First child of the node
 * @param c1 Second child of the node
 * @param pack Pack structure to fill
 */
void pack_4D_node(const int idx,
                  const BVHNode *c0,
                  const BVHNode *c1,
                  PackedBVH &pack);

/**
 * @brief Pack additional data for MB node
 * Store linear bound box of both children of the node
 * @param idx Index to insert data to (should be baseIdx + BVH_NODE_SIZE_BASE)
 * @param c0 First child of the node
 * @param c1 Second child of the node
 * @param pack Pack structure to fill
 */
void pack_MB_node(const int idx,
                  const BoundBox *bb0,
                  const BoundBox *bb1,
                  PackedBVH &pack);

/**
 * @brief Pack an inner node
 * @param e Root node to store
 * @param c0 First child of the node
 * @param c1 Second child of the node
 * @param pack Pack structure to fill
 */
void pack_inner(const BVHStackEntry &e,
                const BVHStackEntry &c0,
                const BVHStackEntry &c1,
                PackedBVH &pack);

CCL_NAMESPACE_END

#endif /* WITH_EMBREE */

#endif /* __BVH_EMBREE_CONVERTER_H__ */
