#include "DRW_engine.h"
#include "DRW_render.h"
#include "BLI_listbase.h"
#include "BLI_linklist.h"
#include "BLI_math_matrix.h"
#include "BLI_task.h"
#include "BLI_utildefines.h"
#include "lanpr_all.h"
#include "lanpr_util.h"
#include "DRW_render.h"
#include "BKE_object.h"
#include "DNA_mesh_types.h"
#include "DNA_camera_types.h"
#include "DNA_modifier_types.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_framebuffer.h"
#include "DNA_lanpr_types.h"
#include "DNA_meshdata_types.h"
#include "BKE_customdata.h"
#include "DEG_depsgraph_query.h"
#include "BKE_camera.h"
#include "BKE_collection.h"
#include "BKE_report.h"
#include "GPU_draw.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_uniformbuffer.h"
#include "GPU_viewport.h"
#include "bmesh.h"
#include "bmesh_class.h"
#include "bmesh_tools.h"

#include "WM_types.h"
#include "WM_api.h"

#include <math.h>
#include "RNA_access.h"
#include "RNA_define.h"

/*

   Ported from NUL4.0

   Author(s):WuYiming - xp8110@outlook.com

 */

struct Object;


int lanpr_triangle_line_imagespace_intersection_v2(SpinLock *spl, LANPR_RenderTriangle *rt, LANPR_RenderLine *rl, Object *cam, tnsMatrix44d vp, real *CameraDir, double *From, double *To);
void lanpr_compute_view_vector(LANPR_RenderBuffer *rb);

int use_smooth_contour_modifier_contour = 0; // debug purpose

/* ====================================== base structures =========================================== */

#define TNS_BOUND_AREA_CROSSES(b1, b2) \
	((b1)[0] < (b2)[1] && (b1)[1] > (b2)[0] && (b1)[3] < (b2)[2] && (b1)[2] > (b2)[3])

void lanpr_make_initial_bounding_areas(LANPR_RenderBuffer *rb) {
	int sp_w = 4;//20;
	int sp_h = 4;//rb->H / (rb->W / sp_w);
	int row, col;
	LANPR_BoundingArea *ba;
	real W = (real)rb->w;
	real H = (real)rb->h;
	real span_w = (real)1 / sp_w * 2.0;
	real span_h = (real)1 / sp_h * 2.0;

	rb->tile_count_x = sp_w;
	rb->tile_count_y = sp_h;
	rb->width_per_tile = span_w;
	rb->height_per_tile = span_h;

	rb->bounding_area_count = sp_w * sp_h;
	rb->initial_bounding_areas = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_BoundingArea) * rb->bounding_area_count);

	for (row = 0; row < sp_h; row++) {
		for (col = 0; col < sp_w; col++) {
			ba = &rb->initial_bounding_areas[row * 4 + col];

			ba->L = span_w * col - 1.0;
			ba->R = (col == sp_w - 1) ? 1.0 : (span_w * (col + 1) - 1.0);
			ba->U = 1.0 - span_h * row;
			ba->B = (row == sp_h - 1) ? -1.0 : (1.0 - span_h * (row + 1));

			ba->CX = (ba->L + ba->R) / 2;
			ba->CY = (ba->U + ba->B) / 2;

			if (row) {
				list_append_pointer_static(&ba->UP, &rb->render_data_pool, &rb->initial_bounding_areas[(row - 1) * 4 + col]);
			}
			if (col) {
				list_append_pointer_static(&ba->LP, &rb->render_data_pool, &rb->initial_bounding_areas[row * 4 + col - 1]);
			}
			if (row != sp_h - 1) {
				list_append_pointer_static(&ba->BP, &rb->render_data_pool, &rb->initial_bounding_areas[(row + 1) * 4 + col]);
			}
			if (col != sp_w - 1) {
				list_append_pointer_static(&ba->RP, &rb->render_data_pool, &rb->initial_bounding_areas[row * 4 + col + 1]);
			}
		}
	}
}
void lanpr_connect_new_bounding_areas(LANPR_RenderBuffer *rb, LANPR_BoundingArea *Root) {
	LANPR_BoundingArea *ba = Root->Child, *tba;
	LinkData *lip, *lip2, *next_lip;
	nStaticMemoryPool *mph = &rb->render_data_pool;

	list_append_pointer_static_pool(mph, &ba[1].RP, &ba[0]);
	list_append_pointer_static_pool(mph, &ba[0].LP, &ba[1]);
	list_append_pointer_static_pool(mph, &ba[1].BP, &ba[2]);
	list_append_pointer_static_pool(mph, &ba[2].UP, &ba[1]);
	list_append_pointer_static_pool(mph, &ba[2].RP, &ba[3]);
	list_append_pointer_static_pool(mph, &ba[3].LP, &ba[2]);
	list_append_pointer_static_pool(mph, &ba[3].UP, &ba[0]);
	list_append_pointer_static_pool(mph, &ba[0].BP, &ba[3]);

	for (lip = Root->LP.first; lip; lip = lip->next) {
		tba = lip->data;
		if (ba[1].U > tba->B && ba[1].B < tba->U) { list_append_pointer_static_pool(mph, &ba[1].LP, tba); list_append_pointer_static_pool(mph, &tba->RP, &ba[1]); }
		if (ba[2].U > tba->B && ba[2].B < tba->U) { list_append_pointer_static_pool(mph, &ba[2].LP, tba); list_append_pointer_static_pool(mph, &tba->RP, &ba[2]); }
	}
	for (lip = Root->RP.first; lip; lip = lip->next) {
		tba = lip->data;
		if (ba[0].U > tba->B && ba[0].B < tba->U) { list_append_pointer_static_pool(mph, &ba[0].RP, tba); list_append_pointer_static_pool(mph, &tba->LP, &ba[0]); }
		if (ba[3].U > tba->B && ba[3].B < tba->U) { list_append_pointer_static_pool(mph, &ba[3].RP, tba); list_append_pointer_static_pool(mph, &tba->LP, &ba[3]); }
	}
	for (lip = Root->UP.first; lip; lip = lip->next) {
		tba = lip->data;
		if (ba[0].R > tba->L && ba[0].L < tba->R) { list_append_pointer_static_pool(mph, &ba[0].UP, tba); list_append_pointer_static_pool(mph, &tba->BP, &ba[0]); }
		if (ba[1].R > tba->L && ba[1].L < tba->R) { list_append_pointer_static_pool(mph, &ba[1].UP, tba); list_append_pointer_static_pool(mph, &tba->BP, &ba[1]); }
	}
	for (lip = Root->BP.first; lip; lip = lip->next) {
		tba = lip->data;
		if (ba[2].R > tba->L && ba[2].L < tba->R) { list_append_pointer_static_pool(mph, &ba[2].BP, tba); list_append_pointer_static_pool(mph, &tba->UP, &ba[2]); }
		if (ba[3].R > tba->L && ba[3].L < tba->R) { list_append_pointer_static_pool(mph, &ba[3].BP, tba); list_append_pointer_static_pool(mph, &tba->UP, &ba[3]); }
	}
	for (lip = Root->LP.first; lip; lip = lip->next) {
		for (lip2 = ((LANPR_BoundingArea *)lip->data)->RP.first; lip2; lip2 = next_lip) {
			next_lip = lip2->next;
			tba = lip2->data;
			if (tba == Root) {
				list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->RP, lip2);
				if (ba[1].U > tba->B && ba[1].B < tba->U) list_append_pointer_static_pool(mph, &tba->RP, &ba[1]);
				if (ba[2].U > tba->B && ba[2].B < tba->U) list_append_pointer_static_pool(mph, &tba->RP, &ba[2]);
			}
		}
	}
	for (lip = Root->RP.first; lip; lip = lip->next) {
		for (lip2 = ((LANPR_BoundingArea *)lip->data)->LP.first; lip2; lip2 = next_lip) {
			next_lip = lip2->next;
			tba = lip2->data;
			if (tba == Root) {
				list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->LP, lip2);
				if (ba[0].U > tba->B && ba[0].B < tba->U) list_append_pointer_static_pool(mph, &tba->LP, &ba[0]);
				if (ba[3].U > tba->B && ba[3].B < tba->U) list_append_pointer_static_pool(mph, &tba->LP, &ba[3]);
			}
		}
	}
	for (lip = Root->UP.first; lip; lip = lip->next) {
		for (lip2 = ((LANPR_BoundingArea *)lip->data)->BP.first; lip2; lip2 = next_lip) {
			next_lip = lip2->next;
			tba = lip2->data;
			if (tba == Root) {
				list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->BP, lip2);
				if (ba[0].R > tba->L && ba[0].L < tba->R) list_append_pointer_static_pool(mph, &tba->UP, &ba[0]);
				if (ba[1].R > tba->L && ba[1].L < tba->R) list_append_pointer_static_pool(mph, &tba->UP, &ba[1]);
			}
		}
	}
	for (lip = Root->BP.first; lip; lip = lip->next) {
		for (lip2 = ((LANPR_BoundingArea *)lip->data)->UP.first; lip2; lip2 = next_lip) {
			next_lip = lip2->next;
			tba = lip2->data;
			if (tba == Root) {
				list_remove_pointer_item_no_free(&((LANPR_BoundingArea *)lip->data)->UP, lip2);
				if (ba[2].R > tba->L && ba[2].L < tba->R) list_append_pointer_static_pool(mph, &tba->BP, &ba[2]);
				if (ba[3].R > tba->L && ba[3].L < tba->R) list_append_pointer_static_pool(mph, &tba->BP, &ba[3]);
			}
		}
	}
	while (list_pop_pointer_no_free(&Root->LP));
	while (list_pop_pointer_no_free(&Root->RP));
	while (list_pop_pointer_no_free(&Root->UP));
	while (list_pop_pointer_no_free(&Root->BP));
}
void lanpr_link_triangle_with_bounding_area(LANPR_RenderBuffer *rb, LANPR_BoundingArea *RootBoundingArea, LANPR_RenderTriangle *rt, real *LRUB, int Recursive);
void lanpr_triangle_enable_intersections_in_bounding_area(LANPR_RenderBuffer *rb, LANPR_RenderTriangle *rt, LANPR_BoundingArea *ba);

void lanpr_split_bounding_area(LANPR_RenderBuffer *rb, LANPR_BoundingArea *Root) {
	LANPR_BoundingArea *ba = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_BoundingArea) * 4);
	LANPR_RenderTriangle *rt;

	ba[0].L = Root->CX;
	ba[0].R = Root->R;
	ba[0].U = Root->U;
	ba[0].B = Root->CY;
	ba[0].CX = (ba[0].L + ba[0].R) / 2;
	ba[0].CY = (ba[0].U + ba[0].B) / 2;

	ba[1].L = Root->L;
	ba[1].R = Root->CX;
	ba[1].U = Root->U;
	ba[1].B = Root->CY;
	ba[1].CX = (ba[1].L + ba[1].R) / 2;
	ba[1].CY = (ba[1].U + ba[1].B) / 2;

	ba[2].L = Root->L;
	ba[2].R = Root->CX;
	ba[2].U = Root->CY;
	ba[2].B = Root->B;
	ba[2].CX = (ba[2].L + ba[2].R) / 2;
	ba[2].CY = (ba[2].U + ba[2].B) / 2;

	ba[3].L = Root->CX;
	ba[3].R = Root->R;
	ba[3].U = Root->CY;
	ba[3].B = Root->B;
	ba[3].CX = (ba[3].L + ba[3].R) / 2;
	ba[3].CY = (ba[3].U + ba[3].B) / 2;

	Root->Child = ba;

	lanpr_connect_new_bounding_areas(rb, Root);

	while (rt = list_pop_pointer_no_free(&Root->LinkedTriangles)) {
		LANPR_BoundingArea *ba = Root->Child;
		real B[4];
		B[0] = MIN3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
		B[1] = MAX3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
		B[2] = MAX3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);
		B[3] = MIN3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);
		if (TNS_BOUND_AREA_CROSSES(B, &ba[0].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[0], rt, B, 0);
		if (TNS_BOUND_AREA_CROSSES(B, &ba[1].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[1], rt, B, 0);
		if (TNS_BOUND_AREA_CROSSES(B, &ba[2].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[2], rt, B, 0);
		if (TNS_BOUND_AREA_CROSSES(B, &ba[3].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[3], rt, B, 0);
	}

	rb->bounding_area_count += 3;
}
int lanpr_line_crosses_bounding_area(LANPR_RenderBuffer *fb, tnsVector2d L, tnsVector2d R, LANPR_BoundingArea *ba) {
	real vx, vy;
	tnsVector4d converted;
	real c1, c;

	if ((converted[0] = (real)ba->L) > MAX2(L[0], R[0])) return 0;
	if ((converted[1] = (real)ba->R) < MIN2(L[0], R[0])) return 0;
	if ((converted[2] = (real)ba->B) > MAX2(L[1], R[1])) return 0;
	if ((converted[3] = (real)ba->U) < MIN2(L[1], R[1])) return 0;

	vx = L[0] - R[0];
	vy = L[1] - R[1];

	c1 = vx * (converted[2] - L[1]) - vy * (converted[0] - L[0]);
	c = c1;

	c1 = vx * (converted[2] - L[1]) - vy * (converted[1] - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	c1 = vx * (converted[3] - L[1]) - vy * (converted[0] - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	c1 = vx * (converted[3] - L[1]) - vy * (converted[1] - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	return 0;
}
int lanpr_triangle_covers_bounding_area(LANPR_RenderBuffer *fb, LANPR_RenderTriangle *rt, LANPR_BoundingArea *ba) {
	tnsVector2d p1, p2, p3, p4;
	real
	*FBC1 = rt->V[0]->FrameBufferCoord,
	*FBC2 = rt->V[1]->FrameBufferCoord,
	*FBC3 = rt->V[2]->FrameBufferCoord;

	p3[0] = p1[0] = (real)ba->L;
	p2[1] = p1[1] = (real)ba->B;
	p2[0] = p4[0] = (real)ba->R;
	p3[1] = p4[1] = (real)ba->U;

	if (FBC1[0] >= p1[0] && FBC1[0] <= p2[0] && FBC1[1] >= p1[1] && FBC1[1] <= p3[1]) return 1;
	if (FBC2[0] >= p1[0] && FBC2[0] <= p2[0] && FBC2[1] >= p1[1] && FBC2[1] <= p3[1]) return 1;
	if (FBC3[0] >= p1[0] && FBC3[0] <= p2[0] && FBC3[1] >= p1[1] && FBC3[1] <= p3[1]) return 1;

	if (lanpr_point_inside_triangled(p1, FBC1, FBC2, FBC3) ||
	    lanpr_point_inside_triangled(p2, FBC1, FBC2, FBC3) ||
	    lanpr_point_inside_triangled(p3, FBC1, FBC2, FBC3) ||
	    lanpr_point_inside_triangled(p4, FBC1, FBC2, FBC3)) return 1;

	if  (lanpr_line_crosses_bounding_area(fb, FBC1, FBC2, ba)) return 1;
	elif(lanpr_line_crosses_bounding_area(fb, FBC2, FBC3, ba)) return 1;
	elif(lanpr_line_crosses_bounding_area(fb, FBC3, FBC1, ba)) return 1;

	return 0;
}
void lanpr_link_triangle_with_bounding_area(LANPR_RenderBuffer *rb, LANPR_BoundingArea *RootBoundingArea, LANPR_RenderTriangle *rt, real *LRUB, int Recursive) {
	if (!lanpr_triangle_covers_bounding_area(rb, rt, RootBoundingArea)) return;
	if (!RootBoundingArea->Child) {
		list_append_pointer_static_pool(&rb->render_data_pool, &RootBoundingArea->LinkedTriangles, rt);
		RootBoundingArea->TriangleCount++;
		if (RootBoundingArea->TriangleCount > 200 && Recursive) {
			lanpr_split_bounding_area(rb, RootBoundingArea);
		}
		if (Recursive && rb->enable_intersections) lanpr_triangle_enable_intersections_in_bounding_area(rb, rt, RootBoundingArea);
	}
	else {
		LANPR_BoundingArea *ba = RootBoundingArea->Child;
		real *B1 = LRUB;
		real B[4];
		if (!LRUB) {
			B[0] = MIN3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
			B[1] = MAX3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
			B[2] = MAX3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);
			B[3] = MIN3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);
			B1 = B;
		}
		if (TNS_BOUND_AREA_CROSSES(B1, &ba[0].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[0], rt, B1, Recursive);
		if (TNS_BOUND_AREA_CROSSES(B1, &ba[1].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[1], rt, B1, Recursive);
		if (TNS_BOUND_AREA_CROSSES(B1, &ba[2].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[2], rt, B1, Recursive);
		if (TNS_BOUND_AREA_CROSSES(B1, &ba[3].L)) lanpr_link_triangle_with_bounding_area(rb, &ba[3], rt, B1, Recursive);
	}
}
void lanpr_link_line_with_bounding_area(LANPR_RenderBuffer *rb, LANPR_BoundingArea *RootBoundingArea, LANPR_RenderLine *rl) {
	list_append_pointer_static_pool(&rb->render_data_pool, &RootBoundingArea->LinkedLines, rl);
}
int lanpr_get_triangle_bounding_areas(LANPR_RenderBuffer *rb, LANPR_RenderTriangle *rt, int *rowBegin, int *rowEnd, int *colBegin, int *colEnd) {
	real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
	real B[4];

	if (!rt->V[0] || !rt->V[1] || !rt->V[2]) return 0;

	B[0] = MIN3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
	B[1] = MAX3(rt->V[0]->FrameBufferCoord[0], rt->V[1]->FrameBufferCoord[0], rt->V[2]->FrameBufferCoord[0]);
	B[2] = MIN3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);
	B[3] = MAX3(rt->V[0]->FrameBufferCoord[1], rt->V[1]->FrameBufferCoord[1], rt->V[2]->FrameBufferCoord[1]);

	if (B[0] > 1 || B[1] < -1 || B[2] > 1 || B[3] < -1) return 0;

	(*colBegin) = (int)((B[0] + 1.0) / sp_w);
	(*colEnd) = (int)((B[1] + 1.0) / sp_w);
	(*rowEnd) = rb->tile_count_y - (int)((B[2] + 1.0) / sp_h) - 1;
	(*rowBegin) = rb->tile_count_y - (int)((B[3] + 1.0) / sp_h) - 1;

	if ((*colEnd) >= rb->tile_count_x) (*colEnd) = rb->tile_count_x - 1;
	if ((*rowEnd) >= rb->tile_count_y) (*rowEnd) = rb->tile_count_y - 1;
	if ((*colBegin) < 0) (*colBegin) = 0;
	if ((*rowBegin) < 0) (*rowBegin) = 0;

	return 1;
}
int lanpr_get_line_bounding_areas(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl, int *rowBegin, int *rowEnd, int *colBegin, int *colEnd) {
	real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
	real B[4];

	if (!rl->L || !rl->R) return 0;

	if (rl->L->FrameBufferCoord[0] != rl->L->FrameBufferCoord[0] || rl->R->FrameBufferCoord[0] != rl->R->FrameBufferCoord[0]) return 0;

	B[0] = MIN2(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0]);
	B[1] = MAX2(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0]);
	B[2] = MIN2(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1]);
	B[3] = MAX2(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1]);

	if (B[0] > 1 || B[1] < -1 || B[2] > 1 || B[3] < -1) return 0;

	(*colBegin) = (int)((B[0] + 1.0) / sp_w);
	(*colEnd) = (int)((B[1] + 1.0) / sp_w);
	(*rowEnd) = rb->tile_count_y - (int)((B[2] + 1.0) / sp_h) - 1;
	(*rowBegin) = rb->tile_count_y - (int)((B[3] + 1.0) / sp_h) - 1;

	if ((*colEnd) >= rb->tile_count_x) (*colEnd) = rb->tile_count_x - 1;
	if ((*rowEnd) >= rb->tile_count_y) (*rowEnd) = rb->tile_count_y - 1;
	if ((*colBegin) < 0) (*colBegin) = 0;
	if ((*rowBegin) < 0) (*rowBegin) = 0;

	return 1;
}
LANPR_BoundingArea *lanpr_get_point_bounding_area(LANPR_RenderBuffer *rb, real x, real y) {
	real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
	int col, row;

	if (x > 1 || x < -1 || y > 1 || y < -1) return 0;

	col = (int)((x + 1.0) / sp_w);
	row = rb->tile_count_y - (int)((y + 1.0) / sp_h) - 1;

	if (col >= rb->tile_count_x) col = rb->tile_count_x - 1;
	if (row >= rb->tile_count_y) row = rb->tile_count_y - 1;
	if (col < 0) col = 0;
	if (row < 0) row = 0;

	return &rb->initial_bounding_areas[row * 4 + col];
}
void lanpr_add_triangles(LANPR_RenderBuffer *rb) {
	LANPR_RenderElementLinkNode *reln;
	LANPR_RenderTriangle *rt;
	//tnsMatrix44d VP;
	Camera *c = ((Camera *)rb->scene->camera);
	int i, lim;
	int x1, x2, y1, y2;
	int r, co;
	//tnsMatrix44d proj, view, result, inv;
	//tmat_make_perspective_matrix_44d(proj, c->FOV, (real)fb->W / (real)fb->H, c->clipsta, c->clipend);
	//tmat_load_identity_44d(view);
	//tObjApplyself_transformMatrix(c, 0);
	//tObjApplyGlobalTransformMatrixReverted(c);
	//tmat_inverse_44d(inv, c->Base.GlobalTransform);
	//tmat_multiply_44d(result, proj, inv);
	//memcpy(proj, result, sizeof(tnsMatrix44d));

	//tnsglobal_TriangleIntersectionCount = 0;

	//tnsset_RenderOverallProgress(rb, NUL_MH2);
	rb->calculation_status = TNS_CALCULATION_INTERSECTION;
	//nulThreadNotifyUsers("tns.render_buffer_list.calculation_status");

	for (reln = rb->triangle_buffer_pointers.first; reln; reln = reln->Item.next) {
		rt = reln->Pointer;
		lim = reln->ElementCount;
		for (i = 0; i < lim; i++) {
			if (rt->cull_status) {
				rt = (void *)(((BYTE *)rt) + rb->triangle_size); continue;
			}
			if (lanpr_get_triangle_bounding_areas(rb, rt, &y1, &y2, &x1, &x2)) {
				for (co = x1; co <= x2; co++) {
					for (r = y1; r <= y2; r++) {
						lanpr_link_triangle_with_bounding_area(rb, &rb->initial_bounding_areas[r * 4 + co], rt, 0, 1);
					}
				}
			}
			else {
				;//throw away.
			}
			rt = (void *)(((BYTE *)rt) + rb->triangle_size);
			//if (tnsglobal_TriangleIntersectionCount >= 2000) {
			//tnsset_PlusRenderIntersectionCount(rb, tnsglobal_TriangleIntersectionCount);
			//tnsglobal_TriangleIntersectionCount = 0;
			//}
		}
	}
	//tnsset_PlusRenderIntersectionCount(rb, tnsglobal_TriangleIntersectionCount);
}
LANPR_BoundingArea *lanpr_get_next_bounding_area(LANPR_BoundingArea *This, LANPR_RenderLine *rl, real x, real y, real k, int PositiveX, int PositiveY, real *NextX, real *NextY) {
	real rx, ry, ux, uy, lx, ly, bx, by;
	real r1, r2;
	LANPR_BoundingArea *ba; LinkData *lip;
	if (PositiveX > 0) {
		rx = This->R;
		ry = y + k * (rx - x);
		if (PositiveY > 0) {
			uy = This->U;
			ux = x + (uy - y) / k;
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], rx);
			r2 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], ux);
			if (MIN2(r1, r2) > 1) return 0;
			if (r1 <= r2) {
				for (lip = This->RP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->U >= ry && ba->B < ry) { *NextX = rx; *NextY = ry; return ba; }
				}
			}
			else {
				for (lip = This->UP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->R >= ux && ba->L < ux) { *NextX = ux; *NextY = uy; return ba; }
				}
			}
		}
		else if (PositiveY < 0) {
			by = This->B;
			bx = x + (by - y) / k;
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], rx);
			r2 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], bx);
			if (MIN2(r1, r2) > 1) return 0;
			if (r1 <= r2) {
				for (lip = This->RP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->U >= ry && ba->B < ry) { *NextX = rx; *NextY = ry; return ba; }
				}
			}
			else {
				for (lip = This->BP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->R >= bx && ba->L < bx) { *NextX = bx; *NextY = by; return ba; }
				}
			}
		}else { // Y diffence == 0
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], This->R);
			if (r1 > 1) return 0;
			for (lip = This->RP.first; lip; lip = lip->next) {
				ba = lip->data;
				if (ba->U >= y && ba->B < y) { *NextX = This->R; *NextY = y; return ba; }
			}
		}
	}else if (PositiveX < 0) { // X diffence < 0
		lx = This->L;
		ly = y + k * (lx - x);
		if (PositiveY > 0) {
			uy = This->U;
			ux = x + (uy - y) / k;
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], lx);
			r2 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], ux);
			if (MIN2(r1, r2) > 1) return 0;
			if (r1 <= r2) {
				for (lip = This->LP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->U >= ly && ba->B < ly) { *NextX = lx; *NextY = ly; return ba; }
				}
			}
			else {
				for (lip = This->UP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->R >= ux && ba->L < ux) { *NextX = ux; *NextY = uy; return ba; }
				}
			}
		}
		else if (PositiveY < 0) {
			by = This->B;
			bx = x + (by - y) / k;
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], lx);
			r2 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], bx);
			if (MIN2(r1, r2) > 1) return 0;
			if (r1 <= r2) {
				for (lip = This->LP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->U >= ly && ba->B < ly) { *NextX = lx; *NextY = ly; return ba; }
				}
			}
			else {
				for (lip = This->BP.first; lip; lip = lip->next) {
					ba = lip->data;
					if (ba->R >= bx && ba->L < bx) { *NextX = bx; *NextY = by; return ba; }
				}
			}
		}else { // Y diffence == 0
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], This->L);
			if (r1 > 1) return 0;
			for (lip = This->LP.first; lip; lip = lip->next) {
				ba = lip->data;
				if (ba->U >= y && ba->B < y) { *NextX = This->L; *NextY = y; return ba; }
			}
		}
	}else { // X difference == 0;
		if (PositiveY > 0) {
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], This->U);
			if (r1 > 1) return 0;
			for (lip = This->UP.first; lip; lip = lip->next) {
				ba = lip->data;
				if (ba->R > x && ba->L <= x) { *NextX = x; *NextY = This->U; return ba; }
			}
		}
		else if (PositiveY < 0) {
			r1 = tMatGetLinearRatio(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], This->B);
			if (r1 > 1) return 0;
			for (lip = This->BP.first; lip; lip = lip->next) {
				ba = lip->data;
				if (ba->R > x && ba->L <= x) { *NextX = x; *NextY = This->B; return ba; }
			}
		}
		else return 0; // segment has no length
	}
	return 0;
}

