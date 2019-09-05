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

struct bContext;
struct bKinematicConstraint;
struct bPoseChannel;
struct BezTriple;
struct ListBase;
struct Object;
struct TransData;
struct TransDataContainer;
struct TransDataCurveHandleFlags;
struct TransInfo;

/* transform_conversions.c */

/* Used by `transform_gizmo_3d.c` */
int count_set_pose_transflags(Object *ob,
                              const int mode,
                              const short around,
                              bool has_translate_rotate[2]);

/* Used by `transform_generics.c` */
void remake_graph_transdata(TransInfo *t, struct ListBase *anim_data);
void flushTransIntFrameActionData(TransInfo *t);
void flushTransGraphData(TransInfo *t);
void flushTransUVs(TransInfo *t);
void flushTransParticles(TransInfo *t);
void flushTransNodes(TransInfo *t);
void flushTransSeq(TransInfo *t);
void flushTransTracking(TransInfo *t);
void flushTransMasking(TransInfo *t);
void flushTransPaintCurve(TransInfo *t);
void restoreMirrorPoseBones(TransDataContainer *tc);
void restoreBones(TransDataContainer *tc);
void transform_autoik_update(TransInfo *t, short mode);
void trans_obdata_in_obmode_update_all(struct TransInfo *t);
void trans_obchild_in_obmode_update_all(struct TransInfo *t);

/* Auto-keyframe applied after transform, returns true if motion paths need to be updated. */
void autokeyframe_object(struct bContext *C,
                         struct Scene *scene,
                         struct ViewLayer *view_layer,
                         struct Object *ob,
                         int tmode);
void autokeyframe_pose(
    struct bContext *C, struct Scene *scene, struct Object *ob, int tmode, short targetless_ik);

/* Test if we need to update motion paths for a given object. */
bool motionpath_need_update_object(struct Scene *scene, struct Object *ob);
bool motionpath_need_update_pose(struct Scene *scene, struct Object *ob);

/* Used by `transform_ops.c` */
int special_transform_moving(TransInfo *t);

/* Used by `transform.c` */
void special_aftertrans_update(struct bContext *C, TransInfo *t);
void sort_trans_data_dist(TransInfo *t);
void createTransData(struct bContext *C, TransInfo *t);
bool clipUVTransform(TransInfo *t, float vec[2], const bool resize);
void clipUVData(TransInfo *t);

/********************* intern **********************/

void transform_around_single_fallback(TransInfo *t);
bool constraints_list_needinv(TransInfo *t, ListBase *list);
void calc_distanceCurveVerts(TransData *head, TransData *tail);
struct TransDataCurveHandleFlags *initTransDataCurveHandles(TransData *td, struct BezTriple *bezt);
bool FrameOnMouseSide(char side, float frame, float cframe);
struct bKinematicConstraint *has_targetless_ik(struct bPoseChannel *pchan);
void clear_trans_object_base_flags(TransInfo *t);

void createTransActionData(bContext *C, TransInfo *t);
void createTransPose(TransInfo *t);
void createTransArmatureVerts(TransInfo *t);
void createTransPaintCurveVerts(bContext *C, TransInfo *t);
void createTransCursor_image(TransInfo *t);
void createTransCursor_view3d(TransInfo *t);
void createTransCurveVerts(TransInfo *t);
void createTransGraphEditData(bContext *C, TransInfo *t);
void createTransGPencil(bContext *C, TransInfo *t);
void createTransLatticeVerts(TransInfo *t);
void createTransMaskingData(bContext *C, TransInfo *t);
void createTransMBallVerts(TransInfo *t);
void createTransEditVerts(TransInfo *t);
void createTransEdge(TransInfo *t);
void createTransUVs(bContext *C, TransInfo *t);
void createTransNlaData(bContext *C, TransInfo *t);
void createTransNodeData(bContext *UNUSED(C), TransInfo *t);
void createTransTrackingData(bContext *C, TransInfo *t);
void cancelTransTracking(TransInfo *t);
void createTransObject(bContext *C, TransInfo *t);
void createTransTexspace(TransInfo *t);
void createTransParticleVerts(bContext *C, TransInfo *t);
void createTransSeqData(bContext *C, TransInfo *t);
#endif
