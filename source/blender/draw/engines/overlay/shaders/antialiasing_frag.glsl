
in vec2 uvs;

#if STEP == 0

uniform sampler2D colorTex;

in vec4 offsets[3];

out vec4 fragColor;

void main()
{
  fragColor = SMAALumaEdgeDetectionPS(uvs, offsets, colorTex).xyxy;
}

#elif STEP == 1

uniform sampler2D edgesTex;
uniform sampler2D areaTex;
uniform sampler2D searchTex;

in vec2 pixcoord;
in vec4 offsets[3];

out vec4 fragColor;

void main()
{
  fragColor = SMAABlendingWeightCalculationPS(
      uvs, pixcoord, offsets, edgesTex, areaTex, searchTex, vec4(0));
}

#elif STEP == 2

uniform sampler2D colorTex;
uniform sampler2D blendTex;

in vec4 offset;

out vec4 fragColor;

void main()
{
  fragColor = SMAANeighborhoodBlendingPS(uvs, offset, colorTex, blendTex);
}

#endif