LANPR_BoundingArea *lanpr_get_bounding_area(LANPR_RenderBuffer *rb, real x, real y) {
	LANPR_BoundingArea *iba;
	real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;
	int c = (int)((x + 1.0) / sp_w);
	int r = rb->tile_count_y - (int)((y + 1.0) / sp_h) - 1;
	if (r < 0) r = 0;
	if (c < 0) c = 0;
	if (r >= rb->tile_count_y) r = rb->tile_count_y - 1;
	if (c >= rb->tile_count_x) c = rb->tile_count_x - 1;

	iba = &rb->initial_bounding_areas[r * 4 + c];
	while (iba->Child) {
		if (x > iba->CX) {
			if (y > iba->CY) iba = &iba->Child[0];
			else iba = &iba->Child[3];
		}
		else {
			if (y > iba->CY) iba = &iba->Child[1];
			else iba = &iba->Child[2];
		}
	}
	return iba;
}
LANPR_BoundingArea *lanpr_get_first_possible_bounding_area(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl) {
	LANPR_BoundingArea *iba;
	real data[2] = { rl->L->FrameBufferCoord[0], rl->L->FrameBufferCoord[1] };
	tnsVector2d LU = { -1, 1 }, RU = { 1, 1 }, LB = { -1, -1 }, RB = { 1, -1 };
	real r = 1, sr = 1;

	if (data[0] > -1 && data[0] < 1 && data[1] > -1 && data[1] < 1) {
		return lanpr_get_bounding_area(rb, data[0], data[1]);
	}
	else {
		if (lanpr_LineIntersectTest2d(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, LU, RU, &sr) && sr < r && sr > 0) r = sr;
		if (lanpr_LineIntersectTest2d(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, LB, RB, &sr) && sr < r && sr > 0) r = sr;
		if (lanpr_LineIntersectTest2d(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, LB, LU, &sr) && sr < r && sr > 0) r = sr;
		if (lanpr_LineIntersectTest2d(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, RB, RU, &sr) && sr < r && sr > 0) r = sr;
		lanpr_LinearInterpolate2dv(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, r, data);

		return lanpr_get_bounding_area(rb, data[0], data[1]);
	}

	real sp_w = rb->width_per_tile, sp_h = rb->height_per_tile;

	return iba;
}


/* ======================================= geometry ============================================ */

void lanpr_cut_render_line(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl, real Begin, real End) {
	LANPR_RenderLineSegment *rls = rl->Segments.first, *irls;
	LANPR_RenderLineSegment *begin_segment = 0, *end_segment = 0;
	LANPR_RenderLineSegment *ns = 0, *ns2 = 0;
	int untouched = 0;

	if (TNS_DOUBLE_CLOSE_ENOUGH(Begin, End)) return;

	if (Begin != Begin)
		Begin = 0;
	if (End != End)
		End = 0;

	if (Begin > End) {
		real t = Begin;
		Begin = End;
		End = t;
	}

	for (rls = rl->Segments.first; rls; rls = rls->Item.next) {
		if (TNS_DOUBLE_CLOSE_ENOUGH(rls->at, Begin)) {
			begin_segment = rls;
			ns = begin_segment;
			break;
		}
		if (!rls->Item.next) {
			break;
		}
		irls = rls->Item.next;
		if (irls->at > Begin + 1e-09 && Begin > rls->at) {
			begin_segment = irls;
			ns = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
			break;
		}
	}
	if (!begin_segment && TNS_DOUBLE_CLOSE_ENOUGH(1, End)) {
		untouched = 1;
	}
	for (rls = begin_segment; rls; rls = rls->Item.next) {
		if (TNS_DOUBLE_CLOSE_ENOUGH(rls->at, End)) {
			end_segment = rls;
			ns2 = end_segment;
			break;
		}
		//irls = rls->Item.next;
		//added this to prevent rls->at == 1.0 (we don't need an end point for this)
		if (!rls->Item.next && TNS_DOUBLE_CLOSE_ENOUGH(1, End)) {
			end_segment = rls;
			ns2 = end_segment;
			untouched = 1;
			break;
		} elif(rls->at > End)
		{
			end_segment = rls;
			ns2 = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
			break;
		}
	}

	if (!ns) ns = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
	if (!ns2) {
		if (untouched) { ns2 = ns; end_segment = ns2; }
		else ns2 = mem_static_aquire_thread(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
	}

	if (begin_segment) {
		if (begin_segment != ns) {
			ns->OcclusionLevel = begin_segment->Item.prev ? (irls = begin_segment->Item.prev)->OcclusionLevel : 0;
			list_insert_item_before(&rl->Segments, (void *)ns, (void *)begin_segment);
		}
	}
	else {
		ns->OcclusionLevel = (irls = rl->Segments.last)->OcclusionLevel;
		BLI_addtail(&rl->Segments, ns);
	}
	if (end_segment) {
		if (end_segment != ns2) {
			ns2->OcclusionLevel = end_segment->Item.prev ? (irls = end_segment->Item.prev)->OcclusionLevel : 0;
			list_insert_item_before(&rl->Segments, (void *)ns2, (void *)end_segment);
		}
	}
	else {
		ns2->OcclusionLevel = (irls = rl->Segments.last)->OcclusionLevel;
		BLI_addtail(&rl->Segments, ns2);
	}

	ns->at = Begin;
	if (!untouched) ns2->at = End;
	else ns2 = ns2->Item.next;

	for (rls = ns; rls && rls != ns2; rls = rls->Item.next) {
		rls->OcclusionLevel++;
	}
}


int lanpr_make_next_occlusion_task_info(LANPR_RenderBuffer *rb, LANPR_RenderTaskInfo *rti) {
	LinkData *data;
	int i;
	int res = 0;

	BLI_spin_lock(&rb->cs_management);

	if (rb->contour_managed) {
		data = rb->contour_managed;
		rti->contour = (void *)data;
		rti->contour_pointers.first = data;
		for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
			data = data->next;
		}
		rb->contour_managed = data;
		rti->contour_pointers.last = data ? data->prev : rb->contours.last;
		res = 1;
	}
	else {
		list_handle_empty(&rti->contour_pointers);
		rti->contour = 0;
	}

	if (rb->intersection_managed) {
		data = rb->intersection_managed;
		rti->intersection = (void *)data;
		rti->intersection_pointers.first = data;
		for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
			data = data->next;
		}
		rb->intersection_managed = data;
		rti->intersection_pointers.last = data ? data->prev : rb->intersection_lines.last;
		res = 1;
	}
	else {
		list_handle_empty(&rti->intersection_pointers);
		rti->intersection = 0;
	}

	if (rb->crease_managed) {
		data = rb->crease_managed;
		rti->crease = (void *)data;
		rti->crease_pointers.first = data;
		for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
			data = data->next;
		}
		rb->crease_managed = data;
		rti->crease_pointers.last = data ? data->prev : rb->crease_lines.last;
		res = 1;
	}
	else {
		list_handle_empty(&rti->crease_pointers);
		rti->crease = 0;
	}

	if (rb->material_managed) {
		data = rb->material_managed;
		rti->material = (void *)data;
		rti->material_pointers.first = data;
		for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
			data = data->next;
		}
		rb->material_managed = data;
		rti->material_pointers.last = data ? data->prev : rb->material_lines.last;
		res = 1;
	}
	else {
		list_handle_empty(&rti->material_pointers);
		rti->material = 0;
	}

	if (rb->edge_mark_managed) {
		data = rb->edge_mark_managed;
		rti->edge_mark = (void *)data;
		rti->edge_mark_pointers.first = data;
		for (i = 0; i < TNS_THREAD_LINE_COUNT && data; i++) {
			data = data->next;
		}
		rb->edge_mark_managed = data;
		rti->edge_mark_pointers.last = data ? data->prev : rb->edge_marks.last;
		res = 1;
	}
	else {
		list_handle_empty(&rti->edge_mark_pointers);
		rti->edge_mark = 0;
	}

	BLI_spin_unlock(&rb->cs_management);

	return res;
}
void lanpr_calculate_single_line_occlusion(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl, int thread_id) {
	real x = rl->L->FrameBufferCoord[0], y = rl->L->FrameBufferCoord[1];
	LANPR_BoundingArea *ba = lanpr_get_first_possible_bounding_area(rb, rl);
	LANPR_BoundingArea *nba = ba;
	LANPR_RenderTriangleThread *rt;
	LinkData *lip;
	Object *c = rb->scene->camera;
	real l, r;
	real k = (rl->R->FrameBufferCoord[1] - rl->L->FrameBufferCoord[1]) / (rl->R->FrameBufferCoord[0] - rl->L->FrameBufferCoord[0] + 1e-30);
	int PositiveX = (rl->R->FrameBufferCoord[0] - rl->L->FrameBufferCoord[0]) > 0 ? 1 : (rl->R->FrameBufferCoord[0] == rl->L->FrameBufferCoord[0] ? 0 : -1);
	int PositiveY = (rl->R->FrameBufferCoord[1] - rl->L->FrameBufferCoord[1]) > 0 ? 1 : (rl->R->FrameBufferCoord[1] == rl->L->FrameBufferCoord[1] ? 0 : -1);

	//printf("PX %d %lf   PY %d %lf\n", PositiveX, rl->R->FrameBufferCoord[0] - rl->L->FrameBufferCoord[0], PositiveY, rl->R->FrameBufferCoord[1] - rl->L->FrameBufferCoord[1]);

	while (nba) {


		for (lip = nba->LinkedTriangles.first; lip; lip = lip->next) {
			rt = lip->data;
			if (rt->testing[thread_id] == rl || rl->L->IntersectWith == (void *)rt || rl->R->IntersectWith == (void *)rt) continue;
			rt->testing[thread_id] = rl;
			if (lanpr_triangle_line_imagespace_intersection_v2(&rb->cs_management, (void *)rt, rl, c, rb->view_projection, rb->view_vector, &l, &r)) {
				lanpr_cut_render_line(rb, rl, l, r);
			}
		}


		nba = lanpr_get_next_bounding_area(nba, rl, x, y, k, PositiveX, PositiveY, &x, &y);
	}
}
void lanpr_THREAD_calculate_line_occlusion(TaskPool *__restrict pool, LANPR_RenderTaskInfo *rti, int threadid) {
	LANPR_RenderBuffer *rb = rti->render_buffer;
	int thread_id = rti->thread_id;
	LinkData *lip;
	int count = 0;

	while (lanpr_make_next_occlusion_task_info(rb, rti)) {

		for (lip = (void *)rti->contour; lip && lip->prev != rti->contour_pointers.last; lip = lip->next) {
			lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
		}

		for (lip = (void *)rti->crease; lip && lip->prev != rti->crease_pointers.last; lip = lip->next) {
			lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
		}

		for (lip = (void *)rti->intersection; lip && lip->prev != rti->intersection_pointers.last; lip = lip->next) {
			lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
		}

		for (lip = (void *)rti->material; lip && lip->prev != rti->material_pointers.last; lip = lip->next) {
			lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
		}

		for (lip = (void *)rti->edge_mark; lip && lip->prev != rti->edge_mark_pointers.last; lip = lip->next) {
			lanpr_calculate_single_line_occlusion(rb, lip->data, rti->thread_id);
		}
	}
}
void lanpr_THREAD_calculate_line_occlusion_begin(LANPR_RenderBuffer *rb) {
	int thread_count = rb->thread_count;
	LANPR_RenderTaskInfo *rti = MEM_callocN(sizeof(LANPR_RenderTaskInfo) * thread_count, "render task info");
	TaskScheduler *scheduler = BLI_task_scheduler_get();
	int i;

	rb->contour_managed = rb->contours.first;
	rb->crease_managed = rb->crease_lines.first;
	rb->intersection_managed = rb->intersection_lines.first;
	rb->material_managed = rb->material_lines.first;
	rb->edge_mark_managed = rb->edge_marks.first;

	TaskPool *tp = BLI_task_pool_create(scheduler, 0);

	for (i = 0; i < thread_count; i++) {
		rti[i].thread_id = i;
		rti[i].render_buffer = rb;
		BLI_task_pool_push(tp, lanpr_THREAD_calculate_line_occlusion, &rti[i], 0, TASK_PRIORITY_HIGH);
	}
	BLI_task_pool_work_and_wait(tp);

	MEM_freeN(rti);
}

