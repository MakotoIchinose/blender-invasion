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
 */

/** \file
 * \ingroup bke
 */

#include <stdlib.h>
#include <string.h>

#include "BLI_utildefines.h"
#include "BLI_string.h"
#include "BLI_string_utils.h"
#include "BLI_listbase.h"

#include "BKE_writesvg.h"
#include "BKE_text.h"

#include "DNA_object_types.h"
#include "DNA_gpencil_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_text_types.h"

#include "DEG_depsgraph.h"

#include "MEM_guardedalloc.h"

static void write_svg_head(Text* ta){
    BKE_text_write(ta, NULL, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
    BKE_text_write(ta, NULL, "<svg width=\"10cm\" height=\"10cm\" viewbox=\"0 0 10 10\""
        "xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");
}

static void write_svg_end(Text* ta){
    BKE_text_write(ta, NULL, "</svg>");
}

typedef int (svg_get_node_callback)(void* container, float *x, float *y, float *z);

static void write_path_from_callback(Text* ta, svg_get_node_callback get_node){
    
}

bool BKE_svg_data_from_gpencil(bGPdata* gpd, Text* ta){
    if(!gpd || !ta){
        return false;
    }
    
    BKE_text_free_lines(ta);

    {
        
        // export paths.

    }

}
