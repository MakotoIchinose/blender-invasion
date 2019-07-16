
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
 * Contributor(s): Hugo Sales
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/* #ifndef __IO_OBJ_H__ */
/* #define __IO_OBJ_H__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "../io_common.h"

struct bContext;

bool OBJ_export(bContext *C, ExportSettings *const settings);
bool OBJ_import(bContext *C, ImportSettings *const settings);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* #endif  /\* __IO_OBJ_H__ *\/ */
