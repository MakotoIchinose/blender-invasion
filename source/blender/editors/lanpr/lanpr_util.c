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
void list_remove_pointer_item_no_free(ListBase *h, LinkData *lip)
{
  BLI_remlink(h, (void *)lip);
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

/* =======================================================================[str] */

void tmat_load_identity_44d(tnsMatrix44d m)
{
  memset(m, 0, sizeof(tnsMatrix44d));
  m[0] = 1.0f;
  m[5] = 1.0f;
  m[10] = 1.0f;
  m[15] = 1.0f;
};

real tmat_length_3d(tnsVector3d l)
{
  return (sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]));
}
real tmat_vector_cross_3d(tnsVector3d result, tnsVector3d l, tnsVector3d r)
{
  result[0] = l[1] * r[2] - l[2] * r[1];
  result[1] = l[2] * r[0] - l[0] * r[2];
  result[2] = l[0] * r[1] - l[1] * r[0];
  return tmat_length_3d(result);
}
void tmat_apply_rotation_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v)
{
  result[0] = mat[0] * v[0] + mat[1] * v[1] + mat[2] * v[2];
  result[1] = mat[4] * v[0] + mat[5] * v[1] + mat[6] * v[2];
  result[2] = mat[8] * v[0] + mat[9] * v[1] + mat[10] * v[2];
}
void tmat_apply_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v)
{
  real w;
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
  w = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * 1;
  result[0] /= w;
  result[1] /= w;
  result[2] /= w;
}
void tmat_apply_normal_transform_43d(tnsVector3d result, tnsMatrix44d mat, tnsVector3d v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
}
void tmat_apply_normal_transform_43df(tnsVector3d result, tnsMatrix44d mat, tnsVector3f v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
}
void tmat_apply_transform_44d(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
  result[3] = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * 1;
}
void tmat_apply_transform_43dfND(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
  result[3] = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * 1;
}
void tmat_apply_transform_43df(tnsVector4d result, tnsMatrix44d mat, tnsVector3f v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * 1;
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * 1;
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * 1;
  /* real w = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * 1; */
  /*  result[0] /= w; */
  /*  result[1] /= w; */
  /*  result[2] /= w; */
}
void tmat_apply_transform_44dTrue(tnsVector4d result, tnsMatrix44d mat, tnsVector4d v)
{
  result[0] = mat[0] * v[0] + mat[4] * v[1] + mat[8] * v[2] + mat[12] * v[3];
  result[1] = mat[1] * v[0] + mat[5] * v[1] + mat[9] * v[2] + mat[13] * v[3];
  result[2] = mat[2] * v[0] + mat[6] * v[1] + mat[10] * v[2] + mat[14] * v[3];
  result[3] = mat[3] * v[0] + mat[7] * v[1] + mat[11] * v[2] + mat[15] * v[3];
}

#define L(row, col) l[(col << 2) + row]
#define R(row, col) r[(col << 2) + row]
#define P(row, col) result[(col << 2) + row]

