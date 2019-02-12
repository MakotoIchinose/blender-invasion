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
 * Copyright (C) 2018 Blender Foundation.
 */

/** \file \ingroup DNA
 *
 * Utilities for stand-alone makesdna.c and Blender to share.
 */

#include <string.h>

#include "MEM_guardedalloc.h"

#include "BLI_sys_types.h"
#include "BLI_utildefines.h"
#include "BLI_assert.h"
#include "BLI_ghash.h"

#include "BLI_memarena.h"

#include "dna_utils.h"

/* -------------------------------------------------------------------- */
/** \name Struct Member Evaluation
 * \{ */

/**
 * Parses the `[n1][n2]...` on the end of an array name
 * and returns the number of array elements `n1 * n2 ...`.
 */
int DNA_elem_array_size(const char *str)
{
	int result = 1;
	int current = 0;
	while (true) {
		char c = *str++;
		switch (c) {
			case '\0':
				return result;
			case '[':
				current = 0;
				break;
			case ']':
				result *= current;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				current = current * 10 + (c - '0');
				break;
			default:
				break;
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Struct Member Manipulation
 * \{ */

static bool is_identifier(const char c)
{
	return ((c >= 'a' && c <= 'z') ||
	        (c >= 'A' && c <= 'Z') ||
	        (c >= '0' && c <= '9') ||
	        (c == '_'));
}

uint DNA_elem_id_offset_start(const char *elem_dna)
{
	uint elem_dna_offset = 0;
	while (!is_identifier(elem_dna[elem_dna_offset])) {
		elem_dna_offset++;
	}
	return elem_dna_offset;
}

uint DNA_elem_id_offset_end(const char *elem_dna)
{
	uint elem_dna_offset = 0;
	while (is_identifier(elem_dna[elem_dna_offset])) {
		elem_dna_offset++;
	}
	return elem_dna_offset;
}

/**
 * Check if 'var' matches '*var[3]' for eg,
 * return true if it does, with start/end offsets.
 */
bool DNA_elem_id_match(
        const char *elem_search, const int elem_search_len,
        const char *elem_dna,
        uint *r_elem_dna_offset)
{
	BLI_assert(strlen(elem_search) == elem_search_len);
	const uint elem_dna_offset = DNA_elem_id_offset_start(elem_dna);
	const char *elem_dna_trim = elem_dna + elem_dna_offset;
	if (strncmp(elem_search, elem_dna_trim, elem_search_len) == 0) {
		const char c = elem_dna_trim[elem_search_len];
		if (c == '\0' || !is_identifier(c)) {
			*r_elem_dna_offset = elem_dna_offset;
			return true;
		}
	}
	return false;
}

/**
 * Return a renamed dna name, allocated from \a mem_arena.
 */
char *DNA_elem_id_rename(
        struct MemArena *mem_arena,
        const char *elem_src, const int elem_src_len,
        const char *elem_dst, const int elem_dst_len,
        const char *elem_dna_src, const int elem_dna_src_len,
        const uint elem_dna_offset_start)
{
	BLI_assert(strlen(elem_src) == elem_src_len);
	BLI_assert(strlen(elem_dst) == elem_dst_len);
	BLI_assert(strlen(elem_dna_src) == elem_dna_src_len);
	BLI_assert(DNA_elem_id_offset_start(elem_dna_src) == elem_dna_offset_start);
	UNUSED_VARS_NDEBUG(elem_src);

	const int elem_final_len = (elem_dna_src_len - elem_src_len) + elem_dst_len;
	char *elem_dna_dst = BLI_memarena_alloc(mem_arena, elem_final_len + 1);
	uint i = 0;
	if (elem_dna_offset_start != 0) {
		memcpy(elem_dna_dst, elem_dna_src, elem_dna_offset_start);
		i = elem_dna_offset_start;
	}
	memcpy(&elem_dna_dst[i], elem_dst, elem_dst_len + 1);
	i += elem_dst_len;
	uint elem_dna_offset_end = elem_dna_offset_start + elem_src_len;
	if (elem_dna_src[elem_dna_offset_end] != '\0') {
		const int elem_dna_tail_len = (elem_dna_src_len - elem_dna_offset_end);
		memcpy(&elem_dna_dst[i], &elem_dna_src[elem_dna_offset_end], elem_dna_tail_len + 1);
		i += elem_dna_tail_len;
	}
	BLI_assert((strlen(elem_dna_dst) == elem_final_len) && (i == elem_final_len));
	return elem_dna_dst;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Versioning
 * \{ */

/* Use for #DNA_VERSIONING_INCLUDE */
#define DNA_MAKESDNA

static void dna_softupdate_ghash_add_pair(GHash *gh, const char *a, const char *b, void *value)
{
	const char **str_pair = MEM_mallocN(sizeof(char *) * 2, __func__);
	str_pair[0] = a;
	str_pair[1] = b;
	BLI_ghash_insert(gh, str_pair, value);
}

static uint strhash_pair_p(const void *ptr)
{
	const char * const *pair = ptr;
	return (BLI_ghashutil_strhash_p(pair[0]) ^
	        BLI_ghashutil_strhash_p(pair[1]));
}

static bool strhash_pair_cmp(const void *a, const void *b)
{
	const char * const *pair_a = a;
	const char * const *pair_b = b;
	return (STREQ(pair_a[0], pair_b[0]) &&
	        STREQ(pair_a[1], pair_b[1])) ? false : true;
}

void DNA_softupdate_maps(
        enum eDNAVersionDir version_dir,
        GHash **r_struct_map, GHash **r_elem_map)
{

	if (r_struct_map != NULL) {
		const char *data[][2] = {
#define DNA_STRUCT_REPLACE(old, new) {#old, #new},
#define DNA_STRUCT_MEMBER_REPLACE(struct_name, old, new)
#include DNA_VERSIONING_DEFINES
#undef DNA_STRUCT_REPLACE
#undef DNA_STRUCT_MEMBER_REPLACE
		};

		int elem_key, elem_val;
		if (version_dir == DNA_VERSION_RUNTIME_FROM_STATIC) {
			elem_key = 0;
			elem_val = 1;

		}
		else {
			elem_key = 1;
			elem_val = 0;
		}

		GHash *struct_map = BLI_ghash_str_new_ex(__func__, ARRAY_SIZE(data));
		for (int i = 0; i < ARRAY_SIZE(data); i++) {
			BLI_ghash_insert(struct_map, (void *)data[i][elem_key], (void *)data[i][elem_val]);
		}
		*r_struct_map = struct_map;
	}

	if (r_elem_map != NULL) {
		const char *data[][3] = {
#define DNA_STRUCT_REPLACE(old, new)
#define DNA_STRUCT_MEMBER_REPLACE(struct_name, old, new) {#struct_name, #old, #new},
#include DNA_VERSIONING_DEFINES
#undef DNA_STRUCT_REPLACE
#undef DNA_STRUCT_MEMBER_REPLACE
		};

		int elem_key, elem_val;
		if (version_dir == DNA_VERSION_RUNTIME_FROM_STATIC) {
			elem_key = 1;
			elem_val = 2;

		}
		else {
			elem_key = 2;
			elem_val = 1;
		}
		GHash *elem_map = BLI_ghash_new_ex(strhash_pair_p, strhash_pair_cmp, __func__, ARRAY_SIZE(data));
		for (int i = 0; i < ARRAY_SIZE(data); i++) {
			dna_softupdate_ghash_add_pair(elem_map, data[i][0], data[i][elem_key], (void *)data[i][elem_val]);
		}
		*r_elem_map = elem_map;
	}
}

#undef DNA_MAKESDNA

/** \} */
