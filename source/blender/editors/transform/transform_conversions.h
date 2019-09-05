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

/* transform_conversions.c */

/* Used by `transform_gizmo_3d.c` */
int count_set_pose_transflags(Object *ob,
                              const int mode,
                              const short around,
                              bool has_translate_rotate[2]);

/* Used by `transform_generics.c` */
void flushTransIntFrameActionData(TransInfo *t);
void flushTransGraphData(TransInfo *t);
void remake_graph_transdata(TransInfo *t, struct ListBase *anim_data);
void flushTransUVs(TransInfo *t);
void flushTransParticles(TransInfo *t);
void flushTransNodes(TransInfo *t);
void flushTransSeq(TransInfo *t);
void flushTransTracking(TransInfo *t);
void flushTransMasking(TransInfo *t);
void flushTransPaintCurve(TransInfo *t);
void restoreMirrorPoseBones(TransDataContainer *tc);
void restoreBones(TransDataContainer *tc);

/* Used by `transform.c` */
bool clipUVTransform(TransInfo *t, float vec[2], const bool resize);
void clipUVData(TransInfo *t);

/********************* intern **********************/

/* helper struct for gp-frame transforms */
typedef struct tGPFtransdata {
  float val;  /* where transdata writes transform */
  int *sdata; /* pointer to gpf->framenum */
} tGPFtransdata;

typedef struct TransDataGraph {
  float unit_scale;
  float offset;
} TransDataGraph;

typedef struct TransDataTracking {
  int mode, flag;

  /* tracks transformation from main window */
  int area;
  const float *relative, *loc;
  float soffset[2], srelative[2];
  float offset[2];

  float (*smarkers)[2];
  int markersnr;
  MovieTrackingMarker *markers;

  /* marker transformation from curves editor */
  float *prev_pos, scale;
  short coord;

  MovieTrackingTrack *track;
  MovieTrackingPlaneTrack *plane_track;
} TransDataTracking;

enum transDataTracking_Mode {
  transDataTracking_ModeTracks = 0,
  transDataTracking_ModeCurves = 1,
  transDataTracking_ModePlaneTracks = 2,
};

typedef struct TransDataMasking {
  bool is_handle;

  float handle[2], orig_handle[2];
  float vec[3][3];
  struct MaskSplinePoint *point;
  float parent_matrix[3][3];
  float parent_inverse_matrix[3][3];
  char orig_handle_type;

  eMaskWhichHandle which_handle;
} TransDataMasking;

typedef struct TransDataPaintCurve {
  struct PaintCurvePoint *pcp; /* initial curve point */
  char id;
} TransDataPaintCurve;

void transform_around_single_fallback(TransInfo *t);
bool constraints_list_needinv(TransInfo *t, ListBase *list);
void calc_distanceCurveVerts(TransData *head, TransData *tail);
struct TransDataCurveHandleFlags *initTransDataCurveHandles(TransData *td, struct BezTriple *bezt);
bool FrameOnMouseSide(char side, float frame, float cframe);
void clear_trans_object_base_flags(TransInfo *t);

/* transform_conversions_action.c */
void createTransActionData(bContext *C, TransInfo *t);

/* transform_conversions_armature.c */
struct bKinematicConstraint *has_targetless_ik(struct bPoseChannel *pchan);
void transform_autoik_update(TransInfo *t, short mode);
void restoreMirrorPoseBones(TransDataContainer *tc);
void restoreBones(TransDataContainer *tc);

void createTransPose(TransInfo *t);
void createTransArmatureVerts(TransInfo *t);

/* transform_conversions_brush.c */
void createTransPaintCurveVerts(bContext *C, TransInfo *t);

/* transform_conversions_cursor.c */
void createTransCursor_image(TransInfo *t);
void createTransCursor_view3d(TransInfo *t);

/* transform_conversions_curve.c */
void createTransCurveVerts(TransInfo *t);

/* transform_conversions_graph.c */
void createTransGraphEditData(bContext *C, TransInfo *t);

/* transform_conversions_gpencil.c */
void createTransGPencil(bContext *C, TransInfo *t);

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

/* transform_conversions_node.c */
void createTransNodeData(bContext *UNUSED(C), TransInfo *t);

/* transform_conversions_tracking.c */
void createTransTrackingData(bContext *C, TransInfo *t);
void cancelTransTracking(TransInfo *t);

/* transform_conversions_mask.c */
void createTransMaskingData(bContext *C, TransInfo *t);

/* transform_conversions_object.c */
void createTransObject(bContext *C, TransInfo *t);
void createTransTexspace(TransInfo *t);

/* Used by `transform_generics.c` (Should be in `transform_conversion.c`?) */
void trans_obdata_in_obmode_update_all(struct TransInfo *t);
void trans_obchild_in_obmode_update_all(struct TransInfo *t);

/* transform_conversions_particle.c */
void createTransParticleVerts(bContext *C, TransInfo *t);

/* transform_conversions_sequencer.c */
void createTransSeqData(bContext *C, TransInfo *t);
#endif
