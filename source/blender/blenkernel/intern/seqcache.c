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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Peter Schlaile <peter [at] schlaile [dot] de> 2010
 *
 * Contributor(s): Sergey Sharybin
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/blenkernel/intern/seqcache.c
 *  \ingroup bke
 */

#include <stddef.h>

#include "BLI_sys_types.h"  /* for intptr_t */

#include "MEM_guardedalloc.h"
#include "MEM_CacheLimiterC-Api.h"

#include "DNA_sequence_types.h"
#include "DNA_scene_types.h"

#include "IMB_moviecache.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "BLI_mempool.h"
#include "BLI_threads.h"
#include "BLI_listbase.h"

#include "BKE_sequencer.h"
#include "BKE_scene.h"

static bool BKE_sequencer_cache_recycle_item(Scene *scene, eSeqStripElemIBuf type);

/* Cache utility functions */

static bool seq_cmp_render_data(const SeqRenderData *a, const SeqRenderData *b)
{
	return ((a->preview_render_size != b->preview_render_size) ||
		(a->rectx != b->rectx) ||
		(a->recty != b->recty) ||
		(a->bmain != b->bmain) ||
		(a->scene != b->scene) ||
		(a->motion_blur_shutter != b->motion_blur_shutter) ||
		(a->motion_blur_samples != b->motion_blur_samples) ||
		(a->scene->r.views_format != b->scene->r.views_format) ||
		(a->view_id != b->view_id));
}

static unsigned int seq_hash_render_data(const SeqRenderData *a)
{
	unsigned int rval = a->rectx + a->recty;

	rval ^= a->preview_render_size;
	rval ^= ((intptr_t) a->bmain) << 6;
	rval ^= ((intptr_t) a->scene) << 6;
	rval ^= (int)(a->motion_blur_shutter * 100.0f) << 10;
	rval ^= a->motion_blur_samples << 16;
	rval ^= ((a->scene->r.views_format * 2) + a->view_id) << 24;

	return rval;
}

static unsigned int seqcache_hashhash(const void *key_)
{
	const SeqCacheKey *key = key_;
	unsigned int rval = seq_hash_render_data(&key->context);

	rval ^= *(const unsigned int *) &key->cfra;
	rval += key->type;
	rval ^= ((intptr_t) key->seq) << 6;

	return rval;
}

static bool seqcache_hashcmp(const void *a_, const void *b_)
{
	const SeqCacheKey *a = a_;
	const SeqCacheKey *b = b_;

	return ((a->seq != b->seq) ||
		(a->cfra != b->cfra) ||
		(a->type != b->type) ||
		seq_cmp_render_data(&a->context, &b->context));
}

void BKE_sequencer_cache_create(Scene *scene)
{
	IMB_moviecache_init();
	scene->seq_cache = IMB_moviecache_create("seqcache", sizeof(SeqCacheKey), seqcache_hashhash, seqcache_hashcmp);
	scene->seq_cache->use_limiter_to_free = false;
}

void BKE_sequencer_cache_lock(Scene *scene)
 {
	PrefetchJob *pfjob = scene->pfjob;
	if (pfjob &&scene->ed->cache_flag & SEQ_CACHE_MEMVIEW_ENABLE) {
		BLI_mutex_lock((ThreadMutex *) pfjob->cache_mutex);
	}
}

void BKE_sequencer_cache_unlock(Scene *scene)
{
	PrefetchJob *pfjob = scene->pfjob;
	if (pfjob && scene->ed->cache_flag & SEQ_CACHE_MEMVIEW_ENABLE) {
		BLI_mutex_unlock((ThreadMutex *) pfjob->cache_mutex);
	}
}