void lanpr_NO_THREAD_calculate_line_occlusion(LANPR_RenderBuffer *rb) {
	LinkData *lip;

	for (lip = rb->contours.first; lip; lip = lip->next) {
		lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
	}

	for (lip = rb->crease_lines.first; lip; lip = lip->next) {
		lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
	}

	for (lip = rb->intersection_lines.first; lip; lip = lip->next) {
		lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
	}

	for (lip = rb->material_lines.first; lip; lip = lip->next) {
		lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
	}

	for (lip = rb->edge_marks.first; lip; lip = lip->next) {
		lanpr_calculate_single_line_occlusion(rb, lip->data, 0);
	}
}


int lanpr_get_normal(tnsVector3d v1, tnsVector3d v2, tnsVector3d v3, tnsVector3d n, tnsVector3d Pos) {
	tnsVector3d vec1, vec2;

	tMatVectorMinus3d(vec1, v2, v1);
	tMatVectorMinus3d(vec2, v3, v1);
	tmat_vector_cross_3d(n, vec1, vec2);
	tmat_normalize_self_3d(n);
	if (Pos && (tmat_dot_3d(n, Pos, 1) < 0)) {
		tMatVectorMultiSelf3d(n, -1.0f);
		return 1;
	}
	return 0;
}

int lanpr_bound_box_crosses(tnsVector4d xxyy1, tnsVector4d xxyy2) {
	real XMax1 = MAX2(xxyy1[0], xxyy1[1]);
	real XMin1 = MIN2(xxyy1[0], xxyy1[1]);
	real YMax1 = MAX2(xxyy1[2], xxyy1[3]);
	real YMin1 = MIN2(xxyy1[2], xxyy1[3]);
	real XMax2 = MAX2(xxyy2[0], xxyy2[1]);
	real XMin2 = MIN2(xxyy2[0], xxyy2[1]);
	real YMax2 = MAX2(xxyy2[2], xxyy2[3]);
	real YMin2 = MIN2(xxyy2[2], xxyy2[3]);

	if (XMax1 < XMin2 || XMin1 > XMax2) return 0;
	if (YMax1 < YMin2 || YMin1 > YMax2) return 0;

	return 1;
}
int lanpr_point_inside_triangled(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2) {
	double cl, c;

	cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
	c = cl;

	cl = (v1[0] - v[0]) * (v2[1] - v[1]) - (v1[1] - v[1]) * (v2[0] - v[0]);
	if (c * cl <= 0) return 0;
	else c = cl;

	cl = (v2[0] - v[0]) * (v0[1] - v[1]) - (v2[1] - v[1]) * (v0[0] - v[0]);
	if (c * cl <= 0) return 0;
	else c = cl;

	cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
	if (c * cl <= 0) return 0;

	return 1;
}
int lanpr_point_on_lined(tnsVector2d v, tnsVector2d v0, tnsVector2d v1) {
	real c1, c2;

	c1 = tMatGetLinearRatio(v0[0], v1[0], v[0]);
	c2 = tMatGetLinearRatio(v0[1], v1[1], v[1]);

	if (TNS_DOUBLE_CLOSE_ENOUGH(c1, c2) && c1 >= 0 && c1 <= 1) return 1;

	return 0;
}
int lanpr_point_triangle_relation(tnsVector2d v, tnsVector2d v0, tnsVector2d v1, tnsVector2d v2) {
	double cl, c;
	real r;
	if (lanpr_point_on_lined(v, v0, v1) || lanpr_point_on_lined(v, v1, v2) || lanpr_point_on_lined(v, v2, v0)) return 1;

	cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
	c = cl;

	cl = (v1[0] - v[0]) * (v2[1] - v[1]) - (v1[1] - v[1]) * (v2[0] - v[0]);
	if ((r = c * cl) < 0) return 0;
	//elif(r == 0) return 1; // removed, point could still be on the extention line of some edge
	else c = cl;

	cl = (v2[0] - v[0]) * (v0[1] - v[1]) - (v2[1] - v[1]) * (v0[0] - v[0]);
	if ((r = c * cl) < 0) return 0;
	//elif(r == 0) return 1;
	else c = cl;

	cl = (v0[0] - v[0]) * (v1[1] - v[1]) - (v0[1] - v[1]) * (v1[0] - v[0]);
	if ((r = c * cl) < 0) return 0;
	elif(r == 0) return 1;

	return 2;
}
int lanpr_point_inside_triangle3d(tnsVector3d v, tnsVector3d v0, tnsVector3d v1, tnsVector3d v2) {
	tnsVector3d L, R;
	tnsVector3d N1, N2;

	tMatVectorMinus3d(L, v1, v0);
	tMatVectorMinus3d(R, v, v1);
	tmat_vector_cross_3d(N1, L, R);

	tMatVectorMinus3d(L, v2, v1);
	tMatVectorMinus3d(R, v, v2);
	tmat_vector_cross_3d(N2, L, R);

	if (tmat_dot_3d(N1, N2, 0) < 0) return 0;

	tMatVectorMinus3d(L, v0, v2);
	tMatVectorMinus3d(R, v, v0);
	tmat_vector_cross_3d(N1, L, R);

	if (tmat_dot_3d(N1, N2, 0) < 0) return 0;

	tMatVectorMinus3d(L, v1, v0);
	tMatVectorMinus3d(R, v, v1);
	tmat_vector_cross_3d(N2, L, R);

	if (tmat_dot_3d(N1, N2, 0) < 0) return 0;

	return 1;
}
int lanpr_point_inside_triangle3de(tnsVector3d v, tnsVector3d v0, tnsVector3d v1, tnsVector3d v2) {
	tnsVector3d L, R;
	tnsVector3d N1, N2;
	real d;

	tMatVectorMinus3d(L, v1, v0);
	tMatVectorMinus3d(R, v, v1);
	//tmat_normalize_self_3d(L);
	//tmat_normalize_self_3d(R);
	tmat_vector_cross_3d(N1, L, R);

	tMatVectorMinus3d(L, v2, v1);
	tMatVectorMinus3d(R, v, v2);
	//tmat_normalize_self_3d(L);
	//tmat_normalize_self_3d(R);
	tmat_vector_cross_3d(N2, L, R);

	if ((d = tmat_dot_3d(N1, N2, 0)) < 0) return 0;
	//if (d<DBL_EPSILON) return -1;

	tMatVectorMinus3d(L, v0, v2);
	tMatVectorMinus3d(R, v, v0);
	//tmat_normalize_self_3d(L);
	//tmat_normalize_self_3d(R);
	tmat_vector_cross_3d(N1, L, R);

	if ((d = tmat_dot_3d(N1, N2, 0)) < 0) return 0;
	//if (d<DBL_EPSILON) return -1;

	tMatVectorMinus3d(L, v1, v0);
	tMatVectorMinus3d(R, v, v1);
	//tmat_normalize_self_3d(L);
	//tmat_normalize_self_3d(R);
	tmat_vector_cross_3d(N2, L, R);

	if ((d = tmat_dot_3d(N1, N2, 0)) < 0) return 0;
	//if (d<DBL_EPSILON) return -1;

	return 1;
}

LANPR_RenderElementLinkNode *lanpr_new_cull_triangle_space64(LANPR_RenderBuffer *rb) {
	LANPR_RenderElementLinkNode *reln;

	LANPR_RenderTriangle *RenderTriangles = MEM_callocN(64 * rb->triangle_size, "render triangle space");//CreateNewBuffer(LANPR_RenderTriangle, 64);

	reln = list_append_pointer_static_sized(&rb->triangle_buffer_pointers, &rb->render_data_pool, RenderTriangles,
	                                        sizeof(LANPR_RenderElementLinkNode));
	reln->ElementCount = 64;
	reln->Additional = 1;

	return reln;
}
LANPR_RenderElementLinkNode *lanpr_new_cull_point_space64(LANPR_RenderBuffer *rb) {
	LANPR_RenderElementLinkNode *reln;

	LANPR_RenderVert *RenderVertices = MEM_callocN(sizeof(LANPR_RenderVert) * 64, "render vert space");//CreateNewBuffer(LANPR_RenderVert, 64);

	reln = list_append_pointer_static_sized(&rb->vertex_buffer_pointers, &rb->render_data_pool, RenderVertices,
	                                        sizeof(LANPR_RenderElementLinkNode));
	reln->ElementCount = 64;
	reln->Additional = 1;

	return reln;
}
void lanpr_calculate_render_triangle_normal(LANPR_RenderTriangle *rt);
void lanpr_assign_render_line_with_triangle(LANPR_RenderTriangle *rt) {
	if (!rt->RL[0]->TL)
		rt->RL[0]->TL = rt;
	elif(!rt->RL[0]->TR)
	rt->RL[0]->TR = rt;

	if (!rt->RL[1]->TL)
		rt->RL[1]->TL = rt;
	elif(!rt->RL[1]->TR)
	rt->RL[1]->TR = rt;

	if (!rt->RL[2]->TL)
		rt->RL[2]->TL = rt;
	elif(!rt->RL[2]->TR)
	rt->RL[2]->TR = rt;
}
void lanpr_post_triangle(LANPR_RenderTriangle *rt, LANPR_RenderTriangle *orig) {
	if (rt->V[0]) tMatVectorAccum3d(rt->gc, rt->V[0]->FrameBufferCoord);
	if (rt->V[1]) tMatVectorAccum3d(rt->gc, rt->V[1]->FrameBufferCoord);
	if (rt->V[2]) tMatVectorAccum3d(rt->gc, rt->V[2]->FrameBufferCoord);
	tMatVectorMultiSelf3d(rt->gc, 1.0f / 3.0f);

	tMatVectorCopy3d(orig->gn, rt->gn);
}

#define RT_AT(head, rb, offset) \
	((BYTE *)head + offset * rb->triangle_size)

