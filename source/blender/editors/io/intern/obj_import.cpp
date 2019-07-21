extern "C" {

#include "BLI_sys_types.h"
#include "BLI_math.h"
#include "BLT_translation.h"

#include "BKE_context.h"
#include "BKE_customdata.h"
#include "BKE_global.h"
#include "BKE_library.h"
#include "BKE_mesh.h"
#include "BKE_mesh_mapping.h"
#include "BKE_mesh_runtime.h"
#include "BKE_object.h"
#include "BKE_scene.h"

#include "DNA_curve_types.h"
#include "DNA_ID.h"
#include "DNA_layer_types.h"
#include "DNA_material_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_object_types.h"
#include "DNA_space_types.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "WM_api.h"
#include "WM_types.h"

#ifdef WITH_PYTHON
#  include "BPY_extern.h"
#endif

#include "ED_object.h"
#include "bmesh.h"
#include "bmesh_tools.h"

#include "obj.h"
#include "../io_common.h"
#include "../io_obj.h"
}

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <tuple>

#define BOOST_SPIRIT_DEBUG
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "common.hpp"
#include "iterators.hpp"

BOOST_FUSION_DEFINE_STRUCT((), obj_face, (int, vx)(int, uv)(int, no))
BOOST_FUSION_DEFINE_STRUCT((), float3, (float, x)(float, y)(float, z))
BOOST_FUSION_DEFINE_STRUCT((), float2, (float, u)(float, v))

namespace {

namespace ph = boost::phoenix;

namespace qi = boost::spirit::qi;
template<typename Result> using rule = qi::rule<const char *, Result>;

template<typename Inner> using vec = std::vector<Inner>;
using str = std::string;

struct OBJImport {
  ImportSettings *settings;
  vec<float3> vertices;
  vec<float2> uvs;
  vec<float3> normals;
  vec<vec<obj_face>> faces;
  // Object *current_object;
  Mesh *current_mesh;
  vec<Mesh *> meshes;

  OBJImport(ImportSettings *settings) : settings(settings)
  {
  }

