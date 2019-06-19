#ifndef __LANPR_ACCESS_H__
#define __LANPR_ACCESS_H__

#include "DNA_gpencil_types.h"
#include "DNA_gpencil_modifier_types.h"

#include "DEG_depsgraph.h"

#include "BKE_gpencil.h"

typedef struct LANPR_RenderLineChain LANPR_RenderLineChain;

void lanpr_generate_gpencil_geometry(
    GpencilModifierData *md, Depsgraph *depsgraph, Object *ob, bGPDlayer *gpl, bGPDframe *gpf);

void lanpr_generate_gpencil_from_chain(
    GpencilModifierData *md, Depsgraph *depsgraph, Object *ob, bGPDlayer *gpl, bGPDframe *gpf);

void lanpr_update_data_for_external(Depsgraph *depsgraph);

int lanpr_count_chain(LANPR_RenderLineChain *rlc);

int lanpr_compute_feature_lines_internal(Depsgraph *depsgraph);

void lanpr_destroy_render_data(struct LANPR_RenderBuffer *rb);

void lanpr_copy_data(Scene *from, Scene *to);

void lanpr_free_everything(Scene *s);

#endif
