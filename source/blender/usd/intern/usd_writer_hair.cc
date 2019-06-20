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
}

void USDHairWriter::do_write(HierarchyContext &context)
{
  ParticleSystem *psys = context.particle_system;
  ParticleCacheKey **cache = psys->pathcache;
  if (cache == nullptr) {
    return;
  }

  pxr::UsdTimeCode timecode = get_export_time_code();
  pxr::UsdGeomBasisCurves curves = pxr::UsdGeomBasisCurves::Define(usd_export_context_.stage,
                                                                   usd_export_context_.usd_path);

  // TODO(Sybren): deal with (psys->part->flag & PART_HAIR_BSPLINE)
  curves.CreateBasisAttr(pxr::VtValue(pxr::UsdGeomTokens->bspline));
  curves.CreateTypeAttr(pxr::VtValue(pxr::UsdGeomTokens->cubic));

  pxr::VtArray<pxr::GfVec3f> points;
  pxr::VtIntArray curve_point_counts;
  curve_point_counts.reserve(psys->totpart);

  ParticleCacheKey *strand;
  for (int strand_index = 0; strand_index < psys->totpart; ++strand_index) {
    strand = cache[strand_index];

    int point_count = strand->segments + 1;
    curve_point_counts.push_back(point_count);
    // colors.push_back(pxr::GfVec3f(strand->col));

    for (int point_index = 0; point_index < point_count; ++point_index, ++strand) {
      points.push_back(pxr::GfVec3f(strand->co));
    }
  }

  curves.CreatePointsAttr().Set(points, timecode);
  curves.CreateCurveVertexCountsAttr().Set(curve_point_counts, timecode);

  if (psys->totpart > 0) {
    pxr::VtArray<pxr::GfVec3f> colors;
    colors.push_back(pxr::GfVec3f(cache[0]->col));
    curves.CreateDisplayColorAttr(pxr::VtValue(colors));
  }
}

bool USDHairWriter::check_is_animated(const HierarchyContext &) const
{
  return true;
}
