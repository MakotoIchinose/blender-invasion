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
#include "BLI_math.h"

#include "BKE_text.h"
#include "BKE_gpencil.h"

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

typedef int (svg_get_path_callback)(void* reader, float* fill_rgba, float* stroke_rgba, float* stroke_width);
typedef int (svg_get_node_callback)(void* reader, float *x, float *y);

#define FAC_255_COLOR3(color) ((int)(255*color[0])),((int)(255*color[1])),((int)(255*color[2]))

static void write_paths_from_callback(void* reader, Text* ta, svg_get_path_callback get_path, svg_get_node_callback get_node){
    int status;
    float fill_color[3], stroke_color[3], stroke_width;
    int fill_color_i[3], stroke_color_i[3];
    float x,y;
    char buf[128];
    int first_in;
    while(get_path(reader,fill_color,stroke_color,&stroke_width)){

        /* beginning of a path item */
        BKE_text_write(ta, NULL, "<path d=\"");
        
        first_in = 1;
        while(get_node(reader,&x,&y)){
            sprintf(buf,"%c %f %f\n",first_in?'M':'L',x,y);
            BKE_text_write(ta, NULL, buf);
            first_in = 0;
        }

        CLAMP3(fill_color,0,1);
        CLAMP3(stroke_color,0,1);

        /* end the path */
        sprintf(buf,"\" fill=#%X%X%X; stroke=#%X%X%X; stroke-width=\"%f\" />\n",
            FAC_255_COLOR3(fill_color), FAC_255_COLOR3(stroke_color), stroke_width);
    }
    
}

typedef struct GPencilSVGReader{
    bGPdata* gpd;
    bGPDlayer* layer;
    bGPDframe* frame;
    bGPDstroke* stroke;
    bGPDspoint* point;
    int point_i;
}GPencilSVGReader;

static int svg_gpencil_get_path_callback(GPencilSVGReader* reader, float* fill_color, float* stroke_color, float* stroke_width){
    GPencilSVGReader* sr = (GPencilSVGReader*)reader;
    if(!sr->stroke){
        if(!sr->frame->strokes.first){
            return 0;
        }
        sr->stroke = sr->frame->strokes.first;
    }else{
        sr->stroke = sr->stroke->next;
        if(!sr->stroke){
            return 0;
        }
    }
    *stroke_width = sr->stroke->thickness;
    
    /* TODO: no material access yet */
    zero_v3(fill_color);
    zero_v3(stroke_color);
    return 1;
}

static int svg_gpencil_get_node_callback(GPencilSVGReader* reader, float* x, float* y){
    GPencilSVGReader* sr = (GPencilSVGReader*)reader;
    if(!sr->point){
        if(!sr->stroke->points){
            return 0;
        }
        sr->point = sr->stroke->points;
        sr->point_i = 0;
    }else{
        if(sr->point_i == sr->stroke->totpoints){
            return 0;
        }
    }
    *x = sr->point[sr->point_i].x;
    *y = sr->point[sr->point_i].y;
    sr->point_i++;
    return 1;
}

bool BKE_svg_data_from_gpencil(bGPdata* gpd, Text* ta, bGPDlayer* layer, int frame){
    if(!gpd || !ta || !gpd->layers.first){
        return false;
    }

    /* Init temp reader */
    GPencilSVGReader gsr = {0};
    gsr.gpd = gpd;
    if(layer){
        gsr.layer = layer;
    }else{
        gsr.layer = gpd->layers.first;
    }
    gsr.frame = BKE_gpencil_layer_getframe(gsr.layer,frame,GP_GETFRAME_USE_PREV);

    if(!gsr.frame){
        return false;
    }
    
    BKE_text_free_lines(ta);

    write_svg_head(ta);

    write_paths_from_callback(&gsr, ta, svg_gpencil_get_path_callback,svg_gpencil_get_node_callback);

    write_svg_end(ta);

    return true;
}