  void v_line(float3 &v)
  {
    this->vertices.push_back(v);
  }
  void vn_line(float3 &no)
  {
    this->normals.push_back(no);
  }
  void vt_line(float2 &uv)
  {
    this->uvs.push_back(uv);
  }
  void f_line(vec<obj_face> &face)
  {
    this->faces.push_back(face);
    this->current_mesh->totloop += face.size();
    ++this->current_mesh->totpoly;
  }
  void g_line(vec<str> &s)
  {
  }
  void o_line(str &o)
  {
    Object *ob = BKE_object_add(
        settings->main, settings->scene, settings->view_layer, OB_MESH, o.c_str());
    this->current_mesh = (Mesh *)ob->data;
    this->meshes.push_back(current_mesh);
  }
  void s_line(int &s)
  {
  }
  void mtllib_line(vec<str> &mtllibs)
  {
  }
  void usemtl_line(str &s)
  {
  }
};

void OBJ_import_start(bContext *C, ImportSettings *const settings)
{
  common::import_start(C, settings);

  OBJImport import{settings};

  boost::iostreams::mapped_file mmap(settings->filepath, boost::iostreams::mapped_file::readonly);
  const char *const begin = mmap.const_data();
  const char *const end = begin + mmap.size() - 1;
  const char *curr = begin;

  /* clang-format off */

  using ign = qi::unused_type;

  // Components
  rule<ign()>    rest          = *qi::omit[qi::char_ - qi::eol];
  rule<ign()>    comment       = '#' >> rest;
  rule<str()>    token         = qi::lexeme[+(qi::graph)];
  rule<float()>  rulef         = qi::float_ | qi::as<float>()[qi::int_];
  rule<float2()> rule2f        = rulef >> rulef;
  rule<float3()> rule3f        = qi::float_ >> qi::float_ >> qi::float_; //rulef >> rulef >> rulef;
  rule<obj_face> rule_obj_face = qi::int_ >> -('/' >> -qi::int_) >> -('/' >> qi::int_);
  rule<int()>    on_off        = qi::lit("off")[qi::_val = 0] |
                                 qi::lit("on")[qi::_val = 1] |
                                 qi::int_[qi::_val = qi::_1];
  // Lines
  rule<float3()>   v_line      = 'v' >> rule3f >> rest;
  rule<float3()>   vn_line     = "vn" >> rule3f >> rest;
  rule<float2()>   vt_line     = "vt" >> rule2f >> rest;
  rule<vec<obj_face>()> f_line = 'f' >> qi::repeat(3, qi::inf)[rule_obj_face] >> rest;
  rule<vec<str>()> g_line      = 'g' >> *token >> rest;
  rule<str()>      o_line      = 'o' >> token >> qi::eol;
  rule<int()>      s_line      = 's' >> on_off >> rest;
  rule<vec<str>()> mtllib_line = "mtllib" >> +token >> rest;
  rule<str()>      usemtl_line = "usemtl" >> token >> rest;

  // When rule x matches, run function of same name on the import
  // object (instance of OBJImport) with the match
#define BIND(x) x[ph::bind(&OBJImport::x, &import, qi::_1)]


  bool result = qi::phrase_parse(first /* modifies */, last,
                                 *(BIND(v_line) |
                                   BIND(vn_line) |
                                   BIND(vt_line) |
                                   BIND(f_line) |
                                   BIND(g_line) |
                                   BIND(o_line) |
                                   BIND(s_line) |
                                   BIND(mtllib_line) |
                                   BIND(usemtl_line)),
                                 qi::blank | qi::char_(0) | comment);
#undef BIND
  /* clang-format on */

  if (!result || curr != end) {
    std::cerr << "Couldn't parse near: " << curr;
    return;
  }

  if (import.meshes.size() >= 2) {
    std::cerr << "CAN'T IMPORT MORE THAN ONE OBJECT YET\n";
    return;
  }

  Mesh *mesh = import.current_mesh;
  mesh->totvert = import.vertices.size();
  mesh->mvert = (struct MVert *)CustomData_add_layer(
      &mesh->vdata, CD_MVERT, CD_CALLOC, NULL, mesh->totvert);

  for (int i = 0; i < mesh->totvert; ++i) {
    mesh->mvert->co[0] = import.vertices[i].x;
    mesh->mvert->co[1] = import.vertices[i].y;
    mesh->mvert->co[2] = import.vertices[i].z;
  }

  // mesh->totpoly = import.faces;
  mesh->mpoly = (MPoly *)CustomData_add_layer(
      &mesh->pdata, CD_MPOLY, CD_CALLOC, NULL, mesh->totpoly);
  mesh->mloop = (MLoop *)CustomData_add_layer(
      &mesh->ldata, CD_MLOOP, CD_CALLOC, NULL, mesh->totloop);

  size_t ls = 0;
  for (size_t i = 0; i < import.faces.size(); ++i) {
    mesh->mpoly[i].loopstart = ls;
    mesh->mpoly[i].totloop = import.faces[i].size();
    for (size_t j = mesh->mpoly[i].loopstart; j < ls + mesh->mpoly[i].totloop; ++ls) {
      mesh->mloop[j].v = import.faces[i][j].vx;
    }
    ls += import.faces[i].size() - 1;
  }

  // Will add the edges. Temporary
  BKE_mesh_calc_edges(mesh, false, false);
  // BKE_mesh_validate(mesh, false, false);

  DEG_id_tag_update(&settings->scene->id, ID_RECALC_COPY_ON_WRITE);
  DEG_relations_tag_update(settings->main);
  WM_event_add_notifier(C, NC_OBJECT | ND_TRANSFORM, NULL);
}

bool OBJ_import_end(bContext *C, ImportSettings *const settings)
{
  return common::import_end(C, settings);
}
}  // namespace

extern "C" {
bool OBJ_import(bContext *C, ImportSettings *const settings)
{
  return common::time(C, settings, &OBJ_import_start, &OBJ_import_end);
}
}
