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

#ifndef __TRANSFORM_OP_H__
#define __TRANSFORM_OP_H__

struct AnimData;
struct bContext;
struct TransData;
struct TransData2D;
struct TransDataContainer;
struct TransInfo;
struct wmEvent;
struct wmKeyConfig;
struct wmOperator;
struct wmOperatorType;

/* NOTE: these defines are saved in keymap files, do not change values but just add new ones */
enum {
  TFM_MODAL_CANCEL = 1,
  TFM_MODAL_CONFIRM = 2,
  TFM_MODAL_TRANSLATE = 3,
  TFM_MODAL_ROTATE = 4,
  TFM_MODAL_RESIZE = 5,
  TFM_MODAL_SNAP_INV_ON = 6,
  TFM_MODAL_SNAP_INV_OFF = 7,
  TFM_MODAL_SNAP_TOGGLE = 8,
  TFM_MODAL_AXIS_X = 9,
  TFM_MODAL_AXIS_Y = 10,
  TFM_MODAL_AXIS_Z = 11,
  TFM_MODAL_PLANE_X = 12,
  TFM_MODAL_PLANE_Y = 13,
  TFM_MODAL_PLANE_Z = 14,
  TFM_MODAL_CONS_OFF = 15,
  TFM_MODAL_ADD_SNAP = 16,
  TFM_MODAL_REMOVE_SNAP = 17,

  /* 18 and 19 used by numinput, defined in transform.h */

  TFM_MODAL_PROPSIZE_UP = 20,
  TFM_MODAL_PROPSIZE_DOWN = 21,
  TFM_MODAL_AUTOIK_LEN_INC = 22,
  TFM_MODAL_AUTOIK_LEN_DEC = 23,

  TFM_MODAL_EDGESLIDE_UP = 24,
  TFM_MODAL_EDGESLIDE_DOWN = 25,

  /* for analog input, like trackpad */
  TFM_MODAL_PROPSIZE = 26,
  /* node editor insert offset (aka auto-offset) direction toggle */
  TFM_MODAL_INSERTOFS_TOGGLE_DIR = 27,
};

/* transform_op.c */
extern const float VecOne[3];

int transform_modal(bContext *C, struct wmOperator *op, const struct wmEvent *event);
void transform_cancel(bContext *C, struct wmOperator *op);
int transform_exec(bContext *C, struct wmOperator *op);
int transform_invoke(bContext *C, struct wmOperator *op, const struct wmEvent *event);
bool transform_poll_property(const bContext *C, struct wmOperator *op, const PropertyRNA *prop);

/* transform_op_mode.c */
void postInputRotation(TransInfo *t, float values[3]);
void applyAspectRatio(TransInfo *t, float vec[2]);
void headerResize(TransInfo *t, const float vec[3], char *str);
void headerRotation(TransInfo *t, char *str, float final);
void ElementResize(TransInfo *t, TransDataContainer *tc, TransData *td, float mat[3][3]);
void ElementRotation_ex(TransInfo *t,
                        TransDataContainer *tc,
                        TransData *td,
                        const float mat[3][3],
                        const float *center);
void ElementRotation(
    TransInfo *t, TransDataContainer *tc, TransData *td, float mat[3][3], const short around);
void protectedTransBits(short protectflag, float vec[3]);
void protectedSizeBits(short protectflag, float size[3]);
void protectedRotateBits(short protectflag, float eul[3], const float oldeul[3]);
void protectedAxisAngleBits(
    short protectflag, float axis[3], float *angle, float oldAxis[3], float oldAngle);
void protectedQuaternionBits(short protectflag, float quat[4], const float oldquat[4]);
void constraintTransLim(TransInfo *t, TransData *td);
void constraintRotLim(TransInfo *t, TransData *td);
void constraintSizeLim(TransInfo *t, TransData *td);
short getAnimEdit_SnapMode(TransInfo *t);
void doAnimEdit_SnapFrame(
    TransInfo *t, TransData *td, TransData2D *td2d, struct AnimData *adt, short autosnap);

/* transform_op_mode_align.c */
void initAlign(TransInfo *t);

/* transform_op_mode_baketime.c */
void initBakeTime(TransInfo *t);

/* transform_op_mode_bend.c */
extern const char OP_BEND[];
void initBend(TransInfo *t);
void TRANSFORM_OT_bend(struct wmOperatorType *ot);

/* transform_op_mode_boneenvelope.c */
void initBoneEnvelope(TransInfo *t);

/* transform_op_mode_boneroll.c */
void initBoneRoll(TransInfo *t);

/* transform_op_mode_bonesize.c */
void initBoneSize(TransInfo *t);

/* transform_op_mode_curveshrinkfatten.c */
void initCurveShrinkFatten(TransInfo *t);

