#include "DRW_engine.h"
#include "DRW_render.h"
#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_math.h"
#include "lanpr_all.h"
#include "DRW_render.h"
#include "BKE_object.h"
#include "DNA_mesh_types.h"
#include "DNA_camera_types.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_framebuffer.h"
#include "DNA_lanpr_types.h"
#include "DNA_meshdata_types.h"
#include "BKE_customdata.h"
#include "DEG_depsgraph_query.h"
#include "GPU_draw.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_uniformbuffer.h"
#include "GPU_viewport.h"
#include "bmesh.h"

#include <math.h>

int lanpr_get_line_bounding_areas(LANPR_RenderBuffer *rb,
                                  LANPR_RenderLine *rl,
                                  int *rowBegin,
                                  int *rowEnd,
                                  int *colBegin,
                                  int *colEnd);
LANPR_BoundingArea *lanpr_get_point_bounding_area(LANPR_RenderBuffer *rb, real x, real y);

#define LANPR_OTHER_RV(rl, rv) ((rv) == (rl)->l ? (rl)->r : (rl)->l)

LANPR_RenderLine *lanpr_get_connected_render_line(LANPR_BoundingArea *ba,
                                                  LANPR_RenderVert *rv,
                                                  LANPR_RenderVert **new_rv)
{
  LinkData *lip;
  LANPR_RenderLine *nrl;
  real cosine;

  for (lip = ba->linked_lines.first; lip; lip = lip->next) {
    nrl = lip->data;

    if ((!(nrl->flags & LANPR_EDGE_FLAG_ALL_TYPE)) || (nrl->flags & LANPR_EDGE_FLAG_CHAIN_PICKED))
      continue;

    // always chain connected lines for now.
    // simplification will take care of the sharp points.
    // if(cosine whatever) continue;

    if (rv != nrl->l && rv != nrl->r) {
      if (nrl->flags & LANPR_EDGE_FLAG_INTERSECTION) {
        if (rv->fbcoord[0] == nrl->l->fbcoord[0] && rv->fbcoord[1] == nrl->l->fbcoord[1]) {
          *new_rv = LANPR_OTHER_RV(nrl, nrl->l);
          return nrl;
        }
        else if (rv->fbcoord[0] == nrl->r->fbcoord[0] && rv->fbcoord[1] == nrl->r->fbcoord[1]) {
          *new_rv = LANPR_OTHER_RV(nrl, nrl->r);
          return nrl;
        }
      }
      continue;
    }

    *new_rv = LANPR_OTHER_RV(nrl, rv);
    return nrl;
  }

  return 0;
}

int lanpr_get_nearby_render_line(LANPR_BoundingArea *ba, LANPR_RenderLine *rl)
{
  // hold on
  return 1;
}

LANPR_RenderLineChain *lanpr_create_render_line_chain(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChain *rlc;
  rlc = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineChain));

  BLI_addtail(&rb->chains, rlc);

  return rlc;
}

LANPR_RenderLineChainItem *lanpr_append_render_line_chain_point(LANPR_RenderBuffer *rb,
                                                                LANPR_RenderLineChain *rlc,
                                                                float x,
                                                                float y,
                                                                float gx,
                                                                float gy,
                                                                float gz,
                                                                float *normal,
                                                                char type,
                                                                int level)
{
  LANPR_RenderLineChainItem *rlci;
  rlci = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineChainItem));

  rlci->pos[0] = x;
  rlci->pos[1] = y;
  rlci->gpos[0] = gx;
  rlci->gpos[1] = gy;
  rlci->gpos[2] = gz;
  copy_v3_v3(rlci->normal, normal);
  rlci->line_type = type & LANPR_EDGE_FLAG_ALL_TYPE;
  rlci->occlusion = level;
  BLI_addtail(&rlc->chain, rlci);

  // printf("a %f,%f %d\n", x, y, level);

  return rlci;
}

