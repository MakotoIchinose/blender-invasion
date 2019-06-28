
uniform sampler2D strokeDepth;

void main()
{
  ivec2 texel = ivec2(gl_FragCoord.xy);
  gl_FragDepth = texelFetch(strokeDepth, texel, 0).r;
}
