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

#ifndef __BLI_DELAUNAY_2D_H__
#define __BLI_DELAUNAY_2D_H__

/** \file
 * \ingroup bli
 */

/**
 * Interface for Constrained Delaunay Triangulation (CDT) in 2D.
 *
 * The input is a set of vertices, edges between those vertices,
 * and faces using those vertices.
 * Those inputs are called "constraints". The output must contain
 * those constraints, or at least edges, points, and vertices that
 * may be pieced together to form the constraints. Part of the
 * work of doing the CDT is to detect intersections and mergers
 * among the input elements, so these routines are also useful
 * for doing 2D intersection.
 *
 * The output is a triangulation of the plane that includes the
 * constraints in the above sense, and also satisfies the
 * "Delaunay condition" as modified to take into account that
 * the constraints must be there: for every non-constrained edge
 * in the output, there is a circle through the endpoints that
 * does not contain any of the vertices directly connected to
 * those endpoints. What this means in practice is that as
 * much as possible the triangles look "nice" -- not too long
 * and skinny.
 *
 * Optionally, the output can be a subset of the triangulation
 * (but still containing all of the constraints), to get the
 * effect of 2D intersection.
 *
 * The underlying method is incremental, but we need to know
 * beforehand a bounding box for all of the constraints.
 * This code can be extended in the future to allow for
 * deletion of constraints, if there is a use in Blender
 * for dynamically maintaining a triangulation.
 */

/**
 * Input to Constrained Delaunay Triangulation.
 * There are verts_len vertices, whose coordinates
 * are given by vert_coords. For the rest of the input,
 * vertices are referred to by indices into that array.
 * Edges and Faces are optional. If provided, they will
 * appear in the output triangulation ("constraints").
 * One can provide faces and not edges -- the edges
 * implied by the faces will be inferred.
 *
 * The edges are given by pairs of vertex indices.
 * The faces are given in a triple `(faces, faces_start_table, faces_len_table)`
 * to represent a list-of-lists as follows:
 * the vertex indices for a counterclockwise traversal of
 * face number `i` starts at `faces_start_table[i]` and has `faces_len_table[i]`
 * elements.
 *
 * The edges implied by the faces are automatically added
 * and need not be put in the edges array, which is intended
 * as a way to specify edges that are not part of any face.
 *
 * Epsilon is used for "is it near enough" distance calculations.
 */
typedef struct CDT_input {
  int verts_len;
  int edges_len;
  int faces_len;
  float (*vert_coords)[2];
  int (*edges)[2];
  int *faces;
  int *faces_start_table;
  int *faces_len_table;
  float epsilon;
} CDT_input;

/**
 * A representation of the triangulation for output.
 * See #CDT_input for the representation of the output
 * vertices, edges, and faces, all represented in
 * a similar way to the input.
 *
 * The output may have merged some input vertices together,
 * if they were closer than some epsilon distance.
 * The output edges may be overlapping sub-segments of some
 * input edges; or they may be new edges for the triangulation.
 * The output faces may be pieces of some input faces, or they
 * may be new.
 *
 * In the same way that faces lists-of-lists were represented by
 * a run-together array and a "start" and "len" extra array,
 * similar triples are used to represent the output to input
 * mapping of vertices, edges, and faces.
 *
 * Those triples are:
 * - verts_orig, verts_orig_start_table, verts_orig_len_table
 * - edges_orig, edges_orig_start_table, edges_orig_len_table
 * - faces_orig, faces_orig_start_table, faces_orig_len_table
 *
 * For edges, the edges_orig triple can also say which original face
 * edge is part of a given output edge. If an index in edges_orig
 * is greater than the input's edges_len, then subtract input's edges_len
 * from it to some number i: then the face edge that starts from the
 * input vertex at input's faces[i] is the corresponding face edge.
 * for convenience, face_edge_offset in the result will be the input's
 * edges_len, so that this conversion can be easily done by the caller.
 */
typedef struct CDT_result {
  int verts_len;
  int edges_len;
  int faces_len;
  int face_edge_offset;
  float (*vert_coords)[2];
  int (*edges)[2];
  int *faces;
  int *faces_start_table;
  int *faces_len_table;
  int *verts_orig;
  int *verts_orig_start_table;
  int *verts_orig_len_table;
  int *edges_orig;
  int *edges_orig_start_table;
  int *edges_orig_len_table;
  int *faces_orig;
  int *faces_orig_start_table;
  int *faces_orig_len_table;
} CDT_result;

/** What triangles and edges of CDT are desired when getting output? */
typedef enum CDT_output_type {
  /** All triangles, outer boundary is convex hull. */
  CDT_FULL,
  /** All triangles fully enclosed by constraint edges or faces. */
  CDT_INSIDE,
  /**  Only point, edge, and face constraints, and their intersections. */
  CDT_CONSTRAINTS,
  /**
   * Like CDT_CONSTRAINTS, but keep enough
   * edges so that any output faces that came from input faces can be made as valid
   * #BMesh faces in Blender: that is,
   * no vertex appears more than once and no isolated holes in faces.
   */
  CDT_CONSTRAINTS_VALID_BMESH
} CDT_output_type;

/**
 * API interface to CDT.
 * This returns a pointer to an allocated CDT_result.
 * When the caller is finished with it, the caller
 * should use #BLI_delaunay_2d_cdt_free() to free it.
 */
CDT_result *BLI_delaunay_2d_cdt_calc(const CDT_input *input, const CDT_output_type output_type);

void BLI_delaunay_2d_cdt_free(CDT_result *result);

#endif /* __BLI_DELAUNAY_2D_H__ */