LANPR_RenderLineChainItem *lanpr_push_render_line_chain_point(LANPR_RenderBuffer *rb,
                                                              LANPR_RenderLineChain *rlc,
                                                              float x,
                                                              float y,
                                                              float gx,
                                                              float gy,
                                                              float gz,
                                                              float *normal,
                                                              char type,
                                                              int level)
{
  LANPR_RenderLineChainItem *rlci;
  rlci = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineChainItem));

  rlci->pos[0] = x;
  rlci->pos[1] = y;
  rlci->gpos[0] = gx;
  rlci->gpos[1] = gy;
  rlci->gpos[2] = gz;
  copy_v3_v3(rlci->normal, normal);
  rlci->line_type = type & LANPR_EDGE_FLAG_ALL_TYPE;
  rlci->occlusion = level;
  BLI_addhead(&rlc->chain, rlci);

  // printf("data %f,%f %d\n", x, y, level);

  return rlci;
}

// refer to http://karthaus.nl/rdp/ for description
void lanpr_reduce_render_line_chain_recursive(LANPR_RenderLineChain *rlc,
                                              LANPR_RenderLineChainItem *from,
                                              LANPR_RenderLineChainItem *to,
                                              float dist_threshold)
{
  LANPR_RenderLineChainItem *rlci, *next_rlci;
  float l[2], r[2], c[2];
  float max_dist = 0;
  LANPR_RenderLineChainItem *max_rlci = 0;

  // find the max distance item
  for (rlci = (LANPR_RenderLineChainItem *)from->item.next; rlci != to; rlci = next_rlci) {
    next_rlci = (LANPR_RenderLineChainItem *)rlci->item.next;

    if (next_rlci &&
        (next_rlci->occlusion != rlci->occlusion || next_rlci->line_type != rlci->line_type))
      continue;

    float dist = dist_to_line_segment_v2(rlci->pos, from->pos, to->pos);
    if (dist > dist_threshold && dist > max_dist) {
      max_dist = dist;
      max_rlci = rlci;
    }
    // if (dist <= dist_threshold) BLI_remlink(&rlc->chain, (void*)rlci);
  }

  if (!max_rlci) {
    if ((LANPR_RenderLineChainItem *)from->item.next == to)
      return;
    for (rlci = (LANPR_RenderLineChainItem *)from->item.next; rlci != to; rlci = next_rlci) {
      next_rlci = (LANPR_RenderLineChainItem *)rlci->item.next;
      if (next_rlci &&
          (next_rlci->occlusion != rlci->occlusion || next_rlci->line_type != rlci->line_type))
        continue;
      BLI_remlink(&rlc->chain, (void *)rlci);
    }
  }
  else {
    if ((LANPR_RenderLineChainItem *)from->item.next != max_rlci)
      lanpr_reduce_render_line_chain_recursive(rlc, from, max_rlci, dist_threshold);
    if ((LANPR_RenderLineChainItem *)to->item.prev != max_rlci)
      lanpr_reduce_render_line_chain_recursive(rlc, max_rlci, to, dist_threshold);
  }
}

