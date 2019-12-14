
uniform sampler2D depthBuf;
uniform float strokeDepth2d;
uniform bool strokeOrder3d;

in vec4 uvcoordsvar;

void main()
{
  float depth = textureLod(depthBuf, uvcoordsvar.xy, 0).r;

  if (strokeOrder3d) {
    gl_FragDepth = depth;
  }
  else {
    vec4 p = ProjectionMatrix * vec4(0.0, 0.0, strokeDepth2d, 1.0);
    gl_FragDepth = (depth != 0) ? ((p.z / p.w) * 0.5 + 0.5) : 1.0;
  }
}
