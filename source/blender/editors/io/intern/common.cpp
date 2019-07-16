extern "C" {
#include "BLI_listbase.h"
#include "BLI_math_matrix.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_modifier_types.h"

#include "BKE_modifier.h"
#include "BKE_modifier.h"
#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"

#include "RNA_access.h"

#include "bmesh.h"
#include "bmesh_tools.h"

#include "../io_common.h"

#include <stdio.h>
}

#include <iostream>
#include <chrono>

#include "common.hpp"
#include "iterators.hpp"

// Anonymous namespace for internal functions
namespace {

inline bool axis_enum_to_index(int &axis)
{
  switch (axis) {
    case AXIS_X:  // Fallthrough
    case AXIS_Y:  // Fallthrough
    case AXIS_Z:
      return false;   // Just swap
    case AXIS_NEG_X:  // Fallthrough
    case AXIS_NEG_Y:  // Fallthrough
    case AXIS_NEG_Z:
      axis -= AXIS_NEG_X;  // Transform the enum into an index
      return true;         // Swap and negate
    default:
      return false;
  }
}

}  // namespace

namespace common {

void name_compat(std::string &ob_name, const std::string &mesh_name)
{
  if (ob_name.compare(mesh_name) != 0) {
    ob_name += "_" + mesh_name;
  }
  size_t start_pos;
  while ((start_pos = ob_name.find(" ")) != std::string::npos)
    ob_name.replace(start_pos, 1, "_");
}

bool object_is_smoke_sim(const Object *const ob)
{
  ModifierData *md = modifiers_findByType((Object *)ob, eModifierType_Smoke);
  if (md) {
    SmokeModifierData *smd = (SmokeModifierData *)md;
    return (smd->type == MOD_SMOKE_TYPE_DOMAIN);
  }
  return false;
}

bool object_type_is_exportable(const Object *const ob)
{
  switch (ob->type) {
    case OB_MESH:
      return !object_is_smoke_sim(ob);
    case OB_CURVE:
    case OB_SURF:
      return true;
    case OB_LAMP:
    case OB_EMPTY:
    case OB_CAMERA:
      return false;
    default:
      // TODO someone Print in debug only
      fprintf(stderr, "Export for this object type is not yet defined %s\n", ob->id.name);
      return false;
  }
}

// Whether the object should be exported
bool should_export_object(const ExportSettings *const settings, const Object *const ob)
{
  if (!object_type_is_exportable(ob))
    return false;
  // If the object is a dupli, it's export satus depends on the parent
  if (!(ob->flag & BASE_FROM_DUPLI)) {
    /* These tests only make sense when the object isn't being instanced
     * into the scene. When it is, its exportability is determined by
     * its dupli-object and the DupliObject::no_draw property. */
    return (settings->selected_only ? ((ob->base_flag & BASE_SELECTED) != 0) : true) &&
           (settings->visible_only ? ((ob->base_flag & BASE_VISIBLE) != 0) : true) &&
           (settings->renderable_only ? ((ob->base_flag & BASE_ENABLED_RENDER) != 0) : true);
  }
  else if (!(ob->parent != NULL && ob->parent->transflag & OB_DUPLI))
    return should_export_object(settings, ob->parent);
  else {
    BLI_assert(!"should_export_object");
    return false;
  }
}

/**
 * Fills in mat with the matrix which converts the Y axis to `forward` and the Z axis to `up`,
 * scaled by `scale`, and where the remaining axis is the cross product of `up` and `forward`
 */
bool get_axis_remap_and_scale_matrix(float (&mat)[4][4], int forward, int up, float scale)
{
  bool negate[4];
  bool temp = axis_enum_to_index(up);  // Modifies up
  negate[up] = temp;
  temp = axis_enum_to_index(forward);
  negate[forward] = temp;
  int other_axis = (3 - forward - up);
  negate[other_axis] = false;
  negate[3] = false;
  // Can't have two axis be the same
  if (forward == up) {
    return false;
  }
  int axis_from = 0;
  for (int axis_to : {other_axis, forward, up, 3}) {
    for (int i = 0; i < 4; ++i) {
      mat[axis_from][i] = 0;
    }
    mat[axis_from][axis_to] = (negate[axis_to] ? -1 : 1) * scale;
    ++axis_from;
  }
  // A bit hacky, but it's a pointer anyway, it just modifies the first three elements
  mat[other_axis][3] = 0;
  cross_v3_v3v3(mat[other_axis], mat[up], mat[forward]);
  return true;
}

float get_unit_scale(const Scene *const scene)
{
  // From collada_internal.cpp
  PointerRNA scene_ptr, unit_settings;
  PropertyRNA *system_ptr, *scale_ptr;
  RNA_id_pointer_create((ID *)&scene->id, &scene_ptr);

  unit_settings = RNA_pointer_get(&scene_ptr, "unit_settings");
  system_ptr = RNA_struct_find_property(&unit_settings, "system");
  scale_ptr = RNA_struct_find_property(&unit_settings, "scale_length");

  int type = RNA_property_enum_get(&unit_settings, system_ptr);
  float scale = 1.0;

  switch (type) {
    case USER_UNIT_NONE:
      scale = 1.0;  // map 1 Blender unit to 1 Meter
      break;
    case USER_UNIT_METRIC:
      scale = RNA_property_float_get(&unit_settings, scale_ptr);
      break;
    case USER_UNIT_IMPERIAL:
      scale = RNA_property_float_get(&unit_settings, scale_ptr);
      // it looks like the conversion to Imperial is done implicitly.
      // So nothing to do here.
      break;
    default:
      BLI_assert("New unit system added but not handled");
  }

  return scale;
}

bool get_final_mesh(const ExportSettings *const settings,
                    const Scene *const escene,
                    const Object *eob,
                    Mesh **mesh /* out */,
                    float (*mat)[4][4] /* out */)
{
  bool r = get_axis_remap_and_scale_matrix(*mat,
                                           settings->axis_forward,
                                           settings->axis_up,
                                           settings->global_scale * get_unit_scale(escene));
  // Failed, up == forward
  if (!r) {
    // Ignore remapping
    scale_m4_fl(*mat, settings->global_scale * get_unit_scale(escene));
  }

  mul_m4_m4m4(*mat, *mat, eob->obmat);

  *mesh = eob->runtime.mesh_eval;

  if (settings->triangulate) {
    struct BMeshCreateParams bmcp = {false};
    struct BMeshFromMeshParams bmfmp = {true, false, false, 0};
    BMesh *bm = BKE_mesh_to_bmesh_ex(*mesh, &bmcp, &bmfmp);

    BM_mesh_triangulate(bm,
                        settings->quad_method,
                        settings->ngon_method,
                        4,
                        false /* tag_only */,
                        NULL,
                        NULL,
                        NULL);

    Mesh *result = BKE_mesh_from_bmesh_for_eval_nomain(bm, 0);
    BM_mesh_free(bm);

    *mesh = result;
    /* Needs to free? */
    return true;
  }
  /* Needs to free? */
  return false;
}

void free_mesh(Mesh *mesh, bool needs_free)
{
  if (needs_free)
    BKE_id_free(NULL, mesh);  // TODO someoene null? (alembic)
}

std::string get_version_string()
{
  return "";  // TODO someone implement
}

void export_start(bContext *UNUSED(C), ExportSettings *const settings)
{
  /* From alembic_capi.cc
   * XXX annoying hack: needed to prevent data corruption when changing
   * scene frame in separate threads
   */
  G.is_rendering = true;  // TODO someone Should use BKE_main_lock(bmain);?
  BKE_spacedata_draw_locks(true);
  // If render_modifiers use render depsgraph, to get render modifiers
  settings->depsgraph = DEG_graph_new(settings->scene,
                                      settings->view_layer,
                                      settings->render_modifiers ? DAG_EVAL_RENDER :
                                                                   DAG_EVAL_VIEWPORT);
  DEG_graph_build_from_view_layer(
      settings->depsgraph, settings->main, settings->scene, settings->view_layer);
  BKE_scene_graph_update_tagged(settings->depsgraph, settings->main);
}

bool export_end(bContext *UNUSED(C), ExportSettings *const settings)
{
  G.is_rendering = false;
  BKE_spacedata_draw_locks(false);
  DEG_graph_free(settings->depsgraph);
  if (settings->format_specific)
    MEM_freeN(settings->format_specific);
  MEM_freeN(settings);
  return true;
}

void import_start(bContext *UNUSED(C), ImportSettings *const settings)
{
  G.is_rendering = true;
  BKE_spacedata_draw_locks(true);
}

bool import_end(bContext *UNUSED(C), ImportSettings *const settings)
{
  G.is_rendering = false;
  BKE_spacedata_draw_locks(false);
  DEG_graph_free(settings->depsgraph);
  if (settings->format_specific)
    MEM_freeN(settings->format_specific);
  MEM_freeN(settings);
  return true;
}

const std::array<float, 3> calculate_normal(const Mesh *const mesh, const MPoly &mp)
{
  float no[3];
  BKE_mesh_calc_poly_normal(&mp, mesh->mloop + mp.loopstart, mesh->mvert, no);
  return std::array<float, 3>{no[0], no[1], no[2]};
}

std::vector<std::array<float, 3>> get_normals(const Mesh *const mesh)
{
  std::vector<std::array<float, 3>> normals{};
  normals.reserve(mesh->totvert);
  const float(*loop_no)[3] = static_cast<float(*)[3]>(
      CustomData_get_layer(&mesh->ldata, CD_NORMAL));
  unsigned loop_index = 0;
  MVert *verts = mesh->mvert;
  MPoly *mp = mesh->mpoly;
  MLoop *mloop = mesh->mloop;
  MLoop *ml = mloop;

  // TODO someone Should -0 be converted to 0?
  if (loop_no) {
    for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
      ml = mesh->mloop + mp->loopstart + (mp->totloop - 1);
      for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
        const float(&no)[3] = loop_no[ml->v];
        normals.push_back(std::array<float, 3>{no[0], no[1], no[2]});
      }
    }
  }
  else {
    float no[3];
    for (int i = 0, e = mesh->totpoly; i < e; ++i, ++mp) {
      ml = mloop + mp->loopstart + (mp->totloop - 1);

      /* Flat shaded, use common normal for all verts. */
      if ((mp->flag & ME_SMOOTH) == 0) {
        BKE_mesh_calc_poly_normal(mp, ml - (mp->totloop - 1), verts, no);
        normals.push_back(std::array<float, 3>{no[0], no[1], no[2]});
        ml -= mp->totloop;
        loop_index += mp->totloop;
      }
      else {
        /* Smooth shaded, use individual vert normals. */
        for (int j = 0; j < mp->totloop; --ml, ++j, ++loop_index) {
          normal_short_to_float_v3(no, verts[ml->v].no);
          normals.push_back(std::array<float, 3>{no[0], no[1], no[2]});
        }
      }
    }
  }
  normals.shrink_to_fit();
  return normals;
}
std::vector<std::array<float, 2>> get_uv(const Mesh *const mesh)
{
  std::vector<std::array<float, 2>> uvs{};
  uvs.reserve(mesh->totloop);
  for (int i = 0, e = mesh->totloop; i < e; ++i) {
    const float(&uv)[2] = mesh->mloopuv[i].uv;
    uvs.push_back(std::array<float, 2>{uv[0], uv[1]});
  }
  return uvs;
}

std::vector<std::array<float, 3>> get_vertices(const Mesh *const mesh)
{
  std::vector<std::array<float, 3>> vxs{};
  vxs.reserve(mesh->totvert);
  for (int i = 0, e = mesh->totvert; i < e; ++i) {
    const MVert &v = mesh->mvert[i];
    vxs.push_back(std::array<float, 3>{v.co[0], v.co[1], v.co[2]});
  }
  return vxs;
}

}  // namespace common
