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
    {SOCK_VECTOR, 1, N_("Vector"), 0.0f, 0.0f, 0.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_VECTOR, 1, N_("Vector"), 0.0f, 0.0f, 0.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("Factor"), 1.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {-1, 0, ""}};

static bNodeSocketTemplate sh_node_vect_math_out[] = {
    {SOCK_VECTOR, 0, N_("Vector")}, {SOCK_FLOAT, 0, N_("Value")}, {-1, 0, ""}};

static void node_shader_exec_vect_math(void *UNUSED(data),
                                       int UNUSED(thread),
                                       bNode *node,
                                       bNodeExecData *UNUSED(execdata),
                                       bNodeStack **in,
                                       bNodeStack **out)
{
  float vec1[3], vec2[3], fac;

  nodestack_get_vec(vec1, SOCK_VECTOR, in[0]);
  nodestack_get_vec(vec2, SOCK_VECTOR, in[1]);
  nodestack_get_vec(&fac, SOCK_FLOAT, in[2]);

  switch (node->custom1) {
    case NODE_VECTOR_MATH_ADD:
      add_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_SUBTRACT:
      sub_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_MULTIPLY:
      mul_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_DIVIDE:
      div_v3_v3v3_safe(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_CROSS_PRODUCT:
      cross_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_PROJECT:
      project_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_REFLECT:
      reflect_v3_v3v3(out[0]->vec, vec1, vec2);
      break;
    case NODE_VECTOR_MATH_AVERAGE:
      add_v3_v3v3(out[0]->vec, vec1, vec2);
      normalize_v3(out[0]->vec);
      break;
    case NODE_VECTOR_MATH_DOT_PRODUCT:
      out[0]->vec[0] = dot_v3v3(vec1, vec2);
      break;
    case NODE_VECTOR_MATH_DISTANCE:
      out[0]->vec[0] = len_v3v3(vec1, vec2);
      break;
    case NODE_VECTOR_MATH_LENGTH:
      out[0]->vec[0] = len_v3(vec1);
      break;
    case NODE_VECTOR_MATH_SCALE:
      mul_v3_v3fl(out[0]->vec, vec1, fac);
      break;
    case NODE_VECTOR_MATH_NORMALIZE:
      normalize_v3_v3(out[0]->vec, vec1);
      break;
    case NODE_VECTOR_MATH_SNAP:
      out[0]->vec[0] = (vec2[0] != 0.0f) ? floorf(vec1[0] / vec2[0]) * vec2[0] : 0.0f;
      out[0]->vec[1] = (vec2[1] != 0.0f) ? floorf(vec1[1] / vec2[1]) * vec2[1] : 0.0f;
      out[0]->vec[2] = (vec2[2] != 0.0f) ? floorf(vec1[2] / vec2[2]) * vec2[2] : 0.0f;
      break;
    case NODE_VECTOR_MATH_MOD:
      out[0]->vec[0] = (vec2[0] != 0.0f) ? fmod(vec1[0], vec2[0]) : 0.0f;
      out[0]->vec[1] = (vec2[0] != 0.0f) ? fmod(vec1[1], vec2[1]) : 0.0f;
      out[0]->vec[2] = (vec2[0] != 0.0f) ? fmod(vec1[2], vec2[2]) : 0.0f;
      break;
    case NODE_VECTOR_MATH_ABS:
      abs_v3_v3(out[0]->vec, vec1);
      break;
    case NODE_VECTOR_MATH_MIN:
      out[0]->vec[0] = fmin(vec1[0], vec2[0]);
      out[0]->vec[1] = fmin(vec1[1], vec2[1]);
      out[0]->vec[2] = fmin(vec1[2], vec2[2]);
      break;
    case NODE_VECTOR_MATH_MAX:
      out[0]->vec[0] = fmax(vec1[0], vec2[0]);
      out[0]->vec[1] = fmax(vec1[1], vec2[1]);
      out[0]->vec[2] = fmax(vec1[2], vec2[2]);
      break;
  }
}

static int gpu_shader_vect_math(GPUMaterial *mat,
                                bNode *node,
                                bNodeExecData *UNUSED(execdata),
                                GPUNodeStack *in,
                                GPUNodeStack *out)
{
  static const char *names[] = {
      [NODE_VECTOR_MATH_ADD] = "vec_math_add",
      [NODE_VECTOR_MATH_SUBTRACT] = "vec_math_sub",
      [NODE_VECTOR_MATH_MULTIPLY] = "vec_math_mul",
      [NODE_VECTOR_MATH_DIVIDE] = "vec_math_div",

      [NODE_VECTOR_MATH_CROSS_PRODUCT] = "vec_math_cross",
      [NODE_VECTOR_MATH_PROJECT] = "vec_math_project",
      [NODE_VECTOR_MATH_REFLECT] = "vec_math_reflect",
      [NODE_VECTOR_MATH_AVERAGE] = "vec_math_average",

      [NODE_VECTOR_MATH_DOT_PRODUCT] = "vec_math_dot",
      [NODE_VECTOR_MATH_DISTANCE] = "vec_math_distance",
      [NODE_VECTOR_MATH_LENGTH] = "vec_math_length",
      [NODE_VECTOR_MATH_SCALE] = "vec_math_scale",
      [NODE_VECTOR_MATH_NORMALIZE] = "vec_math_normalize",

      [NODE_VECTOR_MATH_SNAP] = "vec_math_snap",
      [NODE_VECTOR_MATH_MOD] = "vec_math_mod",
      [NODE_VECTOR_MATH_ABS] = "vec_math_abs",
      [NODE_VECTOR_MATH_MIN] = "vec_math_min",
      [NODE_VECTOR_MATH_MAX] = "vec_math_max",
  };

  switch (node->custom1) {
    case NODE_VECTOR_MATH_LENGTH:
    case NODE_VECTOR_MATH_NORMALIZE:
    case NODE_VECTOR_MATH_ABS: {
      GPUNodeStack tmp_in[2];
      memcpy(&tmp_in[0], &in[0], sizeof(GPUNodeStack));
      memcpy(&tmp_in[1], &in[3], sizeof(GPUNodeStack));
      GPU_stack_link(mat, node, names[node->custom1], tmp_in, out);
      break;
    }
    case NODE_VECTOR_MATH_SCALE: {
      // GPUNodeStack tmp_in[3];
      // memcpy(&tmp_in[0], &in[0], sizeof(GPUNodeStack));
      // memcpy(&tmp_in[1], &in[2], sizeof(GPUNodeStack));
      // memcpy(&tmp_in[2], &in[3], sizeof(GPUNodeStack));

      // tmp_in doesn't work if not linked, use `in` for now.
      GPU_stack_link(mat, node, names[node->custom1], in, out);
      break;
    }
    default: {
      GPUNodeStack tmp_in[3];
      memcpy(&tmp_in[0], &in[0], sizeof(GPUNodeStack));
      memcpy(&tmp_in[1], &in[1], sizeof(GPUNodeStack));
      memcpy(&tmp_in[2], &in[3], sizeof(GPUNodeStack));
      GPU_stack_link(mat, node, names[node->custom1], tmp_in, out);
    }
  }
  return true;
}

static void node_shader_update_vec_math(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *inVecSock = BLI_findlink(&node->inputs, 1);
  bNodeSocket *inFacSock = BLI_findlink(&node->inputs, 2);

  bNodeSocket *outVecSock = BLI_findlink(&node->outputs, 0);
  bNodeSocket *outValSock = BLI_findlink(&node->outputs, 1);

  switch (node->custom1) {
    case NODE_VECTOR_MATH_DOT_PRODUCT:
    case NODE_VECTOR_MATH_DISTANCE:
      inVecSock->flag &= ~SOCK_UNAVAIL;
      inFacSock->flag |= SOCK_UNAVAIL;

      outValSock->flag &= ~SOCK_UNAVAIL;
      outVecSock->flag |= SOCK_UNAVAIL;
      break;
    case NODE_VECTOR_MATH_LENGTH:
      inVecSock->flag |= SOCK_UNAVAIL;
      inFacSock->flag |= SOCK_UNAVAIL;

      outValSock->flag &= ~SOCK_UNAVAIL;
      outVecSock->flag |= SOCK_UNAVAIL;
      break;
    case NODE_VECTOR_MATH_NORMALIZE:
    case NODE_VECTOR_MATH_ABS:
      inVecSock->flag |= SOCK_UNAVAIL;
      inFacSock->flag |= SOCK_UNAVAIL;

      outValSock->flag |= SOCK_UNAVAIL;
      outVecSock->flag &= ~SOCK_UNAVAIL;
      break;
    case NODE_VECTOR_MATH_SCALE:
      inVecSock->flag |= SOCK_UNAVAIL;
      inFacSock->flag &= ~SOCK_UNAVAIL;

      outValSock->flag |= SOCK_UNAVAIL;
      outVecSock->flag &= ~SOCK_UNAVAIL;
      break;
    default:
      inVecSock->flag &= ~SOCK_UNAVAIL;
      inFacSock->flag |= SOCK_UNAVAIL;

      outValSock->flag |= SOCK_UNAVAIL;
      outVecSock->flag &= ~SOCK_UNAVAIL;
  }
}

void register_node_type_sh_vect_math(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_VECT_MATH, "Vector Math", NODE_CLASS_CONVERTOR, 0);
  node_type_socket_templates(&ntype, sh_node_vect_math_in, sh_node_vect_math_out);
  node_type_label(&ntype, node_vect_math_label);
  node_type_storage(&ntype, "", NULL, NULL);
  node_type_exec(&ntype, NULL, NULL, node_shader_exec_vect_math);
  node_type_gpu(&ntype, gpu_shader_vect_math);
  node_type_update(&ntype, node_shader_update_vec_math);

  nodeRegisterType(&ntype);
}
