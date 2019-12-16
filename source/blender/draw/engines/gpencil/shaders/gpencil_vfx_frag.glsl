
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

void main()
{
  vec2 pixel_size = 1.0 / vec2(textureSize(revealBuf, 0).xy);
  vec2 ofs = offset * pixel_size;

  fragColor = vec4(0.0);
  fragRevealage = vec4(0.0);

  /* No blending. */
  fragColor.rgb += texture(colorBuf, uvcoordsvar.xy - ofs).rgb;
  fragColor.rgb += texture(colorBuf, uvcoordsvar.xy).rgb;
  fragColor.rgb += texture(colorBuf, uvcoordsvar.xy + ofs).rgb;

  fragRevealage.rgb += texture(revealBuf, uvcoordsvar.xy - ofs).rgb;
  fragRevealage.rgb += texture(revealBuf, uvcoordsvar.xy).rgb;
  fragRevealage.rgb += texture(revealBuf, uvcoordsvar.xy + ofs).rgb;

  fragColor *= (1.0 / 3.0);
  fragRevealage *= (1.0 / 3.0);
}

#endif