void lanpr_cull_triangles(LANPR_RenderBuffer *rb) {
	LANPR_RenderLine *rl;
	LANPR_RenderTriangle *rt, *rt1, *rt2;
	LANPR_RenderVert *rv;
	LANPR_RenderElementLinkNode *reln, *veln, *teln;
	LANPR_RenderLineSegment *rls;
	real *mv_inverse = rb->vp_inverse;
	real *vp = rb->view_projection;
	int i;
	real a;
	int v_count = 0, t_count = 0;
	Object *o;

	real cam_pos[3];
	Object* cam = ((Object *)rb->scene->camera);
	cam_pos[0] = cam->obmat[3][0];
	cam_pos[1] = cam->obmat[3][1];
	cam_pos[2] = cam->obmat[3][2];

	real view_dir[3], clip_advance[3];
	tMatVectorCopy3d(rb->view_vector,view_dir);
	tMatVectorCopy3d(rb->view_vector,clip_advance);
	tMatVectorMultiSelf3d(clip_advance, -((Camera*)cam->data)->clip_start);
	tMatVectorAccum3d(cam_pos, clip_advance);

	veln = lanpr_new_cull_point_space64(rb);
	teln = lanpr_new_cull_triangle_space64(rb);
	rv = &((LANPR_RenderVert *)veln->Pointer)[v_count];
	rt1 = (void *)(((BYTE *)teln->Pointer) + rb->triangle_size * t_count);

	for (reln = rb->triangle_buffer_pointers.first; reln; reln = reln->Item.next) {
		i = 0;
		if (reln->Additional) continue;
		o = reln->ObjectRef;
		for (i; i < reln->ElementCount; i++) {
			int In1 = 0, In2 = 0, In3 = 0;
			rt = (void *)(((BYTE *)reln->Pointer) + rb->triangle_size * i);
			if (rt->V[0]->FrameBufferCoord[3] < 0) In1 = 1;
			if (rt->V[1]->FrameBufferCoord[3] < 0) In2 = 1;
			if (rt->V[2]->FrameBufferCoord[3] < 0) In3 = 1;

			rt->RL[0]->ObjectRef = o;
			rt->RL[1]->ObjectRef = o;
			rt->RL[2]->ObjectRef = o;

			if (v_count > 60) {
				veln->ElementCount = v_count;
				veln = lanpr_new_cull_point_space64(rb);
				v_count = 0;
			}

			if (t_count > 60) {
				teln->ElementCount = t_count;
				teln = lanpr_new_cull_triangle_space64(rb);
				t_count = 0;
			}

			//if ((!rt->RL[0]->Item.next && !rt->RL[0]->Item.prev) ||
			//    (!rt->RL[1]->Item.next && !rt->RL[1]->Item.prev) ||
			//    (!rt->RL[2]->Item.next && !rt->RL[2]->Item.prev)) {
			//	printf("'"); // means this triangle is lonely????
			//}

			rv = &((LANPR_RenderVert *)veln->Pointer)[v_count];
			rt1 = (void *)(((BYTE *)teln->Pointer) + rb->triangle_size * t_count);
			rt2 = (void *)(((BYTE *)teln->Pointer) + rb->triangle_size * (t_count + 1));

			real vv1[3], vv2[3], dot1, dot2;

			switch (In1 + In2 + In3) {
				case 0:
					continue;
				case 3:
					rt->cull_status = TNS_CULL_DISCARD;
					BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
					BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;
					BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;
					continue;
				case 2:
					rt->cull_status = TNS_CULL_USED;
					if (!In1) {
						tMatVectorMinus3d(vv1,rt->V[0]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[2]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[0]->GLocation[0] + a * rt->V[2]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[0]->GLocation[1] + a * rt->V[2]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[0]->GLocation[2] + a * rt->V[2]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[0]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[1]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[0]->GLocation[0] + a * rt->V[1]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[0]->GLocation[1] + a * rt->V[1]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[0]->GLocation[2] + a * rt->V[1]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = &rv[0];
						rl->TL = rt1;
						rt1->RL[1] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = rt->V[0];
						rl->TL = rt->RL[0]->TL == rt ? rt1 : rt->RL[0]->TL;
						rl->TR = rt->RL[0]->TR == rt ? rt1 : rt->RL[0]->TR;
						rt1->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[0];
						rl->R = &rv[0];
						rl->TL = rt->RL[2]->TL == rt ? rt1 : rt->RL[2]->TL;
						rl->TR = rt->RL[2]->TR == rt ? rt1 : rt->RL[2]->TR;
						rt1->RL[2] = rl;

						rt1->V[0] = rt->V[0];
						rt1->V[1] = &rv[1];
						rt1->V[2] = &rv[0];

						lanpr_post_triangle(rt1, rt);

						v_count += 2;
						t_count += 1;
						continue;
					} elif(!In3) {
						tMatVectorMinus3d(vv1,rt->V[2]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[0]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[2]->GLocation[0] + a * rt->V[0]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[2]->GLocation[1] + a * rt->V[0]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[2]->GLocation[2] + a * rt->V[0]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[2]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[1]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[2]->GLocation[0] + a * rt->V[1]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[2]->GLocation[1] + a * rt->V[1]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[2]->GLocation[2] + a * rt->V[1]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[0];
						rl->R = &rv[1];
						rl->TL = rt1;
						rt1->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = rt->V[2];
						rl->TL = rt->RL[1]->TL == rt ? rt1 : rt->RL[1]->TL;
						rl->TR = rt->RL[1]->TR == rt ? rt1 : rt->RL[1]->TR;
						rt1->RL[1] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[2];
						rl->R = &rv[0];
						rl->TL = rt->RL[2]->TL == rt ? rt1 : rt->RL[2]->TL;
						rl->TR = rt->RL[2]->TR == rt ? rt1 : rt->RL[2]->TR;
						rt1->RL[2] = rl;

						rt1->V[0] = &rv[1];
						rt1->V[1] = rt->V[2];
						rt1->V[2] = &rv[0];

						lanpr_post_triangle(rt1, rt);

						v_count += 2;
						t_count += 1;
						continue;
					} elif(!In2)
					{
						tMatVectorMinus3d(vv1,rt->V[1]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[2]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[1]->GLocation[0] + a * rt->V[2]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[1]->GLocation[1] + a * rt->V[2]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[1]->GLocation[2] + a * rt->V[2]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[1]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[0]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[1]->GLocation[0] + a * rt->V[0]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[1]->GLocation[1] + a * rt->V[0]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[1]->GLocation[2] + a * rt->V[0]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = &rv[0];
						rl->TL = rt1;
						rt1->RL[2] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[0];
						rl->R = rt->V[1];
						rl->TL = rt->RL[0]->TL == rt ? rt1 : rt->RL[0]->TL;
						rl->TR = rt->RL[0]->TR == rt ? rt1 : rt->RL[0]->TR;
						rt1->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[1];
						rl->R = &rv[1];
						rl->TL = rt->RL[1]->TL == rt ? rt1 : rt->RL[1]->TL;
						rl->TR = rt->RL[1]->TR == rt ? rt1 : rt->RL[1]->TR;
						rt1->RL[1] = rl;

						rt1->V[0] = rt->V[1];
						rt1->V[1] = &rv[1];
						rt1->V[2] = &rv[0];

						lanpr_post_triangle(rt1, rt);

						v_count += 2;
						t_count += 1;
						continue;
					}
					break;
				case 1:
					rt->cull_status = TNS_CULL_USED;
					if (In1) {
						tMatVectorMinus3d(vv1,rt->V[0]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[2]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[0]->GLocation[0] + a * rt->V[2]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[0]->GLocation[1] + a * rt->V[2]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[0]->GLocation[2] + a * rt->V[2]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[0]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[1]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[0]->GLocation[0] + a * rt->V[1]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[0]->GLocation[1] + a * rt->V[1]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[0]->GLocation[2] + a * rt->V[1]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = &rv[0];
						rl->TL = rt1;
						rt1->RL[1] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[0];
						rl->R = rt->V[1];
						rl->TL = rt1;
						rl->TR = rt2;
						rt1->RL[2] = rl;
						rt2->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[1];
						rl->R = &rv[1];
						rl->TL = rt->RL[0]->TL == rt ? rt1 : rt->RL[0]->TL;
						rl->TR = rt->RL[0]->TR == rt ? rt1 : rt->RL[0]->TR;
						rt1->RL[0] = rl;

						rt1->V[0] = rt->V[1];
						rt1->V[1] = &rv[1];
						rt1->V[2] = &rv[0];

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[2];
						rl->R = &rv[0];
						rl->TL = rt->RL[2]->TL == rt ? rt1 : rt->RL[2]->TL;
						rl->TR = rt->RL[2]->TR == rt ? rt1 : rt->RL[2]->TR;
						rt2->RL[2] = rl;
						rt2->RL[1] = rt->RL[1];

						rt2->V[0] = &rv[0];
						rt2->V[1] = rt->V[1];
						rt2->V[2] = rt->V[2];

						lanpr_post_triangle(rt1, rt);
						lanpr_post_triangle(rt2, rt);

						v_count += 2;
						t_count += 2;
						continue;
					} elif(In2)	{

						tMatVectorMinus3d(vv1,rt->V[1]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[2]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[1]->GLocation[0] + a * rt->V[2]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[1]->GLocation[1] + a * rt->V[2]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[1]->GLocation[2] + a * rt->V[2]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[1]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[0]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[1]->GLocation[0] + a * rt->V[0]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[1]->GLocation[1] + a * rt->V[0]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[1]->GLocation[2] + a * rt->V[0]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[0]); rt->RL[0]->Item.next = rt->RL[0]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = &rv[0];
						rl->TL = rt1;
						rt1->RL[1] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[0];
						rl->R = rt->V[2];
						rl->TL = rt1;
						rl->TR = rt2;
						rt1->RL[2] = rl;
						rt2->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[2];
						rl->R = &rv[1];
						rl->TL = rt->RL[1]->TL == rt ? rt1 : rt->RL[1]->TL;
						rl->TR = rt->RL[1]->TR == rt ? rt1 : rt->RL[1]->TR;
						rt1->RL[0] = rl;

						rt1->V[0] = rt->V[2];
						rt1->V[1] = &rv[1];
						rt1->V[2] = &rv[0];

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[0];
						rl->R = &rv[0];
						rl->TL = rt->RL[0]->TL == rt ? rt1 : rt->RL[0]->TL;
						rl->TR = rt->RL[0]->TR == rt ? rt1 : rt->RL[0]->TR;
						rt2->RL[2] = rl;
						rt2->RL[1] = rt->RL[2];

						rt2->V[0] = &rv[0];
						rt2->V[1] = rt->V[2];
						rt2->V[2] = rt->V[0];

						lanpr_post_triangle(rt1, rt);
						lanpr_post_triangle(rt2, rt);

						v_count += 2;
						t_count += 2;
						continue;
					} elif(In3) {

						tMatVectorMinus3d(vv1,rt->V[2]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[0]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[0].GLocation[0] = (1-a) * rt->V[2]->GLocation[0] + a * rt->V[0]->GLocation[0];
						rv[0].GLocation[1] = (1-a) * rt->V[2]->GLocation[1] + a * rt->V[0]->GLocation[1];
						rv[0].GLocation[2] = (1-a) * rt->V[2]->GLocation[2] + a * rt->V[0]->GLocation[2];
						tmat_apply_transform_44d(rv[0].FrameBufferCoord,vp,rv[0].GLocation);

						tMatVectorMinus3d(vv1,rt->V[2]->GLocation,cam_pos);
						tMatVectorMinus3d(vv2,cam_pos,rt->V[1]->GLocation);
						dot1 = tmat_dot_3d(vv1,view_dir,0);
						dot2 = tmat_dot_3d(vv2,view_dir,0);
						a = dot1/(dot1+dot2);
						rv[1].GLocation[0] = (1-a) * rt->V[2]->GLocation[0] + a * rt->V[1]->GLocation[0];
						rv[1].GLocation[1] = (1-a) * rt->V[2]->GLocation[1] + a * rt->V[1]->GLocation[1];
						rv[1].GLocation[2] = (1-a) * rt->V[2]->GLocation[2] + a * rt->V[1]->GLocation[2];
						tmat_apply_transform_44d(rv[1].FrameBufferCoord,vp,rv[1].GLocation);

						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[1]); rt->RL[1]->Item.next = rt->RL[1]->Item.prev = 0;
						BLI_remlink(&rb->all_render_lines, (void *)rt->RL[2]); rt->RL[2]->Item.next = rt->RL[2]->Item.prev = 0;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[1];
						rl->R = &rv[0];
						rl->TL = rt1;
						rt1->RL[1] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = &rv[0];
						rl->R = rt->V[0];
						rl->TL = rt1;
						rl->TR = rt2;
						rt1->RL[2] = rl;
						rt2->RL[0] = rl;

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[0];
						rl->R = &rv[1];
						rl->TL = rt->RL[0]->TL == rt ? rt1 : rt->RL[0]->TL;
						rl->TR = rt->RL[0]->TR == rt ? rt1 : rt->RL[0]->TR;
						rt1->RL[0] = rl;

						rt1->V[0] = rt->V[0];
						rt1->V[1] = &rv[1];
						rt1->V[2] = &rv[0];

						rl = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
						rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
						BLI_addtail(&rl->Segments, rls);
						BLI_addtail(&rb->all_render_lines, rl);
						rl->L = rt->V[1];
						rl->R = &rv[0];
						rl->TL = rt->RL[1]->TL == rt ? rt1 : rt->RL[1]->TL;
						rl->TR = rt->RL[1]->TR == rt ? rt1 : rt->RL[1]->TR;
						rt2->RL[2] = rl;
						rt2->RL[1] = rt->RL[0];

						rt2->V[0] = &rv[0];
						rt2->V[1] = rt->V[0];
						rt2->V[2] = rt->V[1];

						lanpr_post_triangle(rt1, rt);
						lanpr_post_triangle(rt2, rt);

						v_count += 2;
						t_count += 2;
						continue;
					}
					break;
			}
		}
		teln->ElementCount = t_count;
		veln->ElementCount = v_count;
	}
}
void lanpr_perspective_division(LANPR_RenderBuffer *rb) {
	LANPR_RenderVert *rv;
	LANPR_RenderElementLinkNode *reln;
	Camera *cam = rb->scene->camera->data;
	int i;

	if (cam->type != CAM_PERSP) return;

	for (reln = rb->vertex_buffer_pointers.first; reln; reln = reln->Item.next) {
		rv = reln->Pointer;
		for (i = 0; i < reln->ElementCount; i++) {
			//if (rv->FrameBufferCoord[2] < -DBL_EPSILON) continue;
			tMatVectorMultiSelf3d(rv[i].FrameBufferCoord, 1 / rv[i].FrameBufferCoord[3]);
			//rv[i].FrameBufferCoord[2] = cam->clipsta * cam->clipend / (cam->clipend - fabs(rv[i].FrameBufferCoord[2]) * (cam->clipend - cam->clipsta));
		}
	}
}

void lanpr_transform_render_vert(BMVert *V, int index, LANPR_RenderVert *RVBuf, real *MVMat, real *MVPMat, Camera *Camera) {//real HeightMultiply, real clipsta, real clipend) {
	LANPR_RenderVert *rv = &RVBuf[index];
	//rv->V = V;
	//V->RV = rv;
	tmat_apply_transform_43df(rv->GLocation, MVMat, V->co);
	tmat_apply_transform_43dfND(rv->FrameBufferCoord, MVPMat, V->co);

	//if(rv->FrameBufferCoord[2]>0)tMatVectorMultiSelf3d(rv->FrameBufferCoord, (1 / rv->FrameBufferCoord[3]));
	//else tMatVectorMultiSelf3d(rv->FrameBufferCoord, -rv->FrameBufferCoord[3]);
	//   rv->FrameBufferCoord[2] = Camera->clipsta* Camera->clipend / (Camera->clipend - fabs(rv->FrameBufferCoord[2]) * (Camera->clipend - Camera->clipsta));
}
void lanpr_calculate_render_triangle_normal(LANPR_RenderTriangle *rt) {
	tnsVector3d L, R;
	tMatVectorMinus3d(L, rt->V[1]->GLocation, rt->V[0]->GLocation);
	tMatVectorMinus3d(R, rt->V[2]->GLocation, rt->V[0]->GLocation);
	tmat_vector_cross_3d(rt->gn, L, R);
	tmat_normalize_self_3d(rt->gn);
}
void lanpr_make_render_geometry_buffers_object(Object *o, real *MVMat, real *MVPMat, LANPR_RenderBuffer *rb) {
	Object *oc;
	Mesh *mo;
	BMesh *bm;
	BMVert *v;
	BMFace *f;
	BMEdge *e;
	BMLoop *loop;
	LANPR_RenderLine *rl;
	LANPR_RenderTriangle *rt;
	tnsMatrix44d new_mvp;
	tnsMatrix44d new_mv;
	tnsMatrix44d self_transform;
	tnsMatrix44d Normal;
	LANPR_RenderElementLinkNode *reln;
	Object *cam_object = rb->scene->camera;
	Camera *c = cam_object->data;
	Material *m;
	LANPR_RenderVert *orv;
	LANPR_RenderLine *orl;
	LANPR_RenderTriangle *ort;
	FreestyleEdge *fe;
	int CanFindFreestyle = 0;
	int i;


	if (o->type == OB_MESH) {

		tmat_obmat_to_16d(o->obmat, self_transform);

		tmat_multiply_44d(new_mvp, MVPMat, self_transform);
		tmat_multiply_44d(new_mv, MVMat, self_transform);

		invert_m4_m4(o->imat, o->obmat);
		transpose_m4(o->imat);
		tmat_obmat_to_16d(o->imat, Normal);


		const BMAllocTemplate allocsize = BMALLOC_TEMPLATE_FROM_ME(((Mesh *)(o->data)));
		bm = BM_mesh_create(&allocsize,
		                    &((struct BMeshCreateParams) {.use_toolflags = true, }));
		BM_mesh_bm_from_me(bm, o->data, &((struct BMeshFromMeshParams) {.calc_face_normal = true, }));
		BM_mesh_elem_hflag_disable_all(bm, BM_FACE | BM_EDGE, BM_ELEM_TAG, false);
		BM_mesh_triangulate(bm, MOD_TRIANGULATE_QUAD_BEAUTY, MOD_TRIANGULATE_NGON_BEAUTY, 4, false, NULL, NULL, NULL);
		BM_mesh_normals_update(bm);
		BM_mesh_elem_table_ensure(bm, BM_VERT | BM_EDGE | BM_FACE);
		BM_mesh_elem_index_ensure(bm, BM_VERT | BM_EDGE | BM_FACE);

		if (CustomData_has_layer(&bm->edata, CD_FREESTYLE_EDGE)) {
			CanFindFreestyle = 1;
		}

		orv = MEM_callocN(sizeof(LANPR_RenderVert) * bm->totvert, "object render verts");
		ort = MEM_callocN(bm->totface * rb->triangle_size, "object render triangles");//CreateNewBuffer(LANPR_RenderTriangle, mo->TriangleCount);
		orl = MEM_callocN(sizeof(LANPR_RenderLine) * bm->totedge, "object render edge");

		reln = list_append_pointer_static_sized(&rb->vertex_buffer_pointers, &rb->render_data_pool, orv,
		                                        sizeof(LANPR_RenderElementLinkNode));
		reln->ElementCount = bm->totvert;
		reln->ObjectRef = o;

		reln = list_append_pointer_static_sized(&rb->line_buffer_pointers, &rb->render_data_pool, orl,
		                                        sizeof(LANPR_RenderElementLinkNode));
		reln->ElementCount = bm->totedge;
		reln->ObjectRef = o;

		reln = list_append_pointer_static_sized(&rb->triangle_buffer_pointers, &rb->render_data_pool, ort,
		                                        sizeof(LANPR_RenderElementLinkNode));
		reln->ElementCount = bm->totface;
		reln->ObjectRef = o;

		for (i = 0; i < bm->totvert; i++) {
			v = BM_vert_at_index(bm, i);
			lanpr_transform_render_vert(v, i, orv, new_mv, new_mvp, c);
		}

		rl = orl;
		for (i = 0; i < bm->totedge; i++) {
			e = BM_edge_at_index(bm, i);
			if (CanFindFreestyle) {
				fe = CustomData_bmesh_get(&bm->edata, e->head.data, CD_FREESTYLE_EDGE);
				if (fe->flag & FREESTYLE_EDGE_MARK) rl->Flags |= LANPR_EDGE_FLAG_EDGE_MARK;
			}
			if (use_smooth_contour_modifier_contour) {
				if (BM_elem_flag_test(e->v1, BM_ELEM_SELECT) && BM_elem_flag_test(e->v2, BM_ELEM_SELECT))
					rl->Flags |= LANPR_EDGE_FLAG_CONTOUR;
			}

			rl->L = &orv[BM_elem_index_get(e->v1)];
			rl->R = &orv[BM_elem_index_get(e->v2)];
			LANPR_RenderLineSegment *rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
			BLI_addtail(&rl->Segments, rls);
			BLI_addtail(&rb->all_render_lines, rl);
			rl++;
		}

		rt = ort;
		for (i = 0; i < bm->totface; i++) {
			f = BM_face_at_index(bm, i);

			loop = f->l_first;
			rt->V[0] = &orv[BM_elem_index_get(loop->v)];
			rt->RL[0] = &orl[BM_elem_index_get(loop->e)];
			loop = loop->next;
			rt->V[1] = &orv[BM_elem_index_get(loop->v)];
			rt->RL[1] = &orl[BM_elem_index_get(loop->e)];
			loop = loop->next;
			rt->V[2] = &orv[BM_elem_index_get(loop->v)];
			rt->RL[2] = &orl[BM_elem_index_get(loop->e)];

			rt->material_id = f->mat_nr;
			rt->gn[0] = f->no[0];
			rt->gn[1] = f->no[1];
			rt->gn[2] = f->no[2];

			tMatVectorAccum3d(rt->gc, rt->V[0]->FrameBufferCoord);
			tMatVectorAccum3d(rt->gc, rt->V[1]->FrameBufferCoord);
			tMatVectorAccum3d(rt->gc, rt->V[2]->FrameBufferCoord);
			tMatVectorMultiSelf3d(rt->gc, 1.0f / 3.0f);
			tmat_apply_normal_transform_43df(rt->gn, Normal, f->no);
			tmat_normalize_self_3d(rt->gn);
			lanpr_assign_render_line_with_triangle(rt);
			//m = tnsGetIndexedMaterial(rb->scene, f->material_id);
			//if(m) m->Previewv_count += (f->TriangleCount*3);

			if (BM_elem_flag_test(f, BM_ELEM_SELECT))
				rt->material_id = 1;

			rt = (LANPR_RenderTriangle *)(((unsigned char *)rt) + rb->triangle_size);
		}

		BM_mesh_free(bm);

	}
}
void lanpr_make_render_geometry_buffers(Depsgraph *depsgraph, Scene *s, Object *c /*camera*/, LANPR_RenderBuffer *rb) {
	Object *o;
	Collection *collection;
	CollectionObject *co;
	tnsMatrix44d obmat16;
	tnsMatrix44d proj, view, result, inv;
	Camera *cam = c->data;

	float sensor = BKE_camera_sensor_size(cam->sensor_fit, cam->sensor_x, cam->sensor_y);
	real fov = focallength_to_fov(cam->lens, sensor);

	memset(rb->material_pointers, 0, sizeof(void *) * 2048);

	real asp = ((real)rb->w / (real)rb->h);

	if (cam->type == CAM_PERSP) {
		tmat_make_perspective_matrix_44d(proj, fov, asp, cam->clip_start, cam->clip_end);
	} elif(cam->type == CAM_ORTHO)
	{
		real w = cam->ortho_scale/2;
		tmat_make_ortho_matrix_44d(proj, -w, w, -w / asp, w / asp, cam->clip_start, cam->clip_end);
	}

	tmat_load_identity_44d(view);

	//tObjApplyself_transformMatrix(c, 0);
	//tObjApplyGlobalTransformMatrixReverted(c);
	tmat_obmat_to_16d(c->obmat, obmat16);
	tmat_inverse_44d(inv, obmat16);
	tmat_multiply_44d(result, proj, inv);
	memcpy(proj, result, sizeof(tnsMatrix44d));
	memcpy(rb->view_projection, proj, sizeof(tnsMatrix44d));

	tmat_inverse_44d(rb->vp_inverse, rb->view_projection);

	void *a;
	while (a = list_pop_pointer(&rb->triangle_buffer_pointers)) FreeMem(a);
	while (a = list_pop_pointer(&rb->vertex_buffer_pointers)) FreeMem(a);

	DEG_OBJECT_ITER_BEGIN(
		depsgraph, o,
		DEG_ITER_OBJECT_FLAG_LINKED_DIRECTLY |
		DEG_ITER_OBJECT_FLAG_VISIBLE |
		DEG_ITER_OBJECT_FLAG_DUPLI |
		DEG_ITER_OBJECT_FLAG_LINKED_VIA_SET)
	{
		lanpr_make_render_geometry_buffers_object(o, view, proj, rb);
	}
	DEG_OBJECT_ITER_END;

	//for (collection = s->master_collection.first; collection; collection = collection->ID.next) {
	//	for (co = collection->gobject.first; co; co = co->next) {
	//		//tObjApplyGlobalTransformMatrixRecursive(o);
	//		lanpr_make_render_geometry_buffers_object(o, view, proj, rb);
	//	}
	//}
}



#define INTERSECT_SORT_MIN_TO_MAX_3(ia, ib, ic, lst) \
	{ \
		lst[0] = TNS_MIN3_INDEX(ia, ib, ic); \
		lst[1] = ( ((ia <= ib && ib <= ic) || (ic <= ib && ib <= ia)) ? 1 : (((ic <= ia && ia <= ib) || (ib < ia && ia <= ic)) ? 0 : 2)); \
		lst[2] = TNS_MAX3_INDEX(ia, ib, ic); \
	}

//ia ib ic are ordered
#define INTERSECT_JUST_GREATER(is, order, num, index) \
	{ \
		index = (num < is[order[0]] ? order[0] : (num < is[order[1]] ? order[1] : (num < is[order[2]] ? order[2] : order[2]))); \
	}

//ia ib ic are ordered
#define INTERSECT_JUST_SMALLER(is, order, num, index) \
	{ \
		index = (num > is[order[2]] ? order[2] : (num > is[order[1]] ? order[1] : (num > is[order[0]] ? order[0] : order[0]))); \
	}

void lanpr_get_interpolate_point2d(tnsVector2d L, tnsVector2d R, real Ratio, tnsVector2d Result) {
	Result[0] = tnsLinearItp(L[0], R[0], Ratio);
	Result[1] = tnsLinearItp(L[1], R[1], Ratio);
}
void lanpr_get_interpolate_point3d(tnsVector2d L, tnsVector2d R, real Ratio, tnsVector2d Result) {
	Result[0] = tnsLinearItp(L[0], R[0], Ratio);
	Result[1] = tnsLinearItp(L[1], R[1], Ratio);
	Result[2] = tnsLinearItp(L[2], R[2], Ratio);
}

int lanpr_get_z_inersect_point(tnsVector3d TL, tnsVector3d TR, tnsVector3d LL, tnsVector3d LR, tnsVector3d IntersectResult) {
	//real lzl = 1 / LL[2], lzr = 1 / LR[2], tzl = 1 / TL[2], tzr = 1 / TR[2];
	real lzl = LL[2], lzr = LR[2], tzl = TL[2], tzr = TR[2];
	real l = tzl - lzl, r = tzr - lzr;
	real ratio;
	int rev = l < 0 ? -1 : 1;//-1:occlude left 1:occlude right

	if (l * r >= 0) {
		if (l == 0) IntersectResult[2] = r > 0 ? -1 : 1;
		else if (r == 0) IntersectResult[2] = l > 0 ? -1 : 1;
		else
			IntersectResult[2] = rev;
		return 0;
	}
	l = fabsf(l);
	r = fabsf(r);
	ratio = l / (l + r);

	IntersectResult[2] = lanpr_LinearInterpolate(lzl, lzr, ratio);
	lanpr_get_interpolate_point2d(LL, LR, ratio, IntersectResult);

	return rev;
}
LANPR_RenderVert *lanpr_find_shaerd_vertex(LANPR_RenderLine *rl, LANPR_RenderTriangle *rt) {
	if (rl->L == rt->V[0] || rl->L == rt->V[1] || rl->L == rt->V[2]) return rl->L;
	elif(rl->R == rt->V[0] || rl->R == rt->V[1] || rl->R == rt->V[2]) return rl->R;
	else return 0;
}
void lanpr_find_edge_no_vertex(LANPR_RenderTriangle *rt, LANPR_RenderVert *rv, tnsVector3d L, tnsVector3d R) {
	if (rt->V[0] == rv) {
		tMatVectorCopy3d(rt->V[1]->FrameBufferCoord, L);
		tMatVectorCopy3d(rt->V[2]->FrameBufferCoord, R);
	} elif(rt->V[1] == rv)
	{
		tMatVectorCopy3d(rt->V[2]->FrameBufferCoord, L);
		tMatVectorCopy3d(rt->V[0]->FrameBufferCoord, R);
	} elif(rt->V[2] == rv)
	{
		tMatVectorCopy3d(rt->V[0]->FrameBufferCoord, L);
		tMatVectorCopy3d(rt->V[1]->FrameBufferCoord, R);
	}
}
LANPR_RenderLine *lanpr_another_edge(LANPR_RenderTriangle *rt, LANPR_RenderVert *rv) {
	if (rt->V[0] == rv) {
		return rt->RL[1];
	} elif(rt->V[1] == rv)
	{
		return rt->RL[2];
	} elif(rt->V[2] == rv)
	{
		return rt->RL[0];
	}
	return 0;
}

int lanpr_share_edge(LANPR_RenderTriangle *rt, LANPR_RenderVert *rv, LANPR_RenderLine *rl) {
	LANPR_RenderVert *another = rv == rl->L ? rl->R : rl->L;

	if (rt->V[0] == rv) {
		if (another == rt->V[1] || another == rt->V[2]) return 1;
		return 0;
	} elif(rt->V[1] == rv)
	{
		if (another == rt->V[0] || another == rt->V[2]) return 1;
		return 0;
	} elif(rt->V[2] == rv)
	{
		if (another == rt->V[0] || another == rt->V[1]) return 1;
		return 0;
	}
	return 0;
}
int lanpr_share_edge_direct(LANPR_RenderTriangle *rt, LANPR_RenderLine *rl) {
	if (rt->RL[0] == rl || rt->RL[1] == rl || rt->RL[2] == rl)
		return 1;
	return 0;
}
int lanpr_TriangleLineImageSpaceIntersectTestOnly(LANPR_RenderTriangle *rt, LANPR_RenderLine *rl, double *From, double *To) {
	double dl, dr;
	double ratio;
	double is[3];
	int order[3];
	int LCross, RCross;
	int a, b, c;
	int ret;
	int noCross = 0;
	tnsVector3d TL, TR, LL, LR;
	tnsVector3d IntersectResult;
	LANPR_RenderVert *Share;
	int StL = 0, StR = 0;
	int OccludeSide;

	double
	*LFBC = rl->L->FrameBufferCoord,
	*RFBC = rl->R->FrameBufferCoord,
	*FBC0 = rt->V[0]->FrameBufferCoord,
	*FBC1 = rt->V[1]->FrameBufferCoord,
	*FBC2 = rt->V[2]->FrameBufferCoord;

	//bound box.
	if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX2(LFBC[2], RFBC[2])) return 0;
	if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN2(LFBC[0], RFBC[0])) return 0;
	if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX2(LFBC[0], RFBC[0])) return 0;
	if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN2(LFBC[1], RFBC[1])) return 0;
	if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX2(LFBC[1], RFBC[1])) return 0;

	if (Share = lanpr_find_shaerd_vertex(rl, rt)) {
		tnsVector3d CL, CR;
		double r;
		int status;
		double r2;

		//if (rl->IgnoreConnectedFace/* && lanpr_share_edge(rt, Share, rl)*/)
		//return 0;

		lanpr_find_edge_no_vertex(rt, Share, CL, CR);
		status = lanpr_LineIntersectTest2d(LFBC, RFBC, CL, CR, &r);

		//LL[2] = 1 / tnsLinearItp(1 / LFBC[2], 1 / RFBC[2], r);
		LL[0] = tnsLinearItp(LFBC[0], RFBC[0], r);
		LL[1] = tnsLinearItp(LFBC[1], RFBC[1], r);
		LL[2] = tnsLinearItp(LFBC[2], RFBC[2], r);

		r2 = lanpr_GetLinearRatio(CL, CR, LL);
		//LR[2] = 1 / tnsLinearItp(1 / CL[2], 1 / CR[2], r2);
		LR[0] = tnsLinearItp(CL[0], CR[0], r2);
		LR[1] = tnsLinearItp(CL[1], CR[1], r2);
		LR[2] = tnsLinearItp(CL[2], CR[2], r2);


		if (LL[2] <= (LR[2] + 0.000000001)) return 0;

		StL = lanpr_point_inside_triangled(LFBC, FBC0, FBC1, FBC2);
		StR = lanpr_point_inside_triangled(RFBC, FBC0, FBC1, FBC2);

		if ((StL && Share == rl->R) ||
		    (StR && Share == rl->L)) {
			*From = 0;
			*To = 1;
			return 1;
		}

		if (!status) return 0;

		if (rl->L == Share) {
			*From = 0;
			*To = r;
		}
		else {
			*From = r;
			*To = 1;
		}
		return 1;

	}

	a = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC0, FBC1, &is[0]);
	b = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC1, FBC2, &is[1]);
	c = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC2, FBC0, &is[2]);

	if (!a && !b && !c) {
		if (!(StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2)) &&
		    !(StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2))) {
			return 0;//not occluding
		}
	}

	StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2);
	StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2);

	INTERSECT_SORT_MIN_TO_MAX_3(is[0], is[1], is[2], order);

	if (StL) {
		INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
		INTERSECT_JUST_GREATER(is, order, -DBL_TRIANGLE_LIM, RCross);
		//if (is[LCross]>=0 || is[RCross] >= 1) return 0;
	} elif(StR)
	{
		INTERSECT_JUST_SMALLER(is, order, 1.0f + DBL_TRIANGLE_LIM, LCross);
		INTERSECT_JUST_GREATER(is, order, 1.0f - DBL_TRIANGLE_LIM, RCross);
		//if (is[LCross] <= 0 || is[RCross] <= 1) return 0;
	}
	else {
		if (a) {
			if (b) {
				LCross = is[0] < is[1] ? 0 : 1;
				RCross = is[0] < is[1] ? 1 : 0;
			}
			else {
				LCross = is[0] < is[2] ? 0 : 2;
				RCross = is[0] < is[2] ? 2 : 0;
			}
		} elif(c)
		{
			LCross = is[1] < is[2] ? 1 : 2;
			RCross = is[1] < is[2] ? 2 : 1;
		}
		else {
			return 0;
		}
		//if (rl->IgnoreConnectedFace/* && lanpr_share_edge(rt, Share, rl)*/)
		//	return 0;
		if (MAX3(FBC0[2], FBC1[2], FBC2[2]) < (MIN2(LFBC[2], RFBC[2]) - 0.000001)) {
			*From = is[LCross];
			*To = is[RCross];
			TNS_CLAMP((*From), 0, 1);
			TNS_CLAMP((*To), 0, 1);
			return 1;
		}
	}

	LL[2] = lanpr_GetLineZ(LFBC, RFBC, is[LCross]);
	LR[2] = lanpr_GetLineZ(LFBC, RFBC, is[RCross]);
	lanpr_get_interpolate_point2d(LFBC, RFBC, is[LCross], LL);
	lanpr_get_interpolate_point2d(LFBC, RFBC, is[RCross], LR);

	TL[2] = lanpr_GetLineZPoint(rt->V[LCross]->FrameBufferCoord, rt->V[(LCross > 1 ? 0 : (LCross + 1))]->FrameBufferCoord, LL);
	TR[2] = lanpr_GetLineZPoint(rt->V[RCross]->FrameBufferCoord, rt->V[(RCross > 1 ? 0 : (RCross + 1))]->FrameBufferCoord, LR);
	tMatVectorCopy2d(LL, TL);
	tMatVectorCopy2d(LR, TR);

	if (OccludeSide = lanpr_get_z_inersect_point(TL, TR, LL, LR, IntersectResult)) {
		real r = lanpr_GetLinearRatio(LFBC, RFBC, IntersectResult);
		if (OccludeSide > 0) {
			if (r > 1 /*|| r < 0*/) return 0;
			*From = MAX2(r, 0);
			*To = MIN2(is[RCross], 1);
		}
		else {
			if (r < 0 /*|| r > 1*/) return 0;
			*From = MAX2(is[LCross], 0);
			*To = MIN2(r, 1);
		}
		//*From = TNS_MAX2(TNS_MAX2(r, is[LCross]), 0);
		//*To = TNS_MIN2(r, TNS_MIN2(is[RCross], 1));
	} elif(IntersectResult[2] < 0)
	{
		if ((!StL) && (!StR) && (a + b + c < 2) || is[LCross] > is[RCross]) return 0;
		*From = is[LCross];
		*To = is[RCross];
	}
	else return 0;

	TNS_CLAMP((*From), 0, 1);
	TNS_CLAMP((*To), 0, 1);

	//if ((TNS_FLOAT_CLOSE_ENOUGH(*From, 0) && TNS_FLOAT_CLOSE_ENOUGH(*To, 1)) ||
	//	(TNS_FLOAT_CLOSE_ENOUGH(*To, 0) && TNS_FLOAT_CLOSE_ENOUGH(*From, 1)) ||
	//	TNS_FLOAT_CLOSE_ENOUGH(*From, *To)) return 0;

	return 1;
}
int lanpr_triangle_line_imagespace_intersection_v2(SpinLock *spl, LANPR_RenderTriangle *rt, LANPR_RenderLine *rl, Object *cam, tnsMatrix44d vp, real *CameraDir, double *From, double *To) {
	double dl, dr;
	double ratio;
	double is[3] = { 0 };
	int order[3];
	int LCross = -1, RCross = -1;
	int a, b, c;
	int ret;
	int noCross = 0;
	tnsVector3d IntersectResult;
	LANPR_RenderVert *Share;
	int StL = 0, StR = 0, In;
	int OccludeSide;

	tnsVector3d LV;
	tnsVector3d RV;
	tnsVector4d vd4;
	real CV[3];
	real DotL, DotR, DotLA, DotRA;
	real DotF;
	LANPR_RenderVert *Result, *rv;
	tnsVector4d GLocation, Trans;
	real Cut = -1;
	int NextCut, NextCut1;
	int status;


	double
	*LFBC = rl->L->FrameBufferCoord,
	*RFBC = rl->R->FrameBufferCoord,
	*FBC0 = rt->V[0]->FrameBufferCoord,
	*FBC1 = rt->V[1]->FrameBufferCoord,
	*FBC2 = rt->V[2]->FrameBufferCoord;

	//printf("(%f %f)(%f %f)(%f %f)   (%f %f)(%f %f)\n", FBC0[0], FBC0[1], FBC1[0], FBC1[1], FBC2[0], FBC2[1], LFBC[0], LFBC[1], RFBC[0], RFBC[1]);

	//bound box.
	//if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX2(LFBC[2], RFBC[2]))
	//	return 0;
	if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN2(LFBC[0], RFBC[0])) return 0;
	if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX2(LFBC[0], RFBC[0])) return 0;
	if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN2(LFBC[1], RFBC[1])) return 0;
	if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX2(LFBC[1], RFBC[1])) return 0;

	if (lanpr_share_edge_direct(rt, rl))
		return 0;

	a = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC0, FBC1, &is[0]);
	b = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC1, FBC2, &is[1]);
	c = lanpr_LineIntersectTest2d(LFBC, RFBC, FBC2, FBC0, &is[2]);

	//printf("abc: %d %d %d\n", a,b,c);

	INTERSECT_SORT_MIN_TO_MAX_3(is[0], is[1], is[2], order);

	tMatVectorMinus3d(LV, rl->L->GLocation, rt->V[0]->GLocation);
	tMatVectorMinus3d(RV, rl->R->GLocation, rt->V[0]->GLocation);

	tMatVectorCopy3d(CameraDir,CV);

	tMatVectorConvert4fd(cam->obmat[3], vd4);
	if (((Camera *)cam->data)->type == CAM_PERSP) tMatVectorMinus3d(CV, vd4, rt->V[0]->GLocation);

	DotL = tmat_dot_3d(LV, rt->gn, 0);
	DotR = tmat_dot_3d(RV, rt->gn, 0);
	DotF = tmat_dot_3d(CV, rt->gn, 0);

	if (!DotF) return 0;


	if (!a && !b && !c) {
		if (!(StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2)) &&
		    !(StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2))) {
			return 0;//not occluding
		}
	}

	StL = lanpr_point_triangle_relation(LFBC, FBC0, FBC1, FBC2);
	StR = lanpr_point_triangle_relation(RFBC, FBC0, FBC1, FBC2);


	//for (rv = rt->intersecting_verts.first; rv; rv = rv->Item.next) {
	//	if (rv->IntersectWith == rt && rv->IntersectingLine == rl) {
	//		Cut = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], rv->FrameBufferCoord[0]);
	//		break;
	//	}
	//}


	DotLA = fabs(DotL); if (DotLA < DBL_EPSILON) { DotLA = 0; DotL = 0; }
	DotRA = fabs(DotR); if (DotRA < DBL_EPSILON) { DotRA = 0; DotR = 0; }
	if (DotL - DotR == 0) Cut = 100000;
	else if (DotL * DotR <= 0) {
		Cut = DotLA / fabs(DotL - DotR);
	}
	else {
		Cut = fabs(DotR + DotL) / fabs(DotL - DotR);
		Cut = DotRA > DotLA ? 1 - Cut : Cut;
	}

	if (((Camera *)cam->data)->type == CAM_PERSP) {
		lanpr_LinearInterpolate3dv(rl->L->GLocation, rl->R->GLocation, Cut, GLocation);
		tmat_apply_transform_44d(Trans, vp, GLocation);
		tMatVectorMultiSelf3d(Trans, (1 / Trans[3]) /**HeightMultiply/2*/);
	}
	else {
		lanpr_LinearInterpolate3dv(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord, Cut, Trans);
		//tmat_apply_transform_44d(Trans, vp, GLocation);
	}

	//Trans[2] = tmat_dist_3dv(GLocation, cam->Base.GLocation);
	//Trans[2] = cam->clipsta*cam->clipend / (cam->clipend - fabs(Trans[2]) * (cam->clipend - cam->clipsta));

	//prevent vertical problem ?
	if(rl->L->FrameBufferCoord[0] != rl->R->FrameBufferCoord[0])
		Cut = tMatGetLinearRatio(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], Trans[0]);
	else
		Cut = tMatGetLinearRatio(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], Trans[1]);

	In = lanpr_point_inside_triangled(Trans, rt->V[0]->FrameBufferCoord, rt->V[1]->FrameBufferCoord, rt->V[2]->FrameBufferCoord);


	if (StL == 2) {
		if (StR == 2) {
			INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
		} elif(StR == 1)
		{
			INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
		} elif(StR == 0)
		{
			INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 0, RCross);
		}
	} elif(StL == 1)
	{
		if (StR == 2) {
			INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
		} elif(StR == 1)
		{
			INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
		} elif(StR == 0)
		{
			INTERSECT_JUST_GREATER(is, order, DBL_TRIANGLE_LIM, RCross);
			if (TNS_ABC(RCross) && is[RCross] > (DBL_TRIANGLE_LIM)) {
				INTERSECT_JUST_SMALLER(is, order, DBL_TRIANGLE_LIM, LCross);
			}
			else {
				INTERSECT_JUST_SMALLER(is, order, -DBL_TRIANGLE_LIM, LCross);
				INTERSECT_JUST_GREATER(is, order, -DBL_TRIANGLE_LIM, RCross);
			}
		}
	} elif(StL == 0)
	{
		if (StR == 2) {
			INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
			INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
		} elif(StR == 1)
		{
			INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
			if (TNS_ABC(LCross) && is[LCross] < (1 - DBL_TRIANGLE_LIM)) {
				INTERSECT_JUST_GREATER(is, order, 1 - DBL_TRIANGLE_LIM, RCross);
			}
			else {
				INTERSECT_JUST_SMALLER(is, order, 1 + DBL_TRIANGLE_LIM, LCross);
				INTERSECT_JUST_GREATER(is, order, 1 + DBL_TRIANGLE_LIM, RCross);
			}
		} elif(StR == 0)
		{
			INTERSECT_JUST_GREATER(is, order, 0, LCross);
			if (TNS_ABC(LCross) && is[LCross] > 0) {
				INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
			}
			else {
				INTERSECT_JUST_GREATER(is, order, is[LCross], LCross);
				INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
			}
		}
	}


	real LF = DotL * DotF, RF = DotR * DotF;
	//int CrossCount = a + b + c;
	//if (CrossCount == 2) {
	//	INTERSECT_JUST_GREATER(is, order, 0, LCross);
	//	if (!TNS_ABC(LCross)) INTERSECT_JUST_GREATER(is, order, is[LCross], LCross);
	//	INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
	//}elif(CrossCount == 1 || StL+StR==1) {
	//	if (StL) {
	//		INTERSECT_JUST_GREATER(is, order, DBL_TRIANGLE_LIM, RCross);
	//		INTERSECT_JUST_SMALLER(is, order, is[RCross], LCross);
	//	}elif(StR) {
	//		INTERSECT_JUST_SMALLER(is, order, 1 - DBL_TRIANGLE_LIM, LCross);
	//		INTERSECT_JUST_GREATER(is, order, is[LCross], RCross);
	//	}
	//}elif(CrossCount == 0) {
	//	INTERSECT_JUST_SMALLER(is, order, 0, LCross);
	//	INTERSECT_JUST_GREATER(is, order, 1, RCross);
	//}

	if (LF <= 0 && RF <= 0 && (DotL || DotR)) {

		*From = MAX2(0, is[LCross]);
		*To = MIN2(1, is[RCross]);
		if (*From >= *To) return 0;
		//printf("1 From %f to %f\n",*From, *To);
		return 1;
	} elif(LF >= 0 && RF <= 0 && (DotL || DotR))
	{
		*From = MAX2(Cut, is[LCross]);
		*To = MIN2(1, is[RCross]);
		if (*From >= *To) return 0;
		//printf("2 From %f to %f\n",*From, *To);
		return 1;
	} elif(LF <= 0 && RF >= 0 && (DotL || DotR))
	{
		*From = MAX2(0, is[LCross]);
		*To = MIN2(Cut, is[RCross]);
		if (*From >= *To) return 0;
		//printf("3 From %f to %f\n",*From, *To);
		return 1;
	}
	else
		return 0;
	return 1;
}

