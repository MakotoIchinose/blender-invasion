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
 * \ingroup edtransform
 */

#ifndef __TRANSFORM_CONVERSIONS_H__
#define __TRANSFORM_CONVERSIONS_H__

struct bKinematicConstraint;
struct bPoseChannel;
struct ListBase;
struct Object;
struct TransDataContainer;
struct TransInfo;

/* when transforming islands */
struct TransIslandData {
  float co[3];
  float axismtx[3][3];
};

/* transform_conversions.c */
void transform_around_single_fallback(TransInfo *t);
int count_set_pose_transflags(Object *ob,
                              const int mode,
                              const short around,
                              bool has_translate_rotate[2]);
bool constraints_list_needinv(TransInfo *t, ListBase *list);

/* transform_conversions_armature.c */
struct bKinematicConstraint *has_targetless_ik(struct bPoseChannel *pchan);
void transform_autoik_update(TransInfo *t, short mode);
void restoreMirrorPoseBones(TransDataContainer *tc);
void restoreBones(TransDataContainer *tc);

void createTransPose(TransInfo *t);
void createTransArmatureVerts(TransInfo *t);

/* transform_conversions_cursor.c */
void createTransCursor_image(TransInfo *t);
void createTransCursor_view3d(TransInfo *t);

/* transform_conversions_mball.c */
void createTransMBallVerts(TransInfo *t);

/* transform_conversions_mesh.c */
void createTransEditVerts(TransInfo *t);
#endif
