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
 * The Original Code is Copyright (C) 2015 Blender Foundation.
 * All rights reserved.
 */


#include "openvdb_level_set.h"
#include "openvdb_util.h"
#include "openvdb_capi.h"
#include "MEM_guardedalloc.h"

void OpenVDB_level_set_remesh(struct OpenVDBRemeshData *rmd){

	std::vector<openvdb::Vec3s> points;
	std::vector<openvdb::Vec3I > triangles;
	std::vector<openvdb::Vec4I > quads;
	std::vector<openvdb::Vec3s> out_points;
	std::vector<openvdb::Vec4I > out_quads;
	const openvdb::math::Transform xform;

	for(int i = 0; i < rmd->totverts; i++) {
		openvdb::Vec3s v(rmd->verts[i * 3 ], rmd->verts[i * 3 + 1], rmd->verts[i * 3 + 2]);
		points.push_back(v);
	}

	for(int i = 0; i < rmd->totfaces; i++) {
		openvdb::Vec3I f(rmd->faces[i * 3 ], rmd->faces[i * 3 + 1], rmd->faces[i * 3 + 2]);
		triangles.push_back(f);
	}

	openvdb::initialize();
	openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform((double)rmd->voxel_size);
	const openvdb::FloatGrid::Ptr grid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(*transform, points, triangles, quads, 1);
	openvdb::tools::volumeToMesh<openvdb::FloatGrid>(*grid, out_points, out_quads, (double)rmd->isovalue);
	rmd->out_verts = (float *)MEM_malloc_arrayN(out_points.size(), 3 * sizeof (float), "openvdb remesher out verts");
	rmd->out_faces = (unsigned int*)MEM_malloc_arrayN(out_quads.size(), 4 * sizeof (unsigned int), "openvdb remesh out quads");
	rmd->out_totverts = out_points.size();
	rmd->out_totfaces = out_quads.size();

	for(int i = 0; i < out_points.size(); i++) {
		rmd->out_verts[i * 3] = out_points[i].x();
		rmd->out_verts[i * 3 + 1] = out_points[i].y();
		rmd->out_verts[i * 3 + 2] = out_points[i].z();
	}

	for(int i = 0; i < out_quads.size(); i++) {
		rmd->out_faces[i * 4] = out_quads[i].x();
		rmd->out_faces[i * 4 + 1] = out_quads[i].y();
		rmd->out_faces[i * 4 + 2] = out_quads[i].z();
		rmd->out_faces[i * 4 + 3] = out_quads[i].w();
	}
}
