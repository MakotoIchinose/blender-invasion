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

#ifdef WITH_OPENVDB
	#include "openvdb_capi.h"
#endif


struct OpenVDBLevelSet *BKE_remesh_voxel_ovdb_mesh_to_level_set_create(Mesh *mesh, struct OpenVDBTransform *transform)
{
	BKE_mesh_runtime_looptri_recalc(mesh);
	const MLoopTri *looptri = BKE_mesh_runtime_looptri_ensure(mesh);
	MVertTri *verttri = MEM_callocN(sizeof(*verttri) * BKE_mesh_runtime_looptri_len(mesh), "remesh_looptri");
	BKE_mesh_runtime_verttri_from_looptri(verttri, mesh->mloop, looptri, BKE_mesh_runtime_looptri_len(mesh));

	int totfaces = BKE_mesh_runtime_looptri_len(mesh);
	unsigned int totverts = mesh->totvert;
	float *verts = (float *)MEM_calloc_arrayN(totverts * 3, sizeof(float), "remesh_input_verts");
	unsigned int *faces = (unsigned int *)MEM_calloc_arrayN(totfaces * 3, sizeof(unsigned int), "remesh_intput_faces");

	for(int i = 0; i < totverts; i++) {
		MVert mvert = mesh->mvert[i];
		verts[i * 3] = mvert.co[0];
		verts[i * 3 + 1] = mvert.co[1];
		verts[i * 3 + 2] = mvert.co[2];
	}

	for(int i = 0; i < totfaces; i++) {
		MVertTri vt = verttri[i];
		faces[i * 3] = vt.tri[0];
		faces[i * 3 + 1] = vt.tri[1];
		faces[i * 3 + 2] = vt.tri[2];
	}

	struct OpenVDBLevelSet *level_set = OpenVDBLevelSet_create(false, NULL);
	OpenVDBLevelSet_mesh_to_level_set(level_set, verts, faces, totverts, totfaces, transform);

	MEM_freeN(verts);
	MEM_freeN(faces);
	MEM_freeN(verttri);

	return level_set;
}

Mesh *BKE_remesh_voxel_ovdb_volume_to_mesh_nomain(struct OpenVDBLevelSet *level_set, double isovalue, double adaptivity,
												  bool relax_disoriented_triangles)
{
	struct OpenVDBVolumeToMeshData output_mesh;
	OpenVDBLevelSet_volume_to_mesh(level_set, &output_mesh, isovalue, 0.0, false);

	Mesh *mesh = BKE_mesh_new_nomain(output_mesh.totvertices, 0, output_mesh.totquads, 0, 0);

	for(int i = 0; i < output_mesh.totvertices; i++) {
		float vco[3] = { output_mesh.vertices[i * 3], output_mesh.vertices[i * 3 + 1], output_mesh.vertices[i * 3 + 2]};
		copy_v3_v3(mesh->mvert[i].co, vco);
	}

	for(int i = 0; i < output_mesh.totquads; i++) {
		mesh->mface[i].v4 = output_mesh.quads[i * 4];
		mesh->mface[i].v3 = output_mesh.quads[i * 4 + 1];
		mesh->mface[i].v2 = output_mesh.quads[i * 4 + 2];
		mesh->mface[i].v1 = output_mesh.quads[i * 4 + 3];
	}

	BKE_mesh_calc_edges_tessface(mesh);
	BKE_mesh_convert_mfaces_to_mpolys(mesh);
	BKE_mesh_calc_normals(mesh);

	MEM_freeN(output_mesh.quads);
	MEM_freeN(output_mesh.vertices);

	return mesh;
}

void BKE_remesh_voxel_init_empty_vertex_color_layer(Mesh *mesh)
{
	MVertCol *color = CustomData_add_layer_named(&mesh->vdata, CD_MVERTCOL, CD_CALLOC, NULL, mesh->totvert, "vcols");
	for (int i = 0; i < mesh->totvert; i++) {
		color[i].r = 255;
		color[i].g = 255;
		color[i].b = 255;
		color[i].a = 255;
	}
}

void BKE_remesh_voxel_reproject_vertex_paint(Mesh *target, Mesh *source)
{
	BVHTreeFromMesh bvhtree = {NULL};
	BKE_bvhtree_from_mesh_get(&bvhtree, source, BVHTREE_FROM_VERTS, 2);
	MVertCol *target_color = CustomData_get_layer(&target->vdata, CD_MVERTCOL);
	MVert *target_verts =  CustomData_get_layer(&target->vdata, CD_MVERT);
	MVertCol *source_color  = CustomData_get_layer(&source->vdata, CD_MVERTCOL);
	for(int i = 0; i < target->totvert; i++) {
		float from_co[3];
		BVHTreeNearest nearest;
		nearest.index = -1;
		nearest.dist_sq = FLT_MAX;
		copy_v3_v3(from_co, target_verts[i].co);
		BLI_bvhtree_find_nearest(bvhtree.tree, from_co, &nearest, bvhtree.nearest_callback, &bvhtree);
		if (nearest.index != -1) {
			target_color[i].r = source_color[nearest.index].r;
			target_color[i].g = source_color[nearest.index].g;
			target_color[i].b = source_color[nearest.index].b;
			target_color[i].a = source_color[nearest.index].a;
		}
	}
	free_bvhtree_from_mesh(&bvhtree);
}
