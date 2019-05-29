#pragma once

#include <string.h>
//#include "lanpr_all.h"
#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_threads.h"

/*

   Ported from NUL4.0

   Author(s):WuYiming - xp8110@outlook.com

 */

#define _CRT_SECURE_NO_WARNINGS
#define BYTE unsigned char

typedef double real;
typedef unsigned long long u64bit;
typedef unsigned int u32bit;
typedef unsigned short u16bit;
typedef unsigned short ushort;
typedef unsigned char u8bit;
typedef char nShortBuf[16];

typedef float tnsMatrix44f[16];

typedef real tnsMatrix44d[16];
typedef real tnsVector2d[2];
typedef real tnsVector3d[3];
typedef real tnsVector4d[4];
typedef float tnsVector3f[3];
typedef float tnsVector4f[4];
typedef int tnsVector2i[2];

#define TNS_PI 3.1415926535897932384626433832795
#define deg(r) r / TNS_PI * 180.0
#define rad(d) d *TNS_PI / 180.0


#define DBL_TRIANGLE_LIM 1e-8
#define DBL_EDGE_LIM 1e-9


#define NUL_MEMORY_POOL_1MB 1048576
#define NUL_MEMORY_POOL_128MB 134217728
#define NUL_MEMORY_POOL_256MB 268435456
#define NUL_MEMORY_POOL_512MB 536870912

typedef struct _Link2 
{
  void *O1;
  void *O2;
  void *pNext;
  void *pPrev;
}_Link2;

typedef struct nMemoryPool
{
  Link Item;
  int NodeSize;
  int CountPerPool;
  ListBase Pools;
}nMemoryPool;

typedef struct nMemoryPoolPart
{
  Link Item;
  ListBase MemoryNodes;
  ListBase FreeMemoryNodes;
  nMemoryPool *PoolRoot;
  //  <------Mem Begin Here.
}nMemoryPoolPart;

typedef struct nMemoryPoolNode
{
  Link Item;
  nMemoryPoolPart *InPool;
  void *DBInst;
  //  <------User Mem Begin Here
}nMemoryPoolNode;

typedef struct nStaticMemoryPoolNode
{
  Link Item;
  int UsedByte;
  //  <----------- User Mem Start Here
}nStaticMemoryPoolNode;

typedef struct nStaticMemoryPool
{
  int EachSize;
  ListBase Pools;
  SpinLock csMem;
}nStaticMemoryPool;

#define CreateNew(Type) MEM_callocN(sizeof(Type), "VOID")  // nutCalloc(sizeof(Type),1)

#define CreateNew_Size(size) nutCalloc(size, 1)

#define CreateNewBuffer(Type, Num) \
  MEM_callocN(sizeof(Type) * Num, "VOID BUFFER")  // nutCalloc(sizeof(Type),Num);


void list_handle_empty(ListBase *h);

void list_clear_prev_next(Link *li);

void list_insert_item_before(ListBase *Handle, Link *toIns, Link *pivot);
void list_insert_item_after(ListBase *Handle, Link *toIns, Link *pivot);
void list_insert_segment_before(ListBase *Handle,
                                Link *Begin,
                                Link *End,
                                Link *pivot);
void lstInsertSegmentAfter(ListBase *Handle, Link *Begin, Link *End, Link *pivot);
int lstHaveItemInList(ListBase *Handle);
void *lst_get_top(ListBase *Handle);

void *list_append_pointer_only(ListBase *h, void *p);
void *list_append_pointer_sized_only(ListBase *h, void *p, int size);
void *list_push_pointer_only(ListBase *h, void *p);
void *list_push_pointer_sized_only(ListBase *h, void *p, int size);

void *list_append_pointer(ListBase *h, void *p);
void *list_append_pointer_sized(ListBase *h, void *p, int size);
void *list_push_pointer(ListBase *h, void *p);
void *list_push_pointer_sized(ListBase *h, void *p, int size);

void *list_append_pointer_static(ListBase *h, nStaticMemoryPool *smp, void *p);
void *list_append_pointer_static_sized(ListBase *h, nStaticMemoryPool *smp, void *p, int size);
void *list_push_pointer_static(ListBase *h, nStaticMemoryPool *smp, void *p);
void *list_push_pointer_static_sized(ListBase *h, nStaticMemoryPool *smp, void *p, int size);

void *list_pop_pointer_only(ListBase *h);
void list_remove_pointer_item_only(ListBase *h, LinkData *lip);
void list_remove_pointer_only(ListBase *h, void *p);
void list_clear_pointer_only(ListBase *h);
void list_generate_pointer_list_only(ListBase *from1, ListBase *from2, ListBase *to);

