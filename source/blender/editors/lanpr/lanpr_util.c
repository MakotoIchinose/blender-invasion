#define _CRT_SEQURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
/* #include <time.h> */
#include "ED_lanpr.h"
#include "BLI_math.h"
#include "MEM_guardedalloc.h"
#include <math.h>

/* ===================================================================[slt] */

void *list_append_pointer_static(ListBase *h, LANPR_StaticMemPool *smp, void *data)
{
  LinkData *lip;
  if (!h) {
    return 0;
  }
  lip = mem_static_aquire(smp, sizeof(LinkData));
  lip->data = data;
  BLI_addtail(h, lip);
  return lip;
}
void *list_append_pointer_static_sized(ListBase *h, LANPR_StaticMemPool *smp, void *data, int size)
{
  LinkData *lip;
  if (!h) {
    return 0;
  }
  lip = mem_static_aquire(smp, size);
  lip->data = data;
  BLI_addtail(h, lip);
  return lip;
}

void *list_append_pointer_static_pool(LANPR_StaticMemPool *mph, ListBase *h, void *data)
{
  LinkData *lip;
  if (!h) {
    return 0;
  }
  lip = mem_static_aquire(mph, sizeof(LinkData));
  lip->data = data;
  BLI_addtail(h, lip);
  return lip;
}
void *list_pop_pointer_no_free(ListBase *h)
{
  LinkData *lip;
  void *rev = 0;
  if (!h) {
    return 0;
  }
  lip = BLI_pophead(h);
  rev = lip ? lip->data : 0;
  return rev;
}

LANPR_StaticMemPoolNode *mem_new_static_pool(LANPR_StaticMemPool *smp)
{
  LANPR_StaticMemPoolNode *smpn = MEM_callocN(LANPR_MEMORY_POOL_128MB, "mempool");
  smpn->used_byte = sizeof(LANPR_StaticMemPoolNode);
  BLI_addhead(&smp->pools, smpn);
  return smpn;
}
void *mem_static_aquire(LANPR_StaticMemPool *smp, int size)
{
  LANPR_StaticMemPoolNode *smpn = smp->pools.first;
  void *ret;

  if (!smpn || (smpn->used_byte + size) > LANPR_MEMORY_POOL_128MB) {
    smpn = mem_new_static_pool(smp);
  }

  ret = ((BYTE *)smpn) + smpn->used_byte;

  smpn->used_byte += size;

  return ret;
}
void *mem_static_aquire_thread(LANPR_StaticMemPool *smp, int size)
{
  LANPR_StaticMemPoolNode *smpn = smp->pools.first;
  void *ret;

  BLI_spin_lock(&smp->cs_mem);

  if (!smpn || (smpn->used_byte + size) > LANPR_MEMORY_POOL_128MB) {
    smpn = mem_new_static_pool(smp);
  }

  ret = ((BYTE *)smpn) + smpn->used_byte;

  smpn->used_byte += size;

  BLI_spin_unlock(&smp->cs_mem);

  return ret;
}
void *mem_static_destroy(LANPR_StaticMemPool *smp)
{
  LANPR_StaticMemPoolNode *smpn;
  void *ret = 0;

  while ((smpn = BLI_pophead(&smp->pools)) != NULL) {
    MEM_freeN(smpn);
  }

  smp->each_size = 0;

  return ret;
}

void tmat_make_perspective_matrix_44f(
    float (*mProjection)[4], float fFov_rad, float fAspect, float zMin, float zMax)
{
  double yMax;
  double yMin;
  double xMin;
  double xMax;

  if (fAspect < 1) {
    yMax = zMin * tan((double)fFov_rad * 0.5f);
    yMin = -yMax;
    xMin = yMin * (double)fAspect;
    xMax = -xMin;
  }
  else {
    xMax = zMin * tan((double)fFov_rad * 0.5f);
    xMin = -xMax;
    yMin = xMin / (double)fAspect;
    yMax = -yMin;
  }

  unit_m4(mProjection);

  mProjection[0][0] = (2.0f * (double)zMin) / (xMax - xMin);
  mProjection[1][1] = (2.0f * (double)zMin) / (yMax - yMin);
  mProjection[2][0] = (xMax + xMin) / (xMax - xMin);
  mProjection[2][1] = (yMax + yMin) / (yMax - yMin);
  mProjection[2][2] = -(((double)zMax + (double)zMin) / ((double)zMax - (double)zMin));
  mProjection[2][3] = -1.0f;
  mProjection[3][2] = -((2.0f * ((double)zMax * (double)zMin)) / ((double)zMax - (double)zMin));
  mProjection[3][3] = 0.0f;
}
