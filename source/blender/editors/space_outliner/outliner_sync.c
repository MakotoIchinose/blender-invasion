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
 *
 * The Original Code is Copyright (C) 2004 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup spoutliner
 */

#include <stdio.h>

#include "DNA_layer_types.h"
#include "DNA_outliner_types.h"
#include "DNA_screen_types.h"
#include "DNA_sequence_types.h"
#include "DNA_space_types.h"

#include "BKE_context.h"
#include "BKE_layer.h"
#include "BKE_main.h"
#include "BKE_sequencer.h"

#include "DEG_depsgraph.h"

#include "ED_object.h"
#include "ED_outliner.h"

#include "WM_api.h"
#include "WM_types.h"

#include "outliner_intern.h"

/* Default value for sync selection state */
short sync_select_dirty_flag = SYNC_SELECT_NONE;

static void outliners_mark_dirty(const bContext *C)
{
  Main *bmain = CTX_data_main(C);
  for (bScreen *screen = bmain->screens.first; screen; screen = screen->id.next) {
    for (ScrArea *sa = screen->areabase.first; sa; sa = sa->next) {
      for (SpaceLink *space = sa->spacedata.first; space; space = space->next) {
        if (space->spacetype == SPACE_OUTLINER) {
          SpaceOutliner *soutliner = (SpaceOutliner *)space;

          /* Mark selection state as dirty */
          soutliner->flag |= SO_IS_DIRTY;
        }
      }
    }
  }
}

/* Sync selection and active flags from outliner to active view layer, bones, and sequencer */
static void outliner_sync_selection_from_outliner(bContext *C, ListBase *tree)
{
  Scene *scene = CTX_data_scene(C);
  ViewLayer *view_layer = CTX_data_view_layer(C);

  for (TreeElement *te = tree->first; te; te = te->next) {
    TreeStoreElem *tselem = TREESTORE(te);

    if (tselem->type == 0) {
      if (te->idcode == ID_OB) {
        Object *ob = (Object *)tselem->id;
        Base *base = (te->directdata) ? (Base *)te->directdata :
                                        BKE_view_layer_base_find(view_layer, ob);

        if (base) {
          if (tselem->flag & TSE_ACTIVE) {
            ED_object_base_activate(C, base);
          }

          if (tselem->flag & TSE_SELECTED) {
            ED_object_base_select(base, BA_SELECT);
          }
          else {
            ED_object_base_select(base, BA_DESELECT);
          }
        }
      }
    }
    else if (tselem->type == TSE_SEQUENCE) {
      printf("\t\tSyncing a sequence: %s\n", te->name);

      Sequence *seq = (Sequence *)tselem->id;

      if (tselem->flag & TSE_ACTIVE) {
        BKE_sequencer_active_set(scene, seq);
      }

      if (tselem->flag & TSE_SELECTED) {
        seq->flag |= SELECT;
      }
      else {
        seq->flag &= ~SELECT;
      }
    }

    outliner_sync_selection_from_outliner(C, &te->subtree);
  }
}

/* Sync selection and active flags from active view layer, bones, and sequences to the outliner */
static void outliner_sync_selection_to_outliner(const bContext *C,
                                                ViewLayer *view_layer,
                                                ListBase *tree)
{
  Scene *scene = CTX_data_scene(C);
  Object *obact = OBACT(view_layer);
  Sequence *seq_act = BKE_sequencer_active_get(scene);

  for (TreeElement *te = tree->first; te; te = te->next) {
    TreeStoreElem *tselem = TREESTORE(te);
    tselem->flag &= ~TSE_ACTIVE;

    if (tselem->type == 0) {
      if (te->idcode == ID_OB) {
        Object *ob = (Object *)tselem->id;
        Base *base = (te->directdata) ? (Base *)te->directdata :
                                        BKE_view_layer_base_find(view_layer, ob);
        const bool is_selected = (base != NULL) && ((base->flag & BASE_SELECTED) != 0);

        if (base && (ob == obact)) {
          tselem->flag |= TSE_ACTIVE;
        }

        if (is_selected) {
          tselem->flag |= TSE_SELECTED;
        }
        else {
          tselem->flag &= ~TSE_SELECTED;
        }
      }
    }
    else if (tselem->type == TSE_SEQUENCE) {
      printf("\t\tSyncing a sequence: %s\n", te->name);

      Sequence *seq = (Sequence *)tselem->id;

      if (seq == seq_act) {
        tselem->flag |= TSE_ACTIVE;
      }

      if (seq->flag & SELECT) {
        tselem->flag |= TSE_SELECTED;
      }
      else {
        tselem->flag &= ~TSE_SELECTED;
      }
    }

    outliner_sync_selection_to_outliner(C, view_layer, &te->subtree);
  }
}

/* Set clean outliner and mark other outliners for syncing */
void outliner_select_sync(bContext *C, SpaceOutliner *soops)
{
  Scene *scene = CTX_data_scene(C);

  puts("Outliner select... Mark other outliners as dirty for syncing");
  outliner_sync_selection_from_outliner(C, &soops->tree);

  // outliner_sync_selection_to_sequencer(C, &soops->tree);
  sync_select_dirty_flag = SYNC_SELECT_NONE;

  /* Don't need to mark self as dirty here... */
  outliners_mark_dirty(C);
  soops->flag &= ~SO_IS_DIRTY;

  /* Update editors */
  DEG_id_tag_update(&scene->id, ID_RECALC_SELECT);
  WM_event_add_notifier(C, NC_SCENE | ND_OB_SELECT, scene);
  WM_event_add_notifier(C, NC_SCENE | ND_SEQUENCER | NA_SELECTED, scene);
}

void outliner_sync_selection(const bContext *C, SpaceOutliner *soops)
{
  ViewLayer *view_layer = CTX_data_view_layer(C);

  /* If 3D view selection occurred, mark outliners as dirty */
  if (sync_select_dirty_flag != SYNC_SELECT_NONE) {
    printf("Marking outliners as dirty\n");

    outliners_mark_dirty(C);

    sync_select_dirty_flag = SYNC_SELECT_NONE;
  }

  /* If dirty, sync from view layer */
  if (soops->flag & SO_IS_DIRTY) {
    printf("\tSyncing dirty outliner...\n");

    outliner_sync_selection_to_outliner(C, view_layer, &soops->tree);

    // if (soops->outlinevis == SO_SEQUENCE) {
    //   printf("\tSyncing sequences...\n");
    //   outliner_sync_selection_from_sequencer(C, &soops->tree);
    // }

    soops->flag &= ~SO_IS_DIRTY;
  }
}
