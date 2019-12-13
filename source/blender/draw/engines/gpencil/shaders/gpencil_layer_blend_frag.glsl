
uniform sampler2D colorBuf;
uniform sampler2D revealBuf;
uniform int blendMode;
uniform float blendOpacity;

in vec4 uvcoordsvar;

/* Reminder: This is considered SRC color in blend equations.
 * Same operation on all buffers. */
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragRevealage;

/* Must match eGPLayerBlendModes */
#define MODE_REGULAR 0
#define MODE_OVERLAY 1
#define MODE_ADD 2
#define MODE_SUB 3
#define MODE_MULTIPLY 4
#define MODE_DIVIDE 5
#define MODE_OVERLAY_SECOND_PASS 999

void main()
{
  vec4 color;

  /* Remember, this is associated alpha (aka. premult). */
  color.rgb = textureLod(colorBuf, uvcoordsvar.xy, 0).rgb;
  /* Stroke only render mono-chromatic revealage. We convert to alpha. */
  color.a = 1.0 - textureLod(revealBuf, uvcoordsvar.xy, 0).r;

  fragColor = vec4(1.0, 0.0, 1.0, 1.0);
  fragRevealage = vec4(1.0, 0.0, 1.0, 1.0);

  switch (blendMode) {
    case MODE_REGULAR:
      /* Reminder: Blending func is premult alpha blend (dst.rgba * (1 - src.a) + src.rgb).*/
      color *= blendOpacity;
      fragColor = color;
      fragRevealage = vec4(0.0, 0.0, 0.0, color.a);
      break;
    case MODE_MULTIPLY:
      /* Reminder: Blending func is multiply blend (dst.rgba * src.rgba).*/
      color.a *= blendOpacity;
      fragRevealage = fragColor = (1.0 - color.a) + color.a * color;
      break;
    case MODE_DIVIDE:
      /* Reminder: Blending func is multiply blend (dst.rgba * src.rgba).*/
      color.a *= blendOpacity;
      fragRevealage = fragColor = clamp(1.0 / (1.0 - color * color.a), 0.0, 1e18);
      break;
    case MODE_OVERLAY:
      /* Reminder: Blending func is multiply blend (dst.rgba * src.rgba).*/
      /**
       * We need to separate the overlay equation into 2 term (one mul and one add).
       * This is the standard overlay equation (per channel):
       * rtn = (src < 0.5) ? (2.0 * src * dst) : (1.0 - 2.0 * (1.0 - src) * (1.0 - dst));
       * We rewrite the second branch like this:
       * rtn = 1 - 2 * (1 - src) * (1 - dst);
       * rtn = 1 - 2 (1 - dst + src * dst - src);
       * rtn = 1 - 2 (1 - dst * (1 - src) - src);
       * rtn = 1 - 2 + dst * (2 - 2 * src) + 2 * src;
       * rtn = (- 1 + 2 * src) + dst * (2 - 2 * src);
       **/
      color = mix(vec4(0.5), color, color.a * blendOpacity);
      vec4 s = step(0.5, color);
      fragRevealage = fragColor = 2.0 * s + 2.0 * color * (1.0 - s * 2.0);
      break;
    case MODE_OVERLAY_SECOND_PASS:
      /* Reminder: Blending func is additive blend (dst.rgba + src.rgba).*/
      color = mix(vec4(0.5), color, color.a * blendOpacity);
      fragRevealage = fragColor = (-1.0 + 2.0 * color) * step(0.5, color);
      break;
    case MODE_SUB:
    case MODE_ADD:
      /* Reminder: Blending func is additive / subtractive blend (dst.rgba +/- src.rgba).*/
      color *= blendOpacity;
      fragColor = color * color.a;
      fragRevealage = vec4(0.0);
      break;
  }
}
