/*
* ***** BEGIN GPL LICENSE BLOCK *****
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
* The Original Code is Copyright (C) 2012 Blender Foundation.
* All rights reserved.
*
*
* Contributor(s): Blender Foundation, Troy James Sobotka
*
* ***** END GPL LICENSE BLOCK *****
*/
#include <string.h>
#include "BLI_listbase.h" /* list data type */
#include "BLI_utildefines.h" /* utility macros */
#include "BLI_string.h" /* string functions */
#include "BKE_context.h" /* context definition */
#include "BKE_screen.h" /* screen definition */
#include "DNA_space_types.h" /* SDNA definition for the space type */
#include "DNA_screen_types.h" /* SDNA definition for the Screen type */
#include "ED_screen.h" /* screen API */
#include "MEM_guardedalloc.h" /* guarded memory allocation */
#include "WM_api.h" /* Window manager */
#include "WM_types.h" /* Window manager types */
#include "UI_view2d.h"
#include "UI_resources.h"
#include "BIF_gl.h"

#include "GPU_immediate.h"
#include "GPU_immediate_util.h"

/* --- init and allocation/deallocation --- */
static SpaceLink *stripeditor_new(const ScrArea *UNUSED(sa), const Scene *scene)
{
	ARegion *ar;
	SpaceStrip *sstrip;

	sstrip = MEM_callocN(sizeof(SpaceStrip), "initstripeditor");
	sstrip->spacetype= SPACE_STRIP;

	/* header */
	ar= MEM_callocN(sizeof(ARegion), "header for strip editor");

	BLI_addtail(&sstrip->regionbase, ar);
	ar->regiontype= RGN_TYPE_HEADER;
	ar->alignment= RGN_ALIGN_BOTTOM;

	/* main region */
	ar= MEM_callocN(sizeof(ARegion), "main region for strip editor");

	BLI_addtail(&sstrip->regionbase, ar);
	ar->regiontype= RGN_TYPE_WINDOW;

	ar->v2d.tot.xmin = 0.0f;
	ar->v2d.tot.ymin = 0.0f;
	ar->v2d.tot.xmax = scene->r.efra;
	ar->v2d.tot.ymax = 8.0f;

	ar->v2d.cur = ar->v2d.tot;

	ar->v2d.min[0] = 10.0f;
	ar->v2d.min[1] = 0.5f;

	ar->v2d.max[0] = MAXFRAMEF;
	ar->v2d.max[1] = MAXSEQ;
	//
	// ar->v2d.minzoom = 0.01f;
	// ar->v2d.maxzoom = 100.0f;

	// ar->v2d.scroll |= (V2D_SCROLL_BOTTOM | V2D_SCROLL_SCALE_HORIZONTAL);
	// ar->v2d.scroll |= (V2D_SCROLL_LEFT | V2D_SCROLL_SCALE_VERTICAL);
	// ar->v2d.keepzoom = 0;
	// ar->v2d.keeptot = 0;
	// ar->v2d.align = V2D_ALIGN_NO_NEG_Y;

	return (SpaceLink *)sstrip;
}

static SpaceLink *stripeditor_duplicate(SpaceLink *sl)
{
	SpaceStrip *sstrip= MEM_dupallocN(sl);
	/* clear or remove stuff from old */
	return (SpaceLink *)sstrip;
}

/* not spacelink itself */
static void stripeditor_free(SpaceLink *UNUSED(sl))
{
}

/* spacetype; init callback */
static void stripeditor_init(struct wmWindowManager *UNUSED(wm), ScrArea *UNUSED(sa))
{
}

/* --- main area --- */
/* add handlers, stuff you only do once or on area/region changes */
static void stripeditor_main_region_init(wmWindowManager *wm, ARegion *ar)
{
	wmKeyMap *keymap;
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_CUSTOM, ar->winx, ar->winy);

	/* own keymap */
	keymap= WM_keymap_find(wm->defaultconf, "Strip Editor", SPACE_STRIP, 0);
	WM_event_add_keymap_handler_bb(&ar->handlers, keymap, &ar->v2d.mask, &ar->winrct);
	printf("stripeditor_main_region_init\n");
}

