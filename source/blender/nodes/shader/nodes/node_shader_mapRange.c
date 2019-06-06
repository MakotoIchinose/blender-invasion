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

/* **************** Map Range ******************** */
static bNodeSocketTemplate sh_node_map_range_in[] = {
    {SOCK_FLOAT, 1, N_("Value"), 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("From Min"), 0.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("From Max"), 1.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("To Min"), 0.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("To Max"), 1.0f, 1.0f, 1.0f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {-1, 0, ""},
};
static bNodeSocketTemplate sh_node_map_range_out[] = {
    {SOCK_FLOAT, 0, N_("Value")},
    {-1, 0, ""},
};

static void node_shader_exec_map_range(void *UNUSED(data),
                                       int UNUSED(thread),
                                       bNode *UNUSED(node),
                                       bNodeExecData *UNUSED(execdata),
                                       bNodeStack **in,
                                       bNodeStack **out)
{
  float value, fromMin, fromMax, toMin, toMax;

  nodestack_get_vec(&value, SOCK_FLOAT, in[0]);
  nodestack_get_vec(&fromMin, SOCK_FLOAT, in[1]);
  nodestack_get_vec(&fromMax, SOCK_FLOAT, in[2]);
  nodestack_get_vec(&toMin, SOCK_FLOAT, in[3]);
  nodestack_get_vec(&toMax, SOCK_FLOAT, in[4]);

  if (fromMax != fromMin) {
    out[0]->vec[0] = toMin + ((value - fromMin) / (fromMax - fromMin)) * (toMax - toMin);
  }
  else {
    out[0]->vec[0] = 0.0f;
  }
}

static int gpu_shader_map_range(GPUMaterial *mat,
                                bNode *node,
                                bNodeExecData *UNUSED(execdata),
                                GPUNodeStack *in,
                                GPUNodeStack *out)
{
  GPU_stack_link(mat, node, "map_range", in, out);
  return 1;
}

void register_node_type_sh_map_range(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_MAP_RANGE, "Map Range", NODE_CLASS_CONVERTOR, 0);
  node_type_socket_templates(&ntype, sh_node_map_range_in, sh_node_map_range_out);
  node_type_exec(&ntype, NULL, NULL, node_shader_exec_map_range);
  node_type_gpu(&ntype, gpu_shader_map_range);

  nodeRegisterType(&ntype);
}
