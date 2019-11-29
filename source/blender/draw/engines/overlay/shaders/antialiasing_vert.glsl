
out vec2 uvs;

#if STEP == 1
out vec2 pixcoord;
#endif

#if STEP == 2
out vec4 offset;
#else
out vec4 offsets[3];
#endif

void main()
{
  int v = gl_VertexID % 3;
  float x = -1.0 + float((v & 1) << 2);
  float y = -1.0 + float((v & 2) << 1);
  gl_Position = vec4(x, y, 1.0, 1.0);
  uvs = (gl_Position.xy * 0.5 + 0.5).xy;

#if STEP == 0
  SMAAEdgeDetectionVS(uvs, offsets);
#elif STEP == 1
  SMAABlendingWeightCalculationVS(uvs, pixcoord, offsets);
#elif STEP == 2
  SMAANeighborhoodBlendingVS(uvs, offset);
#endif
}