void lanpr_NO_THREAD_chain_feature_lines(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  LANPR_RenderLine *rl;
  LANPR_BoundingArea *ba;
  LANPR_RenderLineSegment *rls;
  real *inv = rb->vp_inverse;
  int last_occlusion;

  for (rl = rb->all_render_lines.first; rl; rl = (LANPR_RenderLine *)rl->item.next) {

    if ((!(rl->flags & LANPR_EDGE_FLAG_ALL_TYPE)) || (rl->flags & LANPR_EDGE_FLAG_CHAIN_PICKED))
      continue;

    rl->flags |= LANPR_EDGE_FLAG_CHAIN_PICKED;

    rlc = lanpr_create_render_line_chain(rb);

    rlc->object_ref = rl->object_ref; // can only be the same object in a chain.

    int r1, r2, c1, c2, row, col;
    LANPR_RenderLine *new_rl = rl;
    LANPR_RenderVert *new_rv;
    float N[3] = {0};

    if (rl->tl) {
      N[0] += rl->tl->gn[0];
      N[1] += rl->tl->gn[1];
      N[2] += rl->tl->gn[2];
    }
    if (rl->tr) {
      N[0] += rl->tr->gn[0];
      N[1] += rl->tr->gn[1];
      N[2] += rl->tr->gn[2];
    }
    if (rl->tl || rl->tr) {
      normalize_v3(N);
    }

    // step 1: grow left
    ba = lanpr_get_point_bounding_area(rb, rl->l->fbcoord[0], rl->l->fbcoord[1]);
    new_rv = rl->l;
    rls = rl->segments.first;
    lanpr_push_render_line_chain_point(rb,
                                       rlc,
                                       new_rv->fbcoord[0],
                                       new_rv->fbcoord[1],
                                       new_rv->gloc[0],
                                       new_rv->gloc[1],
                                       new_rv->gloc[2],
                                       N,
                                       rl->flags,
                                       rls->occlusion);
    while (ba && (new_rl = lanpr_get_connected_render_line(ba, new_rv, &new_rv))) {
      new_rl->flags |= LANPR_EDGE_FLAG_CHAIN_PICKED;

      N[0] = N[1] = N[2] = 0;
      if (new_rl->tl) {
        N[0] += new_rl->tl->gn[0];
        N[1] += new_rl->tl->gn[1];
        N[2] += new_rl->tl->gn[2];
      }
      if (new_rl->tr) {
        N[0] += new_rl->tr->gn[0];
        N[1] += new_rl->tr->gn[1];
        N[2] += new_rl->tr->gn[2];
      }
      if (rl->tl || rl->tr) {
        normalize_v3(N);
      }

      if (new_rv == new_rl->l) {
        for (rls = new_rl->segments.last; rls; rls = (LANPR_RenderLineSegment *)rls->item.prev) {
          double gpos[3], lpos[3];
          lanpr_LinearInterpolate3dv(new_rl->l->fbcoord, new_rl->r->fbcoord, rls->at, lpos);
          lanpr_LinearInterpolate3dv(new_rl->l->gloc, new_rl->r->gloc, rls->at, gpos);
          lanpr_push_render_line_chain_point(rb,
                                             rlc,
                                             lpos[0],
                                             lpos[1],
                                             gpos[0],
                                             gpos[1],
                                             gpos[2],
                                             N,
                                             new_rl->flags,
                                             rls->occlusion);
          last_occlusion = rls->occlusion;
        }
      }
      else if (new_rv == new_rl->r) {
        rls = new_rl->segments.first;
        last_occlusion = rls->occlusion;
        rls = (LANPR_RenderLineSegment *)rls->item.next;
        for (rls; rls; rls = (LANPR_RenderLineSegment *)rls->item.next) {
          double gpos[3], lpos[3];
          lanpr_LinearInterpolate3dv(new_rl->l->fbcoord, new_rl->r->fbcoord, rls->at, lpos);
          lanpr_LinearInterpolate3dv(new_rl->l->gloc, new_rl->r->gloc, rls->at, gpos);
          lanpr_push_render_line_chain_point(rb,
                                             rlc,
                                             lpos[0],
                                             lpos[1],
                                             gpos[0],
                                             gpos[1],
                                             gpos[2],
                                             N,
                                             new_rl->flags,
                                             last_occlusion);
          last_occlusion = rls->occlusion;
        }
        lanpr_push_render_line_chain_point(rb,
                                           rlc,
                                           new_rl->r->fbcoord[0],
                                           new_rl->r->fbcoord[1],
                                           new_rl->r->gloc[0],
                                           new_rl->r->gloc[1],
                                           new_rl->r->gloc[2],
                                           N,
                                           new_rl->flags,
                                           last_occlusion);
      }
      ba = lanpr_get_point_bounding_area(rb, new_rv->fbcoord[0], new_rv->fbcoord[1]);
    }

    // step 2: this line
    rls = rl->segments.first;
    last_occlusion = ((LANPR_RenderLineSegment *)rls)->occlusion;
    for (rls = (LANPR_RenderLineSegment *)rls->item.next; rls;
         rls = (LANPR_RenderLineSegment *)rls->item.next) {
      double gpos[3], lpos[3];
      lanpr_LinearInterpolate3dv(rl->l->fbcoord, rl->r->fbcoord, rls->at, lpos);
      lanpr_LinearInterpolate3dv(rl->l->gloc, rl->r->gloc, rls->at, gpos);
      lanpr_append_render_line_chain_point(
          rb, rlc, lpos[0], lpos[1], gpos[0], gpos[1], gpos[2], N, rl->flags, rls->occlusion);
      last_occlusion = rls->occlusion;
    }
    lanpr_append_render_line_chain_point(rb,
                                         rlc,
                                         rl->r->fbcoord[0],
                                         rl->r->fbcoord[1],
                                         rl->r->gloc[0],
                                         rl->r->gloc[1],
                                         rl->r->gloc[2],
                                         N,
                                         rl->flags,
                                         last_occlusion);

    // step 3: grow right
    ba = lanpr_get_point_bounding_area(rb, rl->r->fbcoord[0], rl->r->fbcoord[1]);
    new_rv = rl->r;
    // below already done in step 2
    // lanpr_push_render_line_chain_point(rb,rlc,new_rv->fbcoord[0],new_rv->fbcoord[1],rl->flags,0);
    while (ba && (new_rl = lanpr_get_connected_render_line(ba, new_rv, &new_rv))) {
      new_rl->flags |= LANPR_EDGE_FLAG_CHAIN_PICKED;

      // fix leading vertex type
      rlci = rlc->chain.last;
      rlci->line_type = new_rl->flags & LANPR_EDGE_FLAG_ALL_TYPE;

      if (new_rv == new_rl->l) {
        rls = new_rl->segments.last;
        last_occlusion = rls->occlusion;
        rlci->occlusion = last_occlusion;
        // rls = (LANPR_RenderLineSegment *)rls->item.prev;
        if (rls)
          last_occlusion = rls->occlusion;
        for (rls = new_rl->segments.last; rls; rls = (LANPR_RenderLineSegment *)rls->item.prev) {
          double gpos[3], lpos[3];
          lanpr_LinearInterpolate3dv(new_rl->l->fbcoord, new_rl->r->fbcoord, rls->at, lpos);
          lanpr_LinearInterpolate3dv(new_rl->l->gloc, new_rl->r->gloc, rls->at, gpos);
          last_occlusion = (LANPR_RenderLineSegment *)rls->item.next ?
                               ((LANPR_RenderLineSegment *)rls->item.next)->occlusion :
                               last_occlusion;
          lanpr_append_render_line_chain_point(rb,
                                               rlc,
                                               lpos[0],
                                               lpos[1],
                                               gpos[0],
                                               gpos[1],
                                               gpos[2],
                                               N,
                                               new_rl->flags,
                                               last_occlusion);
        }
      }
      else if (new_rv == new_rl->r) {
        rls = new_rl->segments.first;
        last_occlusion = rls->occlusion;
        rlci->occlusion = last_occlusion;
        rls = (LANPR_RenderLineSegment *)rls->item.next;
        for (rls; rls; rls = (LANPR_RenderLineSegment *)rls->item.next) {
          double gpos[3], lpos[3];
          lanpr_LinearInterpolate3dv(new_rl->l->fbcoord, new_rl->r->fbcoord, rls->at, lpos);
          lanpr_LinearInterpolate3dv(new_rl->l->gloc, new_rl->r->gloc, rls->at, gpos);
          lanpr_append_render_line_chain_point(rb,
                                               rlc,
                                               lpos[0],
                                               lpos[1],
                                               gpos[0],
                                               gpos[1],
                                               gpos[2],
                                               N,
                                               new_rl->flags,
                                               rls->occlusion);
          last_occlusion = rls->occlusion;
        }
        lanpr_append_render_line_chain_point(rb,
                                             rlc,
                                             new_rl->r->fbcoord[0],
                                             new_rl->r->fbcoord[1],
                                             new_rl->r->gloc[0],
                                             new_rl->r->gloc[1],
                                             new_rl->r->gloc[2],
                                             N,
                                             new_rl->flags,
                                             last_occlusion);
      }
      ba = lanpr_get_point_bounding_area(rb, new_rv->fbcoord[0], new_rv->fbcoord[1]);
    }
  }
}

