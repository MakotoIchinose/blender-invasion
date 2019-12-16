
uniform sampler2D colorBuf;
uniform sampler2D revealBuf;
uniform sampler2D maskBuf;
uniform float maskOpacity;
uniform bool isFirstPass;

in vec4 uvcoordsvar;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragRevealage;

void main()
{
  vec3 masked_color = texture(colorBuf, uvcoordsvar.xy).rgb;
  vec3 masked_reveal = texture(revealBuf, uvcoordsvar.xy).rgb;
  float mask = 1.0 - maskOpacity * texture(maskBuf, uvcoordsvar.xy).r;

  if (isFirstPass) {
    /* Blend mode is multiply. */
    fragColor.rgb = fragRevealage.rgb = mix(vec3(1.0), masked_reveal, mask);
    fragColor.a = fragRevealage.a = 1.0;
  }
  else {
    /* Blend mode is additive. */
    fragRevealage = vec4(0.0);
    fragColor.rgb = mix(vec3(0.0), masked_color, mask);
    fragColor.a = 0.0;
  }
}