LANPR_RenderLine *lanpr_triangle_share_edge(LANPR_RenderTriangle *l, LANPR_RenderTriangle *r) {
	if (l->RL[0] == r->RL[0]) return r->RL[0];
	if (l->RL[0] == r->RL[1]) return r->RL[1];
	if (l->RL[0] == r->RL[2]) return r->RL[2];
	if (l->RL[1] == r->RL[0]) return r->RL[0];
	if (l->RL[1] == r->RL[1]) return r->RL[1];
	if (l->RL[1] == r->RL[2]) return r->RL[2];
	if (l->RL[2] == r->RL[0]) return r->RL[0];
	if (l->RL[2] == r->RL[1]) return r->RL[1];
	if (l->RL[2] == r->RL[2]) return r->RL[2];
	return 0;
}
LANPR_RenderVert *lanpr_triangle_share_point(LANPR_RenderTriangle *l, LANPR_RenderTriangle *r) {
	if (l->V[0] == r->V[0]) return r->V[0];
	if (l->V[0] == r->V[1]) return r->V[1];
	if (l->V[0] == r->V[2]) return r->V[2];
	if (l->V[1] == r->V[0]) return r->V[0];
	if (l->V[1] == r->V[1]) return r->V[1];
	if (l->V[1] == r->V[2]) return r->V[2];
	if (l->V[2] == r->V[0]) return r->V[0];
	if (l->V[2] == r->V[1]) return r->V[1];
	if (l->V[2] == r->V[2]) return r->V[2];
	return 0;
}