LANPR_BoundingArea *lanpr_get_point_bounding_area(LANPR_RenderBuffer *rb, real x, real y);
LANPR_BoundingArea *lanpr_get_point_bounding_area_recursive(LANPR_RenderBuffer *rb,
                                                            LANPR_BoundingArea *root,
                                                            LANPR_RenderLineChainItem *rlci)
{
  // if (!root->child) {
  return root;
  //}
  // else {
  //  LANPR_BoundingArea* ch = root->child;
  //  #define IN_BOUND(ba,rlci)\
  //    ba.l<=rlci->pos[0] && ba.r>=rlci->pos[0] && ba.b<=rlci->pos[1] && ba.u>=rlci->pos[1]
  //  if     (IN_BOUND(ch[0],rlci)) return lanpr_get_point_bounding_area_recursive(rb,&ch[0],rlci);
  //  else if(IN_BOUND(ch[1],rlci)) return lanpr_get_point_bounding_area_recursive(rb,&ch[1],rlci);
  //  else if(IN_BOUND(ch[2],rlci)) return lanpr_get_point_bounding_area_recursive(rb,&ch[2],rlci);
  //  else if(IN_BOUND(ch[3],rlci)) return lanpr_get_point_bounding_area_recursive(rb,&ch[3],rlci);
  //  #undef IN_BOUND
  //}
  // return NULL;
}
LANPR_BoundingArea *lanpr_get_end_point_bounding_area(LANPR_RenderBuffer *rb,
                                                      LANPR_RenderLineChainItem *rlci)
{
  LANPR_BoundingArea *root = lanpr_get_point_bounding_area(rb, rlci->pos[0], rlci->pos[1]);
  if (!root)
    return NULL;
  return lanpr_get_point_bounding_area_recursive(rb, root, rlci);
}
// if reduction threshold is even larger than a small bounding area,
// then 1) geometry is simply too dense.
//      2) probably need to add it to root bounding area which has larger surface area then it will
//      cover typical threshold values.
void lanpr_link_point_with_bounding_area_recursive(LANPR_RenderBuffer *rb,
                                                   LANPR_BoundingArea *root,
                                                   LANPR_RenderLineChain *rlc,
                                                   LANPR_RenderLineChainItem *rlci)
{
  // if (!root->child) {
  LANPR_ChainRegisterEntry *cre = list_append_pointer_static_sized(
      &root->linked_chains, &rb->render_data_pool, rlc, sizeof(LANPR_ChainRegisterEntry));
  cre->rlci = rlci;
  if (rlci == rlc->chain.first)
    cre->is_left = 1;
  //}
  // else {
  //  LANPR_BoundingArea* ch = root->child;
  //  #define IN_BOUND(ba,rlci)\
  //    ba.l<=rlci->pos[0] && ba.r>=rlci->pos[0] && ba.b<=rlci->pos[1] && ba.u>=rlci->pos[1]
  //  if     (IN_BOUND(ch[0],rlci))
  //  lanpr_link_point_with_bounding_area_recursive(rb,&ch[0],rlc,rlci); else
  //  if(IN_BOUND(ch[1],rlci)) lanpr_link_point_with_bounding_area_recursive(rb,&ch[1],rlc,rlci);
  //  else if(IN_BOUND(ch[2],rlci))
  //  lanpr_link_point_with_bounding_area_recursive(rb,&ch[2],rlc,rlci); else
  //  if(IN_BOUND(ch[3],rlci)) lanpr_link_point_with_bounding_area_recursive(rb,&ch[3],rlc,rlci);
  //  #undef IN_BOUND
  //}
}

