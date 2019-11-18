
uniform vec4 color;

noperspective in vec2 stipple_coord;
flat in vec2 stipple_start;
flat in vec4 finalColor;

out vec4 fragColor;

void main()
{
  fragColor = finalColor;

  /* Stipple */
  const float dash_width = 6.0;
  const float dash_factor = 0.5;

  float dist = distance(stipple_start, stipple_coord);
  fragColor.a = 1.0;

  if (fract(dist / dash_width) > dash_factor) {
    discard;
  }
}