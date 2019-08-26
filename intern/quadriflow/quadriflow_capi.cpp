
#include "quadriflow_capi.hpp"
#include "config.hpp"
#include "field-math.hpp"
#include "optimizer.hpp"
#include "parametrizer.hpp"
#include "loader.hpp"
#include "MEM_guardedalloc.h"
#include <unordered_map>

using namespace qflow;

struct obj_vertex {
  uint32_t p = (uint32_t)-1;
  uint32_t n = (uint32_t)-1;
  uint32_t uv = (uint32_t)-1;

  inline obj_vertex()
  {
  }

  inline obj_vertex(uint32_t pi)
  {
    p = pi;
  }

  inline bool operator==(const obj_vertex &v) const
  {
    return v.p == p && v.n == n && v.uv == uv;
  }
};

struct obj_vertexHash : std::unary_function<obj_vertex, size_t> {
  std::size_t operator()(const obj_vertex &v) const
  {
    size_t hash = std::hash<uint32_t>()(v.p);
    hash = hash * 37 + std::hash<uint32_t>()(v.uv);
    hash = hash * 37 + std::hash<uint32_t>()(v.n);
    return hash;
  }
};

typedef std::unordered_map<obj_vertex, uint32_t, obj_vertexHash> VertexMap;

void quadriflow_remesh(QuadriflowRemeshData *qrd)
{
  Parametrizer field;

  VertexMap vertexMap;
  int faces = -1;
  faces = qrd->target_faces;

  if (qrd->preserve_sharp) {
    field.flag_preserve_sharp = 1;
  }
  if (qrd->adaptive_scale) {
    field.flag_adaptive_scale = 1;
  }
  if (qrd->minimum_cost_flow) {
    field.flag_minimum_cost_flow = 1;
  }
  if (qrd->aggresive_sat) {
    field.flag_aggresive_sat = 1;
  }
  if (qrd->rng_seed) {
    field.hierarchy.rng_seed = qrd->rng_seed;
  }

  std::vector<Vector3d> positions;
  std::vector<uint32_t> indices;
  std::vector<obj_vertex> vertices;

  for (int i = 0; i < qrd->totverts; i++) {
    Vector3d v(qrd->verts[i * 3], qrd->verts[i * 3 + 1], qrd->verts[i * 3 + 2]);
    positions.push_back(v);
  }

  for (int q = 0; q < qrd->totfaces; q++) {
    Vector3i f(qrd->faces[q * 3], qrd->faces[q * 3 + 1], qrd->faces[q * 3 + 2]);

    obj_vertex tri[6];
    int nVertices = 3;

    tri[0] = obj_vertex(f[0]);
    tri[1] = obj_vertex(f[1]);
    tri[2] = obj_vertex(f[2]);

    for (int i = 0; i < nVertices; ++i) {
      const obj_vertex &v = tri[i];
      VertexMap::const_iterator it = vertexMap.find(v);
      if (it == vertexMap.end()) {
        vertexMap[v] = (uint32_t)vertices.size();
        indices.push_back((uint32_t)vertices.size());
        vertices.push_back(v);
      }
      else {
        indices.push_back(it->second);
      }
    }
  }

  field.F.resize(3, indices.size() / 3);
  memcpy(field.F.data(), indices.data(), sizeof(uint32_t) * indices.size());

  field.V.resize(3, vertices.size());
  for (uint32_t i = 0; i < vertices.size(); ++i)
    field.V.col(i) = positions.at(vertices[i].p);

  field.NormalizeMesh();
  field.Initialize(faces);

  Optimizer::optimize_orientations(field.hierarchy);
  field.ComputeOrientationSingularities();

  if (field.flag_adaptive_scale == 1) {
    field.EstimateSlope();
  }
  Optimizer::optimize_scale(field.hierarchy, field.rho, field.flag_adaptive_scale);
  field.flag_adaptive_scale = 1;

  Optimizer::optimize_positions(field.hierarchy, field.flag_adaptive_scale);

  field.ComputePositionSingularities();
  field.ComputeIndexMap();

  qrd->out_totverts = field.O_compact.size();
  qrd->out_totfaces = field.F_compact.size();

  qrd->out_verts = (float *)MEM_malloc_arrayN(
      qrd->out_totverts, 3 * sizeof(float), "quadriflow remesher out verts");
  qrd->out_faces = (unsigned int *)MEM_malloc_arrayN(
      qrd->out_totfaces, 4 * sizeof(unsigned int), "quadriflow remesh out quads");

  for (int i = 0; i < qrd->out_totverts; i++) {
    auto t = field.O_compact[i] * field.normalize_scale + field.normalize_offset;
    qrd->out_verts[i * 3] = t[0];
    qrd->out_verts[i * 3 + 1] = t[1];
    qrd->out_verts[i * 3 + 2] = t[2];
  }

  for (int i = 0; i < qrd->out_totfaces; i++) {
    qrd->out_faces[i * 4] = field.F_compact[i][0];
    qrd->out_faces[i * 4 + 1] = field.F_compact[i][1];
    qrd->out_faces[i * 4 + 2] = field.F_compact[i][2];
    qrd->out_faces[i * 4 + 3] = field.F_compact[i][3];
  }
}
