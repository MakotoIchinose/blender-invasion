
uniform vec3 screen_vecs[2];
uniform float pixel_size;

in vec3 pos;

/* Instance */
in vec3 inst_pos;

flat out vec4 finalColor;

float mul_project_m4_v3_zfac(in vec3 co)
{
  return (ViewProjectionMatrix[0][3] * co.x) + (ViewProjectionMatrix[1][3] * co.y) +
         (ViewProjectionMatrix[2][3] * co.z) + ViewProjectionMatrix[3][3];
}

void main()
{
  finalColor = colorLight;

  /* Relative to DPI scalling. Have constant screen size. */
  vec3 screen_pos = screen_vecs[0].xyz * pos.x + screen_vecs[1].xyz * pos.y;
  vec3 p = inst_pos;
  p.z *= (pos.z == 0.0) ? 0.0 : 1.0;
  float screen_size = mul_project_m4_v3_zfac(p) * pixel_size;
  vec3 world_pos = p + screen_pos * screen_size;

  gl_Position = point_world_to_ndc(world_pos);
}