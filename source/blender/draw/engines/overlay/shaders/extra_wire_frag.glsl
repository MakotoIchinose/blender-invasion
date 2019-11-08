
uniform vec4 color;

noperspective in vec4 finalColor;

out vec4 fragColor;

void main()
{
  fragColor = finalColor;

  if (fragColor.a < 0.0) {
    /* Stipple */
    const float dash_width = 6.0;
    const float dash_factor = 0.5;

    float dist = abs(fragColor.a);
    fragColor.a = 1.0;

    if (fract(dist / dash_width) > dash_factor) {
      discard;
    }
  }
}