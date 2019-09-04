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
struct BezTriple;
struct ListBase;
struct Object;
struct TransData;
struct TransDataContainer;
struct TransDataCurveHandleFlags;
struct TransInfo;

/* helper struct for gp-frame transforms (only used here) */
typedef struct tGPFtransdata {
  float val;  /* where transdata writes transform */
  int *sdata; /* pointer to gpf->framenum */
} tGPFtransdata;

typedef struct TransDataGraph {
  float unit_scale;
  float offset;
} TransDataGraph;

/* transform_conversions.c */
void transform_around_single_fallback(TransInfo *t);
int count_set_pose_transflags(Object *ob,
                              const int mode,
                              const short around,
                              bool has_translate_rotate[2]);
bool constraints_list_needinv(TransInfo *t, ListBase *list);
void calc_distanceCurveVerts(TransData *head, TransData *tail);
struct TransDataCurveHandleFlags *initTransDataCurveHandles(TransData *td, struct BezTriple *bezt);
bool FrameOnMouseSide(char side, float frame, float cframe);
void clear_trans_object_base_flags(TransInfo *t);

void flushTransIntFrameActionData(TransInfo *t);
void flushTransGraphData(TransInfo *t);
void remake_graph_transdata(TransInfo *t, struct ListBase *anim_data);
void flushTransUVs(TransInfo *t);
void flushTransParticles(TransInfo *t);
bool clipUVTransform(TransInfo *t, float vec[2], const bool resize);
void clipUVData(TransInfo *t);
void flushTransNodes(TransInfo *t);
void flushTransSeq(TransInfo *t);
void flushTransTracking(TransInfo *t);
void flushTransMasking(TransInfo *t);
void flushTransPaintCurve(TransInfo *t);
void restoreMirrorPoseBones(TransDataContainer *tc);
void restoreBones(TransDataContainer *tc);

/* transform_conversions_actiondata.c */
void createTransActionData(bContext *C, TransInfo *t);

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

/* transform_conversions_curve.c */
void createTransCurveVerts(TransInfo *t);

/* transform_conversions_graph.c */
void createTransGraphEditData(bContext *C, TransInfo *t);

/* transform_conversions_lattice.c */
void createTransLatticeVerts(TransInfo *t);

/* transform_conversions_mball.c */
void createTransMBallVerts(TransInfo *t);

/* transform_conversions_mesh.c */
void createTransEditVerts(TransInfo *t);
void createTransEdge(TransInfo *t);
void createTransUVs(bContext *C, TransInfo *t);

/* transform_conversions_nla.c */
void createTransNlaData(bContext *C, TransInfo *t);

/* transform_object.c */
void createTransObject(bContext *C, TransInfo *t);
void createTransTexspace(TransInfo *t);
void trans_obdata_in_obmode_update_all(struct TransInfo *t);
void trans_obchild_in_obmode_update_all(struct TransInfo *t);

/* transform_conversions_particle.c */
void createTransParticleVerts(bContext *C, TransInfo *t);

/* transform_conversions_sequencer.c */
void createTransSeqData(bContext *C, TransInfo *t);
#endif
