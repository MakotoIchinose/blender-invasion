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

#include "DNA_armature_types.h"
#include "DNA_layer_types.h"
#include "DNA_outliner_types.h"
#include "DNA_screen_types.h"
#include "DNA_sequence_types.h"
#include "DNA_space_types.h"

#include "BKE_armature.h"
#include "BKE_context.h"
#include "BKE_layer.h"
#include "BKE_main.h"
#include "BKE_sequencer.h"

#include "DEG_depsgraph.h"

#include "ED_armature.h"
#include "ED_object.h"
#include "ED_outliner.h"

#include "WM_api.h"
#include "WM_types.h"

#include "outliner_intern.h"

void ED_outliner_select_sync_from_object_tag(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  wm->outliner_sync_select_dirty |= WM_OUTLINER_SYNC_SELECT_FROM_OBJECT;
}

void ED_outliner_select_sync_from_edit_bone_tag(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  wm->outliner_sync_select_dirty |= WM_OUTLINER_SYNC_SELECT_FROM_EDIT_BONE;
}

void ED_outliner_select_sync_from_pose_bone_tag(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  wm->outliner_sync_select_dirty |= WM_OUTLINER_SYNC_SELECT_FROM_POSE_BONE;
}

void ED_outliner_select_sync_from_sequence_tag(bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  wm->outliner_sync_select_dirty |= WM_OUTLINER_SYNC_SELECT_FROM_SEQUENCE;
}

bool ED_outliner_select_sync_is_dirty(const bContext *C)
{
  wmWindowManager *wm = CTX_wm_manager(C);
  return wm->outliner_sync_select_dirty != 0;
}

void ED_outliner_select_sync_flag_outliners(const bContext *C)
{
  Main *bmain = CTX_data_main(C);
  wmWindowManager *wm = CTX_wm_manager(C);

  for (bScreen *screen = bmain->screens.first; screen; screen = screen->id.next) {
    for (ScrArea *sa = screen->areabase.first; sa; sa = sa->next) {
      for (SpaceLink *sl = sa->spacedata.first; sl; sl = sl->next) {
        if (sl->spacetype == SPACE_OUTLINER) {
          SpaceOutliner *soutliner = (SpaceOutliner *)sl;

          /* Mark selection state as dirty */
          soutliner->sync_select_dirty |= wm->outliner_sync_select_dirty;
        }
      }
    }
  }

  /* Clear global sync flag */
  wm->outliner_sync_select_dirty = 0;
}

/* Outliner sync select dirty flags are not enough to determine which types to sync,
 * outliner display mode also needs to be considered. This stores the types of data
 * to sync to increase code clarity */
typedef struct SyncSelectTypes {
  bool object;
  bool edit_bone;
  bool pose_bone;
  bool sequence;
} SyncSelectTypes;

/* Current dirty flags and outliner display mode determine which type of syncing should occur.
 * This is to ensure sync flag data is not lost */
static void set_sync_select_types(SpaceOutliner *soops, SyncSelectTypes *sync_types)
{
  sync_types->object = (soops->outlinevis != SO_SEQUENCE) &&
                       (soops->sync_select_dirty & WM_OUTLINER_SYNC_SELECT_FROM_OBJECT);
  sync_types->edit_bone = (soops->outlinevis != SO_SEQUENCE) &&
                          (soops->sync_select_dirty & WM_OUTLINER_SYNC_SELECT_FROM_EDIT_BONE);
  sync_types->pose_bone = (soops->outlinevis != SO_SEQUENCE) &&
                          (soops->sync_select_dirty & WM_OUTLINER_SYNC_SELECT_FROM_POSE_BONE);
  sync_types->sequence = (soops->outlinevis == SO_SEQUENCE) &&
                         (soops->sync_select_dirty & WM_OUTLINER_SYNC_SELECT_FROM_SEQUENCE);
}

static void outliner_select_sync_to_object(ViewLayer *view_layer,
                                           TreeElement *te,
                                           TreeStoreElem *tselem)
{
  Object *ob = (Object *)tselem->id;
  Base *base = (te->directdata) ? (Base *)te->directdata :
                                  BKE_view_layer_base_find(view_layer, ob);

  if (base && (base->flag & BASE_SELECTABLE)) {
    if (tselem->flag & TSE_SELECTED) {
      ED_object_base_select(base, BA_SELECT);
    }
    else {
      ED_object_base_select(base, BA_DESELECT);
    }
  }
}

