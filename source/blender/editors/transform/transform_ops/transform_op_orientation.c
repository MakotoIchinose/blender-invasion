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
 * \ingroup edtransform
 */

#include "MEM_guardedalloc.h"

#include "DNA_anim_types.h"
#include "DNA_armature_types.h"
#include "DNA_constraint_types.h"
#include "DNA_mask_types.h"
#include "DNA_mesh_types.h"
#include "DNA_movieclip_types.h"
#include "DNA_scene_types.h" /* PET modes */
#include "DNA_workspace_types.h"
#include "DNA_gpencil_types.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_report.h"
#include "BKE_editmesh.h"
#include "BKE_layer.h"
#include "BKE_scene.h"

#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "ED_image.h"
#include "ED_keyframing.h"
#include "ED_screen.h"
#include "ED_space_api.h"
#include "ED_markers.h"
#include "ED_view3d.h"
#include "ED_mesh.h"
#include "ED_clip.h"
#include "ED_node.h"
#include "ED_gpencil.h"
#include "ED_sculpt.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "BLF_api.h"
#include "BLT_translation.h"

#include "WM_api.h"
#include "WM_message.h"
#include "WM_types.h"
#include "WM_toolsystem.h"

#include "UI_view2d.h"
#include "UI_interface.h"
#include "UI_interface_icons.h"
#include "UI_resources.h"

#include "ED_screen.h"
/* for USE_LOOPSLIDE_HACK only */
#include "ED_mesh.h"

#include "transform.h"
#include "transform_convert.h"
#include "transform_op.h"

static int select_orientation_exec(bContext *C, wmOperator *op)
{
  Scene *scene = CTX_data_scene(C);
  View3D *v3d = CTX_wm_view3d(C);

  int orientation = RNA_enum_get(op->ptr, "orientation");

  BKE_scene_orientation_slot_set_index(&scene->orientation_slots[SCE_ORIENT_DEFAULT], orientation);

  WM_event_add_notifier(C, NC_SCENE | ND_TOOLSETTINGS, NULL);
  WM_event_add_notifier(C, NC_SPACE | ND_SPACE_VIEW3D, v3d);

  struct wmMsgBus *mbus = CTX_wm_message_bus(C);
  WM_msg_publish_rna_prop(mbus, &scene->id, scene, TransformOrientationSlot, type);

  return OPERATOR_FINISHED;
}

static int select_orientation_invoke(bContext *C,
                                     wmOperator *UNUSED(op),
                                     const wmEvent *UNUSED(event))
{
  uiPopupMenu *pup;
  uiLayout *layout;

  pup = UI_popup_menu_begin(C, IFACE_("Orientation"), ICON_NONE);
  layout = UI_popup_menu_layout(pup);
  uiItemsEnumO(layout, "TRANSFORM_OT_select_orientation", "orientation");
  UI_popup_menu_end(C, pup);

  return OPERATOR_INTERFACE;
}

void TRANSFORM_OT_select_orientation(struct wmOperatorType *ot)
{
  PropertyRNA *prop;

  /* identifiers */
  ot->name = "Select Orientation";
  ot->description = "Select transformation orientation";
  ot->idname = "TRANSFORM_OT_select_orientation";
  ot->flag = OPTYPE_UNDO;

  /* api callbacks */
  ot->invoke = select_orientation_invoke;
  ot->exec = select_orientation_exec;
  ot->poll = ED_operator_view3d_active;

  prop = RNA_def_property(ot->srna, "orientation", PROP_ENUM, PROP_NONE);
  RNA_def_property_ui_text(prop, "Orientation", "Transformation orientation");
  RNA_def_enum_funcs(prop, rna_TransformOrientation_itemf);
}

static int delete_orientation_exec(bContext *C, wmOperator *UNUSED(op))
{
  Scene *scene = CTX_data_scene(C);
  BIF_removeTransformOrientationIndex(C,
                                      scene->orientation_slots[SCE_ORIENT_DEFAULT].index_custom);

  WM_event_add_notifier(C, NC_SCENE | NA_EDITED, scene);

  return OPERATOR_FINISHED;
}

static int delete_orientation_invoke(bContext *C, wmOperator *op, const wmEvent *UNUSED(event))
{
  return delete_orientation_exec(C, op);
}

static bool delete_orientation_poll(bContext *C)
{
  Scene *scene = CTX_data_scene(C);

  if (ED_operator_areaactive(C) == 0) {
    return 0;
  }

  return ((scene->orientation_slots[SCE_ORIENT_DEFAULT].type >= V3D_ORIENT_CUSTOM) &&
          (scene->orientation_slots[SCE_ORIENT_DEFAULT].index_custom != -1));
}

void TRANSFORM_OT_delete_orientation(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Delete Orientation";
  ot->description = "Delete transformation orientation";
  ot->idname = "TRANSFORM_OT_delete_orientation";
  ot->flag = OPTYPE_UNDO;

  /* api callbacks */
  ot->invoke = delete_orientation_invoke;
  ot->exec = delete_orientation_exec;
  ot->poll = delete_orientation_poll;
}

static int create_orientation_exec(bContext *C, wmOperator *op)
{
  char name[MAX_NAME];
  const bool use = RNA_boolean_get(op->ptr, "use");
  const bool overwrite = RNA_boolean_get(op->ptr, "overwrite");
  const bool use_view = RNA_boolean_get(op->ptr, "use_view");
  View3D *v3d = CTX_wm_view3d(C);

  RNA_string_get(op->ptr, "name", name);

  if (use && !v3d) {
    BKE_report(op->reports,
               RPT_ERROR,
               "Create Orientation's 'use' parameter only valid in a 3DView context");
    return OPERATOR_CANCELLED;
  }

  BIF_createTransformOrientation(C, op->reports, name, use_view, use, overwrite);

  WM_event_add_notifier(C, NC_SPACE | ND_SPACE_VIEW3D, v3d);
  WM_event_add_notifier(C, NC_SCENE | NA_EDITED, CTX_data_scene(C));

  return OPERATOR_FINISHED;
}

void TRANSFORM_OT_create_orientation(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Create Orientation";
  ot->description = "Create transformation orientation from selection";
  ot->idname = "TRANSFORM_OT_create_orientation";
  ot->flag = OPTYPE_REGISTER | OPTYPE_UNDO;

  /* api callbacks */
  ot->exec = create_orientation_exec;
  ot->poll = ED_operator_areaactive;

  RNA_def_string(ot->srna, "name", NULL, MAX_NAME, "Name", "Name of the new custom orientation");
  RNA_def_boolean(
      ot->srna,
      "use_view",
      false,
      "Use View",
      "Use the current view instead of the active object to create the new orientation");

  WM_operatortype_props_advanced_begin(ot);

  RNA_def_boolean(
      ot->srna, "use", false, "Use after creation", "Select orientation after its creation");
  RNA_def_boolean(ot->srna,
                  "overwrite",
                  false,
                  "Overwrite previous",
                  "Overwrite previously created orientation with same name");
}
