#ifndef __LANPR_ACCESS_H__
#define __LANPR_ACCESS_H__

#include "DNA_gpencil_types.h"
#include "DNA_gpencil_modifier_types.h"

#include "DEG_depsgraph.h"

#include "BKE_gpencil.h"

typedef struct LANPR_RenderLineChain LANPR_RenderLineChain;

/* GPencil */
void lanpr_generate_gpencil_from_chain(Depsgraph *depsgraph,
                                       struct Object *ob,
                                       bGPDlayer *gpl,
                                       bGPDframe *gpf,
                                       int qi_begin,
                                       int qi_end,
                                       int material_nr,
                                       struct Collection *col,
                                       int types);

int ED_lanpr_count_chain(LANPR_RenderLineChain *rlc);
void ED_lanpr_chain_clear_picked_flag(struct LANPR_RenderBuffer *rb);

int ED_lanpr_compute_feature_lines_internal(struct Depsgraph *depsgraph, int instersections_only);

/* LANPR data */
void ED_lanpr_destroy_render_data(struct LANPR_RenderBuffer *rb);
void ED_lanpr_copy_data(struct Scene *from, struct Scene *to);
void ED_lanpr_free_everything(struct Scene *s);

/* State polls */
bool ED_lanpr_dpix_shader_error();
bool ED_lanpr_disable_edge_splits(struct Scene *s);

#endif
