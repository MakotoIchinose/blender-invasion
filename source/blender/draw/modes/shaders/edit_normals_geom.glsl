
layout(points) in;
layout(line_strip, max_vertices=2) out;

flat in vec4 v1[1];
flat in vec4 v2[1];

void main()
{
	for (int v = 0; v < 2; v++) {
		gl_Position = (v == 0) ? v1[0] : v2[0];
#ifdef USE_WORLD_CLIP_PLANES
		gl_ClipDistance[0] = gl_in[0].gl_ClipDistance[0];
		gl_ClipDistance[1] = gl_in[0].gl_ClipDistance[1];
		gl_ClipDistance[2] = gl_in[0].gl_ClipDistance[2];
		gl_ClipDistance[3] = gl_in[0].gl_ClipDistance[3];
		gl_ClipDistance[4] = gl_in[0].gl_ClipDistance[4];
		gl_ClipDistance[5] = gl_in[0].gl_ClipDistance[5];
#endif
		EmitVertex();
	}
	EndPrimitive();
}
