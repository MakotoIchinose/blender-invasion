
uniform sampler2D colorBuf;
uniform sampler2D revealBuf;

in vec4 uvcoordsvar;

/* Reminder: This is considered SRC color in blend equations.
 * Same operation on all buffers. */
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragRevealage;

#if defined(COMPOSITE)

uniform bool isFirstPass;

void main()
{
  if (isFirstPass) {
    /* Blend mode is multiply. */
    fragColor.rgb = fragRevealage.rgb = texture(revealBuf, uvcoordsvar.xy).rgb;
    fragColor.a = fragRevealage.a = 1.0;
  }
  else {
    /* Blend mode is additive. */
    fragRevealage = vec4(0.0);
    fragColor.rgb = texture(colorBuf, uvcoordsvar.xy).rgb;
    fragColor.a = 0.0;
  }
}

#elif defined(BLUR)

uniform vec2 offset;
uniform int sampCount;

float gaussian_weight(float x)
{
  return exp(-x * x / (2.0 * 0.35 * 0.35));
}

void main()
{
  vec2 pixel_size = 1.0 / vec2(textureSize(revealBuf, 0).xy);
  vec2 ofs = offset * pixel_size;

  fragColor = vec4(0.0);
  fragRevealage = vec4(0.0);

  /* No blending. */
  float weight_accum = 0.0;
  for (int i = -sampCount; i <= sampCount; i++) {
    float x = float(i) / float(sampCount);
    float weight = gaussian_weight(x);
    weight_accum += weight;
    fragColor.rgb += texture(colorBuf, uvcoordsvar.xy - ofs * x).rgb * weight;
    fragRevealage.rgb += texture(revealBuf, uvcoordsvar.xy - ofs * x).rgb * weight;
  }

  fragColor /= weight_accum;
  fragRevealage /= weight_accum;
}

#endif