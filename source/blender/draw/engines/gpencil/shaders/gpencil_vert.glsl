
in vec3 pos;

void main()
{
  gl_Position = point_world_to_ndc(pos);
}