void *list_pop_pointer(ListBase *h);
void list_remove_pointer_item(ListBase *h, LinkData *lip);
void list_remove_pointer(ListBase *h, void *p);
void list_clear_pointer(ListBase *h);
void list_generate_pointer_list(ListBase *from1, ListBase *from2, ListBase *to);

void list_copy_handle(ListBase *target, ListBase *src);

void *list_append_pointer_static_pool(nStaticMemoryPool *mph, ListBase *h, void *p);
void *list_pop_pointer_no_free(ListBase *h);
void list_remove_pointer_item_no_free(ListBase *h, LinkData *lip);

void list_move_up(ListBase *h, Link *li);
void list_move_down(ListBase *h, Link *li);

void lstAddElement(ListBase *hlst, void *ext);
void lstDestroyElementList(ListBase *hlst);

nStaticMemoryPoolNode *mem_new_static_pool(nStaticMemoryPool *smp);
void *mem_static_aquire(nStaticMemoryPool *smp, int size);
void *mem_static_aquire_thread(nStaticMemoryPool *smp, int size);
void *mem_static_destroy(nStaticMemoryPool *smp);

void tmat_obmat_to_16d(float obmat[4][4], tnsMatrix44d out);

real tmat_dist_idv2(real x1, real y1, real x2, real y2);
real tmat_dist_3dv(tnsVector3d l, tnsVector3d r);
real tmat_dist_2dv(tnsVector2d l, tnsVector2d r);

real tmat_length_3d(tnsVector3d l);
real tmat_length_2d(tnsVector3d l);
void tmat_normalize_3d(tnsVector3d result, tnsVector3d l);
void tmat_normalize_3f(tnsVector3f result, tnsVector3f l);
void tmat_normalize_self_3d(tnsVector3d result);
real tmat_dot_3d(tnsVector3d l, tnsVector3d r, int normalize);
real tmat_dot_3df(tnsVector3d l, tnsVector3f r, int normalize);
real tmat_dot_2d(tnsVector2d l, tnsVector2d r, int normalize);
real tmat_vector_cross_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);
real tmat_angle_rad_3d(tnsVector3d from, tnsVector3d to, tnsVector3d PositiveReference);
void tmat_apply_rotation_33d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_rotation_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_transform_43dfND(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_normal_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v);
void tmat_apply_normal_transform_43df(tnsVector3d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_transform_44d(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v);
void tmat_apply_transform_43df(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v);
void tmat_apply_transform_44dTrue(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v);

void tmat_load_identity_44d(tnsMatrix44d m);
void tmat_make_ortho_matrix_44d(
    tnsMatrix44d mProjection, real xMin, real xMax, real yMin, real yMax, real zMin, real zMax);
void tmat_make_perspective_matrix_44d(
    tnsMatrix44d mProjection, real fFov_rad, real fAspect, real zMin, real zMax);
void tmat_make_translation_matrix_44d(tnsMatrix44d mTrans, real x, real y, real z);
void tmat_make_rotation_matrix_44d(tnsMatrix44d m, real angle_rad, real x, real y, real z);
void tmat_make_scale_matrix_44d(tnsMatrix44d m, real x, real y, real z);
void tmat_make_viewport_matrix_44d(tnsMatrix44d m, real w, real h, real Far, real Near);
void tmat_multiply_44d(tnsMatrix44d result, tnsMatrix44d l, tnsMatrix44d r);
void tmat_inverse_44d(tnsMatrix44d inverse, tnsMatrix44d mat);
void tmat_make_rotation_x_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_make_rotation_y_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_make_rotation_z_matrix_44d(tnsMatrix44d m, real angle_rad);
void tmat_remove_translation_44d(tnsMatrix44d result, tnsMatrix44d mat);
void tmat_clear_translation_44d(tnsMatrix44d mat);

real tmat_angle_rad_3d(tnsVector3d from, tnsVector3d to, tnsVector3d PositiveReference);
real tmat_length_3d(tnsVector3d l);
void tmat_normalize_2d(tnsVector2d result, tnsVector2d l);
void tmat_normalize_3d(tnsVector3d result, tnsVector3d l);
void tmat_normalize_self_3d(tnsVector3d result);
real tmat_dot_3d(tnsVector3d l, tnsVector3d r, int normalize);
real tmat_vector_cross_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);
void tmat_vector_cross_only_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r);