/* draw the complete main area */
static void stripeditor_main_region_draw(const bContext *C, ARegion *ar)
{
	/* draw entirely, view changes should be handled here */
	SpaceScript *sscript = (SpaceScript *)CTX_wm_space_strip(C);
	View2D *v2d = &ar->v2d;
	// View2DScrollers *scrollers;

	UI_view2d_view_ortho(v2d);

	unsigned int pos = GWN_vertformat_attr_add(immVertexFormat(), "pos", GWN_COMP_F32, 2, GWN_FETCH_FLOAT);
	immBindBuiltinProgram(GPU_SHADER_2D_UNIFORM_COLOR);
	immUniformThemeColorShade(TH_BACK, +50);

	printf("v2d.cur: xmin: %g, ymax: %g, xmax: %g, ymin: %g\n", v2d->cur.xmin, v2d->cur.ymax, v2d->cur.xmax, v2d->cur.ymin);
	immRectf(pos, v2d->cur.xmin, v2d->cur.ymax, v2d->cur.xmax, v2d->cur.ymin);

	immUnbindProgram();
	/* reset view matrix */
	UI_view2d_view_restore(C);

	/* scrollers */
	// scrollers = UI_view2d_scrollers_calc(C, v2d, V2D_ARG_DUMMY, V2D_ARG_DUMMY, V2D_ARG_DUMMY,
	// V2D_GRID_CLAMP);
	// UI_view2d_scrollers_draw(C, v2d, scrollers);
	// UI_view2d_scrollers_free(scrollers);
	printf("stripeditor_main_area_draw\n");
}

static void stripeditor_main_region_listener(bScreen *UNUSED(sc), ScrArea *sa, ARegion *ar, wmNotifier *wmn)
{
}

/* --- header area --- */
/* add handlers, stuff you only do once or on area/region changes */
static void stripeditor_header_region_init(wmWindowManager *UNUSED(wm), ARegion *ar)
{
	ED_region_header_init(ar);
}

static void stripeditor_header_region_draw(const bContext *C, ARegion *ar)
{
	ED_region_header(C, ar);
}

static void stripeditor_operatortypes(void)
{
}

static void stripeditor_keymap(struct wmKeyConfig *keyconf)
{
}

/* only called once, from space/spacetypes.c */
void ED_spacetype_strip(void)
{
	/* Allocate the SpaceType */
	SpaceType *st= MEM_callocN(sizeof(SpaceType), "spacetype strip");
	ARegionType *art;

	/* Initialize type(spaceid) and name */
	st->spaceid= SPACE_STRIP;
	BLI_strncpy(st->name, "Strip Editor", BKE_ST_MAXNAME);

	/* assign the callback functions for new, init, free and duplicate */
	st->new= stripeditor_new;
	st->free= stripeditor_free;
	st->init= stripeditor_init;
	st->duplicate= stripeditor_duplicate;

	/* assign the callback functions to initialize the operator types and thekeymap */
	st->operatortypes= stripeditor_operatortypes;
	st->keymap= stripeditor_keymap;

	/* regions: main region */
	art= MEM_callocN(sizeof(ARegionType), "spacetype strip region");
	art->regionid = RGN_TYPE_WINDOW;
	art->keymapflag= ED_KEYMAP_UI | ED_KEYMAP_VIEW2D;
	art->init= stripeditor_main_region_init;
	art->draw= stripeditor_main_region_draw;
	// art->listener= stripeditor_main_region_listener;

	BLI_addhead(&st->regiontypes, art);

	/* regions: header */
	art = MEM_callocN(sizeof(ARegionType), "spacetype strip region");
	art->regionid = RGN_TYPE_HEADER;
	art->prefsizey = HEADERY;
	art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_HEADER;

	/* region callbacks: header */
	art->init = stripeditor_header_region_init;
	art->draw = stripeditor_header_region_draw;

	BLI_addhead(&st->regiontypes, art);

	BKE_spacetype_register(st);
}