static Sequence *BKE_sequencer_cache_get_base_seq(const SeqRenderData *context, ListBase *seqbase, Sequence *seq, float cfra)
{
	Scene *scene = context->scene;
	if (context->special_seq_update) {
		return seq;
	}

	Sequence *seq_arr[MAXSEQ + 1];
	int count = get_shown_sequences(seqbase, cfra, context->chanshown, (Sequence **)&seq_arr);

	if (count == 0) {
		return NULL;
	}

	count--;
	if (scene->ed->cache_flag & SEQ_CACHE_STORE_FINAL_OUT)
	{
		return seq_arr[count];
	}

	eSeqStripElemIBuf type = scene->ed->cache_flag;
	while (count >= 0) {
		if (BKE_sequencer_is_cache_type_created(context, seq_arr[count], cfra, type)) {
			return seq_arr[count];
		}

		if (seq_arr[count]->type & SEQ_TYPE_EFFECT && seq_arr[count]->seq1 &&
			BKE_sequencer_is_cache_type_created(context, seq_arr[count]->seq1, cfra, type)) {
			return seq_arr[count]->seq1;
		}
		if (seq_arr[count]->type & SEQ_TYPE_EFFECT && seq_arr[count]->seq2 &&
			BKE_sequencer_is_cache_type_created(context, seq_arr[count]->seq2, cfra, type)) {
			return seq_arr[count]->seq2;
		}
		if (seq_arr[count]->type & SEQ_TYPE_EFFECT && seq_arr[count]->seq3 &&
			BKE_sequencer_is_cache_type_created(context, seq_arr[count]->seq3, cfra, type)) {
			return seq_arr[count]->seq3;
		}

		if (seq_arr[count]->type & SEQ_TYPE_META) {
			int offset;
			seqbase = BKE_sequence_seqbase_get(seq_arr[count], &offset);

			if (seqbase && !BLI_listbase_is_empty(seqbase)) {
				return BKE_sequencer_cache_get_base_seq(context, seqbase, seq, cfra);
			}
		}
		count--;
	}

	return NULL;
}

static eSeqStripElemIBuf BKE_sequencer_cache_get_base_cache_type(const SeqRenderData *context, Sequence *seq, float cfra)
{
	Scene *scene = context->scene;
	eSeqStripElemIBuf type = 0;

	if (!context->special_seq_update) {
		seq = BKE_sequencer_cache_get_base_seq(context, scene->ed->seqbasep, seq, cfra);
	}

	if (seq) {
		if (scene->ed->cache_flag & SEQ_CACHE_STORE_RAW &&
			BKE_sequencer_is_cache_type_created(context, seq, cfra, SEQ_CACHE_STORE_RAW))
		{
			type = SEQ_CACHE_STORE_RAW;
		}

		if (scene->ed->cache_flag & SEQ_CACHE_STORE_PREPROCESSED &&
			BKE_sequencer_is_cache_type_created(context, seq, cfra, SEQ_CACHE_STORE_PREPROCESSED))
		{
			type = SEQ_CACHE_STORE_PREPROCESSED;
		}

		if (scene->ed->cache_flag & SEQ_CACHE_STORE_COMPOSITE &&
			!context->special_seq_update &&
			BKE_sequencer_is_cache_type_created(context, seq, cfra, SEQ_CACHE_STORE_COMPOSITE))
		{
			type = SEQ_CACHE_STORE_COMPOSITE;
		}

		if (scene->ed->cache_flag & SEQ_CACHE_STORE_FINAL_OUT &&
			!context->special_seq_update)
		{
			type = SEQ_CACHE_STORE_FINAL_OUT;
		}
	}
	return type;
}

/* Cache stats & lookup functions */

struct ImBuf *BKE_sequencer_cache_get(const SeqRenderData *context, Sequence *seq, float cfra, eSeqStripElemIBuf type)
{
	Scene *scene = context->scene;

	if (context->is_prefetch_render) {
		if (!context->scene->pfjob) {
			return NULL;
		}

		scene = context->scene->pfjob->scene;
		seq = BKE_sequencer_prefetch_find_seq_in_scene(seq, scene);
		context = &scene->pfjob->context;
	}

	if (!scene->seq_cache) {
		BKE_sequencer_cache_create(scene);
		return NULL;
	}

	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	ImBuf *imb = NULL;

	if (cache && seq) {
		SeqCacheKey key;

		key.seq = seq;
		key.context = *context;
		key.cfra = cfra - seq->start;
		key.type = type;

		imb = IMB_moviecache_get(cache, &key);
	}
	BKE_sequencer_cache_unlock(scene);

	return imb;
}