void lanpr_link_chain_with_bounding_areas(LANPR_RenderBuffer *rb, LANPR_RenderLineChain *rlc)
{
  LANPR_RenderLineChainItem *pl = rlc->chain.first;
  LANPR_RenderLineChainItem *pr = rlc->chain.last;
  LANPR_BoundingArea *ba1 = lanpr_get_point_bounding_area(rb, pl->pos[0], pl->pos[1]);
  LANPR_BoundingArea *ba2 = lanpr_get_point_bounding_area(rb, pr->pos[0], pr->pos[1]);

  if (ba1)
    lanpr_link_point_with_bounding_area_recursive(rb, ba1, rlc, pl);
  if (ba2)
    lanpr_link_point_with_bounding_area_recursive(rb, ba2, rlc, pr);
}

void lanpr_split_chains_for_fixed_occlusion(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChain *rlc, *new_rlc;
  LANPR_RenderLineChainItem *rlci, *next_rlci;
  ListBase swap = {0};

  swap.first = rb->chains.first;
  swap.last = rb->chains.last;

  rb->chains.last = rb->chains.first = NULL;

  while (rlc = BLI_pophead(&swap)) {
    rlc->item.next = rlc->item.prev = NULL;
    BLI_addtail(&rb->chains, rlc);
    LANPR_RenderLineChainItem *first_rlci = (LANPR_RenderLineChainItem *)rlc->chain.first;
    int fixed_occ = first_rlci->occlusion;
    for (rlci = (LANPR_RenderLineChainItem *)first_rlci->item.next; rlci; rlci = next_rlci) {
      next_rlci = (LANPR_RenderLineChainItem *)rlci->item.next;
      if (rlci->occlusion != fixed_occ) {
        new_rlc = lanpr_create_render_line_chain(rb);
        new_rlc->chain.first = rlci;
        new_rlc->chain.last = rlc->chain.last;
        rlc->chain.last = rlci->item.prev;
        ((LANPR_RenderLineChainItem *)rlc->chain.last)->item.next = 0;
        rlci->item.prev = 0;

        // end the previous one
        lanpr_append_render_line_chain_point(rb,
                                             rlc,
                                             rlci->pos[0],
                                             rlci->pos[1],
                                             rlci->gpos[0],
                                             rlci->gpos[1],
                                             rlci->gpos[2],
                                             rlci->normal,
                                             rlci->line_type,
                                             fixed_occ);

        rlc = new_rlc;
        fixed_occ = rlci->occlusion;
      }
    }
  }
  for (rlc = rb->chains.first; rlc; rlc = (LANPR_RenderLineChain *)rlc->item.next) {
    lanpr_link_chain_with_bounding_areas(rb, rlc);
  }
}