void tmat_obmat_to_16d(float obmat[4][4], tnsMatrix44d out)
{
  out[0] = obmat[0][0];
  out[1] = obmat[0][1];
  out[2] = obmat[0][2];
  out[3] = obmat[0][3];
  out[4] = obmat[1][0];
  out[5] = obmat[1][1];
  out[6] = obmat[1][2];
  out[7] = obmat[1][3];
  out[8] = obmat[2][0];
  out[9] = obmat[2][1];
  out[10] = obmat[2][2];
  out[11] = obmat[2][3];
  out[12] = obmat[3][0];
  out[13] = obmat[3][1];
  out[14] = obmat[3][2];
  out[15] = obmat[3][3];
}
void tmat_multiply_44d(tnsMatrix44d result, tnsMatrix44d l, tnsMatrix44d r)
{
  int i;
  for (i = 0; i < 4; i++) {
    real ai0 = L(i, 0), ai1 = L(i, 1), ai2 = L(i, 2), ai3 = L(i, 3);
    P(i, 0) = ai0 * R(0, 0) + ai1 * R(1, 0) + ai2 * R(2, 0) + ai3 * R(3, 0);
    P(i, 1) = ai0 * R(0, 1) + ai1 * R(1, 1) + ai2 * R(2, 1) + ai3 * R(3, 1);
    P(i, 2) = ai0 * R(0, 2) + ai1 * R(1, 2) + ai2 * R(2, 2) + ai3 * R(3, 2);
    P(i, 3) = ai0 * R(0, 3) + ai1 * R(1, 3) + ai2 * R(2, 3) + ai3 * R(3, 3);
  }
};
#undef L
#undef R
#undef P
void tmat_inverse_44d(tnsMatrix44d inverse, tnsMatrix44d mat)
{
  int i, j, k;
  double temp;
  tnsMatrix44d tempmat;
  real max;
  int maxj;

  tmat_load_identity_44d(inverse);

  memcpy(tempmat, mat, sizeof(tnsMatrix44d));

  for (i = 0; i < 4; i++) {
    /* Look for row with max pivot */
    max = fabsf(tempmat[i * 5]);
    maxj = i;
    for (j = i + 1; j < 4; j++) {
      if (fabsf(tempmat[j * 4 + i]) > max) {
        max = fabsf(tempmat[j * 4 + i]);
        maxj = j;
      }
    }

    /* Swap rows if necessary */
    if (maxj != i) {
      for (k = 0; k < 4; k++) {
        real t;
        t = tempmat[i * 4 + k];
        tempmat[i * 4 + k] = tempmat[maxj * 4 + k];
        tempmat[maxj * 4 + k] = t;

        t = inverse[i * 4 + k];
        inverse[i * 4 + k] = inverse[maxj * 4 + k];
        inverse[maxj * 4 + k] = t;
      }
    }

    /*  if (UNLIKELY(tempmat[i][i] == 0.0f)) { */
    /* 	return false;  No non-zero pivot  */
    /* } */

    temp = (double)tempmat[i * 5];
    for (k = 0; k < 4; k++) {
      tempmat[i * 4 + k] = (real)((double)tempmat[i * 4 + k] / temp);
      inverse[i * 4 + k] = (real)((double)inverse[i * 4 + k] / temp);
    }
    for (j = 0; j < 4; j++) {
      if (j != i) {
        temp = tempmat[j * 4 + i];
        for (k = 0; k < 4; k++) {
          tempmat[j * 4 + k] -= (real)((double)tempmat[i * 4 + k] * temp);
          inverse[j * 4 + k] -= (real)((double)inverse[i * 4 + k] * temp);
        }
      }
    }
  }
}
void tmat_make_translation_matrix_44d(tnsMatrix44d mTrans, real x, real y, real z)
{
  tmat_load_identity_44d(mTrans);
  mTrans[12] = x;
  mTrans[13] = y;
  mTrans[14] = z;
}
void tmat_make_perspective_matrix_44f(
    float (*mProjection)[4], float fFov_rad, float fAspect, float zMin, float zMax)
{
  float yMax;
  float yMin;
  float xMin;
  float xMax;

  if (fAspect < 1) {
    yMax = zMin * tanf(fFov_rad * 0.5f);
    yMin = -yMax;
    xMin = yMin * fAspect;
    xMax = -xMin;
  }
  else {
    xMax = zMin * tanf(fFov_rad * 0.5f);
    xMin = -xMax;
    yMin = xMin / fAspect;
    yMax = -yMin;
  }

  unit_m4(mProjection);

  mProjection[0][0] = (2.0f * zMin) / (xMax - xMin);
  mProjection[1][1] = (2.0f * zMin) / (yMax - yMin);
  mProjection[2][0] = (xMax + xMin) / (xMax - xMin);
  mProjection[2][1] = (yMax + yMin) / (yMax - yMin);
  mProjection[2][2] = -((zMax + zMin) / (zMax - zMin));
  mProjection[2][3] = -1.0f;
  mProjection[3][2] = -((2.0f * (zMax * zMin)) / (zMax - zMin));
  mProjection[3][3] = 0.0f;
}
void tmat_make_ortho_matrix_44d(
    tnsMatrix44d mProjection, real xMin, real xMax, real yMin, real yMax, real zMin, real zMax)
{
  tmat_load_identity_44d(mProjection);

  mProjection[0] = 2.0f / (xMax - xMin);
  mProjection[5] = 2.0f / (yMax - yMin);
  mProjection[10] = -2.0f / (zMax - zMin);
  mProjection[12] = -((xMax + xMin) / (xMax - xMin));
  mProjection[13] = -((yMax + yMin) / (yMax - yMin));
  mProjection[14] = -((zMax + zMin) / (zMax - zMin));
  mProjection[15] = 1.0f;
}
void tmat_make_rotation_matrix_44d(tnsMatrix44d m, real angle_rad, real x, real y, real z)
{
  real mag, s, c;
  real xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

  s = (real)sin(angle_rad);
  c = (real)cos(angle_rad);

  mag = (real)sqrt(x * x + y * y + z * z);

  /*  Identity matrix */
  if (mag == 0.0f) {
    tmat_load_identity_44d(m);
    return;
  }

  /*  Rotation matrix is normalized */
  x /= mag;
  y /= mag;
  z /= mag;

#define M(row, col) m[col * 4 + row]

  xx = x * x;
  yy = y * y;
  zz = z * z;
  xy = x * y;
  yz = y * z;
  zx = z * x;
  xs = x * s;
  ys = y * s;
  zs = z * s;
  one_c = 1.0f - c;

  M(0, 0) = (one_c * xx) + c;
  M(0, 1) = (one_c * xy) + zs;
  M(0, 2) = (one_c * zx) + ys;
  M(0, 3) = 0.0f;

  M(1, 0) = (one_c * xy) - zs;
  M(1, 1) = (one_c * yy) + c;
  M(1, 2) = (one_c * yz) + xs;
  M(1, 3) = 0.0f;

  M(2, 0) = (one_c * zx) + ys;
  M(2, 1) = (one_c * yz) - xs;
  M(2, 2) = (one_c * zz) + c;
  M(2, 3) = 0.0f;

  M(3, 0) = 0.0f;
  M(3, 1) = 0.0f;
  M(3, 2) = 0.0f;
  M(3, 3) = 1.0f;

#undef M
}
void tmat_make_rotation_x_matrix_44d(tnsMatrix44d m, real angle_rad)
{
  tmat_load_identity_44d(m);
  m[5] = cos(angle_rad);
  m[6] = sin(angle_rad);
  m[9] = -sin(angle_rad);
  m[10] = cos(angle_rad);
}
void tmat_make_rotation_y_matrix_44d(tnsMatrix44d m, real angle_rad)
{
  tmat_load_identity_44d(m);
  m[0] = cos(angle_rad);
  m[2] = -sin(angle_rad);
  m[8] = sin(angle_rad);
  m[10] = cos(angle_rad);
}
void tmat_make_rotation_z_matrix_44d(tnsMatrix44d m, real angle_rad)
{
  tmat_load_identity_44d(m);
  m[0] = cos(angle_rad);
  m[1] = sin(angle_rad);
  m[4] = -sin(angle_rad);
  m[5] = cos(angle_rad);
}
void tmat_make_scale_matrix_44d(tnsMatrix44d m, real x, real y, real z)
{
  tmat_load_identity_44d(m);
  m[0] = x;
  m[5] = y;
  m[10] = z;
}
void tmat_make_viewport_matrix_44d(tnsMatrix44d m, real w, real h, real Far, real Near)
{
  tmat_load_identity_44d(m);
  m[0] = w / 2;
  m[5] = h / 2;
  m[10] = (Far - Near) / 2;
  m[12] = w / 2;
  m[13] = h / 2;
  m[14] = (Far + Near) / 2;
  m[15] = 1;
  /*  m[0] = 2/w; */
  /*  m[5] = 2/h; */
  /*  m[10] = 1; */
  /*  m[12] = 2/w; */
  /*  m[13] = 2/h; */
  /*  m[14] = 1; */
  /*  m[15] = 1; */
}