void BKE_sequencer_cache_put(const SeqRenderData *context, Sequence *seq, float cfra, eSeqStripElemIBuf type, ImBuf *i, float cost)
{
	Scene *scene = context->scene;
	PrefetchJob *pfjob = scene->pfjob;

	if (i == NULL || context->skip_cache || context->is_proxy_render || !seq) {
		return;
	}

	if (context->is_prefetch_render) {
		scene = pfjob->scene;
		context = &pfjob->context;
		seq = BKE_sequencer_prefetch_find_seq_in_scene(seq, scene);
	}

	/* Prevent reinserting, it breaks cache key linking */
	if (BKE_sequencer_cache_get(context, seq, cfra, type)) {
		return;
	}

	BKE_sequencer_cache_lock(scene);
	if (!scene->seq_cache) {
		BKE_sequencer_cache_create(scene);
	}
	MovieCache *cache = scene->seq_cache;
	SeqCacheKey key;

	key.seq = seq;
	key.context = *context;
	key.nfra = cfra;
	key.cfra = cfra - seq->start;
	key.type = type;
	key.cost = cost;
	key.cache_owner = cache;

	key.link_prev = NULL;
	key.link_next = NULL;
	key.free_after_render = true;
	key.is_base_frame = false;

	/* Item stored for later use */
	if (scene->ed->cache_flag & type) {
		key.free_after_render = false;
		key.link_prev = cache->last_key;

		/* Last render in stack - should be freed first, then follow links */
		if (type == BKE_sequencer_cache_get_base_cache_type(context, seq, cfra) &&
			seq == BKE_sequencer_cache_get_base_seq(context, scene->ed->seqbasep, seq, cfra))
		{
			key.is_base_frame = true;
		}
	}

	if (BKE_sequencer_cache_recycle_item(scene, type)){
		MovieCacheKey *temp_last_key = cache->last_key;

		IMB_moviecache_put(cache, &key, i);

		/* Restore pointer to previous item as this one will be freed when stack is rendered */
		if (key.free_after_render) {
			cache->last_key = temp_last_key;
		}

		/* Set last_key's reference to this key so we can look up chain backwards
		* Item is already put in cache, so cache->last_key points to current key;
		*/
		if (scene->ed->cache_flag & type && temp_last_key) {
			((SeqCacheKey *)temp_last_key->userkey)->link_next = cache->last_key;
		}

		/* Reset linking */
		if (key.is_base_frame) {
			cache->last_key = NULL;
		}
	}
	else {
		/* prefetch overrun */
		pfjob->stop = true;
	}
	BKE_sequencer_cache_unlock(scene);
}

int BKE_sequencer_cache_count_cheap_frames(Scene *scene, float min_cost)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return 0;
	}

	int num_frames = 0;
	GHashIterator gh_iter;
	GHASH_ITER(gh_iter, cache->hash) {
		MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);
		SeqCacheKey *skey = key->userkey;

		if (skey->is_base_frame && skey->cost > min_cost)
		{
			num_frames++;
		}
	}

	BKE_sequencer_cache_unlock(scene);
	return num_frames;
}

static int BKE_sequencer_cache_count_base_frames(Scene *scene, float min_cost)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return 0;
	}

	int num_frames = 0;
	GHashIterator gh_iter;
	GHASH_ITER(gh_iter, cache->hash) {
		MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);
		SeqCacheKey *skey = key->userkey;

		if (skey->is_base_frame)
		{
			num_frames++;
		}
	}

	BKE_sequencer_cache_unlock(scene);
	return num_frames;
}

static int compare_float(const void *a, const void *b)
{
	float fa = *(const float*)a;
	float fb = *(const float*)b;
	return (fa > fb) - (fa < fb);
}

float BKE_sequencer_set_new_min_expensive_val(Scene *scene, int num_cheap_frames)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return 0;
	}

	BKE_sequencer_cache_unlock(scene);
	int num_items = BKE_sequencer_cache_count_base_frames(scene, 0);
	BKE_sequencer_cache_lock(scene);
	int i = 0;
	float *arr = MEM_callocN(sizeof(float) * num_items, "Array of frame costs");
	float min_expensive_val;

	GHashIterator gh_iter;
	GHASH_ITER(gh_iter, cache->hash) {
		struct MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);
		struct SeqCacheKey *skey = key->userkey;
		if (skey->is_base_frame) {
			arr[i] = skey->cost;
			i++;
		}
	}

	qsort(arr, num_items, sizeof(float), compare_float);
	num_cheap_frames = num_cheap_frames + 1;
	if (num_cheap_frames > num_items - 1) {
		num_cheap_frames = num_items - 1;
	}
	min_expensive_val = arr[num_cheap_frames];
	MEM_freeN(arr);

	float diff = 0;
	if (min_expensive_val > 1) {
		diff = min_expensive_val - cache->expensive_min_val;
		cache->expensive_min_val = min_expensive_val;
	}

	BKE_sequencer_cache_unlock(scene);
	return diff;
}