LANPR_RenderVert *lanpr_triangle_line_intersection_test(LANPR_RenderBuffer *rb, LANPR_RenderLine *rl, LANPR_RenderTriangle *rt, LANPR_RenderTriangle *testing, LANPR_RenderVert *Last) {
	tnsVector3d LV;
	tnsVector3d RV;
	real DotL, DotR;
	LANPR_RenderVert *Result, *rv;
	tnsVector3d GLocation;
	LANPR_RenderVert *L = rl->L, *R = rl->R;
	int result;

	int i;

	for (rv = testing->intersecting_verts.first; rv; rv = rv->Item.next) {
		if (rv->IntersectWith == rt && rv->IntersectingLine == rl)
			return rv;
	}


	tMatVectorMinus3d(LV, L->GLocation, testing->V[0]->GLocation);
	tMatVectorMinus3d(RV, R->GLocation, testing->V[0]->GLocation);

	DotL = tmat_dot_3d(LV, testing->gn, 0);
	DotR = tmat_dot_3d(RV, testing->gn, 0);

	if (DotL * DotR > 0 || (!DotL && !DotR))
		return 0;

	DotL = fabs(DotL);
	DotR = fabs(DotR);

	lanpr_LinearInterpolate3dv(L->GLocation, R->GLocation, DotL / (DotL + DotR), GLocation);

	if (Last && TNS_DOUBLE_CLOSE_ENOUGH(Last->GLocation[0], GLocation[0])
	    && TNS_DOUBLE_CLOSE_ENOUGH(Last->GLocation[1], GLocation[1])
	    && TNS_DOUBLE_CLOSE_ENOUGH(Last->GLocation[2], GLocation[2])) {

		Last->IntersectintLine2 = rl;
		return 0;
	}

	if (!(result = lanpr_point_inside_triangle3de(GLocation, testing->V[0]->GLocation, testing->V[1]->GLocation, testing->V[2]->GLocation)))
		return 0;
	/*elif(result < 0) {
	   return 0;
	   }*/

	Result = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderVert));

	if (DotL > 0 || DotR < 0) Result->Positive = 1; else Result->Positive = 0;

	//Result->IntersectingOnFace = testing;
	Result->EdgeUsed = 1;
	//Result->IntersectL = L;
	Result->V = (void *)R; //Caution!
	                       //Result->IntersectWith = rt;
	tMatVectorCopy3d(GLocation, Result->GLocation);

	BLI_addtail(&testing->intersecting_verts, Result);

	return Result;
}
LANPR_RenderLine *lanpr_triangle_generate_intersection_line_only(LANPR_RenderBuffer *rb, LANPR_RenderTriangle *rt, LANPR_RenderTriangle *testing) {
	LANPR_RenderVert *L = 0, *R = 0;
	LANPR_RenderVert **Next = &L;
	LANPR_RenderLine *Result;
	LANPR_RenderVert *E0T = 0;
	LANPR_RenderVert *E1T = 0;
	LANPR_RenderVert *E2T = 0;
	LANPR_RenderVert *TE0 = 0;
	LANPR_RenderVert *TE1 = 0;
	LANPR_RenderVert *TE2 = 0;
	tnsVector3d cl;// rb->scene->ActiveCamera->GLocation;
	real ZMax = ((Camera *)rb->scene->camera->data)->clip_end;
	real ZMin = ((Camera *)rb->scene->camera->data)->clip_start;
	LANPR_RenderVert *Share = lanpr_triangle_share_point(testing, rt);

	tMatVectorConvert3fd(rb->scene->camera->obmat[3], cl);

	if (Share) {
		LANPR_RenderVert *NewShare;
		LANPR_RenderLine *rl = lanpr_another_edge(rt, Share);

		L = NewShare = mem_static_aquire(&rb->render_data_pool, (sizeof(LANPR_RenderVert)));

		NewShare->Positive = 1;
		NewShare->EdgeUsed = 1;
		//NewShare->IntersectL = L;
		NewShare->V = (void *)R; //Caution!
		//Result->IntersectWith = rt;
		tMatVectorCopy3d(Share->GLocation, NewShare->GLocation);

		R = lanpr_triangle_line_intersection_test(rb, rl, rt, testing, 0);

		if (!R) {
			rl = lanpr_another_edge(testing, Share);
			R = lanpr_triangle_line_intersection_test(rb, rl, testing, rt, 0);
			if (!R) return 0;
			BLI_addtail(&testing->intersecting_verts, NewShare);
		}
		else {
			BLI_addtail(&rt->intersecting_verts, NewShare);
		}

	}
	else {
		if (!rt->RL[0] || !rt->RL[1] || !rt->RL[2]) return 0; // shouldn't need this, there must be problems in culling.
		E0T = lanpr_triangle_line_intersection_test(rb, rt->RL[0], rt, testing, 0); if (E0T && (!(*Next))) { (*Next) = E0T; (*Next)->IntersectingLine = rt->RL[0];  Next = &R; }
		E1T = lanpr_triangle_line_intersection_test(rb, rt->RL[1], rt, testing, L); if (E1T && (!(*Next))) { (*Next) = E1T; (*Next)->IntersectingLine = rt->RL[1];  Next = &R; }
		if (!(*Next)) E2T = lanpr_triangle_line_intersection_test(rb, rt->RL[2], rt, testing, L); if (E2T && (!(*Next))) { (*Next) = E2T; (*Next)->IntersectingLine = rt->RL[2];  Next = &R; }

		if (!(*Next)) TE0 = lanpr_triangle_line_intersection_test(rb, testing->RL[0], testing, rt, L); if (TE0 && (!(*Next))) { (*Next) = TE0; (*Next)->IntersectingLine = testing->RL[0]; Next = &R; }
		if (!(*Next)) TE1 = lanpr_triangle_line_intersection_test(rb, testing->RL[1], testing, rt, L); if (TE1 && (!(*Next))) { (*Next) = TE1; (*Next)->IntersectingLine = testing->RL[1]; Next = &R; }
		if (!(*Next)) TE2 = lanpr_triangle_line_intersection_test(rb, testing->RL[2], testing, rt, L); if (TE2 && (!(*Next))) { (*Next) = TE2; (*Next)->IntersectingLine = testing->RL[2]; Next = &R; }

		if (!(*Next)) return 0;
	}
	tmat_apply_transform_44d(L->FrameBufferCoord, rb->view_projection, L->GLocation);
	tmat_apply_transform_44d(R->FrameBufferCoord, rb->view_projection, R->GLocation);
	tMatVectorMultiSelf3d(L->FrameBufferCoord, (1 / L->FrameBufferCoord[3]) /**HeightMultiply/2*/);
	tMatVectorMultiSelf3d(R->FrameBufferCoord, (1 / R->FrameBufferCoord[3]) /**HeightMultiply/2*/);

	L->FrameBufferCoord[2] = ZMin * ZMax / (ZMax - fabs(L->FrameBufferCoord[2]) * (ZMax - ZMin));
	R->FrameBufferCoord[2] = ZMin * ZMax / (ZMax - fabs(R->FrameBufferCoord[2]) * (ZMax - ZMin));

	L->IntersectWith = rt;
	R->IntersectWith = testing;

	///*((1 / rl->L->FrameBufferCoord[3])*rb->FrameBuffer->H / 2)

	Result = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLine));
	Result->L = L;
	Result->R = R;
	Result->TL = rt;
	Result->TR = testing;
	LANPR_RenderLineSegment *rls = mem_static_aquire(&rb->render_data_pool, sizeof(LANPR_RenderLineSegment));
	BLI_addtail(&Result->Segments, rls);
	BLI_addtail(&rb->all_render_lines, Result);
	Result->Flags |= LANPR_EDGE_FLAG_INTERSECTION;
	list_append_pointer_static(&rb->intersection_lines, &rb->render_data_pool, Result);
	int r1, r2, c1, c2, row, col;
	if (lanpr_get_line_bounding_areas(rb, Result, &r1, &r2, &c1, &c2)) {
		for (row = r1; row != r2 + 1; row++) {
			for (col = c1; col != c2 + 1; col++) {
				lanpr_link_line_with_bounding_area(rb, &rb->initial_bounding_areas[row * 4 + col], Result);
			}
		}
	}

	//printf("intersection (%f %f)-(%f %f)\n",Result->L->FrameBufferCoord[0], Result->L->FrameBufferCoord[1],Result->R->FrameBufferCoord[0], Result->R->FrameBufferCoord[1]);

	//tnsglobal_TriangleIntersectionCount++;

	rb->intersection_count++;

	return Result;
}
void lanpr_triangle_enable_intersections_in_bounding_area(LANPR_RenderBuffer *rb, LANPR_RenderTriangle *rt, LANPR_BoundingArea *ba) {
	tnsVector3d n, c = { 0 };
	tnsVector3d TL, TR;
	LANPR_RenderTriangle *TestingTriangle;
	LANPR_RenderLine *TestingLine;
	LANPR_RenderLine *Result = 0;
	LANPR_RenderVert *rv;
	LinkData *lip, *next_lip;
	real l, r;
	int a = 0;

	real
	*FBC0 = rt->V[0]->FrameBufferCoord,
	*FBC1 = rt->V[1]->FrameBufferCoord,
	*FBC2 = rt->V[2]->FrameBufferCoord;

	if (ba->Child){
		lanpr_triangle_enable_intersections_in_bounding_area(rb,rt,&ba->Child[0]);
		lanpr_triangle_enable_intersections_in_bounding_area(rb,rt,&ba->Child[1]);
		lanpr_triangle_enable_intersections_in_bounding_area(rb,rt,&ba->Child[2]);
		lanpr_triangle_enable_intersections_in_bounding_area(rb,rt,&ba->Child[3]);
		return;
	}

	for (lip = ba->LinkedTriangles.first; lip; lip = next_lip) {
		next_lip = lip->next;
		TestingTriangle = lip->data;
		if (TestingTriangle == rt || TestingTriangle->testing == rt || lanpr_triangle_share_edge(rt, TestingTriangle)){
			
			continue;
		}

		TestingTriangle->testing = rt;
		real
		*RFBC0 = TestingTriangle->V[0]->FrameBufferCoord,
		*RFBC1 = TestingTriangle->V[1]->FrameBufferCoord,
		*RFBC2 = TestingTriangle->V[2]->FrameBufferCoord;

		if (MIN3(FBC0[2], FBC1[2], FBC2[2]) > MAX3(RFBC0[2], RFBC1[2], RFBC2[2])) continue;
		if (MAX3(FBC0[2], FBC1[2], FBC2[2]) < MIN3(RFBC0[2], RFBC1[2], RFBC2[2])) continue;
		if (MIN3(FBC0[0], FBC1[0], FBC2[0]) > MAX3(RFBC0[0], RFBC1[0], RFBC2[0])) continue;
		if (MAX3(FBC0[0], FBC1[0], FBC2[0]) < MIN3(RFBC0[0], RFBC1[0], RFBC2[0])) continue;
		if (MIN3(FBC0[1], FBC1[1], FBC2[1]) > MAX3(RFBC0[1], RFBC1[1], RFBC2[1])) continue;
		if (MAX3(FBC0[1], FBC1[1], FBC2[1]) < MIN3(RFBC0[1], RFBC1[1], RFBC2[1])) continue;


		Result = lanpr_triangle_generate_intersection_line_only(rb, rt, TestingTriangle);
	}

}