static void outliner_select_sync_to_edit_bone(TreeElement *te, TreeStoreElem *tselem)
{
  Object *ob = (Object *)tselem->id;
  bArmature *arm = ob->data;
  EditBone *ebone = (EditBone *)te->directdata;

  if (EBONE_SELECTABLE(arm, ebone)) {
    if (tselem->flag & TSE_SELECTED) {
      ebone->flag |= (BONE_SELECTED | BONE_TIPSEL | BONE_ROOTSEL);
    }
    else {
      ebone->flag &= ~(BONE_SELECTED | BONE_TIPSEL | BONE_ROOTSEL);
    }
  }
}

static void outliner_select_sync_to_pose_bone(TreeElement *te, TreeStoreElem *tselem)
{
  Object *ob = (Object *)tselem->id;
  bArmature *arm = ob->data;
  bPoseChannel *pchan = (bPoseChannel *)te->directdata;

  if (PBONE_SELECTABLE(arm, pchan->bone)) {
    if (tselem->flag & TSE_SELECTED) {
      pchan->bone->flag |= BONE_SELECTED;
    }
    else {
      pchan->bone->flag &= ~BONE_SELECTED;
    }
  }

  /* TODO: (Zachman) move these updates */
  DEG_id_tag_update(&arm->id, ID_RECALC_SELECT);
  WM_main_add_notifier(NC_OBJECT | ND_BONE_SELECT, ob);
}

