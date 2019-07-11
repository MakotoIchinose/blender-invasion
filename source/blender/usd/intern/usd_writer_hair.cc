#include "usd_writer_hair.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "BKE_particle.h"

#include "DNA_particle_types.h"
}

USDHairWriter::USDHairWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
  printf("\033[39;1mHAIR writer\033[0m created at %s\n", ctx.usd_path.GetString().c_str());
}

void USDHairWriter::do_write(HierarchyContext &context)
{
  ParticleSystem *psys = context.particle_system;
  pxr::UsdTimeCode timecode = get_export_time_code();

  printf("\033[34;1mHAIR writer\033[0m writing %s %s\n",
         context.object->id.name + 2,
         psys->part->id.name + 2);
  pxr::UsdGeomBasisCurves curves = pxr::UsdGeomBasisCurves::Define(stage, usd_path_);

  // TODO: deal with (psys->part->flag & PART_HAIR_BSPLINE)
  curves.CreateBasisAttr(pxr::VtValue(pxr::UsdGeomTokens->bspline));
  curves.CreateTypeAttr(pxr::VtValue(pxr::UsdGeomTokens->cubic));

  pxr::VtArray<pxr::GfVec3f> points;
  pxr::VtIntArray curve_point_counts;
  pxr::VtArray<pxr::GfVec3f> colors;
  curve_point_counts.reserve(psys->totpart);
  colors.reserve(psys->totpart);

  ParticleCacheKey **cache = psys->pathcache;
  ParticleCacheKey *strand;
  for (int strand_index = 0; strand_index < psys->totpart; ++strand_index) {
    strand = cache[strand_index];

    int point_count = strand->segments + 1;
    curve_point_counts.push_back(point_count);
    colors.push_back(pxr::GfVec3f(strand->col));

    for (int point_index = 0; point_index < point_count; ++point_index, ++strand) {
      points.push_back(pxr::GfVec3f(strand->co));
    }
  }

  curves.CreatePointsAttr().Set(points, timecode);
  curves.CreateCurveVertexCountsAttr().Set(curve_point_counts, timecode);
  curves.CreateDisplayColorAttr(pxr::VtValue(colors));

  if (psys->totpart > 0) {
    // pxr::VtArray<pxr::GfVec3f> colors;
    // colors.push_back(pxr::GfVec3f(cache[0]->col));
  }
}

bool USDHairWriter::check_is_animated(const HierarchyContext &) const
{
  return true;
}