int lanpr_line_crosses_frame(tnsVector2d L, tnsVector2d R) {
	real vx, vy;
	tnsVector4d converted;
	real c1, c;

	if (-1 > MAX2(L[0], R[0])) return 0;
	if (1 < MIN2(L[0], R[0])) return 0;
	if (-1 > MAX2(L[1], R[1])) return 0;
	if (1 < MIN2(L[1], R[1])) return 0;

	vx = L[0] - R[0];
	vy = L[1] - R[1];

	c1 = vx * (-1 - L[1]) - vy * (-1 - L[0]);
	c = c1;

	c1 = vx * (-1 - L[1]) - vy * (1 - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	c1 = vx * (1 - L[1]) - vy * (-1 - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	c1 = vx * (1 - L[1]) - vy * (1 - L[0]);
	if (c1 * c <= 0) return 1;
	else c = c1;

	return 0;
}

void lanpr_compute_view_vector(LANPR_RenderBuffer *rb) {
	tnsVector3d Direction = { 0, 0, 1 };
	tnsVector3d Trans;
	tnsMatrix44d inv;
	tnsMatrix44d obmat;

	tmat_obmat_to_16d(rb->scene->camera->obmat, obmat);
	tmat_inverse_44d(inv, obmat);
	tmat_apply_rotation_43d(Trans, inv, Direction);
	tMatVectorCopy3d(Trans, rb->view_vector);
	//tMatVectorMultiSelf3d(Trans, -1);
	//tMatVectorCopy3d(Trans, ((Camera*)rb->scene->camera)->RenderViewDir);
}

void lanpr_compute_scene_contours(LANPR_RenderBuffer *rb, float threshold) {
	real *view_vector = rb->view_vector;
	BMEdge *e;
	real Dot1 = 0, Dot2 = 0;
	real Result;
	tnsVector4d GNormal;
	tnsVector3d cam_location;
	int Add = 0;
	Object *cam_obj = rb->scene->camera;
	Camera *c = cam_obj->data;
	LANPR_RenderLine *rl;
	int contour_count = 0;
	int crease_count = 0;
	int MaterialCount = 0;

	rb->overall_progress = 20;
	rb->calculation_status = TNS_CALCULATION_CONTOUR;
	//nulThreadNotifyUsers("tns.render_buffer_list.calculation_status");

	if (c->type == CAM_ORTHO) {
		lanpr_compute_view_vector(rb);
	}

	for (rl = rb->all_render_lines.first; rl; rl = rl->Item.next) {
		//if(rl->testing)
		//if (!lanpr_line_crosses_frame(rl->L->FrameBufferCoord, rl->R->FrameBufferCoord))
		//	continue;

		Add = 0; Dot1 = 0; Dot2 = 0;

		if (c->type == CAM_PERSP) {
			tMatVectorConvert3fd(cam_obj->obmat[3], cam_location);
			tMatVectorMinus3d(view_vector, rl->L->GLocation, cam_location);
		}

		if (use_smooth_contour_modifier_contour) {
			if (rl->Flags & LANPR_EDGE_FLAG_CONTOUR) Add = 1;
		}else {
			if (rl->TL) Dot1 = tmat_dot_3d(view_vector, rl->TL->gn, 0); else Add = 1;
			if (rl->TR) Dot2 = tmat_dot_3d(view_vector, rl->TR->gn, 0); else Add = 1;
		}

		if (!Add) {
			if ((Result = Dot1 * Dot2) <= 0 && (Dot1 + Dot2)) Add = 1;
			elif(tmat_dot_3d(rl->TL->gn, rl->TR->gn, 0) < threshold) Add = 2;
			elif(rl->TL && rl->TR && rl->TL->material_id != rl->TR->material_id) Add = 3;
		}

		if (Add == 1) {
			rl->Flags |= LANPR_EDGE_FLAG_CONTOUR;
			list_append_pointer_static(&rb->contours, &rb->render_data_pool, rl);
			contour_count++;
		} elif(Add == 2)
		{
			rl->Flags |= LANPR_EDGE_FLAG_CREASE;
			list_append_pointer_static(&rb->crease_lines, &rb->render_data_pool, rl);
			crease_count++;
		} elif(Add == 3)
		{
			rl->Flags |= LANPR_EDGE_FLAG_MATERIAL;
			list_append_pointer_static(&rb->material_lines, &rb->render_data_pool, rl);
			MaterialCount++;
		}
		if (rl->Flags & LANPR_EDGE_FLAG_EDGE_MARK) {
			// no need to mark again
			Add = 4;
			list_append_pointer_static(&rb->edge_marks, &rb->render_data_pool, rl);
			//continue;
		}
		if (Add) {
			int r1, r2, c1, c2, row, col;
			if (lanpr_get_line_bounding_areas(rb, rl, &r1, &r2, &c1, &c2)) {
				for (row = r1; row != r2 + 1; row++) {
					for (col = c1; col != c2 + 1; col++) {
						lanpr_link_line_with_bounding_area(rb, &rb->initial_bounding_areas[row * 4 + col], rl);
					}
				}
			}
		}

		//line count reserved for feature such as progress feedback
	}
}


void lanpr_clear_render_state(LANPR_RenderBuffer *rb) {
	rb->contour_count = 0;
	rb->contour_managed = 0;
	rb->intersection_count = 0;
	rb->intersection_managed = 0;
	rb->material_line_count = 0;
	rb->material_managed = 0;
	rb->crease_count = 0;
	rb->crease_managed = 0;
	rb->calculation_status = TNS_CALCULATION_IDLE;
}


/* ====================================== render control ======================================= */

extern LANPR_SharedResource lanpr_share;

void lanpr_destroy_render_data(LANPR_RenderBuffer *rb) {
	LANPR_RenderElementLinkNode *reln;

	rb->contour_count = 0;
	rb->contour_managed = 0;
	rb->intersection_count = 0;
	rb->intersection_managed = 0;
	rb->material_line_count = 0;
	rb->material_managed = 0;
	rb->crease_count = 0;
	rb->crease_managed = 0;
	rb->edge_mark_count = 0;
	rb->edge_mark_managed = 0;
	rb->calculation_status = TNS_CALCULATION_IDLE;

	list_handle_empty(&rb->contours);
	list_handle_empty(&rb->intersection_lines);
	list_handle_empty(&rb->crease_lines);
	list_handle_empty(&rb->material_lines);
	list_handle_empty(&rb->edge_marks);
	list_handle_empty(&rb->all_render_lines);
	list_handle_empty(&rb->chains);

	while (reln = BLI_pophead(&rb->vertex_buffer_pointers)) {
		MEM_freeN(reln->Pointer);
	}

	while (reln = BLI_pophead(&rb->line_buffer_pointers)) {
		MEM_freeN(reln->Pointer);
	}

	while (reln = BLI_pophead(&rb->triangle_buffer_pointers)) {
		MEM_freeN(reln->Pointer);
	}

	BLI_spin_end(&rb->cs_data);
	BLI_spin_end(&rb->cs_info);
	BLI_spin_end(&rb->cs_management);
	BLI_spin_end(&rb->render_data_pool.csMem);

	mem_static_destroy(&rb->render_data_pool);
}

LANPR_RenderBuffer *lanpr_create_render_buffer(SceneLANPR *lanpr) {
	if (lanpr_share.render_buffer_shared) {
		lanpr->render_buffer = lanpr_share.render_buffer_shared;
		lanpr_destroy_render_data(lanpr->render_buffer);
		return lanpr->render_buffer;
		//lanpr_destroy_render_data(lanpr->render_buffer);
		//MEM_freeN(lanpr->render_buffer);
	}

	LANPR_RenderBuffer *rb = MEM_callocN(sizeof(LANPR_RenderBuffer), "creating LANPR render buffer");

	lanpr->render_buffer = rb;
	lanpr_share.render_buffer_shared = rb;

	rb->cached_for_frame = -1;

	BLI_spin_init(&rb->cs_data);
	BLI_spin_init(&rb->cs_info);
	BLI_spin_init(&rb->cs_management);
	BLI_spin_init(&rb->render_data_pool.csMem);

	return rb;
}

void lanpr_rebuild_render_draw_command(LANPR_RenderBuffer *rb, LANPR_LineLayer *ll);

int lanpr_get_render_triangle_size(LANPR_RenderBuffer *rb) {
	if (rb->thread_count == 0) rb->thread_count = BKE_render_num_threads(&rb->scene->r);
	return sizeof(LANPR_RenderTriangle) + (sizeof(LANPR_RenderLine *) * rb->thread_count);
}

static char Message[] = "Please fill in these fields before exporting image:";
static char MessageFolder[] = "    - Output folder";
static char MessagePrefix[] = "    - File name prefix";
static char MessageConnector[] = "    - File name connector";
static char MessageLayerName[] = "    - One or more layers have empty/illegal names.";
static char MessageSuccess[] = "Sucessfully Saved Image(s).";
static char MessageHalfSuccess[] = "Some image(s) failed to save.";
static char MessageFailed[] = "No saving action performed.";

//int ACTINV_SaveRenderBufferPreview(nActuatorIntern* a, nEvent* e) {
//	LANPR_RenderBuffer* rb = a->This->EndInstance;
//	LANPR_LineStyle* ll;
//	char FullPath[1024] = "";
//
//	if (!rb) return;
//
//	tnsFrameBuffer *fb = rb->FrameBuffer;
//
//	if (fb->OutputMode == TNS_OUTPUT_MODE_COMBINED) {
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix || !fb->ImagePrefix->Ptr)) {
//			nPanelMessageList List = {0};
//			nulAddPanelMessage(&List, Message);
//			if ((!fb->OutputFolder || !fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder);
//			if ((!fb->ImagePrefix || !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//			nulAddPanelMessage(&List, MessageFailed);
//			nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//			return NUL_FINISHED;
//		}
//		strcat(FullPath, fb->OutputFolder->Ptr);
//		strcat(FullPath, fb->ImagePrefix->Ptr);
//		lanpr_SaveRenderBufferPreviewAsImage(rb, FullPath, 0, 0);
//	}elif(fb->OutputMode == TNS_OUTPUT_MODE_PER_LAYER) {
//		nPanelMessageList List = { 0 };
//		int unnamed = 0;
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix || !fb->ImagePrefix->Ptr)|| (!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) {
//			nulAddPanelMessage(&List, Message);
//			if ((!fb->OutputFolder||!fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder);
//			if ((!fb->ImagePrefix|| !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//			if ((!fb->ImageNameConnector|| !fb->ImageNameConnector->Ptr)) nulAddPanelMessage(&List, MessageConnector);
//			nulAddPanelMessage(&List, MessageFailed);
//			nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//			return NUL_FINISHED;
//		}
//		for (ll = lanpr->line_layers.first; ll; ll = ll->Item.next) {
//			FullPath[0] = 0;
//			if ((!ll->Name || !ll->Name->Ptr) && !unnamed) {
//				nulAddPanelMessage(&List, MessageHalfSuccess);
//				nulAddPanelMessage(&List, MessageLayerName);
//				unnamed = 1;
//				continue;
//			}
//			strcat(FullPath, fb->OutputFolder->Ptr);
//			strcat(FullPath, fb->ImagePrefix->Ptr);
//			strcat(FullPath, fb->ImageNameConnector->Ptr);
//			strcat(FullPath, ll->Name->Ptr);
//			lanpr_SaveRenderBufferPreviewAsImage(rb, FullPath, ll, 0);
//		}
//		if(unnamed)nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//	}
//
//	return NUL_FINISHED;
//}
//int ACTINV_SaveSingleLayer(nActuator* a, nEvent* e) {
//	LANPR_LineStyle* ll = a->This->EndInstance;
//	char FullPath[1024] = "";
//	int fail = 0;
//
//	if (!ll)return;
//
//	tnsFrameBuffer* fb = ll->ParentRB->FrameBuffer;
//
//	if (!fb) return;
//
//	nPanelMessageList List = { 0 };
//
//	if ((!fb->OutputFolder || !fb->OutputFolder->Ptr) || (!fb->ImagePrefix || !fb->ImagePrefix->Ptr) || (!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) {
//		nulAddPanelMessage(&List, Message);
//		if ((!fb->OutputFolder || !fb->OutputFolder->Ptr)) nulAddPanelMessage(&List, MessageFolder);
//		if ((!fb->ImagePrefix || !fb->ImagePrefix->Ptr)) nulAddPanelMessage(&List, MessagePrefix);
//		if ((!fb->ImageNameConnector || !fb->ImageNameConnector->Ptr)) nulAddPanelMessage(&List, MessageConnector);
//		fail = 1;
//	}
//	if (!ll->Name || !ll->Name->Ptr) {
//		nulAddPanelMessage(&List, MessageHalfSuccess);
//		nulAddPanelMessage(&List, MessageLayerName);
//		fail = 1;
//	}
//	if (fail) {
//		nulAddPanelMessage(&List, MessageFailed);
//		nulEnableMultiMessagePanel(a, 0, "Caution", &List, e->x, e->y, 500, e);
//		return NUL_FINISHED;
//	}
//
//
//	FullPath[0] = 0;
//	strcat(FullPath, fb->OutputFolder->Ptr);
//	strcat(FullPath, fb->ImagePrefix->Ptr);
//	strcat(FullPath, fb->ImageNameConnector->Ptr);
//	strcat(FullPath, ll->Name->Ptr);
//	lanpr_SaveRenderBufferPreviewAsImage(ll->ParentRB, FullPath, ll, 0);
//
//
//	return NUL_FINISHED;
//}


int lanpr_count_this_line(LANPR_RenderLine *rl, LANPR_LineLayer *ll) {
	LANPR_LineLayerComponent *llc = ll->components.first;
	int AndResult = 1, OrResult = 0;
	if (!llc) return 1;
	for (llc; llc; llc = llc->next) {
		if (llc->component_mode == LANPR_COMPONENT_MODE_ALL) { OrResult = 1; }
		elif(llc->component_mode == LANPR_COMPONENT_MODE_OBJECT && llc->object_select)
		{
			if (rl->ObjectRef && rl->ObjectRef->id.orig_id == &llc->object_select->id) { OrResult = 1;}
			else {
				AndResult = 0;
			}
		} elif(llc->component_mode == LANPR_COMPONENT_MODE_MATERIAL && llc->material_select)
		{
			if ((rl->TL && rl->TL->material_id == llc->material_select->index) || (rl->TR && rl->TR->material_id == llc->material_select->index) || (!(rl->Flags & LANPR_EDGE_FLAG_INTERSECTION))) { OrResult = 1; }
			else {
				AndResult = 0;
			}
		} elif(llc->component_mode == LANPR_COMPONENT_MODE_COLLECTION && llc->collection_select)
		{
			if (BKE_collection_has_object(llc->collection_select, (Object *)rl->ObjectRef->id.orig_id)) { OrResult = 1; }
			else {
				AndResult = 0;
			}
		}
	}
	if (ll->logic_mode == LANPR_COMPONENT_LOGIG_OR) return OrResult;
	else return AndResult;
}
long lanpr_count_leveled_edge_segment_count(ListBase *LineList, LANPR_LineLayer *ll) {
	LinkData *lip;
	LANPR_RenderLine *rl;
	LANPR_RenderLineSegment *rls;
	Object *o;
	int not = 0;
	long Count = 0;
	for (lip = LineList->first; lip; lip = lip->next) {
		rl = lip->data;
		if (!lanpr_count_this_line(rl, ll)) continue;

		for (rls = rl->Segments.first; rls; rls = rls->Item.next) {

			if (rls->OcclusionLevel >= ll->qi_begin && rls->OcclusionLevel <= ll->qi_end) Count++;
		}
	}
	return Count;
}
long lanpr_count_intersection_segment_count(LANPR_RenderBuffer *rb) {
	LANPR_RenderLine *rl;
	LANPR_RenderLineSegment *rls;
	long Count = 0;
	for (rl = rb->intersection_lines.first; rl; rl = rl->Item.next) {
		Count++;
	}
	return Count;
}
void *lanpr_make_leveled_edge_vertex_array(LANPR_RenderBuffer *rb, ListBase *LineList, float *VertexArray, float* NormalArray, float** NextNormal, LANPR_LineLayer *ll, float componet_id) {
	LinkData *lip;
	LANPR_RenderLine *rl;
	LANPR_RenderLineSegment *rls, *irls;
	Object *o;
	real W = rb->w / 2, H = rb->h / 2;
	long i = 0;
	float *V = VertexArray;
	float *N = NormalArray;
	for (lip = LineList->first; lip; lip = lip->next) {
		rl = lip->data;
		if (!lanpr_count_this_line(rl, ll)) continue;

		for (rls = rl->Segments.first; rls; rls = rls->Item.next) {
			if (rls->OcclusionLevel >= ll->qi_begin && rls->OcclusionLevel <= ll->qi_end) {

				if (rl->TL) {
					N[0] += rl->TL->gn[0];
					N[1] += rl->TL->gn[1];
					N[2] += rl->TL->gn[2];
				}
				if (rl->TR) {
					N[0] += rl->TR->gn[0];
					N[1] += rl->TR->gn[1];
					N[2] += rl->TR->gn[2];
				}
				if (rl->TL || rl->TR) {
					normalize_v3(N);
					copy_v3_v3(&N[3], N);
				}
				N += 6;

				CLAMP(rls->at, 0, 1);
				if(irls = rls->Item.next) CLAMP(irls->at, 0, 1);

				*V = tnsLinearItp(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], rls->at);
				V++;
				*V = tnsLinearItp(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], rls->at);
				V++;
				*V = componet_id;
				V++;
				*V = tnsLinearItp(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], irls ? irls->at : 1);
				V++;
				*V = tnsLinearItp(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], irls ? irls->at : 1);
				V++;
				*V = componet_id;
				V++;
			}
		}
	}
	*NextNormal = N;
	return V;
}

void lanpr_chain_generate_draw_command(LANPR_RenderBuffer *rb);

/* ============================================ viewport display ================================================= */

void lanpr_rebuild_render_draw_command(LANPR_RenderBuffer *rb, LANPR_LineLayer *ll) {
	int Count = 0;
	int level;
	float *V, *tv, *N, *tn;
	int i;
	int VertCount;

	if (ll->type == TNS_COMMAND_LINE) {
		static GPUVertFormat format = { 0 };
		static struct { uint pos, normal; } attr_id;
		if (format.attr_len == 0) {
			attr_id.pos = GPU_vertformat_attr_add(&format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
			attr_id.normal = GPU_vertformat_attr_add(&format, "normal", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
		}

		GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);

		if (ll->enable_contour) Count += lanpr_count_leveled_edge_segment_count(&rb->contours, ll);
		if (ll->enable_crease) Count += lanpr_count_leveled_edge_segment_count(&rb->crease_lines, ll);
		if (ll->enable_intersection) Count += lanpr_count_leveled_edge_segment_count(&rb->intersection_lines, ll);
		if (ll->enable_edge_mark) Count += lanpr_count_leveled_edge_segment_count(&rb->edge_marks, ll);
		if (ll->enable_material_seperate) Count += lanpr_count_leveled_edge_segment_count(&rb->material_lines, ll);

		VertCount = Count * 2;

		if (!VertCount) return;

		GPU_vertbuf_data_alloc(vbo, VertCount);

		tv = V = CreateNewBuffer(float, 6 * Count);
		tn = N = CreateNewBuffer(float, 6 * Count);

		if (ll->enable_contour) tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->contours, tv, tn, &tn, ll, 1.0f);
		if (ll->enable_crease) tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->crease_lines, tv, tn, &tn, ll, 2.0f);
		if (ll->enable_material_seperate) tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->material_lines, tv, tn, &tn, ll, 3.0f);
		if (ll->enable_edge_mark) tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->edge_marks, tv, tn, &tn, ll, 4.0f);
		if (ll->enable_intersection) tv = lanpr_make_leveled_edge_vertex_array(rb, &rb->intersection_lines, tv, tn, &tn, ll, 5.0f);


		for (i = 0; i < VertCount; i++) {
			GPU_vertbuf_attr_set(vbo, attr_id.pos,    i, &V[i * 3]);
			GPU_vertbuf_attr_set(vbo, attr_id.normal, i, &N[i * 3]);
		}

		FreeMem(V);
		FreeMem(N);

		ll->batch = GPU_batch_create_ex(GPU_PRIM_LINES, vbo, 0, GPU_USAGE_DYNAMIC | GPU_BATCH_OWNS_VBO);

		return;
	}

	//if (ll->type == TNS_COMMAND_MATERIAL || ll->type == TNS_COMMAND_EDGE) {
	// later implement ....
	//}

}
void lanpr_rebuild_all_command(SceneLANPR *lanpr) {
	LANPR_LineLayer *ll;
	if (!lanpr || !lanpr->render_buffer) return;

	if (lanpr->enable_chaining) {
		lanpr_chain_generate_draw_command(lanpr->render_buffer);
	}
	else {
		for (ll = lanpr->line_layers.first; ll; ll = ll->next) {
			if (ll->batch) GPU_batch_discard(ll->batch);
			lanpr_rebuild_render_draw_command(lanpr->render_buffer, ll);
		}
	}
}

void lanpr_viewport_draw_offline_result(LANPR_TextureList *txl, LANPR_FramebufferList *fbl, LANPR_PassList *psl, LANPR_PrivateData *pd, SceneLANPR *lanpr) {
	float clear_col[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float clear_depth = 1.0f;
	uint clear_stencil = 0xFF;

	DefaultTextureList *dtxl = DRW_viewport_texture_list_get();
	DefaultFramebufferList *dfbl = DRW_viewport_framebuffer_list_get();

	int texw = GPU_texture_width(dtxl->color), texh = GPU_texture_height(dtxl->color);
	int tsize = texw * texh;

	const DRWContextState *draw_ctx = DRW_context_state_get();
	Scene *scene = DEG_get_evaluated_scene(draw_ctx->depsgraph);
	View3D *v3d = draw_ctx->v3d;
	Object *camera;
	if (v3d) {
		RegionView3D *rv3d = draw_ctx->rv3d;
		camera = (rv3d->persp == RV3D_CAMOB) ? v3d->camera : NULL;
	}
	else {
		camera = scene->camera;
	}



	GPU_framebuffer_bind(fbl->dpix_transform);
	DRW_draw_pass(psl->dpix_transform_pass);

	GPU_framebuffer_bind(fbl->dpix_preview);
	eGPUFrameBufferBits clear_bits = GPU_COLOR_BIT;
	GPU_framebuffer_clear(fbl->dpix_preview, clear_bits, lanpr->background_color, clear_depth, clear_stencil);
	DRW_draw_pass(psl->dpix_preview_pass);

	GPU_framebuffer_bind(dfbl->default_fb);
	GPU_framebuffer_clear(dfbl->default_fb, clear_bits, lanpr->background_color, clear_depth, clear_stencil);
	DRW_multisamples_resolve(txl->depth, txl->color, 1);
}


void lanpr_NO_THREAD_chain_feature_lines(LANPR_RenderBuffer *rb, float dist_threshold);

void lanpr_calculate_normal_object_vector(LANPR_LineLayer* ll, float* normal_object_direction) {
	Object* ob;
	switch (ll->normal_mode) {
	case LANPR_NORMAL_DONT_CARE:
		return;
	case LANPR_NORMAL_DIRECTIONAL:
		if (!(ob = ll->normal_control_object)) {
			normal_object_direction[0] = 0;
			normal_object_direction[1] = 0;
			normal_object_direction[2] = 1; // default z up direction
		}else {
			float dir[3] = {0,0,1};
			float mat[3][3];
			copy_m3_m4(mat, ob->obmat);
			mul_v3_m3v3(normal_object_direction, mat, dir);
			normalize_v3(normal_object_direction);
		}
		return;
	case LANPR_NORMAL_POINT:
		if (!(ob = ll->normal_control_object)) {
			normal_object_direction[0] = 0;
			normal_object_direction[1] = 0;
			normal_object_direction[2] = 0; // default origin position
		}
		else {
			normal_object_direction[0] = ob->obmat[3][0];
			normal_object_direction[1] = ob->obmat[3][1];
			normal_object_direction[2] = ob->obmat[3][2];
		}
		return;
	case LANPR_NORMAL_2D:
		return;
	}
}

void lanpr_software_draw_scene(void *vedata, GPUFrameBuffer *dfb, int is_render) {
	LANPR_LineLayer *ll;
	LANPR_PassList *psl = ((LANPR_Data *)vedata)->psl;
	LANPR_TextureList *txl = ((LANPR_Data *)vedata)->txl;
	LANPR_StorageList *stl = ((LANPR_Data *)vedata)->stl;
	LANPR_FramebufferList *fbl = ((LANPR_Data *)vedata)->fbl;
	LANPR_PrivateData *pd = stl->g_data;
	const DRWContextState *draw_ctx = DRW_context_state_get();
	Scene *scene = DEG_get_evaluated_scene(draw_ctx->depsgraph);
	SceneLANPR *lanpr = &scene->lanpr;
	View3D *v3d = draw_ctx->v3d;
	float indentity_mat[4][4];
	static float normal_object_direction[3] = { 0,0,1 };

	if (is_render) {
		lanpr_rebuild_all_command(lanpr);
	}
	else {
		if (lanpr_during_render()) {
			printf("LANPR Warning: To avoid resource duplication, viewport will not display when rendering is in progress\n");
			return; // don't draw viewport during render
		}
	}

	float clear_col[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float clear_depth = 1.0f;
	uint clear_stencil = 0xFF;
	eGPUFrameBufferBits clear_bits = GPU_DEPTH_BIT | GPU_COLOR_BIT;

	//Object *camera;
	//if (v3d) {
	//	RegionView3D *rv3d = draw_ctx->rv3d;
	//	camera = (rv3d->persp == RV3D_CAMOB) ? v3d->camera : NULL;
	//}
	//else {
	//	camera = scene->camera;
	//}

	GPU_framebuffer_bind(fbl->software_ms);
	GPU_framebuffer_clear(fbl->software_ms, clear_bits, lanpr->background_color, clear_depth, clear_stencil);

	if (lanpr->render_buffer) {

		int texw = GPU_texture_width(txl->ms_resolve_color), texh = GPU_texture_height(txl->ms_resolve_color);;
		pd->output_viewport[2] = scene->r.xsch;
		pd->output_viewport[3] = scene->r.ysch;
		pd->dpix_viewport[2] = texw;
		pd->dpix_viewport[3] = texh;

		unit_m4(indentity_mat);
		
		DRWView *view = DRW_view_create(indentity_mat, indentity_mat, NULL, NULL, NULL);
		DRW_view_default_set(view);
		DRW_view_set_active(view);

		if (lanpr->enable_chaining && lanpr->render_buffer->chain_draw_batch) {
			for (ll = lanpr->line_layers.last; ll; ll = ll->prev) {
				LANPR_RenderBuffer *rb;
				psl->software_pass = DRW_pass_create("Software Render Preview", DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL);
				rb = lanpr->render_buffer;
				rb->ChainShgrp = DRW_shgroup_create(lanpr_share.software_chaining_shader, psl->software_pass);

				lanpr_calculate_normal_object_vector(ll, normal_object_direction);

				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "color", ll->color, 1);
				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "crease_color", ll->crease_color, 1);
				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "material_color", ll->material_color, 1);
				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "edge_mark_color", ll->edge_mark_color, 1);
				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "intersection_color", ll->intersection_color, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness", &ll->thickness, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness_crease", &ll->thickness_crease, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness_material", &ll->thickness_material, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness_edge_mark", &ll->thickness_edge_mark, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "thickness_intersection", &ll->thickness_intersection, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_contour", &ll->enable_contour, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_crease", &ll->enable_crease, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_material", &ll->enable_material_seperate, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_edge_mark", &ll->enable_edge_mark, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "enable_intersection", &ll->enable_intersection, 1);

				DRW_shgroup_uniform_int(rb->ChainShgrp, "normal_mode", &ll->normal_mode, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "normal_effect_inverse", &ll->normal_effect_inverse, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_ramp_begin", &ll->normal_ramp_begin, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_ramp_end", &ll->normal_ramp_end, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_thickness_begin", &ll->normal_thickness_begin, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "normal_thickness_end", &ll->normal_thickness_end, 1);
				DRW_shgroup_uniform_vec3(rb->ChainShgrp, "normal_direction", normal_object_direction, 1);

				DRW_shgroup_uniform_int(rb->ChainShgrp, "occlusion_level_begin", &ll->qi_begin, 1);
				DRW_shgroup_uniform_int(rb->ChainShgrp, "occlusion_level_end", &ll->qi_end, 1);

				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "preview_viewport", stl->g_data->dpix_viewport, 1);
				DRW_shgroup_uniform_vec4(rb->ChainShgrp, "output_viewport", stl->g_data->output_viewport, 1);

				float *tld = &lanpr->taper_left_distance, *tls = &lanpr->taper_left_strength,
				      *trd = &lanpr->taper_right_distance, *trs = &lanpr->taper_right_strength;

				DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_l_dist", tld, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_l_strength", tls, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_r_dist", lanpr->use_same_taper ? tld : trd, 1);
				DRW_shgroup_uniform_float(rb->ChainShgrp, "taper_r_strength", lanpr->use_same_taper ? tls : trs, 1);

				//need to add component enable/disable option.
				DRW_shgroup_call(rb->ChainShgrp, lanpr->render_buffer->chain_draw_batch, NULL);
				// debug purpose
				//DRW_draw_pass(psl->color_pass);
				//DRW_draw_pass(psl->color_pass);
				DRW_draw_pass(psl->software_pass);
			}
		} elif(!lanpr->enable_chaining)
		{
			for (ll = lanpr->line_layers.last; ll; ll = ll->prev) {
				if (ll->batch) {
					psl->software_pass = DRW_pass_create("Software Render Preview", DRW_STATE_WRITE_COLOR | DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL);
					ll->shgrp = DRW_shgroup_create(lanpr_share.software_shader, psl->software_pass);

					lanpr_calculate_normal_object_vector(ll, normal_object_direction);

					DRW_shgroup_uniform_vec4(ll->shgrp, "color", ll->color, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "crease_color", ll->crease_color, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "material_color", ll->material_color, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "edge_mark_color", ll->edge_mark_color, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "intersection_color", ll->intersection_color, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "thickness", &ll->thickness, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "thickness_crease", &ll->thickness_crease, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "thickness_material", &ll->thickness_material, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "thickness_edge_mark", &ll->thickness_edge_mark, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "thickness_intersection", &ll->thickness_intersection, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "preview_viewport", stl->g_data->dpix_viewport, 1);
					DRW_shgroup_uniform_vec4(ll->shgrp, "output_viewport", stl->g_data->output_viewport, 1);

					DRW_shgroup_uniform_int(ll->shgrp, "normal_mode", &ll->normal_mode, 1);
					DRW_shgroup_uniform_int(ll->shgrp, "normal_effect_inverse", &ll->normal_effect_inverse, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "normal_ramp_begin", &ll->normal_ramp_begin, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "normal_ramp_end", &ll->normal_ramp_end, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "normal_thickness_begin", &ll->normal_thickness_begin, 1);
					DRW_shgroup_uniform_float(ll->shgrp, "normal_thickness_end", &ll->normal_thickness_end, 1);
					DRW_shgroup_uniform_vec3(ll->shgrp, "normal_direction", normal_object_direction, 1);

					DRW_shgroup_call(ll->shgrp, ll->batch, NULL);
					DRW_draw_pass(psl->software_pass);
				}
			}
		}
	}

	GPU_framebuffer_blit(fbl->software_ms, 0, dfb, 0, GPU_COLOR_BIT);
}

