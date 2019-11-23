
uniform sampler2D srcTexture;
uniform float alpha;

in vec4 uvcoordsvar;

out vec4 fragColor;

void main()
{
#ifdef USE_FXAA
  vec2 invSize = 1.0 / vec2(textureSize(srcTexture, 0).xy);
  fragColor = FxaaPixelShader(uvcoordsvar.st, srcTexture, invSize, 1.0, 0.063, 0.0312);
#else
  fragColor = texture(srcTexture, uvcoordsvar.st) * alpha;
#endif
}
