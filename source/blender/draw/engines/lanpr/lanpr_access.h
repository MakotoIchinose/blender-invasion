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

bool ED_lanpr_dpix_shader_error(void);
bool ED_lanpr_disable_edge_splits(struct Scene *s);

#endif
