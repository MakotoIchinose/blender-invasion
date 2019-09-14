# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# ***** END GPL LICENSE BLOCK *****

set(INC
  ../include
  ../../blenfont
  ../../blenkernel
  ../../blenlib
  ../../blentranslation
  ../../bmesh
  ../../editors/transform
  ../../gpu
  ../../ikplugin
  ../../makesdna
  ../../makesrna
  ../../render/extern/include
  ../../depsgraph
  ../../windowmanager
  ../../../../intern/glew-mx
  ../../../../intern/guardedalloc
)

set(INC_SYS
  ${GLEW_INCLUDE_PATH}
)

set(SRC
  transform.c
  transform_constraints.c
  transform_generics.c
  transform_gizmo/transform_gizmo_2d.c
  transform_gizmo/transform_gizmo_3d.c
  transform_gizmo/transform_gizmo_extrude_3d.c
  transform_input.c
  transform_orientations.c

  transform_convert/transform_convert.c
  transform_convert/transform_convert_action.c
  transform_convert/transform_convert_armature.c
  transform_convert/transform_convert_cursor.c
  transform_convert/transform_convert_curve.c
  transform_convert/transform_convert_gpencil.c
  transform_convert/transform_convert_graph.c
  transform_convert/transform_convert_lattice.c
  transform_convert/transform_convert_mask.c
  transform_convert/transform_convert_mball.c
  transform_convert/transform_convert_mesh.c
  transform_convert/transform_convert_nla.c
  transform_convert/transform_convert_node.c
  transform_convert/transform_convert_object.c
  transform_convert/transform_convert_paintcurve.c
  transform_convert/transform_convert_particle.c
  transform_convert/transform_convert_sculpt.c
  transform_convert/transform_convert_sequencer.c
  transform_convert/transform_convert_tracking.c

  transform_ops/transform_op.c
  transform_ops/transform_op_mode.c
  transform_ops/transform_op_mode_align.c
  transform_ops/transform_op_mode_baketime.c
  transform_ops/transform_op_mode_bend.c
  transform_ops/transform_op_mode_boneenvelope.c
  transform_ops/transform_op_mode_boneroll.c
  transform_ops/transform_op_mode_bonesize.c
  transform_ops/transform_op_mode_curveshrinkfatten.c
  transform_ops/transform_op_mode_edge_bevelweight.c
  transform_ops/transform_op_mode_edge_crease.c
  transform_ops/transform_op_mode_edge_rotate_normal.c
  transform_ops/transform_op_mode_edge_seq_slide.c
  transform_ops/transform_op_mode_edge_slide.c
  transform_ops/transform_op_mode_gpopacity.c
  transform_ops/transform_op_mode_gpshrinkfatten.c
  transform_ops/transform_op_mode_maskshrinkfatten.c
  transform_ops/transform_op_mode_mirror.c
  transform_ops/transform_op_mode_push_pull.c
  transform_ops/transform_op_mode_resize.c
  transform_ops/transform_op_mode_rotate.c
  transform_ops/transform_op_mode_shear.c
  transform_ops/transform_op_mode_shrink_fatten.c
  transform_ops/transform_op_mode_skin_resize.c
  transform_ops/transform_op_mode_tilt.c
  transform_ops/transform_op_mode_timescale.c
  transform_ops/transform_op_mode_timeslide.c
  transform_ops/transform_op_mode_timetranslate.c
  transform_ops/transform_op_mode_tosphere.c
  transform_ops/transform_op_mode_trackball.c
  transform_ops/transform_op_mode_translate.c
  transform_ops/transform_op_mode_vert_slide.c
  transform_ops/transform_op_orientation.c

  transform_snap/transform_snap.c
  transform_snap/transform_snap_object.c

  transform.h
  transform_convert.h
  transform_op.h
)

set(LIB
  bf_blenfont
  bf_blenkernel
  bf_blenlib
  bf_bmesh
  bf_editor_mask
  bf_gpu
)

if(WITH_INTERNATIONAL)
  add_definitions(-DWITH_INTERNATIONAL)
endif()

add_definitions(${GL_DEFINITIONS})

blender_add_lib(bf_editor_transform "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
