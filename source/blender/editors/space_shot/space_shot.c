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

/* --- init and allocation/deallocation --- */
static SpaceLink *shoteditor_new(const bContext *UNUSED(C))
{
	ARegion *ar;
	SpaceShot *sshot;
	sshot = MEM_callocN(sizeof(SpaceShot), "initshoteditor");
	sshot->spacetype= SPACE_SHOT;

	/* header */
	ar= MEM_callocN(sizeof(ARegion), "header for shot editor");
	BLI_addtail(&sshot->regionbase, ar);
	ar->regiontype= RGN_TYPE_HEADER;
	ar->alignment= RGN_ALIGN_BOTTOM;

	/* main area */
	ar= MEM_callocN(sizeof(ARegion), "main area for shot editor");
	BLI_addtail(&sshot->regionbase, ar);
	ar->regiontype= RGN_TYPE_WINDOW;

	/* view settings */
	ar->v2d.scroll |= (V2D_SCROLL_RIGHT);
	ar->v2d.align |= V2D_ALIGN_NO_NEG_X|V2D_ALIGN_NO_NEG_Y; /* align bottom left */
	ar->v2d.keepofs |= V2D_LOCKOFS_X;
	ar->v2d.keepzoom = (V2D_LOCKZOOM_X|V2D_LOCKZOOM_Y|V2D_LIMITZOOM|V2D_KEEPASPECT);
	ar->v2d.keeptot= V2D_KEEPTOT_BOUNDS;
	ar->v2d.minzoom= ar->v2d.maxzoom= 1.0f;
	return (SpaceLink *)sshot;
	printf("shoteditor_new");
}

static SpaceLink *shoteditor_duplicate(SpaceLink *sl)
{
	SpaceShot *sshot= MEM_dupallocN(sl);
	/* clear or remove stuff from old */
	return (SpaceLink *)sshot;
}

/* not spacelink itself */
static void shoteditor_free(SpaceLink *UNUSED(sl))
{
}

/* spacetype; init callback */
static void shoteditor_init(struct wmWindowManager *UNUSED(wm), ScrArea *UNUSED(sa))
{
}

/* --- main area --- */
/* add handlers, stuff you only do once or on area/region changes */
static void shoteditor_main_region_init(wmWindowManager *wm, ARegion *ar)
{
}

/* draw the complete main area */
static void shoteditor_main_region_draw(const bContext *C, ARegion *ar)
{
}

static void shoteditor_main_region_listener(ARegion *ar, wmNotifier *wmn)
{
}

/* --- header area --- */
/* add handlers, stuff you only do once or on area/region changes */
static void shoteditor_header_region_init(wmWindowManager *UNUSED(wm), ARegion *ar)
{
}

static void shoteditor_header_region_draw(const bContext *C, ARegion *ar)
{
}

static void shoteditor_header_region_listener(ARegion *ar, wmNotifier *wmn)
{
}

static void shoteditor_operatortypes(void)
{
}

static void shoteditor_keymap(struct wmKeyConfig *keyconf)
{
}

/* --- main area --- */
/* add handlers, stuff you only do once or on area/region changes */
static void shoteditor_main_area_init(wmWindowManager *wm, ARegion *ar)
{
	wmKeyMap *keymap;
	UI_view2d_region_reinit(&ar->v2d, V2D_COMMONVIEW_CUSTOM, ar->winx, ar->winy);

	/* own keymap */
	keymap= WM_keymap_find(wm->defaultconf, "Shot Editor", SPACE_SHOT, 0);
	WM_event_add_keymap_handler(&ar->handlers, keymap);
}

/* only called once, from space/spacetypes.c */
void ED_spacetype_shot(void)
{
	/* Allocate the SpaceType */
	SpaceType *st= MEM_callocN(sizeof(SpaceType), "spacetype shot");
	ARegionType *art;
	/* Initialize type(spaceid) and name */
	st->spaceid= SPACE_SHOT;
	BLI_strncpy(st->name, "Shot Editor", BKE_ST_MAXNAME);
	/* assign the callback functions for new, init, free and duplicate */
	st->new= shoteditor_new;
	st->free= shoteditor_free;
	st->init= shoteditor_init;
	st->duplicate= shoteditor_duplicate;
	/* assign the callback functions to initialize the operator types and thekeymap */
	st->operatortypes= shoteditor_operatortypes;
	st->keymap= shoteditor_keymap;
	/* regions: main region */
	art= MEM_callocN(sizeof(ARegionType), "spacetype shot region");
	art->regionid = RGN_TYPE_WINDOW;
	art->keymapflag= ED_KEYMAP_UI|ED_KEYMAP_VIEW2D;
	/* region callbacks: main region */
	art->init= shoteditor_main_region_init;
	art->draw= shoteditor_main_region_draw;
	art->listener= shoteditor_main_region_listener;
	BLI_addhead(&st->regiontypes, art);
	/* regions: header */
	art= MEM_callocN(sizeof(ARegionType), "spacetype shot region");
	art->regionid = RGN_TYPE_HEADER;
	art->prefsizey= HEADERY;
	art->keymapflag= ED_KEYMAP_UI|ED_KEYMAP_VIEW2D|ED_KEYMAP_HEADER;
	/* region callbacks: header */
	art->listener= shoteditor_header_region_listener;
	art->init= shoteditor_header_region_init;
	art->draw= shoteditor_header_region_draw;
	BLI_addhead(&st->regiontypes, art);
	BKE_spacetype_register(st);
}

static void shoteditor_main_area_draw(const bContext *C, ARegion *ar)
{
	/* draw entirely, view changes should be handled here */
	SpaceShot *sshot= CTX_wm_space_info(C);
	View2D *v2d= &ar->v2d;
	View2DScrollers *scrollers;

	/* clear and setup matrix */
	UI_ThemeClearColor(TH_BACK);
	glClear(GL_COLOR_BUFFER_BIT);

	/* quick way to avoid drawing if not big enough */
	if(ar->winy < 16)
	return;

	/* set view matrix */
	UI_view2d_totRect_set(v2d, ar->winx-1, ar->winy-1);
	UI_view2d_view_ortho(v2d);

	/* do drawing here */

	/* reset view matrix */
	UI_view2d_totRect_set(v2d, ar->winx-1, ar->winy-1);
	UI_view2d_view_restore(C);

	/* scrollers */
	scrollers= UI_view2d_scrollers_calc(C, v2d, V2D_ARG_DUMMY, V2D_ARG_DUMMY, V2D_ARG_DUMMY,
	V2D_GRID_CLAMP);
	UI_view2d_scrollers_draw(C, v2d, scrollers);
	UI_view2d_scrollers_free(scrollers);
	printf("shoteditor_main_area_draw");
}

/* add handlers, stuff you only do once or on area/region changes */
static void shoteditor_header_area_init(wmWindowManager *UNUSED(wm), ARegion *ar)
{
	ED_region_header_init(ar);
}

static void shoteditor_header_area_draw(const bContext *C, ARegion *ar)
{
	ED_region_header(C, ar);
}
