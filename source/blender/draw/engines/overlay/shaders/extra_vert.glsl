
uniform vec3 screen_vecs[2];
uniform float pixel_size;

in vec3 pos;
in int vclass;

/* Instance */
in mat4 inst_obmat;
in vec4 color;

#define lamp_area_size inst_data.xy
#define lamp_clip_sta inst_data.z
#define lamp_clip_end inst_data.w

#define lamp_spot_cosine inst_data.x
#define lamp_spot_blend inst_data.y

#define VCLASS_LIGHT_AREA_SHAPE (1 << 0)
#define VCLASS_LIGHT_SPOT_SHAPE (1 << 1)
#define VCLASS_LIGHT_SPOT_BLEND (1 << 2)
#define VCLASS_LIGHT_SPOT_CONE (1 << 3)
#define VCLASS_LIGHT_DIST (1 << 4)

#define VCLASS_SCREENSPACE (1 << 8)
#define VCLASS_SCREENALIGNED (1 << 9)

flat out vec4 finalColor;

float mul_project_m4_v3_zfac(in vec3 co)
{
  return (ViewProjectionMatrix[0][3] * co.x) + (ViewProjectionMatrix[1][3] * co.y) +
         (ViewProjectionMatrix[2][3] * co.z) + ViewProjectionMatrix[3][3];
}

void main()
{
  /* Extract data packed inside the unused mat4 members. */
  vec4 inst_data = vec4(inst_obmat[0][3], inst_obmat[1][3], inst_obmat[2][3], inst_obmat[3][3]);
  mat4 obmat = inst_obmat;
  obmat[0][3] = obmat[1][3] = obmat[2][3] = 0.0;
  obmat[3][3] = 1.0;

  float lamp_spot_sine;
  vec3 vpos = pos;
  vec3 vofs = vec3(0.0);
  if ((vclass & VCLASS_LIGHT_AREA_SHAPE) != 0) {
    /* HACK: use alpha color for spots to pass the area_size. */
    if (color.a < 0.0) {
      lamp_area_size.xy = vec2(-color.a);
    }
    vpos.xy *= lamp_area_size.xy;
  }
  else if ((vclass & VCLASS_LIGHT_SPOT_SHAPE) != 0) {
    lamp_spot_sine = sqrt(1.0 - lamp_spot_cosine * lamp_spot_cosine);
    lamp_spot_sine *= ((vclass & VCLASS_LIGHT_SPOT_BLEND) != 0) ? lamp_spot_blend : 1.0;
    vpos = vec3(pos.xy * lamp_spot_sine, -lamp_spot_cosine);
  }
  else if ((vclass & VCLASS_LIGHT_DIST) != 0) {
    vofs.xy = vec2(0.0);
    vofs.z = -mix(lamp_clip_sta, lamp_clip_end, pos.z) / length(obmat[2].xyz);
    vpos.z = 0.0;
  }

  finalColor = color;
  if (color.a < 0.0) {
    finalColor.a = 1.0;
  }

  vec3 world_pos;
  if ((vclass & VCLASS_SCREENSPACE) != 0) {
    /* Relative to DPI scalling. Have constant screen size. */
    vec3 screen_pos = screen_vecs[0].xyz * vpos.x + screen_vecs[1].xyz * vpos.y;
    vec3 p = (obmat * vec4(vofs, 1.0)).xyz;
    float screen_size = mul_project_m4_v3_zfac(p) * pixel_size;
    world_pos = p + screen_pos * screen_size;
  }
  else if ((vclass & VCLASS_SCREENALIGNED) != 0) {
    /* World sized, camera facing geometry. */
    vec3 screen_pos = screen_vecs[0].xyz * vpos.x + screen_vecs[1].xyz * vpos.y;
    world_pos = (obmat * vec4(vofs, 1.0)).xyz + screen_pos;
  }
  else {
    world_pos = (obmat * vec4(vofs + vpos, 1.0)).xyz;
  }

  if ((vclass & VCLASS_LIGHT_SPOT_CONE) != 0) {
    /* Compute point on the cone before and after this one. */
    vec2 perp = vec2(pos.y, -pos.x);
    const float incr_angle = 2.0 * 3.1415 / 32.0;
    const vec2 slope = vec2(cos(incr_angle), sin(incr_angle));
    vec3 p0 = vec3((pos.xy * slope.x + perp * slope.y) * lamp_spot_sine, -lamp_spot_cosine);
    vec3 p1 = vec3((pos.xy * slope.x - perp * slope.y) * lamp_spot_sine, -lamp_spot_cosine);
    p0 = (obmat * vec4(p0, 1.0)).xyz;
    p1 = (obmat * vec4(p1, 1.0)).xyz;
    /* Compute normals of each side. */
    vec3 edge = obmat[3].xyz - world_pos;
    vec3 n0 = normalize(cross(edge, p0 - world_pos));
    vec3 n1 = normalize(cross(edge, world_pos - p1));
    bool persp = (ProjectionMatrix[3][3] == 0.0);
    vec3 V = (persp) ? normalize(ViewMatrixInverse[3].xyz - world_pos) : ViewMatrixInverse[2].xyz;
    /* Discard non-silhouete edges.  */
    bool facing0 = dot(n0, V) > 0.0;
    bool facing1 = dot(n1, V) > 0.0;
    if (facing0 == facing1) {
      /* Hide line by making it cover 0 pixels. */
      world_pos = obmat[3].xyz;
    }
  }

  gl_Position = point_world_to_ndc(world_pos);
}