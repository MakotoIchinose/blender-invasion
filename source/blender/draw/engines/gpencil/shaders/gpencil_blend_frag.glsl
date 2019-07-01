
uniform sampler2D strokeColor;
uniform int mode;

#define MODE_REGULAR 0
#define MODE_OVERLAY 1
#define MODE_ADD 2
#define MODE_SUB 3
#define MODE_MULTIPLY 4
#define MODE_DIVIDE 5

/* Blend equation is : FragColor0 + FragColor1 * DstColor */
layout(location = 0, index = 0) out vec4 FragColor0;
layout(location = 0, index = 1) out vec4 FragColor1;

void main()
{
  ivec2 texel = ivec2(gl_FragCoord.xy);
  vec4 src = texelFetch(strokeColor, texel, 0).rgba;

  switch (mode) {
    case MODE_REGULAR:
      FragColor0 = src;
      FragColor1 = 1.0 - vec4(src.a);
      break;
    case MODE_OVERLAY:
      /* Original overlay is :
       * overlay = (a < 0.5) ? (2 * a * b) : (1 - 2 * (1 - a) * (1 - b));
       * But we are using dual source blending. So we need to rewrite it as a function of b (DST).
       * overlay = 1 - 2 * (1 - a) * (1 - b);
       * overlay = 1 + (2 * a - 2) * (1 - b);
       * overlay = 1 + (2 * a - 2) - (2 * a - 2) * b;
       * overlay = (2 * a - 1) + (2 - 2 * a) * b;
       *
       * With the right coefficient, we can get the 2 * a * b from the same equation.
       * q0 = 0, q1 = -1;
       * overlay = (2 * a - 1) * q0 + (2 * q0 - 2 * q1 * a) * b;
       * overlay = 2 * a * b;
       * */
      src.rgb = mix(vec3(0.5), src.rgb, src.a);
      vec3 q0 = vec3(greaterThanEqual(src.rgb, vec3(0.5)));
      vec3 q1 = q0 * 2.0 - 1.0;
      vec3 src2 = 2.0 * src.rgb;
      FragColor0.rgb = src2 * q0 - q0;
      FragColor1.rgb = 2.0 * q0 - src2 * q1;
      break;
    case MODE_ADD:
      FragColor0.rgb = src.rgb * src.a;
      FragColor1.rgb = vec3(1.0);
      break;
    case MODE_SUB:
      FragColor0.rgb = -src.rgb * src.a;
      FragColor1.rgb = vec3(1.0);
      break;
    case MODE_MULTIPLY:
      FragColor0.rgb = vec3(0.0);
      FragColor1.rgb = mix(vec3(1.0), src.rgb, src.a);
      break;
    case MODE_DIVIDE:
      FragColor0.rgb = vec3(0.0);
      FragColor1.rgb = 1.0 / mix(vec3(1.0), src.rgb, src.a);
      break;
    default:
      FragColor0 = vec4(0.0);
      FragColor1 = vec4(1.0);
      break;
  }

  FragColor0.a = 1.0 - clamp(dot(vec3(1.0 / 3.0), FragColor1.rgb), 0.0, 1.0);
  FragColor1.a = 1.0 - FragColor0.a;
}
