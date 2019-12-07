
in int mat;
in vec3 pos;

void main()
{
  gl_Position = point_world_to_ndc(pos);

  if (mat == -1) {
    /* Enpoints, we discard the vertices. */
    gl_Position = vec4(0.0, 0.0, -3e36, 0.0);
  }
}
