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
  /*  Split edge one time and move the created vert to new_pos */
  BMVert *vert;

  vert = bmesh_kernel_split_edge_make_vert(bm, edge->v1, edge, NULL);

  copy_v3_v3(vert->co, new_pos);

  return vert;
}

void lanpr_generate_gpencil_from_chain(Depsgraph *depsgraph,
                                       Object *ob,
                                       bGPDlayer *gpl,
                                       bGPDframe *gpf,
                                       int qi_begin,
                                       int qi_end,
                                       int material_nr)
{
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  LANPR_RenderBuffer *rb = lanpr_share.render_buffer_shared;

  if (rb == NULL) {
    printf("NULL LANPR rb!\n");
    return;
  }
  if (scene->lanpr.master_mode != LANPR_MASTER_MODE_SOFTWARE) {
    return;
  }

  int color_idx = 0;
  int tot_points = 0;
  short thickness = 1;

  float mat[4][4];

  unit_m4(mat);

  /*  Split countour lines at occlution points and deselect occluded segment */
  LANPR_RenderLine *rl;
  LANPR_RenderLineSegment *rls, *irls;

  LANPR_RenderLineChain *rlc;
  LANPR_RenderLineChainItem *rlci;
  for (rlc = rb->chains.first; rlc; rlc = (LANPR_RenderLineChain *)rlc->item.next) {

    if (!rlc->object_ref) {
      continue; /*  XXX: intersection lines are lost */
    }
    if (rlc->level > qi_end || rlc->level < qi_begin) {
      continue;
    }
    if (ob && &ob->id != rlc->object_ref->id.orig_id) {
      continue;
    }

    int array_idx = 0;
    int count = lanpr_count_chain(rlc);
    bGPDstroke *gps = BKE_gpencil_add_stroke(gpf, color_idx, count, thickness);

    float *stroke_data = BLI_array_alloca(stroke_data, count * GP_PRIM_DATABUF_SIZE);

    for (rlci = rlc->chain.first; rlci; rlci = (LANPR_RenderLineChainItem *)rlci->item.next) {
      float opatity = 1.0f; /* rlci->occlusion ? 0.0f : 1.0f; */
      stroke_data[array_idx] = rlci->gpos[0];
      stroke_data[array_idx + 1] = rlci->gpos[1];
      stroke_data[array_idx + 2] = rlci->gpos[2];
      stroke_data[array_idx + 3] = 1;       /*  thickness */
      stroke_data[array_idx + 4] = opatity; /*  hardness? */
      array_idx += 5;
    }

    BKE_gpencil_stroke_add_points(gps, stroke_data, count, mat);
    gps->mat_nr = material_nr;
  }
}

void lanpr_update_data_for_external(Depsgraph *depsgraph)
{
  Scene *scene = DEG_get_evaluated_scene(depsgraph);
  SceneLANPR *lanpr = &scene->lanpr;
  if (lanpr->master_mode != LANPR_MASTER_MODE_SOFTWARE) {
    return;
  }
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

  /*  render_buffer now only accessible from lanpr_share */
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
