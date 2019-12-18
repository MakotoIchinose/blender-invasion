
uniform sampler2D colorBuf;
uniform sampler2D revealBuf;

in vec4 uvcoordsvar;

/* Reminder: This is considered SRC color in blend equations.
 * Same operation on all buffers. */
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragRevealage;

float gaussian_weight(float x)
{
  return exp(-x * x / (2.0 * 0.35 * 0.35));
}

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
    vec2 uv = uvcoordsvar.xy + ofs * x;
    fragColor.rgb += texture(colorBuf, uv).rgb * weight;
    fragRevealage.rgb += texture(revealBuf, uv).rgb * weight;
  }

  fragColor /= weight_accum;
  fragRevealage /= weight_accum;
}

#elif defined(GLOW)

uniform vec3 glowColor;
uniform vec2 offset;
uniform int sampCount;
uniform vec3 threshold;
uniform bool useAlphaMode;

void main()
{
  vec2 pixel_size = 1.0 / vec2(textureSize(revealBuf, 0).xy);
  vec2 ofs = offset * pixel_size;

  fragColor = vec4(0.0);

  /* In first pass we copy the reveal buffer. This let us do the alpha under if needed. */
  fragRevealage = texture(revealBuf, uvcoordsvar.xy);

  float weight_accum = 0.0;
  for (int i = -sampCount; i <= sampCount; i++) {
    float x = float(i) / float(sampCount);
    float weight = gaussian_weight(x);
    weight_accum += weight;
    vec2 uv = uvcoordsvar.xy + ofs * x;
    vec3 col = texture(colorBuf, uv).rgb;
    if (threshold.x > -1.0) {
      if (threshold.y > -1.0) {
        if (all(lessThan(vec3(0.05), abs(col - threshold)))) {
          weight = 0.0;
        }
      }
      else {
        if (dot(col, vec3(1.0 / 3.0)) < threshold.x) {
          weight = 0.0;
        }
      }
    }
    fragColor.rgb += col * weight;
  }

  fragColor *= glowColor.rgbb / weight_accum;

  if (useAlphaMode) {
    /* Equivalent to alpha under. */
    fragColor *= fragRevealage;
  }

  if (threshold.x == -1.0) {
    /* Blend Mode is additive in 2nd pass. Don't modify revealage. */
    fragRevealage = vec4(0.0);
  }
}

#elif defined(SHADOW)

uniform vec4 shadowColor;
uniform vec2 uvRotX;
uniform vec2 uvRotY;
uniform vec2 uvOffset;
uniform vec2 blurDir;
uniform vec2 waveDir;
uniform vec2 waveOffset;
uniform float wavePhase;
uniform int sampCount;
uniform bool isFirstPass;

vec2 compute_uvs(float x)
{
  vec2 uv = uvcoordsvar.xy;
  /* Tranform UV (loc, rot, scale) */
  uv = uv.x * uvRotX + uv.y * uvRotY + uvOffset;
  uv += blurDir * x;
  /* Wave deform. */
  float wave_time = dot(uv, waveDir.xy);
  uv += sin(wave_time + wavePhase) * waveOffset;
  return uv;
}

void main()
{
  /* Blur revealage buffer. */
  fragRevealage = vec4(0.0);
  float weight_accum = 0.0;
  for (int i = -sampCount; i <= sampCount; i++) {
    float x = float(i) / float(sampCount);
    float weight = gaussian_weight(x);
    weight_accum += weight;
    vec2 uv = compute_uvs(x);
    vec3 col = texture(revealBuf, uv).rgb;
    if (any(not(equal(vec2(0.0), floor(uv))))) {
      col = vec3(1.0);
    }
    fragRevealage.rgb += col * weight;
  }
  fragRevealage /= weight_accum;

  /* No blending in first pass, alpha over premult in second pass. */
  if (isFirstPass) {
    /* In first pass we copy the reveal buffer. This let us do alpha under in second pass. */
    fragColor = texture(revealBuf, uvcoordsvar.xy);
  }
  else {
    /* fragRevealage is blurred shadow. */
    float shadow_fac = 1.0 - clamp(dot(vec3(0.333334), fragRevealage.rgb), 0.0, 1.0);
    /* Premult by foreground revealage (alpha under). */
    vec3 original_revealage = texture(colorBuf, uvcoordsvar.xy).rgb;
    shadow_fac *= clamp(dot(vec3(0.333334), original_revealage), 0.0, 1.0);
    /* Modulate by opacity */
    shadow_fac *= shadowColor.a;
    /* Apply shadow color. */
    fragColor.rgb = mix(vec3(0.0), shadowColor.rgb, shadow_fac);
    /* Alpha over (mask behind the shadow). */
    fragColor.a = shadow_fac;

    fragRevealage.rgb = original_revealage * (1.0 - shadow_fac);
    /* Replace the whole revealage buffer. */
    fragRevealage.a = 1.0;
  }
}

#elif defined(PIXELIZE)

uniform vec2 targetPixelSize;
uniform vec2 targetPixelOffset;
uniform vec2 accumOffset;
uniform int sampCount;

void main()
{
  vec2 pixel = floor((uvcoordsvar.xy - targetPixelOffset) / targetPixelSize);
  vec2 uv = (pixel + 0.5) * targetPixelSize + targetPixelOffset;

  fragColor = vec4(0.0);
  fragRevealage = vec4(0.0);

  for (int i = -sampCount; i <= sampCount; i++) {
    float x = float(i) / float(sampCount + 1);
    vec2 uv_ofs = uv + accumOffset * 0.5 * x;
    fragColor += texture(colorBuf, uv_ofs);
    fragRevealage += texture(revealBuf, uv_ofs);
  }

  fragColor /= float(sampCount) * 2.0 + 1.0;
  fragRevealage /= float(sampCount) * 2.0 + 1.0;
}

#endif