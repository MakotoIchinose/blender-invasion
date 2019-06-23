#include "DNA_gpencil_types.h"
#include "DNA_gpencil_modifier_types.h"

#include "BLI_blenlib.h"
#include "BLI_rand.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "BLI_linklist.h"
#include "BLI_alloca.h"

#include "BKE_gpencil.h"
#include "BKE_gpencil_modifier.h"
#include "BKE_modifier.h"
#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_object.h"
#include "BKE_main.h"
#include "BKE_scene.h"
#include "BKE_layer.h"
#include "BKE_library_query.h"
#include "BKE_collection.h"
#include "BKE_mesh.h"
#include "BKE_mesh_mapping.h"

#include "lanpr_all.h"
#include "lanpr_access.h"

extern LANPR_SharedResource lanpr_share;

static BMVert *split_edge_and_move(BMesh *bm, BMEdge *edge, const float new_pos[3])
{
  // Split edge one time and move the created vert to new_pos
  BMVert *vert;

  vert = bmesh_kernel_split_edge_make_vert(bm, edge->v1, edge, NULL);

  copy_v3_v3(vert->co, new_pos);

  return vert;
}

void lanpr_generate_gpencil_geometry(
    GpencilModifierData *md, Depsgraph *depsgraph, Object *ob, bGPDlayer *gpl, bGPDframe *gpf)
{
  StrokeGpencilModifierData *gpmd = (StrokeGpencilModifierData *)md;
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  LANPR_RenderBuffer *rb = lanpr_share.render_buffer_shared;

  if (gpmd->object == NULL) {
    printf("NULL object!\n");
    return;
  }

  if (rb == NULL) {
    printf("NULL LANPR rb!\n");
    return;
  }

  int color_idx = 0;
  int tot_points = 0;
  short thickness = 1;

  float mat[4][4];

  unit_m4(mat);

  BMesh *bm;

  bm = BKE_mesh_to_bmesh_ex(gpmd->object->data,
                            &(struct BMeshCreateParams){0},
                            &(struct BMeshFromMeshParams){
                                .calc_face_normal = true,
                                .cd_mask_extra = CD_MASK_ORIGINDEX,
                            });

  // Split countour lines at occlution points and deselect occluded segment
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls, *irls;
  for (rl = rb->all_render_lines.first; rl; rl = (LANPR_RenderLine *)rl->item.next) {
    BMEdge *e = BM_edge_at_index_find(bm, rl->edge_idx);
    BMVert *v1 = e->v1;  // Segment goes from v1 to v2
    BMVert *v2 = e->v2;

    BMVert *cur_vert = v1;
    for (rls = rl->segments.first; rls; rls = (LANPR_RenderLineSegment *)rls->item.next) {
      irls = (LANPR_RenderLineSegment *)rls->item.next;

      if (rls->occlusion != 0) {
        BM_elem_flag_disable(cur_vert, BM_ELEM_SELECT);
      }

      if (!irls) {
        break;
      }

      // safety reasons
      CLAMP(rls->at, 0, 1);
      CLAMP(irls->at, 0, 1);

      if (irls->at == 1.0f) {
        if (irls->occlusion != 0) {
          BM_elem_flag_disable(v2, BM_ELEM_SELECT);
        }
        break;
      }

      float split_pos[3];

      interp_v3_v3v3(split_pos, v1->co, v2->co, irls->at);

      cur_vert = split_edge_and_move(bm, e, split_pos);

      e = BM_edge_exists(cur_vert, v2);
    }
  }

  // Chain together strokes
  BMVert *vert;
  BMIter iter;

  BM_ITER_MESH (vert, &iter, bm, BM_VERTS_OF_MESH) {

    // Have we already used this vert?
    // if(!BM_elem_flag_test(vert, BM_ELEM_SELECT)){
    //	continue;
    //}

    BMVert *prepend_vert = NULL;
    BMVert *next_vert = vert;
    // Chain together the C verts and export them as GP strokes (chain in object space)
    BMVert *edge_vert;
    BMEdge *e;
    BMIter iter_e;

    LinkNodePair chain = {NULL, NULL};

    int connected_c_verts;

    while (next_vert != NULL) {

      connected_c_verts = 0;
      vert = next_vert;

      BLI_linklist_append(&chain, vert);

      BM_elem_flag_disable(vert, BM_ELEM_SELECT);

      BM_ITER_ELEM (e, &iter_e, vert, BM_EDGES_OF_VERT) {
        edge_vert = BM_edge_other_vert(e, vert);

        if (BM_elem_flag_test(edge_vert, BM_ELEM_SELECT)) {
          if (connected_c_verts == 0) {
            next_vert = edge_vert;
          }
          else if (connected_c_verts == 1 && prepend_vert == NULL) {
            prepend_vert = edge_vert;
          }
          else {
            printf("C verts not connected in a simple line!\n");
          }
          connected_c_verts++;
        }
      }

      if (connected_c_verts == 0) {
        next_vert = NULL;
      }
    }

    LinkNode *pre_list = chain.list;

    while (prepend_vert != NULL) {

      connected_c_verts = 0;
      vert = prepend_vert;

      BLI_linklist_prepend(&pre_list, vert);

      BM_elem_flag_disable(vert, BM_ELEM_SELECT);

      BM_ITER_ELEM (e, &iter_e, vert, BM_EDGES_OF_VERT) {
        edge_vert = BM_edge_other_vert(e, vert);

        if (BM_elem_flag_test(edge_vert, BM_ELEM_SELECT)) {
          if (connected_c_verts == 0) {
            prepend_vert = edge_vert;
          }
          else {
            printf("C verts not connected in a simple line!\n");
          }
          connected_c_verts++;
        }
      }

      if (connected_c_verts == 0) {
        prepend_vert = NULL;
      }
    }

    tot_points = BLI_linklist_count(pre_list);

    printf("Tot points: %d\n", tot_points);

    if (tot_points <= 1) {
      // Don't draw a stroke, chain too short.
      printf("Chain to short\n");
      continue;
    }

    float *stroke_data = BLI_array_alloca(stroke_data, tot_points * GP_PRIM_DATABUF_SIZE);

    int array_idx = 0;

    for (LinkNode *entry = pre_list; entry; entry = entry->next) {
      vert = entry->link;
      stroke_data[array_idx] = vert->co[0];
      stroke_data[array_idx + 1] = vert->co[1];
      stroke_data[array_idx + 2] = vert->co[2];

      stroke_data[array_idx + 3] = 1.0f;  // thickness
      stroke_data[array_idx + 4] = 1.0f;  // hardness?

      array_idx += 5;
    }

    /* generate stroke */
    bGPDstroke *gps;
    gps = BKE_gpencil_add_stroke(gpf, color_idx, tot_points, thickness);
    BKE_gpencil_stroke_add_points(gps, stroke_data, tot_points, mat);

    BLI_linklist_free(pre_list, NULL);
  }

  BM_mesh_free(bm);
}

