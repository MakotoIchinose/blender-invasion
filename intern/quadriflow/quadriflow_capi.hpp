#ifndef QUADRIFLOW_CAPI_HPP
#define QUADRIFLOW_CAPI_HPP

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	float *verts;
	unsigned int *faces;
	int totfaces;
	int totverts;

	float *out_verts;
	unsigned int *out_faces;
	int out_totverts;
	int out_totfaces;

	int target_faces;
	bool preserve_sharp;
	bool adaptive_scale;
	bool minimum_cost_flow;
	bool aggresive_sat;
  int rng_seed;
} QuadriflowRemeshData;

void quadriflow_remesh(QuadriflowRemeshData* qrd);

#ifdef __cplusplus
}
#endif

#endif // QUADRIFLOW_CAPI_HPP
