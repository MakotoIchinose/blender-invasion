
uniform vec4 color;

flat in vec4 finalColor;

out vec4 fragColor;

void main()
{
  fragColor = finalColor;
}