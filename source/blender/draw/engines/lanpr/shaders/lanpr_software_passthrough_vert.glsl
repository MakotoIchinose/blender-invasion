in vec4 pos;
in vec2 uvs;
in vec3 normal;
in int type;
in int level;

out vec2 gOffset;
out int gType;
out int gLevel;
out vec3 gNormal;

void main()
{
  vec4 p = pos;

  gOffset = uvs;
  gType = type;
  gLevel = level;
  gNormal = normal;
  gl_Position = vec4(vec3(p), 1);
}