// note: segment type (crease/material/contour...) is ambiguous after this.
void lanpr_connect_two_chains(LANPR_RenderBuffer *rb,
                              LANPR_RenderLineChain *onto,
                              LANPR_RenderLineChain *sub,
                              int reverse_1,
                              int reverse_2)
{
  if (!reverse_1) {   // L--R L-R
    if (reverse_2) {  // L--R R-L
      BLI_listbase_reverse(&sub->chain);
    }
    ((LANPR_RenderLineChainItem *)onto->chain.last)->item.next = sub->chain.first;
    ((LANPR_RenderLineChainItem *)sub->chain.first)->item.prev = onto->chain.last;
    onto->chain.last = sub->chain.last;
  }
  else {               // L-R L--R
    if (!reverse_2) {  // R-L L--R
      BLI_listbase_reverse(&sub->chain);
    }
    ((LANPR_RenderLineChainItem *)sub->chain.last)->item.next = onto->chain.first;
    ((LANPR_RenderLineChainItem *)onto->chain.first)->item.prev = sub->chain.last;
    onto->chain.first = sub->chain.first;
  }
  //((LANPR_RenderLineChainItem*)sub->chain.first)->occlusion =
  //((LANPR_RenderLineChainItem*)onto->chain.first)->occlusion;
  //((LANPR_RenderLineChainItem*)onto->chain.last)->occlusion =
  //((LANPR_RenderLineChainItem*)onto->chain.first)->occlusion;
  //((LANPR_RenderLineChainItem*)sub->chain.last)->occlusion =
  //((LANPR_RenderLineChainItem*)onto->chain.first)->occlusion;
}

