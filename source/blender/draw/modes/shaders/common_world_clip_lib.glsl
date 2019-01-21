#ifdef USE_WORLD_CLIP_PLANES
#ifdef GPU_VERTEX_SHADER
uniform vec4 WorldClipPlanes[6];
void world_clip_planes_calc_clip_distance(vec3 wpos)
{
	gl_ClipDistance[0] = dot(WorldClipPlanes[0].xyz, wpos) + WorldClipPlanes[0].w;
	gl_ClipDistance[1] = dot(WorldClipPlanes[1].xyz, wpos) + WorldClipPlanes[1].w;
	gl_ClipDistance[2] = dot(WorldClipPlanes[2].xyz, wpos) + WorldClipPlanes[2].w;
	gl_ClipDistance[3] = dot(WorldClipPlanes[3].xyz, wpos) + WorldClipPlanes[3].w;
	gl_ClipDistance[4] = dot(WorldClipPlanes[4].xyz, wpos) + WorldClipPlanes[4].w;
	gl_ClipDistance[5] = dot(WorldClipPlanes[5].xyz, wpos) + WorldClipPlanes[5].w;
}
#endif
#endif
