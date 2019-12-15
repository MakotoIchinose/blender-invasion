
uniform sampler2D colorBuf;
uniform sampler2D revealBuf;
uniform sampler2D maskBuf;
uniform float maskOpacity;
uniform bool isFirstPass;

in vec4 uvcoordsvar;

out vec4 fragColor;

void main()
{
  vec3 masked_color = texture(colorBuf, uvcoordsvar.xy).rgb;
  vec3 masked_reveal = texture(revealBuf, uvcoordsvar.xy).rgb;
  float mask = 1.0 - maskOpacity * texture(maskBuf, uvcoordsvar.xy).r;

  if (isFirstPass) {
    /* Blend mode is multiply. */
    fragColor.rgb = mix(vec3(1.0), masked_reveal, mask);
    fragColor.a = 1.0;
  }
  else {
    /* Blend mode is additive. */
    fragColor.rgb = mix(vec3(0.0), masked_color, mask);
    fragColor.a = 0.0;
  }
}
