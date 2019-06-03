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

/* **************** MAPPING  ******************** */
static bNodeSocketTemplate sh_node_mapping_in[] = {
    {SOCK_VECTOR, 1, N_("Vector"), 0.0f, 0.0f, 0.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_NONE},
    {SOCK_VECTOR, 1, N_("Location"), 0.0f, 0.0f, 0.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_TRANSLATION},
    {SOCK_VECTOR, 1, N_("Rotation"), 0.0f, 0.0f, 0.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_EULER},
    {SOCK_VECTOR, 1, N_("Scale"), 1.0f, 1.0f, 1.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_XYZ},
    {-1, 0, ""},
};

static bNodeSocketTemplate sh_node_mapping_out[] = {
    {SOCK_VECTOR, 0, N_("Vector")},
    {-1, 0, ""},
};

static void node_shader_exec_mapping(void *UNUSED(data),
                                     int UNUSED(thread),
                                     bNode *node,
                                     bNodeExecData *UNUSED(execdata),
                                     bNodeStack **in,
                                     bNodeStack **out)
{
  float *vec = out[0]->vec;
  float loc[3], rot[3], size[3];
  nodestack_get_vec(vec, SOCK_VECTOR, in[0]);
  nodestack_get_vec(loc, SOCK_VECTOR, in[1]);
  nodestack_get_vec(rot, SOCK_VECTOR, in[2]);
  nodestack_get_vec(size, SOCK_VECTOR, in[3]);

  float smat[4][4], rmat[4][4], tmat[4][4], mat[4][4];

  size_to_mat4(smat, size);
  eul_to_mat4(rmat, rot);
  unit_m4(tmat);
  copy_v3_v3(tmat[3], loc);

  if (node->custom1 == NODE_MAPPING_TYPE_TEXTURE) {
    mul_m4_series(mat, tmat, rmat, smat);
    invert_m4(mat);
  }
  else if (node->custom1 == NODE_MAPPING_TYPE_POINT) {
    mul_m4_series(mat, tmat, rmat, smat);
  }
  else if (node->custom1 == NODE_MAPPING_TYPE_VECTOR) {
    mul_m4_m4m4(mat, rmat, smat);
  }
  else if (node->custom1 == NODE_MAPPING_TYPE_NORMAL) {
    mul_m4_m4m4(mat, rmat, smat);
    invert_m4(mat);
    transpose_m4(mat);
  }

  mul_m4_v3(mat, vec);

  if (node->custom1 == NODE_MAPPING_TYPE_NORMAL) {
    normalize_v3(vec);
  }
}

static int gpu_shader_mapping(GPUMaterial *mat,
                              bNode *node,
                              bNodeExecData *UNUSED(execdata),
                              GPUNodeStack *in,
                              GPUNodeStack *out)
{
  static const char *names[] = {
      [NODE_MAPPING_TYPE_POINT] = "mapping_point",
      [NODE_MAPPING_TYPE_TEXTURE] = "mapping_texture",
      [NODE_MAPPING_TYPE_VECTOR] = "mapping_vector",
      [NODE_MAPPING_TYPE_NORMAL] = "mapping_normal",
  };

  GPU_stack_link(mat, node, names[node->custom1], in, out);
  return true;
}

static void node_shader_update_mapping(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *inLocSock = BLI_findlink(&node->inputs, 1);

  if (node->custom1 == NODE_MAPPING_TYPE_VECTOR || node->custom1 == NODE_MAPPING_TYPE_NORMAL) {
    inLocSock->flag |= SOCK_UNAVAIL;
  }
  else {
    inLocSock->flag &= ~SOCK_UNAVAIL;
  }
}

void register_node_type_sh_mapping(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_MAPPING, "Mapping", NODE_CLASS_OP_VECTOR, 0);
  node_type_socket_templates(&ntype, sh_node_mapping_in, sh_node_mapping_out);
  node_type_storage(&ntype, "", NULL, NULL);
  node_type_exec(&ntype, NULL, NULL, node_shader_exec_mapping);
  node_type_gpu(&ntype, gpu_shader_mapping);
  node_type_update(&ntype, node_shader_update_mapping);

  nodeRegisterType(&ntype);
}
