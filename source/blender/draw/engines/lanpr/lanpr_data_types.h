#ifndef __LANPR_DATA_TYPES_H__
#define __LANPR_DATA_TYPES_H__
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
 * Copyright 2016, Blender Foundation.
 *
 * Contributor(s): Yiming Wu
 *
 */

/** \file
 * \ingroup draw
 */

#include "lanpr_util.h"

typedef struct LANPR_TextureSample {
  struct LANPR_TextureSample *next, *prev;
  int X, Y;
  float Z; /*  for future usage */
} LANPR_TextureSample;

typedef struct LANPR_LineStripPoint {
  struct LANPR_LineStripPoint *next, *prev;
  float P[3];
} LANPR_LineStripPoint;

typedef struct LANPR_LineStrip {
  struct LANPR_LineStrip *next, *prev;
  ListBase points;
  int point_count;
  float total_length;
} LANPR_LineStrip;

typedef struct LANPR_RenderTriangle {
  struct LANPR_RenderTriangle *next, *prev;
  struct LANPR_RenderVert *v[3];
  struct LANPR_RenderLine *rl[3];
  real gn[3];
  real gc[3];
  /*  struct BMFace *F; */
  short material_id;
  ListBase intersecting_verts;
  char cull_status;
  struct LANPR_RenderTriangle *testing; /*  Should Be tRT** testing[NumOfThreads] */
} LANPR_RenderTriangle;

typedef struct LANPR_RenderTriangleThread {
  struct LANPR_RenderTriangle base;
  struct LANPR_RenderLine *testing[127]; /*  max thread support; */
} LANPR_RenderTriangleThread;

typedef struct LANPR_RenderElementLinkNode {
  struct LANPR_RenderElementLinkNode *next, *prev;
  void *pointer;
  int element_count;
  void *object_ref;
  char additional;
} LANPR_RenderElementLinkNode;

typedef struct LANPR_RenderLineSegment {
  struct LANPR_RenderLineSegment *next, *prev;
  real at;                  /*  at==0: left    at==1: right  (this is in 2D projected space) */
  real at_global;           /*  to reconstruct 3d stroke     (XXX: implement global space?) */
  u8bit occlusion;          /*  after "at" point */
  short material_mask_mark; /*  e.g. to determine lines beind a glass window material. */
} LANPR_RenderLineSegment;

typedef struct LANPR_RenderVert {
  struct LANPR_RenderVert *next, *prev;
  real gloc[4];
  real fbcoord[4];
  int fbcoordi[2];
  struct BMVert *v; /*  Used As r When Intersecting */
  struct LANPR_RenderLine *intersecting_line;
  struct LANPR_RenderLine *intersecting_line2;
  struct LANPR_RenderTriangle *intersecting_with; /*    positive 1         Negative 0 */
  /*  tnsRenderTriangle* IntersectingOnFace;       /*          <|               |> */
  char positive;  /*                  l---->|----->r	l---->|----->r */
  char edge_used; /*                       <|		          |> */
} LANPR_RenderVert;

#define LANPR_EDGE_FLAG_EDGE_MARK 1
#define LANPR_EDGE_FLAG_CONTOUR 2
#define LANPR_EDGE_FLAG_CREASE 4
#define LANPR_EDGE_FLAG_MATERIAL 8
#define LANPR_EDGE_FLAG_INTERSECTION 16
#define LANPR_EDGE_FLAG_FLOATING 32 /*  floating edge, unimplemented yet */
#define LANPR_EDGE_FLAG_CHAIN_PICKED 64

#define LANPR_EDGE_FLAG_ALL_TYPE 0x3f

typedef struct LANPR_RenderLine {
  struct LANPR_RenderLine *next, *prev;
  struct LANPR_RenderVert *l, *r;
  struct LANPR_RenderTriangle *tl, *tr;
  ListBase segments;
  /*  tnsEdge*       Edge;/* should be edge material */
  /*  tnsRenderTriangle* testing;/* Should Be tRT** testing[NumOfThreads]	struct Materil */
  /*  *MaterialRef; */
  char min_occ;
  char flags; /*  also for line type determination on chainning */

  /*  still need this entry because culled lines will not add to object reln node */
  struct Object *object_ref;

  int edge_idx; /*  for gpencil stroke modifier */
} LANPR_RenderLine;

typedef struct LANPR_RenderLineChain {
  struct LANPR_RenderLineChain *next, *prev;
  ListBase chain;
  /*  int         SegmentCount;  /*  we count before draw cmd. */
  float length; /*  calculated before draw cmd. */
  char picked;  /*  used when re-connecting and gp stroke generation */
  char level;
  int  type; /* Chain now only contains one type of segments */
  struct Object *object_ref;
} LANPR_RenderLineChain;

typedef struct LANPR_RenderLineChainItem {
  struct LANPR_RenderLineChainItem *next, *prev;
  float pos[3];  /*  need z value for fading */
  float gpos[3]; /*  for restore position to 3d space */
  float normal[3];
  char line_type; /*       style of [1]       style of [2] */
  char occlusion; /*  [1]--------------->[2]---------------->[3]--.... */
} LANPR_RenderLineChainItem;

typedef struct LANPR_ChainRegisterEntry {
  struct LANPR_ChainRegisterEntry *next, *prev;
  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  char picked;
  char is_left; /*  left/right mark. Because we revert list in chaining and we need the flag. */
} LANPR_ChainRegisterEntry;

#endif