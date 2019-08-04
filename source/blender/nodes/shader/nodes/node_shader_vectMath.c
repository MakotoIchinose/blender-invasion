/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2005 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup shdnodes
 */

#include "node_shader_util.h"

/* **************** VECTOR MATH ******************** */
static bNodeSocketTemplate sh_node_vect_math_in[] = {
    {SOCK_VECTOR, 1, N_("A"), 0.0f, 0.0f, 0.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_VECTOR, 1, N_("B"), 0.0f, 0.0f, 0.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("Scale"), 1.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {-1, 0, ""}};

static bNodeSocketTemplate sh_node_vect_math_out[] = {
    {SOCK_VECTOR, 0, N_("Vector")}, {SOCK_FLOAT, 0, N_("Value")}, {-1, 0, ""}};

static int gpu_shader_vect_math(GPUMaterial *mat,
                                bNode *node,
                                bNodeExecData *UNUSED(execdata),
                                GPUNodeStack *in,
                                GPUNodeStack *out)
{
  static const char *names[] = {
      [NODE_VECTOR_MATH_ADD] = "vec_math_add",
      [NODE_VECTOR_MATH_SUBTRACT] = "vec_math_subtract",
      [NODE_VECTOR_MATH_MULTIPLY] = "vec_math_multiply",
      [NODE_VECTOR_MATH_DIVIDE] = "vec_math_divide",

      [NODE_VECTOR_MATH_CROSS_PRODUCT] = "vec_math_cross",
      [NODE_VECTOR_MATH_PROJECT] = "vec_math_project",
      [NODE_VECTOR_MATH_REFLECT] = "vec_math_reflect",
      [NODE_VECTOR_MATH_DOT_PRODUCT] = "vec_math_dot",

      [NODE_VECTOR_MATH_DISTANCE] = "vec_math_distance",
      [NODE_VECTOR_MATH_LENGTH] = "vec_math_length",
      [NODE_VECTOR_MATH_SCALE] = "vec_math_scale",
      [NODE_VECTOR_MATH_NORMALIZE] = "vec_math_normalize",

      [NODE_VECTOR_MATH_SNAP] = "vec_math_snap",
      [NODE_VECTOR_MATH_FLOOR] = "vec_math_floor",
      [NODE_VECTOR_MATH_MODULO] = "vec_math_modulo",
      [NODE_VECTOR_MATH_ABSOLUTE] = "vec_math_absolute",
      [NODE_VECTOR_MATH_MINIMUM] = "vec_math_minimum",
      [NODE_VECTOR_MATH_MAXIMUM] = "vec_math_maximum",
  };

  GPU_stack_link(mat, node, names[node->custom1], in, out);
  return true;
}

static void node_shader_update_vec_math(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *sockB = nodeFindSocket(node, SOCK_IN, "B");
  bNodeSocket *sockScale = nodeFindSocket(node, SOCK_IN, "Scale");

  bNodeSocket *sockVector = nodeFindSocket(node, SOCK_OUT, "Vector");
  bNodeSocket *sockValue = nodeFindSocket(node, SOCK_OUT, "Value");

  nodeSetSocketAvailability(sockB,
                            !ELEM(node->custom1,
                                  NODE_VECTOR_MATH_SCALE,
                                  NODE_VECTOR_MATH_FLOOR,
                                  NODE_VECTOR_MATH_LENGTH,
                                  NODE_VECTOR_MATH_ABSOLUTE,
                                  NODE_VECTOR_MATH_NORMALIZE));
  nodeSetSocketAvailability(sockScale, node->custom1 == NODE_VECTOR_MATH_SCALE);
  nodeSetSocketAvailability(sockVector,
                            !ELEM(node->custom1,
                                  NODE_VECTOR_MATH_LENGTH,
                                  NODE_VECTOR_MATH_DISTANCE,
                                  NODE_VECTOR_MATH_DOT_PRODUCT));
  nodeSetSocketAvailability(sockValue,
                            ELEM(node->custom1,
                                 NODE_VECTOR_MATH_LENGTH,
                                 NODE_VECTOR_MATH_DISTANCE,
                                 NODE_VECTOR_MATH_DOT_PRODUCT));
}

void register_node_type_sh_vect_math(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_VECTOR_MATH, "Vector Math", NODE_CLASS_CONVERTOR, 0);
  node_type_socket_templates(&ntype, sh_node_vect_math_in, sh_node_vect_math_out);
  node_type_label(&ntype, node_vect_math_label);
  node_type_gpu(&ntype, gpu_shader_vect_math);
  node_type_update(&ntype, node_shader_update_vec_math);

  nodeRegisterType(&ntype);
}