// this only does head-tail connection.
// overlapping / tiny isolated segment / loop reduction not implemented here yet.
void lanpr_connect_chains_image_space(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChain *rlc, *new_rlc;
  LANPR_RenderLineChainItem *rlci, *next_rlci;
  LANPR_BoundingArea *ba;
  LANPR_ChainRegisterEntry *cre, *next_cre, *closest_cre;
  float dist;
  int occlusion;
  ListBase swap = {0};

  swap.first = rb->chains.first;
  swap.last = rb->chains.last;

  rb->chains.last = rb->chains.first = NULL;

  while (rlc = BLI_pophead(&swap)) {
    rlc->item.next = rlc->item.prev = NULL;
    BLI_addtail(&rb->chains, rlc);
    if (rlc->picked)
      continue;

    rlc->picked = 1;

    occlusion = ((LANPR_RenderLineChainItem *)rlc->chain.first)->occlusion;

    rlci = rlc->chain.last;
    while (ba = lanpr_get_end_point_bounding_area(rb, rlci)) {
      dist = 100.0f;
      closest_cre = NULL;
      if (!ba->linked_chains.first)
        break;
      for (cre = ba->linked_chains.first; cre; cre = next_cre) {
        next_cre = (LANPR_ChainRegisterEntry *)cre->item.next;
        if (cre->rlc->object_ref!=rlc) continue;
        if (cre->rlc == rlc ||
            ((LANPR_RenderLineChainItem *)cre->rlc->chain.first)->occlusion != occlusion)
          continue;
        if (cre->rlc->picked) {
          BLI_remlink(&ba->linked_chains, cre);
          continue;
        }
        float new_len = len_v2v2(cre->rlci->pos, rlci->pos);
        if (new_len < dist) {
          closest_cre = cre;
          dist = new_len;
        }
      }
      if (dist < rb->scene->lanpr.chaining_threshold && closest_cre) {
        closest_cre->picked = 1;
        closest_cre->rlc->picked = 1;
        BLI_remlink(&ba->linked_chains, cre);
        if (closest_cre->is_left) {
          lanpr_connect_two_chains(rb, rlc, closest_cre->rlc, 0, 0);
        }
        else {
          lanpr_connect_two_chains(rb, rlc, closest_cre->rlc, 0, 1);
        }
        BLI_remlink(&swap, closest_cre->rlc);
      }
      else {
        break;
      }
      rlci = rlc->chain.last;
    }

    rlci = rlc->chain.first;
    while (ba = lanpr_get_end_point_bounding_area(rb, rlci)) {
      dist = 100.0f;
      closest_cre = NULL;
      if (!ba->linked_chains.first)
        break;
      for (cre = ba->linked_chains.first; cre; cre = next_cre) {
        next_cre = (LANPR_ChainRegisterEntry *)cre->item.next;
        if (cre->rlc == rlc ||
            ((LANPR_RenderLineChainItem *)cre->rlc->chain.first)->occlusion != occlusion)
          continue;
        if (cre->rlc->picked) {
          BLI_remlink(&ba->linked_chains, cre);
          continue;
        }
        float new_len = len_v2v2(cre->rlci->pos, rlci->pos);
        if (new_len < dist) {
          closest_cre = cre;
          dist = new_len;
        }
      }
      if (dist < rb->scene->lanpr.chaining_threshold && closest_cre) {
        closest_cre->picked = 1;
        closest_cre->rlc->picked = 1;
        BLI_remlink(&ba->linked_chains, cre);
        if (closest_cre->is_left) {
          lanpr_connect_two_chains(rb, rlc, closest_cre->rlc, 1, 0);
        }
        else {
          lanpr_connect_two_chains(rb, rlc, closest_cre->rlc, 1, 1);
        }
        BLI_remlink(&swap, closest_cre->rlc);
      }
      else {
        break;
      }
      rlci = rlc->chain.first;
    }
  }
}

int lanpr_count_chain(LANPR_RenderLineChain *rlc)
{
  LANPR_RenderLineChainItem *rlci;
  int Count = 0;
  for (rlci = rlc->chain.first; rlci; rlci = (LANPR_RenderLineChainItem *)rlci->item.next) {
    Count++;
  }
  return Count;
}

float lanpr_compute_chain_length(LANPR_RenderLineChain *rlc, float *lengths, int begin_index)
{
  LANPR_RenderLineChainItem *rlci;
  int i = 0;
  float offset_accum = 0;
  float dist;
  float last_point[2];

  rlci = rlc->chain.first;
  copy_v2_v2(last_point, rlci->pos);
  for (rlci = rlc->chain.first; rlci; rlci = (LANPR_RenderLineChainItem *)rlci->item.next) {
    dist = len_v2v2(rlci->pos, last_point);
    offset_accum += dist;
    lengths[begin_index + i] = offset_accum;
    copy_v2_v2(last_point, rlci->pos);
    i++;
  }
  return offset_accum;
}

int lanpr_get_gpu_line_type(LANPR_RenderLineChainItem *rlci)
{
  switch (rlci->line_type) {
    case LANPR_EDGE_FLAG_CONTOUR:
      return 0;
    case LANPR_EDGE_FLAG_CREASE:
      return 1;
    case LANPR_EDGE_FLAG_MATERIAL:
      return 2;
    case LANPR_EDGE_FLAG_EDGE_MARK:
      return 3;
    case LANPR_EDGE_FLAG_INTERSECTION:
      return 4;
    default:
      return 0;
  }
}

