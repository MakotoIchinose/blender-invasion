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
 * The Original Code is Copyright (C) 2011 Blender Foundation.
 * All rights reserved.
 *
 * Contributor(s): Blender Foundation,
 *                 Sergey Sharybin
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef __IMB_MOVIECACHE_H__
#define __IMB_MOVIECACHE_H__

/** \file IMB_moviecache.h
 *  \ingroup imbuf
 *  \author Sergey Sharybin
 */

#include "../memutil/MEM_CacheLimiterC-Api.h"
#include "BLI_utildefines.h"
#include "BLI_ghash.h"

/* Cache system for movie data - now supports storing ImBufs only
 * Supposed to provide unified cache system for movie clips, sequencer and
 * other movie-related areas */

struct ImBuf;

typedef void (*MovieCacheGetKeyDataFP) (void *userkey, int *framenr, int *proxy, int *render_flags);

typedef void  *(*MovieCacheGetPriorityDataFP) (void *userkey);
typedef int(*MovieCacheGetItemPriorityFP) (void *last_userkey, void *priority_data);
typedef void(*MovieCachePriorityDeleterFP) (void *priority_data);

typedef struct MovieCache {
	char name[64];

	struct GHash *hash;
	GHashHashFP hashfp;
	GHashCmpFP cmpfp;
	MovieCacheGetKeyDataFP getdatafp;

	MovieCacheGetPriorityDataFP getprioritydatafp;
	MovieCacheGetItemPriorityFP getitempriorityfp;
	MovieCachePriorityDeleterFP prioritydeleterfp;

	struct BLI_mempool *keys_pool;
	struct BLI_mempool *items_pool;
	struct BLI_mempool *userkeys_pool;

	MEM_CacheLimiterC *limiter;

	int keysize;
	float expensive_min_val;

	void *last_userkey;
	struct MovieCacheKey *last_key;

	int totseg, *points, proxy, render_flags;  /* for visual statistics optimization */
	bool use_limiter_to_free;
	int pad;
} MovieCache;

typedef struct MovieCacheKey {
	struct MovieCache *cache_owner;
	void *userkey;
} MovieCacheKey;

typedef struct MovieCacheItem {
	struct MovieCache *cache_owner;
	struct ImBuf *ibuf;
	MEM_CacheLimiterHandleC *c_handle;
	void *priority_data;
} MovieCacheItem;

void IMB_moviecache_init(void);
void IMB_moviecache_destruct(void);

struct MovieCache *IMB_moviecache_create(const char *name, int keysize, GHashHashFP hashfp, GHashCmpFP cmpfp);
void IMB_moviecache_set_getdata_callback(struct MovieCache *cache, MovieCacheGetKeyDataFP getdatafp);
void IMB_moviecache_set_priority_callback(struct MovieCache *cache, MovieCacheGetPriorityDataFP getprioritydatafp,
                                          MovieCacheGetItemPriorityFP getitempriorityfp,
                                          MovieCachePriorityDeleterFP prioritydeleterfp);

void IMB_moviecache_put(struct MovieCache *cache, void *userkey, struct ImBuf *ibuf);
bool IMB_moviecache_put_if_possible(struct MovieCache *cache, void *userkey, struct ImBuf *ibuf);
struct ImBuf *IMB_moviecache_get(struct MovieCache *cache, void *userkey);
bool IMB_moviecache_has_frame(struct MovieCache *cache, void *userkey);
void IMB_moviecache_free(struct MovieCache *cache);

void IMB_moviecache_cleanup(struct MovieCache *cache,
                            bool (cleanup_check_cb) (struct ImBuf *ibuf, void *userkey, void *userdata),
                            void *userdata);

void IMB_moviecache_get_cache_segments(struct MovieCache *cache, int proxy, int render_flags, int *totseg_r, int **points_r);
size_t IMB_moviecache_get_mem_total(MovieCache *cache);
size_t IMB_moviecache_get_mem_used(MovieCache *cache);

struct MovieCacheIter;
struct MovieCacheIter *IMB_moviecacheIter_new(struct MovieCache *cache);
void IMB_moviecacheIter_free(struct MovieCacheIter *iter);
bool IMB_moviecacheIter_done(struct MovieCacheIter *iter);
void IMB_moviecacheIter_step(struct MovieCacheIter *iter);
struct ImBuf *IMB_moviecacheIter_getImBuf(struct MovieCacheIter *iter);
void *IMB_moviecacheIter_getUserKey(struct MovieCacheIter *iter);

#endif
