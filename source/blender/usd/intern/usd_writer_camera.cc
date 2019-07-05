#include "usd_writer_camera.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "BKE_camera.h"

#include "DNA_camera_types.h"
#include "DNA_scene_types.h"
}

USDCameraWriter::USDCameraWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
}

bool USDCameraWriter::is_supported(const Object *object) const
{
  Camera *camera = static_cast<Camera *>(object->data);
  return camera->type == CAM_PERSP;
}

void USDCameraWriter::do_write(HierarchyContext &context)
{
  pxr::UsdTimeCode timecode = get_export_time_code();
  pxr::UsdGeomCamera usd_camera = pxr::UsdGeomCamera::Define(stage, usd_path_);

  Camera *camera = static_cast<Camera *>(context.object->data);
  Scene *scene = DEG_get_evaluated_scene(depsgraph);

  usd_camera.CreateProjectionAttr().Set(pxr::UsdGeomTokens->perspective);

  /* USD stores the focal length in "millimeters or tenths of world units", because at some point
   * they decided world units might be centimeters. Quite confusing, as the USD Viewer shows the
   * correct FoV when we write millimeters and not "tenths of world units".
   */
  usd_camera.CreateFocalLengthAttr().Set(camera->lens, timecode);

  float aperture_x, aperture_y;
  BKE_camera_sensor_size_for_render(camera, &scene->r, &aperture_x, &aperture_y);

  float film_aspect = aperture_x / aperture_y;
  usd_camera.CreateHorizontalApertureAttr().Set(aperture_x, timecode);
  usd_camera.CreateVerticalApertureAttr().Set(aperture_y, timecode);
  usd_camera.CreateHorizontalApertureOffsetAttr().Set(aperture_x * camera->shiftx, timecode);
  usd_camera.CreateVerticalApertureOffsetAttr().Set(aperture_y * camera->shifty * film_aspect,
                                                    timecode);

  usd_camera.CreateClippingRangeAttr().Set(
      pxr::VtValue(pxr::GfVec2f(camera->clip_start, camera->clip_end)), timecode);

  // Write DoF-related attributes.
  if (camera->dof.flag & CAM_DOF_ENABLED) {
    usd_camera.CreateFStopAttr().Set(camera->dof.aperture_fstop, timecode);

    float focus_distance = scene->unit.scale_length *
                           BKE_camera_object_dof_distance(context.object);
    usd_camera.CreateFocusDistanceAttr().Set(focus_distance, timecode);
  }
}