void lanpr_chain_generate_draw_command(LANPR_RenderBuffer *rb)
{
  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  int vert_count = 0;
  int i = 0;
  int arg;
  float total_length;
  float *lengths;
  float length_target[2];

  static GPUVertFormat format = {0};
  static struct {
    uint pos, uvs, normal, type, level;
  } attr_id;
  if (format.attr_len == 0) {
    attr_id.pos = GPU_vertformat_attr_add(&format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
    attr_id.uvs = GPU_vertformat_attr_add(&format, "uvs", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
    attr_id.normal = GPU_vertformat_attr_add(&format, "normal", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
    attr_id.type = GPU_vertformat_attr_add(&format, "type", GPU_COMP_I32, 1, GPU_FETCH_INT);
    attr_id.level = GPU_vertformat_attr_add(&format, "level", GPU_COMP_I32, 1, GPU_FETCH_INT);
  }

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);

  for (rlc = rb->chains.first; rlc; rlc = (LANPR_RenderLineChain *)rlc->item.next) {
    int count = lanpr_count_chain(rlc);
    // printf("seg contains %d verts\n", count);
    vert_count += count;
  }

  GPU_vertbuf_data_alloc(vbo, vert_count + 1);  // serve as end point's adj.

  lengths = MEM_callocN(sizeof(float) * vert_count, "chain lengths");

  GPUIndexBufBuilder elb;
  GPU_indexbuf_init_ex(&elb, GPU_PRIM_LINES_ADJ, vert_count * 4, vert_count);

  for (rlc = rb->chains.first; rlc; rlc = (LANPR_RenderLineChain *)rlc->item.next) {

    total_length = lanpr_compute_chain_length(rlc, lengths, i);

    for (rlci = rlc->chain.first; rlci; rlci = (LANPR_RenderLineChainItem *)rlci->item.next) {

      length_target[0] = lengths[i];
      length_target[1] = total_length - lengths[i];

      GPU_vertbuf_attr_set(vbo, attr_id.pos, i, rlci->pos);
      GPU_vertbuf_attr_set(vbo, attr_id.normal, i, rlci->normal);
      GPU_vertbuf_attr_set(vbo, attr_id.uvs, i, length_target);

      arg = lanpr_get_gpu_line_type(rlci);
      GPU_vertbuf_attr_set(vbo, attr_id.type, i, &arg);

      arg = (int)rlci->occlusion;
      GPU_vertbuf_attr_set(vbo, attr_id.level, i, &arg);

      if (rlci == rlc->chain.last) {
        if (rlci->item.prev == rlc->chain.first) {
          length_target[1] = total_length;
          GPU_vertbuf_attr_set(vbo, attr_id.uvs, i, length_target);
        }
        i++;
        continue;
      }

      if (rlci == rlc->chain.first) {
        if (rlci->item.next == rlc->chain.last)
          GPU_indexbuf_add_line_adj_verts(&elb, vert_count, i, i + 1, vert_count);
        else
          GPU_indexbuf_add_line_adj_verts(&elb, vert_count, i, i + 1, i + 2);
      }
      else {
        if (rlci->item.next == rlc->chain.last)
          GPU_indexbuf_add_line_adj_verts(&elb, i - 1, i, i + 1, vert_count);
        else
          GPU_indexbuf_add_line_adj_verts(&elb, i - 1, i, i + 1, i + 2);
      }

      i++;
    }
  }
  // set end point flag value.
  length_target[0] = 3e30f;
  length_target[1] = 3e30f;
  GPU_vertbuf_attr_set(vbo, attr_id.pos, vert_count, length_target);

  MEM_freeN(lengths);

  if (rb->chain_draw_batch)
    GPU_batch_discard(rb->chain_draw_batch);
  rb->chain_draw_batch = GPU_batch_create_ex(
      GPU_PRIM_LINES_ADJ, vbo, GPU_indexbuf_build(&elb), GPU_USAGE_DYNAMIC | GPU_BATCH_OWNS_VBO);
}