void lanpr_generate_gpencil_from_chain(
    GpencilModifierData *md, Depsgraph *depsgraph, Object *ob, bGPDlayer *gpl, bGPDframe *gpf)
{
  StrokeGpencilModifierData *gpmd = (StrokeGpencilModifierData *)md;
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  LANPR_RenderBuffer *rb = lanpr_share.render_buffer_shared;

  if (rb == NULL) {
    printf("NULL LANPR rb!\n");
    return;
  }
  if (scene->lanpr.master_mode != LANPR_MASTER_MODE_SOFTWARE)
    return;

  int color_idx = 0;
  int tot_points = 0;
  short thickness = 1;

  float mat[4][4];

  unit_m4(mat);

  // Split countour lines at occlution points and deselect occluded segment
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls, *irls;

  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  for (rlc = rb->chains.first; rlc; rlc = (LANPR_RenderLineChain *)rlc->item.next) {

    if (!rlc->object_ref)
      continue;  // XXX: intersection lines are lost

    if (ob && ob != rlc->object_ref->id.orig_id)
      continue;

    int array_idx = 0;
    int count = lanpr_count_chain(rlc);
    bGPDstroke *gps = BKE_gpencil_add_stroke(gpf, color_idx, count, thickness);

    float *stroke_data = BLI_array_alloca(stroke_data, count * GP_PRIM_DATABUF_SIZE);

    for (rlci = rlc->chain.first; rlci; rlci = (LANPR_RenderLineChainItem *)rlci->item.next) {
      float opatity = rlci->occlusion ? 0.0f : 1.0f;
      stroke_data[array_idx] = rlci->gpos[0];
      stroke_data[array_idx + 1] = rlci->gpos[1];
      stroke_data[array_idx + 2] = rlci->gpos[2];
      stroke_data[array_idx + 3] = opatity;  // thickness
      stroke_data[array_idx + 4] = opatity;  // hardness?
      array_idx += 5;
    }

    BKE_gpencil_stroke_add_points(gps, stroke_data, count, mat);
  }
}

void lanpr_update_data_for_external(Depsgraph *depsgraph)
{
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  SceneLANPR *lanpr = &scene->lanpr;
  if (lanpr->master_mode != LANPR_MASTER_MODE_SOFTWARE)
    return;
  if (!lanpr_share.render_buffer_shared ||
      lanpr_share.render_buffer_shared->cached_for_frame != scene->r.cfra) {
    lanpr_compute_feature_lines_internal(depsgraph);
  }
}

void lanpr_copy_data(Scene *from, Scene *to)
{
  SceneLANPR *lanpr = &from->lanpr;
  LANPR_RenderBuffer *rb = lanpr_share.render_buffer_shared, *new_rb;
  LANPR_LineLayer *ll, *new_ll;
  LANPR_LineLayerComponent *llc, *new_llc;

  list_handle_empty(&to->lanpr.line_layers);

  for (ll = lanpr->line_layers.first; ll; ll = ll->next) {
    new_ll = MEM_callocN(sizeof(LANPR_LineLayer), "Copied Line Layer");
    memcpy(new_ll, ll, sizeof(LANPR_LineLayer));
    list_handle_empty(&new_ll->components);
    new_ll->next = new_ll->prev = NULL;
    BLI_addtail(&to->lanpr.line_layers, new_ll);
    for (llc = ll->components.first; llc; llc = llc->next) {
      new_llc = MEM_callocN(sizeof(LANPR_LineLayerComponent), "Copied Line Layer Component");
      memcpy(new_llc, llc, sizeof(LANPR_LineLayerComponent));
      new_llc->next = new_llc->prev = NULL;
      BLI_addtail(&new_ll->components, new_llc);
    }
  }

  // render_buffer now only accessible from lanpr_share
}

void lanpr_free_everything(Scene *s)
{
  SceneLANPR *lanpr = &s->lanpr;
  LANPR_LineLayer *ll;
  LANPR_LineLayerComponent *llc;

  while (ll = BLI_pophead(&lanpr->line_layers)) {
    while (llc = BLI_pophead(&ll->components))
      MEM_freeN(llc);
    MEM_freeN(ll);
  }
}