/* Cache free functions */

static void moviecache_keyfree(void *val)
{
	MovieCacheKey *key = val;

	BLI_mempool_free(key->cache_owner->userkeys_pool, key->userkey);
	BLI_mempool_free(key->cache_owner->keys_pool, key);
}

static void moviecache_valfree(void *val)
{
	MovieCacheItem *item = (MovieCacheItem *)val;
	MovieCache *cache = item->cache_owner;

	if (item->ibuf) {
		MEM_CacheLimiter_unmanage(item->c_handle);
		IMB_freeImBuf(item->ibuf);
	}

	if (item->priority_data && cache->prioritydeleterfp) {
		cache->prioritydeleterfp(item->priority_data);
	}

	BLI_mempool_free(item->cache_owner->items_pool, item);
}

void BKE_sequencer_cache_destruct(Scene *scene)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache || !scene->ed) {
		BKE_sequencer_cache_unlock(scene);
		return;
	}

	IMB_moviecache_free(cache);
	scene->seq_cache = NULL;
	BKE_sequencer_cache_unlock(scene);
}

void BKE_sequencer_cache_cleanup(Scene *scene)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return;
	}

	GHashIterator gh_iter;
	BLI_ghashIterator_init(&gh_iter, cache->hash);
	while (!BLI_ghashIterator_done(&gh_iter)) {
		MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);

		BLI_ghashIterator_step(&gh_iter);
		BLI_ghash_remove(cache->hash, key, moviecache_keyfree, moviecache_valfree);
	}
	cache->expensive_min_val = 1;
	cache->last_key = NULL;
	BKE_sequencer_cache_unlock(scene);
}

/* Rendering: [start] k1 ---> k2 ---> k3 [is_base_frame]
 *
 * k1->link_prev = NULL;	k1->link_next = 2;
 * k2->link_prev = 1;		k2->link_next = 3;
 * k3->link_prev = 2;		k3->link_next = NULL;
 *
 * rendered image is freed first. If k3 was being destroyed,
 * we have to set k2->is_base_frame
 */
static void BKE_sequencer_cache_relink_keys(MovieCacheKey *link_next, MovieCacheKey *link_prev)
{
	if (link_next) {
		((SeqCacheKey *)link_next->userkey)->link_prev = link_prev;
	}
	if (link_prev) {
		((SeqCacheKey *)link_prev->userkey)->link_next = link_next;
		if (link_next == NULL) {
			((SeqCacheKey *)link_prev->userkey)->is_base_frame = true;
		}
	}
}

void BKE_sequencer_cache_cleanup_sequence(Scene *scene, Sequence *seq)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return;
	}

	GHashIterator gh_iter;
	BLI_ghashIterator_init(&gh_iter, cache->hash);
	while (!BLI_ghashIterator_done(&gh_iter)) {
		MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);
		SeqCacheKey *skey = key->userkey;
		BLI_ghashIterator_step(&gh_iter);

		if (skey->seq == seq) {
			/* Relink keys, so we don't end up with orphaned keys */
			if (skey->link_next || skey->link_prev) {
				BKE_sequencer_cache_relink_keys(skey->link_next, skey->link_prev);
			}

			BLI_ghash_remove(cache->hash, key, moviecache_keyfree, moviecache_valfree);
		}
	}
	cache->last_key = NULL;
	BKE_sequencer_cache_unlock(scene);
}

void BKE_sequencer_cache_free_temp_cache(Scene *scene)
{
	BKE_sequencer_cache_lock(scene);
	MovieCache *cache = scene->seq_cache;
	if (!cache) {
		BKE_sequencer_cache_unlock(scene);
		return;
	}

	GHashIterator gh_iter;
	BLI_ghashIterator_init(&gh_iter, cache->hash);
	while (!BLI_ghashIterator_done(&gh_iter)) {
		MovieCacheKey *key = BLI_ghashIterator_getKey(&gh_iter);
		SeqCacheKey *skey = key->userkey;
		BLI_ghashIterator_step(&gh_iter);

		if (skey->free_after_render) {
			BLI_ghash_remove(cache->hash, key, moviecache_keyfree, moviecache_valfree);
		}
	}
	BKE_sequencer_cache_unlock(scene);
}

