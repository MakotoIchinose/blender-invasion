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
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup bke
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <ctype.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "DNA_object_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_library.h"
#include "BKE_customdata.h"
#include "BKE_bvhutils.h"
#include "BKE_remesh.h"

#ifdef WITH_QEX
#  include "igl_capi.h"
#  include "qex_capi.h"
#endif

Mesh *BKE_remesh_quad(
    Mesh *mesh, float gradient_size, float stiffness, int iter, bool direct_round)
{

#ifdef WITH_QEX
  Mesh *result = NULL;

  BKE_mesh_runtime_looptri_recalc(mesh);
  const MLoopTri *looptri = BKE_mesh_runtime_looptri_ensure(mesh);
  int num_tris = BKE_mesh_runtime_looptri_len(mesh);
  int num_verts = mesh->totvert;
  qex_TriMesh *trimesh = MEM_callocN(sizeof(qex_TriMesh), "trimesh");
  qex_QuadMesh quadmesh;  //= MEM_callocN(sizeof(qex_QuadMesh), "quadmesh");
  // qex_Valence *valence;

  MVertTri *verttri = MEM_callocN(sizeof(*verttri) * (size_t)num_tris, "remesh_looptri");
  BKE_mesh_runtime_verttri_from_looptri(
      verttri, mesh->mloop, looptri, BKE_mesh_runtime_looptri_len(mesh));

  float(*verts)[3] = MEM_calloc_arrayN((size_t)num_verts, sizeof(float) * 3, "verts");
  int(*tris)[3] = MEM_calloc_arrayN((size_t)num_tris, sizeof(int) * 3, "tris");
  float(*uv_tris)[3][2] = MEM_calloc_arrayN((size_t)num_tris, sizeof(float) * 3 * 2, "uv_tris");

  // fill data
  for (int i = 0; i < num_verts; i++) {
    copy_v3_v3(verts[i], mesh->mvert[i].co);
  }

  for (int i = 0; i < num_tris; i++) {
    tris[i][0] = verttri[i].tri[0];
    tris[i][1] = verttri[i].tri[1];
    tris[i][2] = verttri[i].tri[2];
  }

  // mixed integer quadrangulation
  // double stiffness = 5.0;
  // double gradient_size = 50.0;
  // double iter = 0;
  // bool direct_round = false;

  // find pairs of verts of original edges (= marked with seam)
  bool *hard_edges = MEM_callocN(sizeof(bool) * 3 * num_tris, "hard edges");
  // int *hard_verts = MEM_mallocN(sizeof(int) * mesh->totvert, "hard_verts");

  for (int i = 0; i < num_tris; i++) {
    int r_edges[3];
    BKE_mesh_looptri_get_real_edges(mesh, &looptri[i], r_edges);

    for (int j = 0; j < 3; j++) {
      bool seam = (r_edges[j] > -1) && (mesh->medge[r_edges[j]].flag & ME_SHARP);

      if (seam) {
        hard_edges[i * 3 + j] = true;
      }
      else {
        hard_edges[i * 3 + j] = false;
      }
    }
  }

  igl_miq(verts,
          tris,
          num_verts,
          num_tris,
          uv_tris,
          gradient_size,
          iter,
          stiffness,
          direct_round,
          hard_edges);

  // build trimesh
  trimesh->tri_count = (unsigned int)num_tris;
  trimesh->vertex_count = (unsigned int)num_verts;
  trimesh->vertices = MEM_calloc_arrayN(sizeof(qex_Point3), (size_t)num_verts, "trimesh vertices");
  trimesh->tris = MEM_calloc_arrayN(sizeof(qex_Tri), (size_t)num_tris, "trimesh tris");
  trimesh->uvTris = MEM_calloc_arrayN(sizeof(qex_UVTri), (size_t)num_tris, "trimesh uvtris");

  for (int i = 0; i < num_verts; i++) {
    trimesh->vertices[i].x[0] = verts[i][0];
    trimesh->vertices[i].x[1] = verts[i][1];
    trimesh->vertices[i].x[2] = verts[i][2];
  }

  for (int i = 0; i < num_tris; i++) {
    trimesh->tris[i].indices[0] = tris[i][0];
    trimesh->tris[i].indices[1] = tris[i][1];
    trimesh->tris[i].indices[2] = tris[i][2];
  }

  for (int i = 0; i < num_tris; i++) {

    trimesh->uvTris[i].uvs[0].x[0] = uv_tris[i][0][0];
    trimesh->uvTris[i].uvs[0].x[1] = uv_tris[i][0][1];

    trimesh->uvTris[i].uvs[1].x[0] = uv_tris[i][1][0];
    trimesh->uvTris[i].uvs[1].x[1] = uv_tris[i][1][1];

    trimesh->uvTris[i].uvs[2].x[0] = uv_tris[i][2][0];
    trimesh->uvTris[i].uvs[2].x[1] = uv_tris[i][2][1];
  }

  // quad extraction
  extractQuadMesh(trimesh, NULL, &quadmesh);

  // rebuild mesh
  if (quadmesh.quad_count > 0) {
    result = BKE_mesh_new_nomain(quadmesh.vertex_count, 0, quadmesh.quad_count, 0, 0);
    for (int i = 0; i < quadmesh.vertex_count; i++) {
      result->mvert[i].co[0] = quadmesh.vertices[i].x[0];
      result->mvert[i].co[1] = quadmesh.vertices[i].x[1];
      result->mvert[i].co[2] = quadmesh.vertices[i].x[2];
    }

    for (int i = 0; i < quadmesh.quad_count; i++) {
      result->mface[i].v4 = quadmesh.quads[i].indices[0];
      result->mface[i].v3 = quadmesh.quads[i].indices[1];
      result->mface[i].v2 = quadmesh.quads[i].indices[2];
      result->mface[i].v1 = quadmesh.quads[i].indices[3];
      result->mface->mat_nr = 0;
    }

    // build edges, tessfaces, normals
    BKE_mesh_calc_edges_tessface(result);
    BKE_mesh_convert_mfaces_to_mpolys(result);
    BKE_mesh_calc_normals(result);
    BKE_mesh_update_customdata_pointers(result, true);
  }

  // free stuff
  MEM_freeN(verts);
  MEM_freeN(tris);
  MEM_freeN(uv_tris);

  MEM_freeN(trimesh->vertices);
  MEM_freeN(trimesh->tris);
  MEM_freeN(trimesh->uvTris);
  MEM_freeN(trimesh);
  MEM_freeN(verttri);

  // if (quadmesh->vertices)
  //    free(quadmesh->vertices);
  // if (quadmesh->quads)
  //    free(quadmesh->quads);
  // MEM_freeN(quadmesh);

  return result;
  // remap customdata (reproject)
  // BVH...
#else
  return mesh;
#endif
}
