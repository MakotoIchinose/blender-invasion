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

/* **************** SCALAR MATH ******************** */
static bNodeSocketTemplate sh_node_math_in[] = {
    {SOCK_FLOAT, 1, N_("Value"), 0.5f, 0.5f, 0.5f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {SOCK_FLOAT, 1, N_("Value"), 0.5f, 0.5f, 0.5f, 1.0f, -10000.0f, 10000.0f, PROP_NONE},
    {-1, 0, ""}};

static bNodeSocketTemplate sh_node_math_out[] = {{SOCK_FLOAT, 0, N_("Value")}, {-1, 0, ""}};

static void node_shader_exec_math(void *UNUSED(data),
                                  int UNUSED(thread),
                                  bNode *node,
                                  bNodeExecData *UNUSED(execdata),
                                  bNodeStack **in,
                                  bNodeStack **out)
{
  float a, b, r = 0.0f;

  nodestack_get_vec(&a, SOCK_FLOAT, in[0]);
  nodestack_get_vec(&b, SOCK_FLOAT, in[1]);

  switch (node->custom1) {
    case NODE_MATH_ADD:
      r = a + b;
      break;
    case NODE_MATH_SUB:
      r = a - b;
      break;
    case NODE_MATH_MUL:
      r = a * b;
      break;
    case NODE_MATH_DIVIDE: {
      r = (b != 0.0f) ? a / b : 0.0f;
      break;
    }
    case NODE_MATH_SIN: {
      r = sinf(a);
      break;
    }
    case NODE_MATH_COS: {
      r = cosf(a);
      break;
    }
    case NODE_MATH_TAN: {
      r = tanf(a);
      break;
    }
    case NODE_MATH_ASIN: {
      r = (a <= 1.0f && a >= -1.0f) ? asinf(a) : 0.0f;
      break;
    }
    case NODE_MATH_ACOS: {
      r = (a <= 1.0f && a >= -1.0f) ? acosf(a) : 0.0f;
      break;
    }
    case NODE_MATH_ATAN: {
      r = atan(a);
      break;
    }
    case NODE_MATH_LOG: {
      r = (a > 0.0f && b > 0.0f) ? log(a) / log(b) : 0.0f;
      break;
    }
    case NODE_MATH_MIN: {
      r = (a < b) ? a : b;
      break;
    }
    case NODE_MATH_MAX: {
      r = (a > b) ? a : b;
      break;
    }
    case NODE_MATH_ROUND: {
      r = (a < 0.0f) ? (int)(a - 0.5f) : (int)(a + 0.5f);
      break;
    }
    case NODE_MATH_LESS: {
      r = (a < b) ? 1.0f : 0.0f;
      break;
    }
    case NODE_MATH_GREATER: {
      r = (a > b) ? 1.0f : 0.0f;
      break;
    }
    case NODE_MATH_MOD: {
      r = (b != 0.0f) ? fmod(a, b) : 0.0f;
      break;
    }
    case NODE_MATH_ABS: {
      r = fabsf(a);
      break;
    }
    case NODE_MATH_ATAN2: {
      r = atan2(a, b);
      break;
    }
    case NODE_MATH_FLOOR: {
      r = floorf(a);
      break;
    }
    case NODE_MATH_CEIL: {
      r = ceilf(a);
      break;
    }
    case NODE_MATH_FRACT: {
      r = a - floorf(a);
      break;
    }
    case NODE_MATH_SQRT: {
      r = (a > 0.0f) ? sqrt(a) : 0.0f;
      break;
    }
    case NODE_MATH_POW: {
      /* Only raise negative numbers by full integers */
      if (a >= 0.0f) {
        r = pow(a, b);
      }
      else {
        float y_mod_1 = fabsf(fmodf(b, 1.0f));
        /* If input value is not nearly an integer, fall back to zero, nicer than straight rounding
         */
        if (y_mod_1 > 0.999f || y_mod_1 < 0.001f) {
          r = powf(a, floorf(b + 0.5f));
        }
        else {
          r = 0.0f;
        }
      }
      break;
    }
    default:
      r = 0.0f;
  }
  if (node->custom2 & SHD_MATH_CLAMP) {
    CLAMP(r, 0.0f, 1.0f);
  }
  out[0]->vec[0] = r;
}

static int gpu_shader_math(GPUMaterial *mat,
                           bNode *node,
                           bNodeExecData *UNUSED(execdata),
                           GPUNodeStack *in,
                           GPUNodeStack *out)
{
  static const char *names[] = {
      [NODE_MATH_ADD] = "math_add",
      [NODE_MATH_SUB] = "math_subtract",
      [NODE_MATH_MUL] = "math_multiply",
      [NODE_MATH_DIVIDE] = "math_divide",
      [NODE_MATH_SIN] = "math_sine",
      [NODE_MATH_COS] = "math_cosine",
      [NODE_MATH_TAN] = "math_tangent",
      [NODE_MATH_ASIN] = "math_asin",
      [NODE_MATH_ACOS] = "math_acos",
      [NODE_MATH_ATAN] = "math_atan",
      [NODE_MATH_POW] = "math_pow",
      [NODE_MATH_LOG] = "math_log",
      [NODE_MATH_MIN] = "math_min",
      [NODE_MATH_MAX] = "math_max",
      [NODE_MATH_ROUND] = "math_round",
      [NODE_MATH_LESS] = "math_less_than",
      [NODE_MATH_GREATER] = "math_greater_than",
      [NODE_MATH_MOD] = "math_modulo",
      [NODE_MATH_ABS] = "math_abs",
      [NODE_MATH_ATAN2] = "math_atan2",
      [NODE_MATH_FLOOR] = "math_floor",
      [NODE_MATH_CEIL] = "math_ceil",
      [NODE_MATH_FRACT] = "math_fract",
      [NODE_MATH_SQRT] = "math_sqrt",
  };

  switch (node->custom1) {
    case NODE_MATH_SIN:
    case NODE_MATH_COS:
    case NODE_MATH_TAN:
    case NODE_MATH_ASIN:
    case NODE_MATH_ACOS:
    case NODE_MATH_ATAN:
    case NODE_MATH_ROUND:
    case NODE_MATH_ABS:
    case NODE_MATH_FLOOR:
    case NODE_MATH_FRACT:
    case NODE_MATH_CEIL:
    case NODE_MATH_SQRT: {
      /* use only first item and terminator */
      GPUNodeStack tmp_in[2];
      memcpy(&tmp_in[0], &in[0], sizeof(GPUNodeStack));
      memcpy(&tmp_in[1], &in[2], sizeof(GPUNodeStack));
      GPU_stack_link(mat, node, names[node->custom1], tmp_in, out);
      break;
    }
    default:
      GPU_stack_link(mat, node, names[node->custom1], in, out);
  }

  if (node->custom2 & SHD_MATH_CLAMP) {
    float min[3] = {0.0f, 0.0f, 0.0f};
    float max[3] = {1.0f, 1.0f, 1.0f};
    GPU_link(mat, "clamp_val", out[0].link, GPU_constant(min), GPU_constant(max), &out[0].link);
  }

  return 1;
}

static void node_shader_update_math(bNodeTree *UNUSED(ntree), bNode *node)
{
  bNodeSocket *sock = BLI_findlink(&node->inputs, 1);

  switch (node->custom1) {
    case NODE_MATH_SIN:
    case NODE_MATH_COS:
    case NODE_MATH_TAN:
    case NODE_MATH_ASIN:
    case NODE_MATH_ACOS:
    case NODE_MATH_ATAN:
    case NODE_MATH_ROUND:
    case NODE_MATH_ABS:
    case NODE_MATH_FLOOR:
    case NODE_MATH_FRACT:
    case NODE_MATH_CEIL:
    case NODE_MATH_SQRT:
      sock->flag |= SOCK_UNAVAIL;
      break;
    default:
      sock->flag &= ~SOCK_UNAVAIL;
  }
}

void register_node_type_sh_math(void)
{
  static bNodeType ntype;

  sh_node_type_base(&ntype, SH_NODE_MATH, "Math", NODE_CLASS_CONVERTOR, 0);
  node_type_socket_templates(&ntype, sh_node_math_in, sh_node_math_out);
  node_type_label(&ntype, node_math_label);
  node_type_storage(&ntype, "", NULL, NULL);
  node_type_exec(&ntype, NULL, NULL, node_shader_exec_math);
  node_type_gpu(&ntype, gpu_shader_math);
  node_type_update(&ntype, node_shader_update_math);

  nodeRegisterType(&ntype);
}