static void BKE_sequencer_cache_recycle_linked(Scene *scene, MovieCacheKey *base)
{
	MovieCache *cache = scene->seq_cache;
	int i = 0;
	while (base) {
		MovieCacheKey *prev = ((SeqCacheKey *)base->userkey)->link_prev;
		BLI_ghash_remove(cache->hash, base, moviecache_keyfree, moviecache_valfree);
		base = prev;
		i++;
	}
}

/* Free only keys with is_base_frame set
 * Sources(other types) for a frame must be freed all at once
 */
static bool BKE_sequencer_cache_recycle_item(Scene *scene, eSeqStripElemIBuf type)
{
	MovieCache *cache = scene->seq_cache;

	if (!cache) {
		return false;
	}

	Editing *ed = scene->ed;
	int cfra = scene->r.cfra;
	int cfra_freed = -1;
	MovieCacheKey *finalkey = NULL;

	while (IMB_moviecache_get_mem_used(cache) > IMB_moviecache_get_mem_total(cache)) {
		/*leftmost key*/
		MovieCacheKey *lkey = NULL;
		/*rightmost key*/
		MovieCacheKey *rkey = NULL;
		finalkey = NULL;
		MovieCacheKey *key = NULL;

		GHashIterator gh_iter;
		BLI_ghashIterator_init(&gh_iter, cache->hash);
		int total_count = 0;
		int cheap_count = 0;

		while (!BLI_ghashIterator_done(&gh_iter)) {
			key = BLI_ghashIterator_getKey(&gh_iter);
			SeqCacheKey *skey = key->userkey;
			BLI_ghashIterator_step(&gh_iter);

			if (!skey->is_base_frame) {
				continue;
			}
			total_count++;
			if (skey->cost <= cache->expensive_min_val) {
				cheap_count++;
				if (lkey) {
					if (skey->nfra < ((SeqCacheKey *)lkey->userkey)->nfra) {
						lkey = key;
					}
				}
				else {
					lkey = key;
				}
				if (rkey) {
					if (skey->nfra > ((SeqCacheKey *)rkey->userkey)->nfra) {
						rkey = key;
					}
				}
				else {
					rkey = key;
				}
			}
		}

		if (rkey || lkey) {
			if (ed->cache_flag & SEQ_CACHE_PREFETCH_ENABLE) {
				/* lkey must be left to playhead to be removed else remove rkey */
				if (lkey && ((SeqCacheKey *)lkey->userkey)->nfra < cfra) {
					finalkey = lkey;
				}
				else {
					if (rkey) {
						finalkey = rkey;
					}
				}
			}
			else {
				/* Without prefetch remove most distant frame from playhead */
				int l_diff = 0;
				int r_diff = 0;
				if (lkey) {
					l_diff = cfra - ((SeqCacheKey *)lkey->userkey)->nfra;
				}
				if (rkey) {
					r_diff = ((SeqCacheKey *)rkey->userkey)->nfra - cfra;
				}
				if (l_diff > r_diff) {
					if (lkey) {
						finalkey = lkey;
					}
				}
				else {
					if (rkey) {
						finalkey = rkey;
					}
				}
			}

			if (((SeqCacheKey *)finalkey->userkey)->nfra > cfra_freed) {
				cfra_freed = ((SeqCacheKey *)finalkey->userkey)->nfra;
			}

			if ((cfra_freed && cfra_freed != (scene->pfjob->cfra + scene->pfjob->progress)) ||
				!scene->pfjob->running)
			{
				BKE_sequencer_cache_recycle_linked(scene, finalkey);
			}
			else {
				/* Prefetch overrun */
				scene->pfjob->stop = true;
				return false;
			}
		}
		else { /* No frames to free were found */

			/* Calculate prefetch_store_ratio if not using prefetching
			 * We need at least 10 samples for reasonable granularity of 0.1
			 */
			float cost_diff = 0;
			if (total_count > 10) {
				float prefetch_store_ratio = (float)cheap_count / (float)total_count;
				if (prefetch_store_ratio < ed->prefetch_store_ratio) {
					int min_frames = total_count * ed->prefetch_store_ratio;
					/* We are already locked. */
					BKE_sequencer_cache_unlock(scene);
					cost_diff = BKE_sequencer_set_new_min_expensive_val(scene, min_frames);
					BKE_sequencer_cache_lock(scene);
				}
			}

			/* Frame cost has been altered, so this is OK */
			if (cost_diff > 0) {
				continue;
			}
			else {
				/* Not enough memory to continue operation */
				scene->pfjob->stop = true;
				return false;
			}
		}
	}
	return true;
}
