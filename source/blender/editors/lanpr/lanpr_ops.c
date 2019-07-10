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
 */

/** \file
 * \ingroup editor/lanpr
 */

#include "ED_lanpr.h"

#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_math_matrix.h"
#include "BLI_task.h"
#include "BLI_utildefines.h"
#include "ED_lanpr.h"
#include "BKE_object.h"
#include "DNA_mesh_types.h"
#include "DNA_camera_types.h"
#include "DNA_modifier_types.h"
#include "DNA_text_types.h"
#include "DNA_lanpr_types.h"
#include "DNA_scene_types.h"
#include "DNA_meshdata_types.h"
#include "BKE_customdata.h"
#include "DEG_depsgraph_query.h"
#include "BKE_camera.h"
#include "BKE_collection.h"
#include "BKE_report.h"
#include "BKE_screen.h"
#include "BKE_text.h"
#include "BKE_context.h"

#include "bmesh.h"
#include "bmesh_class.h"
#include "bmesh_tools.h"

#include "WM_types.h"
#include "WM_api.h"

#include "ED_svg.h"
#include "BKE_text.h"

extern LANPR_SharedResource lanpr_share;
extern const char *RE_engine_id_BLENDER_LANPR;
struct Object;


static int lanpr_export_svg_exec(struct bContext *C, wmOperator *op)
{
  LANPR_RenderBuffer *rb = lanpr_share.render_buffer_shared;
  SceneLANPR *lanpr =
      &rb->scene->lanpr; /* XXX: This is not evaluated for copy_on_write stuff... */
  LANPR_LineLayer *ll;

  for (ll = lanpr->line_layers.first; ll; ll = ll->next) {
    Text *ta = BKE_text_add(CTX_data_main(C), "exported_svg");
    ED_svg_data_from_lanpr_chain(ta, rb, ll);
  }

  return OPERATOR_FINISHED;
}

static bool lanpr_render_buffer_found(struct bContext *C)
{
  if (lanpr_share.render_buffer_shared) {
    return true;
  }
  return false;
}

void SCENE_OT_lanpr_export_svg(wmOperatorType *ot)
{
  PropertyRNA *prop;

  /* identifiers */
  ot->name = "Export LANPR to SVG";
  ot->description = "Export LANPR render result into a SVG file";
  ot->idname = "SCENE_OT_lanpr_export_svg";

  /* callbacks */
  ot->exec = lanpr_export_svg_exec;
  ot->poll = lanpr_render_buffer_found;

  /* flag */
  ot->flag = OPTYPE_USE_EVAL_DATA;

  /* properties */
  /* Should have: facing, layer, visibility, file split... */
}