static void outliner_select_sync_to_sequence(Scene *scene, TreeStoreElem *tselem)
{
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

/* Sync selection and active flags from outliner to active view layer, bones, and sequencer */
static void outliner_sync_selection_from_outliner(Scene *scene,
                                                  ViewLayer *view_layer,
                                                  ListBase *tree,
                                                  const short sync_select_dirty)
{

  for (TreeElement *te = tree->first; te; te = te->next) {
    TreeStoreElem *tselem = TREESTORE(te);

    if (tselem->type == 0 && te->idcode == ID_OB) {
      outliner_select_sync_to_object(view_layer, te, tselem);
    }
    else if (tselem->type == TSE_EBONE) {
      outliner_select_sync_to_edit_bone(te, tselem);
    }
    else if (tselem->type == TSE_POSE_CHANNEL) {
      outliner_select_sync_to_pose_bone(te, tselem);
    }
    else if (tselem->type == TSE_SEQUENCE) {
      outliner_select_sync_to_sequence(scene, tselem);
    }

    outliner_sync_selection_from_outliner(scene, view_layer, &te->subtree, sync_select_dirty);
  }
}

static void outliner_select_sync_from_object(ViewLayer *view_layer,
                                             SpaceOutliner *soops,
                                             Object *obact,
                                             TreeElement *te,
                                             TreeStoreElem *tselem)
{
  Object *ob = (Object *)tselem->id;
  Base *base = (te->directdata) ? (Base *)te->directdata :
                                  BKE_view_layer_base_find(view_layer, ob);
  const bool is_selected = (base != NULL) && ((base->flag & BASE_SELECTED) != 0);

  if (base && (ob == obact)) {
    outliner_element_activate(soops, tselem);
  }

  if (is_selected) {
    tselem->flag |= TSE_SELECTED;
  }
  else {
    tselem->flag &= ~TSE_SELECTED;
  }
}

static void outliner_select_sync_from_edit_bone(SpaceOutliner *soops,
                                                EditBone *ebone_active,
                                                TreeElement *te,
                                                TreeStoreElem *tselem)
{
  EditBone *ebone = (EditBone *)te->directdata;

  if (ebone == ebone_active) {
    outliner_element_activate(soops, tselem);
  }

  if (ebone->flag & BONE_SELECTED) {
    tselem->flag |= TSE_SELECTED;
  }
  else {
    tselem->flag &= ~TSE_SELECTED;
  }
}

static void outliner_select_sync_from_pose_bone(SpaceOutliner *soops,
                                                bPoseChannel *pchan_active,
                                                TreeElement *te,
                                                TreeStoreElem *tselem)
{
  bPoseChannel *pchan = (bPoseChannel *)te->directdata;
  Bone *bone = pchan->bone;

  if (pchan == pchan_active) {
    outliner_element_activate(soops, tselem);
  }

  if (bone->flag & BONE_SELECTED) {
    tselem->flag |= TSE_SELECTED;
  }
  else {
    tselem->flag &= ~TSE_SELECTED;
  }
}

static void outliner_select_sync_from_sequence(SpaceOutliner *soops,
                                               Sequence *sequence_active,
                                               TreeStoreElem *tselem)
{
  Sequence *seq = (Sequence *)tselem->id;

  if (seq == sequence_active) {
    outliner_element_activate(soops, tselem);
  }

  if (seq->flag & SELECT) {
    tselem->flag |= TSE_SELECTED;
  }
  else {
    tselem->flag &= ~TSE_SELECTED;
  }
}

/* Sync selection and active flags from active view layer, bones, and sequences to the outliner */
static void outliner_sync_selection_to_outliner(const bContext *C,
                                                ViewLayer *view_layer,
                                                SpaceOutliner *soops,
                                                ListBase *tree,
                                                SyncSelectTypes sync_types)
{
  Scene *scene = CTX_data_scene(C);
  Object *obact = OBACT(view_layer);
  EditBone *ebone_active = CTX_data_active_bone(C);
  bPoseChannel *pchan_active = CTX_data_active_pose_bone(C);
  Sequence *sequence_active = BKE_sequencer_active_get(scene);

  for (TreeElement *te = tree->first; te; te = te->next) {
    TreeStoreElem *tselem = TREESTORE(te);

    if (sync_types.object && tselem->type == 0 && te->idcode == ID_OB) {
      outliner_select_sync_from_object(view_layer, soops, obact, te, tselem);
    }
    else if (sync_types.edit_bone && tselem->type == TSE_EBONE) {
      outliner_select_sync_from_edit_bone(soops, ebone_active, te, tselem);
    }
    else if (sync_types.pose_bone && tselem->type == TSE_POSE_CHANNEL) {
      outliner_select_sync_from_pose_bone(soops, pchan_active, te, tselem);
    }
    else if (sync_types.sequence && tselem->type == TSE_SEQUENCE) {
      outliner_select_sync_from_sequence(soops, sequence_active, tselem);
    }

    /* Sync subtree elements */
    outliner_sync_selection_to_outliner(C, view_layer, soops, &te->subtree, sync_types);
  }
}

/* Set clean outliner and mark other outliners for syncing */
void ED_outliner_select_sync_from_outliner(bContext *C, SpaceOutliner *soops)
{
  /* Don't sync in certain outliner display modes */
  if (ELEM(soops->outlinevis, SO_LIBRARIES, SO_DATA_API, SO_ID_ORPHANS)) {
    return;
  }

  Scene *scene = CTX_data_scene(C);
  ViewLayer *view_layer = CTX_data_view_layer(C);

  outliner_sync_selection_from_outliner(scene, view_layer, &soops->tree, soops->sync_select_dirty);

  ED_outliner_select_sync_from_object_tag(C);
  ED_outliner_select_sync_from_edit_bone_tag(C);
  ED_outliner_select_sync_from_pose_bone_tag(C);
  ED_outliner_select_sync_from_sequence_tag(C);
  ED_outliner_select_sync_flag_outliners(C);
  soops->sync_select_dirty &= ~WM_OUTLINER_SYNC_SELECT_FROM_ALL;

  /* Update editors */
  DEG_id_tag_update(&scene->id, ID_RECALC_SELECT);
  WM_event_add_notifier(C, NC_SCENE | ND_OB_SELECT, scene);
  WM_event_add_notifier(C, NC_SCENE | ND_SEQUENCER | NA_SELECTED, scene);
}

void outliner_sync_selection(const bContext *C, SpaceOutliner *soops)
{
  /* If outliner is dirty sync from the current view layer and clear the dirty flag. */
  if (soops->sync_select_dirty & WM_OUTLINER_SYNC_SELECT_FROM_ALL) {
    ViewLayer *view_layer = CTX_data_view_layer(C);

    SyncSelectTypes sync_types;
    set_sync_select_types(soops, &sync_types);

    outliner_sync_selection_to_outliner(C, view_layer, soops, &soops->tree, sync_types);

    if (sync_types.object) {
      soops->sync_select_dirty &= ~WM_OUTLINER_SYNC_SELECT_FROM_OBJECT;
    }
    if (sync_types.edit_bone) {
      soops->sync_select_dirty &= ~WM_OUTLINER_SYNC_SELECT_FROM_EDIT_BONE;
    }
    if (sync_types.pose_bone) {
      soops->sync_select_dirty &= ~WM_OUTLINER_SYNC_SELECT_FROM_POSE_BONE;
    }
    if (sync_types.sequence) {
      soops->sync_select_dirty &= ~WM_OUTLINER_SYNC_SELECT_FROM_SEQUENCE;
    }
  }
}