/* transform_op_mode_edge_bevelweight.c */
extern const char OP_EDGE_BWEIGHT[];
void initBevelWeight(TransInfo *t);
void TRANSFORM_OT_edge_bevelweight(struct wmOperatorType *ot);

/* transform_op_mode_edge_crease.c */
extern const char OP_EDGE_CREASE[];
void initCrease(TransInfo *t);
void TRANSFORM_OT_edge_crease(struct wmOperatorType *ot);

/* transform_op_mode_edge_rotate_normal.c */
extern const char OP_NORMAL_ROTATION[];
void initNormalRotation(TransInfo *t);
void TRANSFORM_OT_rotate_normal(struct wmOperatorType *ot);

/* transform_op_mode_edge_seq_slide.c */
extern const char OP_SEQ_SLIDE[];
void initSeqSlide(TransInfo *t);
void TRANSFORM_OT_seq_slide(struct wmOperatorType *ot);

/* transform_op_mode_edge_slide.c */
extern const char OP_EDGE_SLIDE[];
void drawEdgeSlide(TransInfo *t);
void doEdgeSlide(TransInfo *t, float perc);
void initEdgeSlide_ex(
    TransInfo *t, bool use_double_side, bool use_even, bool flipped, bool use_clamp);
void initEdgeSlide(TransInfo *t);
void TRANSFORM_OT_edge_slide(struct wmOperatorType *ot);

/* transform_op_mode_gpopacity.c */
void initGPOpacity(TransInfo *t);

/* transform_op_mode_gpshrinkfatten.c */
void initGPShrinkFatten(TransInfo *t);

/* transform_op_mode_maskshrinkfatten.c */
void initMaskShrinkFatten(TransInfo *t);

/* transform_op_mode_mirror.c */
extern const char OP_MIRROR[];
void initMirror(TransInfo *t);
void TRANSFORM_OT_mirror(struct wmOperatorType *ot);

/* transform_op_mode_push_pull.c */
extern const char OP_PUSH_PULL[];
void initPushPull(TransInfo *t);
void TRANSFORM_OT_push_pull(struct wmOperatorType *ot);

/* transform_op_mode_resize.c */
extern const char OP_RESIZE[];
void initResize(TransInfo *t);
void TRANSFORM_OT_resize(struct wmOperatorType *ot);

/* transform_op_mode_rotate.c */
extern const char OP_ROTATION[];
void initRotation(TransInfo *t);
void TRANSFORM_OT_rotate(struct wmOperatorType *ot);

/* transform_op_mode_shear.c */
extern const char OP_SHEAR[];
void initShear(TransInfo *t);
void TRANSFORM_OT_shear(struct wmOperatorType *ot);

/* transform_op_mode_shrink_fatten.c */
extern const char OP_SHRINK_FATTEN[];
void initShrinkFatten(TransInfo *t);
void TRANSFORM_OT_shrink_fatten(struct wmOperatorType *ot);

/* transform_op_mode_skin_resize.c */
extern const char OP_SKIN_RESIZE[];
void initSkinResize(TransInfo *t);
void TRANSFORM_OT_skin_resize(struct wmOperatorType *ot);

/* transform_op_mode_tilt.c */
extern const char OP_TILT[];
void initTilt(TransInfo *t);
void TRANSFORM_OT_tilt(struct wmOperatorType *ot);

/* transform_op_mode_timescale.c */
void initTimeScale(TransInfo *t);

/* transform_op_mode_timeslide.c */
void initTimeSlide(TransInfo *t);

/* transform_op_mode_timetranslate.c */
void initTimeTranslate(TransInfo *t);

/* transform_op_mode_tosphere.c */
extern const char OP_TOSPHERE[];
void initToSphere(TransInfo *t);
void TRANSFORM_OT_tosphere(struct wmOperatorType *ot);

/* transform_op_mode_trackball.c */
extern const char OP_TRACKBALL[];
void initTrackball(TransInfo *t);
void TRANSFORM_OT_trackball(struct wmOperatorType *ot);

/* transform_op_mode_translate.c */
extern const char OP_TRANSLATION[];
void initTranslation(TransInfo *t);
void TRANSFORM_OT_translate(struct wmOperatorType *ot);

/* transform_op_mode_vert_slide.c */
extern const char OP_VERT_SLIDE[];
void drawVertSlide(TransInfo *t);
void doVertSlide(TransInfo *t, float perc);
void initVertSlide_ex(TransInfo *t, bool use_even, bool flipped, bool use_clamp);
void initVertSlide(TransInfo *t);
void TRANSFORM_OT_vert_slide(struct wmOperatorType *ot);

/* transform_op_orientation.c */
void TRANSFORM_OT_select_orientation(struct wmOperatorType *ot);
void TRANSFORM_OT_delete_orientation(struct wmOperatorType *ot);
void TRANSFORM_OT_create_orientation(struct wmOperatorType *ot);

#endif
