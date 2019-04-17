/*# ***** BEGIN GPL LICENSE BLOCK *****
*
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
* The Original Code is Copyright (C) 2015, Blender Foundation
* All rights reserved.
* ***** END GPL LICENSE BLOCK ******/


#include <igl/avg_edge_length.h>
#include <igl/barycenter.h>
#include <igl/comb_cross_field.h>
#include <igl/comb_frame_field.h>
#include <igl/compute_frame_field_bisectors.h>
#include <igl/cross_field_missmatch.h>
#include <igl/cut_mesh_from_singularities.h>
#include <igl/find_cross_field_singularities.h>
#include <igl/local_basis.h>
#include <igl/rotate_vectors.h>
#include <igl/copyleft/comiso/miq.h>
#include <igl/copyleft/comiso/nrosy.h>
#include <igl/PI.h>
#include <sstream>

#include "igl_capi.h"


void igl_miq(float (*verts)[3] , int (*tris)[3], int num_verts, int num_tris, float (*uv_tris)[3][2],
            double gradient_size, double iter, double stiffness, bool direct_round, bool *hard_edges)
{

    using namespace Eigen;

    // Input mesh
    MatrixXd V;
    MatrixXi F;

    // Face barycenters
    MatrixXd B;

    // Cross field
    MatrixXd X1,X2;

    // Bisector field
    MatrixXd BIS1, BIS2;

    // Combed bisector
    MatrixXd BIS1_combed, BIS2_combed;

    // Per-corner, integer mismatches
    Matrix<int, Dynamic, 3> MMatch;

    // Field singularities
    Matrix<int, Dynamic, 1> isSingularity, singularityIndex;

    // Per corner seams
    Matrix<int, Dynamic, 3> Seams;

    // Combed field
    MatrixXd X1_combed, X2_combed;

    // Global parametrization (with seams)
    MatrixXd UV_seams;
    MatrixXi FUV_seams;

    // Global parametrization
    MatrixXd UV;
    MatrixXi FUV;

    V.resize(num_verts, 3);
    F.resize(num_tris, 3);
    UV.resize(num_tris, 3*2);
    FUV.resize(num_tris, 3);

    for (int i = 0; i < num_verts; i++)
    {
     V(i, 0) = verts[i][0];
     V(i, 1) = verts[i][1];
     V(i, 2) = verts[i][2];
    }

    for (int i = 0; i < num_tris; i++)
    {
     F(i, 0) = tris[i][0];
     F(i, 1) = tris[i][1];
     F(i, 2) = tris[i][2];
    }

    //double gradient_size = 50;
    //double iter = 0;
    //double stiffness = 5.0;
    //bool direct_round = 0;

    // Compute face barycenters
    igl::barycenter(V, F, B);

    // Compute scale for visualizing fields
   // global_scale =  .5*igl::avg_edge_length(V, F);

    // Contrain one face
    VectorXi b(1);
    b << 0;
    MatrixXd bc(1, 3);
    bc << 1, 0, 0;

    // Create a smooth 4-RoSy field
    VectorXd S;
    igl::copyleft::comiso::nrosy(V, F, b, bc, VectorXi(), VectorXd(), MatrixXd(), 4, 0.5, X1, S);

    // Find the orthogonal vector
    MatrixXd B1, B2, B3;
    igl::local_basis(V, F, B1, B2, B3);
    X2 = igl::rotate_vectors(X1, VectorXd::Constant(1, igl::PI / 2), B1, B2);

    // Always work on the bisectors, it is more general
    igl::compute_frame_field_bisectors(V, F, X1, X2, BIS1, BIS2);

    // Comb the field, implicitly defining the seams
    igl::comb_cross_field(V, F, BIS1, BIS2, BIS1_combed, BIS2_combed);

    // Find the integer mismatches
    igl::cross_field_missmatch(V, F, BIS1_combed, BIS2_combed, true, MMatch);

    // Find the singularities
    igl::find_cross_field_singularities(V, F, MMatch, isSingularity, singularityIndex);

    // Cut the mesh, duplicating all vertices on the seams
    igl::cut_mesh_from_singularities(V, F, MMatch, Seams);

    // Comb the frame-field accordingly
    igl::comb_frame_field(V, F, X1, X2, BIS1_combed, BIS2_combed, X1_combed, X2_combed);


    //hard verts (optionally mark too)
    std::vector<int> hverts;

    //hard edges
    std::vector<std::vector<int>> hedges;
    for (int i = 0; i < num_tris; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (hard_edges[i * 3 + j])
            {
                 std::vector<int> hedge;
                 hedge.push_back(i);
                 hedge.push_back(j);
                 hedges.push_back(hedge);
            }
        }
    }

    // Global parametrization
    igl::copyleft::comiso::miq(V,
           F,
           X1_combed,
           X2_combed,
           MMatch,
           isSingularity,
           Seams,
           UV,
           FUV,
           gradient_size,
           stiffness,
           direct_round,
           iter,
           5,
           true, true, hverts, hedges);

    // Global parametrization (with seams, only for demonstration)
    /*igl::copyleft::comiso::miq(V,
         F,
         X1_combed,
         X2_combed,
         MMatch,
         isSingularity,
         Seams,
         UV_seams,
         FUV_seams,
         gradient_size,
         stiffness,
         direct_round,
         iter,
         5,
         false);*/

    //output UVs (triangular parametrization)

    for (int i = 0; i < num_tris; i++)
    {
      //uv face with 3 verts, 2 coords each
      uv_tris[i][0][0] = UV(FUV(i, 0),0);
      uv_tris[i][0][1] = UV(FUV(i, 0),1);

      uv_tris[i][1][0] = UV(FUV(i, 1),0);
      uv_tris[i][1][1] = UV(FUV(i, 1),1);

      uv_tris[i][2][0] = UV(FUV(i, 2),0);
      uv_tris[i][2][1] = UV(FUV(i, 2),1);
    }
}

