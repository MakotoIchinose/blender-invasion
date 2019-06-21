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
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup editor/io
 */

#ifdef WITH_USD
#  include "DNA_space_types.h"

#  include "BKE_context.h"
#  include "BKE_main.h"
#  include "BKE_report.h"

#  include "BLI_path_util.h"
#  include "BLI_string.h"

#  include "RNA_access.h"
#  include "RNA_define.h"

#  include "WM_api.h"
#  include "WM_types.h"

#  include "io_usd.h"
#  include "usd.h"

static int wm_usd_export_invoke(bContext *C, wmOperator *op, const wmEvent *event)
{
  if (!RNA_struct_property_is_set(op->ptr, "as_background_job")) {
    RNA_boolean_set(op->ptr, "as_background_job", true);
  }

  if (!RNA_struct_property_is_set(op->ptr, "filepath")) {
    Main *bmain = CTX_data_main(C);
    char filepath[FILE_MAX];

    if (BKE_main_blendfile_path(bmain)[0] == '\0') {
      BLI_strncpy(filepath, "untitled", sizeof(filepath));
    }
    else {
      BLI_strncpy(filepath, BKE_main_blendfile_path(bmain), sizeof(filepath));
    }

    BLI_path_extension_replace(filepath, sizeof(filepath), ".usdc");
    RNA_string_set(op->ptr, "filepath", filepath);
  }

  WM_event_add_fileselect(C, op);

  return OPERATOR_RUNNING_MODAL;

  UNUSED_VARS(event);
}

static int wm_usd_export_exec(bContext *C, wmOperator *op)
{
  if (!RNA_struct_property_is_set(op->ptr, "filepath")) {
    BKE_report(op->reports, RPT_ERROR, "No filename given");
    return OPERATOR_CANCELLED;
  }

  char filename[FILE_MAX];
  RNA_string_get(op->ptr, "filepath", filename);

  /* Take some defaults from the scene, if not specified explicitly. */
  Scene *scene = CTX_data_scene(C);

  const bool as_background_job = RNA_boolean_get(op->ptr, "as_background_job");
  const bool selected_objects_only = RNA_boolean_get(op->ptr, "selected_objects_only");
  const bool visible_objects_only = RNA_boolean_get(op->ptr, "visible_objects_only");

  struct USDExportParams params = {
      .selected_objects_only = selected_objects_only,
      .visible_objects_only = visible_objects_only,
  };

  bool ok = USD_export(scene, C, filename, &params, as_background_job);

  return as_background_job || ok ? OPERATOR_FINISHED : OPERATOR_CANCELLED;
}

void WM_OT_usd_export(struct wmOperatorType *ot)
{
  ot->name = "Export USD";
  ot->description = "Export current scene in a USD archive";
  ot->idname = "WM_OT_usd_export";

  ot->invoke = wm_usd_export_invoke;
  ot->exec = wm_usd_export_exec;
  ot->poll = WM_operator_winactive;

  WM_operator_properties_filesel(
      ot, 0, FILE_BLENDER, FILE_SAVE, WM_FILESEL_FILEPATH, FILE_DEFAULTDISPLAY, FILE_SORT_ALPHA);

  RNA_def_boolean(ot->srna,
                  "selected_objects_only",
                  false,
                  "Only export selected objects",
                  "Only selected objects are exported. Unselected parents of selected objects are "
                  "exported as empty transform.");

  RNA_def_boolean(ot->srna,
                  "visible_objects_only",
                  false,
                  "Only export visible objects",
                  "Only visible objects are exported. Invisible parents of visible objects are "
                  "exported as empty transform.");

  RNA_def_boolean(
      ot->srna,
      "as_background_job",
      false,
      "Run as Background Job",
      "Enable this to run the import in the background, disable to block Blender while importing. "
      "This option is deprecated; EXECUTE this operator to run in the foreground, and INVOKE it "
      "to run as a background job");
}

#endif /* WITH_USD */
