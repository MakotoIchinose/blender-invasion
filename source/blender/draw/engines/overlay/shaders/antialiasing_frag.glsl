
uniform sampler2D srcTexture;
uniform float alpha;

in vec4 uvcoordsvar;

#ifdef USE_ACCUM
layout(location = 0, index = 0) out vec4 fragColor;
layout(location = 0, index = 1) out vec4 historyWeight;
#else
out vec4 fragColor;
#endif

void main()
{
#ifdef USE_ACCUM
  fragColor = texture(srcTexture, uvcoordsvar.st) * alpha;
  historyWeight = vec4(1.0 - alpha);
#elif defined(USE_FXAA)
  vec2 invSize = 1.0 / vec2(textureSize(srcTexture, 0).xy);
  fragColor = FxaaPixelShader(uvcoordsvar.st, srcTexture, invSize, 0.75, 0.063, 0.0312);
#else
  fragColor = texture(srcTexture, uvcoordsvar.st) * alpha;
#endif
}