/* ============================================ operators ========================================= */

int lanpr_compute_feature_lines_internal(Depsgraph *depsgraph, SceneLANPR *lanpr, Scene *scene) {
	LANPR_RenderBuffer *rb;

	rb = lanpr_create_render_buffer(lanpr);
	rb->scene = scene;
	rb->w = scene->r.xsch;
	rb->h = scene->r.ysch;
	rb->enable_intersections = lanpr->enable_intersections;

	rb->triangle_size = lanpr_get_render_triangle_size(rb);

	lanpr_make_render_geometry_buffers(depsgraph, scene, scene->camera, rb);

	lanpr_compute_view_vector(rb);
	lanpr_cull_triangles(rb);

	lanpr_perspective_division(rb);

	lanpr_make_initial_bounding_areas(rb);

	lanpr_compute_scene_contours(rb, lanpr->crease_threshold);

	lanpr_add_triangles(rb);

	lanpr_THREAD_calculate_line_occlusion_begin(rb);

	if (lanpr->enable_chaining) {
		lanpr_NO_THREAD_chain_feature_lines(rb, 0.00001);// should use user_adjustable value
	}

	rb->cached_for_frame = scene->r.cfra;

	return OPERATOR_FINISHED;
}

//seems we don't quite need this operator...
static int lanpr_clear_render_buffer_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;
	LANPR_RenderBuffer *rb;
	Depsgraph *depsgraph = CTX_data_depsgraph(C);

	lanpr_destroy_render_data(lanpr->render_buffer);

	return OPERATOR_FINISHED;
}
static int lanpr_compute_feature_lines_exec(struct bContext *C, struct wmOperator *op){
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	if (!scene->camera) {
		BKE_report(op->reports, RPT_ERROR, "There is no active camera in this scene!");
		printf("LANPR Warning: There is no active camera in this scene!\n");
		return OPERATOR_FINISHED;
	}

	return lanpr_compute_feature_lines_internal(CTX_data_depsgraph(C), lanpr, scene);
}
static void lanpr_compute_feature_lines_cancel(struct bContext *C, struct wmOperator *op){

	return;
}

void SCENE_OT_lanpr_calculate_feature_lines(struct wmOperatorType *ot){

	/* identifiers */
	ot->name = "Calculate Feature Lines";
	ot->description = "LANPR calculates feature line in current scene";
	ot->idname = "SCENE_OT_lanpr_calculate";

	/* api callbacks */
	//ot->invoke = screen_render_invoke; /* why we need both invoke and exec? */
	//ot->modal = screen_render_modal;
	ot->cancel = lanpr_compute_feature_lines_cancel;
	ot->exec = lanpr_compute_feature_lines_exec;
}

LANPR_LineLayer *lanpr_new_line_layer(SceneLANPR *lanpr){
	LANPR_LineLayer *ll = MEM_callocN(sizeof(LANPR_LineLayer), "Line Layer");
	BLI_addtail(&lanpr->line_layers, ll);
	lanpr->active_layer = ll;
	return ll;
}
LANPR_LineLayerComponent *lanpr_new_line_component(SceneLANPR *lanpr) {
	if (!lanpr->active_layer) return 0;
	LANPR_LineLayer *ll = lanpr->active_layer;

	LANPR_LineLayerComponent *llc = MEM_callocN(sizeof(LANPR_LineLayerComponent), "Line Component");
	BLI_addtail(&ll->components, llc);

	return llc;
}

int lanpr_add_line_layer_exec(struct bContext *C, struct wmOperator *op){
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	lanpr_new_line_layer(lanpr);

	return OPERATOR_FINISHED;
}
int lanpr_delete_line_layer_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	LANPR_LineLayer *ll = lanpr->active_layer;

	if (!ll) return OPERATOR_FINISHED;

	if (ll->prev) lanpr->active_layer = ll->prev;
	elif(ll->next) lanpr->active_layer = ll->next;
	else lanpr->active_layer = 0;

	BLI_remlink(&scene->lanpr.line_layers, ll);

	//if (ll->batch) GPU_batch_discard(ll->batch);

	MEM_freeN(ll);

	return OPERATOR_FINISHED;
}
int lanpr_move_line_layer_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	LANPR_LineLayer *ll = lanpr->active_layer;

	if (!ll) return OPERATOR_FINISHED;

	int dir = RNA_enum_get(op->ptr, "direction");

	if (dir == 1 && ll->prev) {
		BLI_remlink(&lanpr->line_layers, ll);
		BLI_insertlinkbefore(&lanpr->line_layers, ll->prev, ll);
	} elif(dir == -1 && ll->next)
	{
		BLI_remlink(&lanpr->line_layers, ll);
		BLI_insertlinkafter(&lanpr->line_layers, ll->next, ll);
	}

	return OPERATOR_FINISHED;
}
int lanpr_add_line_component_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	lanpr_new_line_component(lanpr);

	return OPERATOR_FINISHED;
}
int lanpr_delete_line_component_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;
	LANPR_LineLayer *ll = lanpr->active_layer;
	LANPR_LineLayerComponent *llc;
	int i = 0;

	if (!ll) return OPERATOR_FINISHED;

	int index = RNA_int_get(op->ptr, "index");

	for (llc = ll->components.first; llc; llc = llc->next) {
		if (index == i) break;
		i++;
	}

	if (llc) {
		BLI_remlink(&ll->components, llc);
		MEM_freeN(llc);
	}

	return OPERATOR_FINISHED;
}
int lanpr_rebuild_all_commands_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	lanpr_rebuild_all_command(lanpr);
	return OPERATOR_FINISHED;
}
int lanpr_enable_all_line_types_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;
	LANPR_LineLayer *ll;

	if (!(ll = lanpr->active_layer)) return OPERATOR_FINISHED;

	ll->enable_contour = 1;
	ll->enable_crease = 1;
	ll->enable_edge_mark = 1;
	ll->enable_material_seperate = 1;
	ll->enable_intersection = 1;

	copy_v3_v3(ll->crease_color, ll->color);
	copy_v3_v3(ll->edge_mark_color, ll->color);
	copy_v3_v3(ll->material_color, ll->color);
	copy_v3_v3(ll->intersection_color, ll->color);

	ll->thickness_crease = 1;
	ll->thickness_material = 1;
	ll->thickness_edge_mark = 1;
	ll->thickness_intersection = 1;

	return OPERATOR_FINISHED;
}
int lanpr_auto_create_line_layer_exec(struct bContext *C, struct wmOperator *op) {
	Scene *scene = CTX_data_scene(C);
	SceneLANPR *lanpr = &scene->lanpr;

	LANPR_LineLayer *ll;

	ll = lanpr_new_line_layer(lanpr);
	ll->thickness = 1.7;
	ll->color[0] = 1;
	ll->color[1] = 1;
	ll->color[2] = 1;

	lanpr_enable_all_line_types_exec(C, op);

	ll = lanpr_new_line_layer(lanpr);
	ll->thickness = 0.9;
	ll->qi_begin = 1;
	ll->qi_end = 1;
	ll->color[0] = 0.314;
	ll->color[1] = 0.596;
	ll->color[2] = 1;

	lanpr_enable_all_line_types_exec(C, op);

	ll = lanpr_new_line_layer(lanpr);
	ll->thickness = 0.7;
	ll->qi_begin = 2;
	ll->qi_end = 2;
	ll->color[0] = 0.135;
	ll->color[1] = 0.304;
	ll->color[2] = 0.508;

	lanpr_enable_all_line_types_exec(C, op);

	lanpr_rebuild_all_command(lanpr);

	return OPERATOR_FINISHED;
}


void SCENE_OT_lanpr_add_line_layer(struct wmOperatorType *ot){

	ot->name = "Add Line Layer";
	ot->description = "Add a new line layer";
	ot->idname = "SCENE_OT_lanpr_add_line_layer";

	ot->exec = lanpr_add_line_layer_exec;

}
void SCENE_OT_lanpr_delete_line_layer(struct wmOperatorType *ot){

	ot->name = "Delete Line Layer";
	ot->description = "Delete selected line layer";
	ot->idname = "SCENE_OT_lanpr_delete_line_layer";

	ot->exec = lanpr_delete_line_layer_exec;

}
void SCENE_OT_lanpr_rebuild_all_commands(struct wmOperatorType *ot) {

	ot->name = "Refresh Drawing Commands";
	ot->description = "Refresh LANPR line layer drawing commands";
	ot->idname = "SCENE_OT_lanpr_rebuild_all_commands";

	ot->exec = lanpr_rebuild_all_commands_exec;

}
void SCENE_OT_lanpr_auto_create_line_layer(struct wmOperatorType *ot) {

	ot->name = "Auto Create Line Layer";
	ot->description = "Automatically create defalt line layer config";
	ot->idname = "SCENE_OT_lanpr_auto_create_line_layer";

	ot->exec = lanpr_auto_create_line_layer_exec;

}
void SCENE_OT_lanpr_move_line_layer(struct wmOperatorType *ot) {
	static const EnumPropertyItem line_layer_move[] = {
		{ 1, "UP", 0, "Up", "" },
		{ -1, "DOWN", 0, "Down", "" },
		{ 0, NULL, 0, NULL, NULL }
	};

	ot->name = "Move Line Layer";
	ot->description = "Move LANPR line layer up and down";
	ot->idname = "SCENE_OT_lanpr_move_line_layer";

	//this need property to assign up/down direction

	ot->exec = lanpr_move_line_layer_exec;

	RNA_def_enum(ot->srna, "direction", line_layer_move, 0, "Direction",
	             "Direction to move the active line layer towards");
}
void SCENE_OT_lanpr_enable_all_line_types(struct wmOperatorType *ot) {
	ot->name = "Enable All Line Types";
	ot->description = "Enable All Line Types In This Line Layer.";
	ot->idname = "SCENE_OT_lanpr_enable_all_line_types";

	ot->exec = lanpr_enable_all_line_types_exec;

}

void SCENE_OT_lanpr_add_line_component(struct wmOperatorType *ot) {

	ot->name = "Add Line Component";
	ot->description = "Add a new line Component";
	ot->idname = "SCENE_OT_lanpr_add_line_component";

	ot->exec = lanpr_add_line_component_exec;

}
void SCENE_OT_lanpr_delete_line_component(struct wmOperatorType *ot) {

	ot->name = "Delete Line Component";
	ot->description = "Delete selected line component";
	ot->idname = "SCENE_OT_lanpr_delete_line_component";

	ot->exec = lanpr_delete_line_component_exec;

	RNA_def_int(ot->srna, "index", 0, 0, 10000, "index", "index of this line component", 0, 10000);
}

#ifdef USE_LANPR_HINT

// how to access LANPR's occlusion info after LANPR software mode calculation

// You can access descrete occlusion data from every edge,
// but you can also access occlusion using LANPR's chain data.
// Two examples are given.

// [1.descrete occlusion data for edges]======================================================================================
//
// LANPR occlusion related data storage :
//
// LANPR_RenderBuffer :: all_render_lines   ====>  All LANPR_RenderLine nodes. Each node for a singe edge on the mesh.
//                                               Only features lines are in this list.
//                                               LANPR_RenderLine stores a list of occlusion info in LANPR_RenderLineSegment.
//
// LANPR_RenderBuffer :: contours (and crease/material_lines/Intersections/edge_marks)
//                                        ====>  ListItemPointers to LANPR_RenderLine nodes.
//                                               Use these lists to access individual line types for convenience.
//                                               For how to access this list, refer to this file line 728-730 as an example.
//
// LANPR_RenderLine :: Segments           ====>  List of LANPR_RenderLineSegment to represent occlusion info.
//                                               See below for how occlusion is reoresented in Renderline and RenderLineSegment.
//
//  RenderLine Diagram:
//    +[RenderLine]
//       [Segments]
//         [Segment]  at=0      occlusion_level=0
//         [Segment]  at=0.5    occlusion_level=1
//         [Segment]  at=0.7    occlusion_level=0
//
//  Then you get a line with such occlusion:
//  [L]|-------------------------|=========|-----------[R]
//
//  the beginning to 50% of the line :   Not occluded
//  50% to 70% of the line :             Occluded 1 time
//  70% to the end of the line :         Not occluded
//
//  cut positions are linear interpolated in image space from line->L->FrameBufferCoord to line->R->FrameBufferCoord (always L to R)
//                    ~~~~~~~~~~~~~~~~~~~    ~~~~~~~~~~~
//  to see an example of iterating occlusion data for all lines for drawing, see below or refer to this file line 2930.
//
//  [Iterating occlusion data]
void lanpr_iterate_renderline_and_occlusion(LANPR_RenderBuffer *rb, double* V_OUT, double Occ_OUT) {
	LinkData *lip;
	LANPR_RenderLine *rl;
	LANPR_RenderLineSegment *rls, *irls;
	double *V = V_Out;
	double *O = Occ_OUT;

	for (rl = rb->all_render_lines->first; rl; rl = lip->next) {
		for (rls = rl->Segments.first; rls; rls = rls->Item.next) {

			irls = rls->Item.next;

			//safety reasons
			CLAMP(rls->at, 0, 1);
			if (irls)CLAMP(irls->at, 0, 1);

			//segment begin at some X and Y 
			//tnsLinearItp() is a linear interpolate function.
			*V = tnsLinearItp(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], rls->at); V++;
			*V = tnsLinearItp(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], rls->at); V++;
			*O = rls->OcclusionLevel; *O++;
			
			//segment end at some X and Y
			*V = tnsLinearItp(rl->L->FrameBufferCoord[0], rl->R->FrameBufferCoord[0], irls ? irls->at : 1); V++;
			*V = tnsLinearItp(rl->L->FrameBufferCoord[1], rl->R->FrameBufferCoord[1], irls ? irls->at : 1); V++;
			*O = rls->OcclusionLevel; *O++;

			// please note that LANPR_RenderVert::FrameBufferCoord is in NDC coorninates
			// to transform it into screen pixel space use rb->W/2 and rb->H/2

		}
	}
}


// [2. LANPR's chain data]======================================================================================
//
// Chain data storage: (Also see lanpr_all.h line 485 for diagram)
//
// LANPR_RenderBuffer :: chains          ====> For storing LANPR_RenderLineChain nodes.
//                                             Only available when chaining enabled and calculation is done.
//
// LANPR_RenderLineChain :: Chain        ====> LANPR_RenderLineChainItem node list.
//
// LANPR_RenderLineChainItem :: pos      ====> pos[0] and pos[1] for x y NDC coordinates, pos[2] reserved, do not use.
// 
// LANPR_RenderLineChainItem :: LineType and OcclusionLevel   ====> See lanpr_all.h line 485 for diagram.
//                                                                  These 2 fields in the last ChainItem of a Chain is not used.
//
// to see an example of accessing occlusion data as a whole chain, see below or refer to lanpr_chain.c line 336.
//
// [Iteration chains and occlusion data for each chain segments]
int lanpr_iterate_chains_and_occlusion(LANPR_RenderBuffer *rb) {
	LANPR_RenderLineChainItem *rlci;
	LANPR_RenderLineChain *rlc;
	for (rlc = rb->chains.first; rlc; rlc = rlc->Item.next) {
		for (rlci = rlc->Chain.first; rlci; rlci = rlci->Item.next) {
			//VL = rlci->pos;
			//VR = ((LANPR_RenderLineChainItem*)rlci->Item.next)->pos;
			//line_type_of_this_segment = rlci->LineType;          // ====> values are defined in lanpr_all.h line 456.
			//occlusion_of_this_segment = rlci->OcclusionLevel;
		}
	}
}




#endif

