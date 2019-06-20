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
 * \ingroup USD
 */

#include "../usd.h"
#include "debug_timer.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

#include "DNA_scene_types.h"

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_scene.h"
/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#define new extern_new
#include "BKE_screen.h"
#undef new

#include "BLI_fileops.h"
#include "BLI_string.h"

#include "MEM_guardedalloc.h"

#include "WM_api.h"
#include "WM_types.h"
}

struct ExportSettings {
  Scene *scene;
  /** Scene layer to export; all its objects will be exported, unless selected_only=true. */
  ViewLayer *view_layer;
  Depsgraph *depsgraph;

  USDExportParams params;
};

struct ExportJobData {
  ViewLayer *view_layer;
  Main *bmain;

  char filename[1024];
  ExportSettings settings;

  short *stop;
  short *do_update;
  float *progress;

  bool was_canceled;
  bool export_ok;
};

static void export_startjob(void *customdata, short *stop, short *do_update, float *progress)
{
  Timer timer_("Export to USD");
  ExportJobData *data = static_cast<ExportJobData *>(customdata);

  data->stop = stop;
  data->do_update = do_update;
  data->progress = progress;
  data->was_canceled = false;

  /* XXX annoying hack: needed to prevent data corruption when changing
   * scene frame in separate threads
   */
  G.is_rendering = true;
  BKE_spacedata_draw_locks(true);

  G.is_break = false;

  {
    DEG_graph_build_from_view_layer(
        data->settings.depsgraph, data->bmain, data->settings.scene, data->view_layer);
    BKE_scene_graph_update_tagged(data->settings.depsgraph, data->bmain);
  }

  Scene *scene = data->settings.scene; /* for the CFRA macro */
  const int orig_frame = CFRA;

  {
    Timer usd_write_timer_("Writing to USD");

    // Create a stage with the Only Correct up-axis.
    pxr::UsdStageRefPtr usd_stage = pxr::UsdStage::CreateNew(data->filename);
    usd_stage->SetMetadata(pxr::UsdGeomTokens->upAxis, pxr::VtValue(pxr::UsdGeomTokens->z));

    USDHierarchyIterator iter(data->settings.depsgraph, usd_stage);

    // This should be done for every frame, when exporting animation:
    iter.iterate();

    iter.release_writers();
    usd_stage->GetRootLayer()->Save();
  }

  *progress = 1.0;

  if (CFRA != orig_frame) {
    CFRA = orig_frame;

    BKE_scene_graph_update_for_newframe(data->settings.depsgraph, data->bmain);
  }

  data->export_ok = !data->was_canceled;
}

static void export_endjob(void *customdata)
{
  ExportJobData *data = static_cast<ExportJobData *>(customdata);

  DEG_graph_free(data->settings.depsgraph);

  if (data->was_canceled && BLI_exists(data->filename)) {
    BLI_delete(data->filename, false, false);
  }

  G.is_rendering = false;
  BKE_spacedata_draw_locks(false);
}

bool USD_export(Scene *scene,
                bContext *C,
                const char *filepath,
                const struct USDExportParams *params,
                bool as_background_job)
{
  ExportJobData *job = static_cast<ExportJobData *>(
      MEM_mallocN(sizeof(ExportJobData), "ExportJobData"));

  job->view_layer = CTX_data_view_layer(C);
  job->bmain = CTX_data_main(C);
  job->export_ok = false;
  BLI_strncpy(job->filename, filepath, 1024);

  job->settings.scene = scene;
  job->settings.depsgraph = DEG_graph_new(scene, job->view_layer, DAG_EVAL_RENDER);

  /* TODO(Sybren): for now we only export the active scene layer.
   * Later in the 2.8 development process this may be replaced by using
   * a specific collection for Alembic I/O, which can then be toggled
   * between "real" objects and cached Alembic files. */
  job->settings.view_layer = job->view_layer;

  job->settings.params = *params;

  bool export_ok = false;
  if (as_background_job) {
    wmJob *wm_job = WM_jobs_get(CTX_wm_manager(C),
                                CTX_wm_window(C),
                                job->settings.scene,
                                "USD Export",
                                WM_JOB_PROGRESS,
                                WM_JOB_TYPE_ALEMBIC);

    /* setup job */
    WM_jobs_customdata_set(wm_job, job, MEM_freeN);
    WM_jobs_timer(wm_job, 0.1, NC_SCENE | ND_FRAME, NC_SCENE | ND_FRAME);
    WM_jobs_callbacks(wm_job, export_startjob, NULL, NULL, export_endjob);

    WM_jobs_start(CTX_wm_manager(C), wm_job);
  }
  else {
    /* Fake a job context, so that we don't need NULL pointer checks while exporting. */
    short stop = 0, do_update = 0;
    float progress = 0.f;

    export_startjob(job, &stop, &do_update, &progress);
    export_endjob(job);
    export_ok = job->export_ok;

    MEM_freeN(job);
  }

  return export_ok;
}
