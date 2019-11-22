
uniform vec3 wireColor;
uniform vec3 rimColor;

in vec3 finalColor;
flat in float edgeSharpness;

out vec4 fragColor;

void main()
{
  if (edgeSharpness < 0.0) {
    discard;
  }

  fragColor.rgb = finalColor;
  fragColor.a = 1.0f;
}
