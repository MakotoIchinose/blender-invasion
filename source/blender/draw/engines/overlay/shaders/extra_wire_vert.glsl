
in vec3 pos;
in vec4 color;
in int colorid; /* if equal 0 (i.e: Not specified) use color attrib and stippling. */

noperspective out vec2 stipple_coord;
flat out vec2 stipple_start;
flat out vec4 finalColor;

vec2 screen_position(vec4 p)
{
  vec2 s = p.xy / p.w;
  return s * sizeViewport.xy * 0.5;
}

void main()
{
  vec3 world_pos = point_object_to_world(pos);
  gl_Position = point_world_to_ndc(world_pos);

  stipple_coord = stipple_start = vec2(0.0);

#ifdef OBJECT_WIRE
  /* Extract data packed inside the unused mat4 members. */
  finalColor = vec4(ModelMatrix[0][3], ModelMatrix[1][3], ModelMatrix[2][3], ModelMatrix[3][3]);
#else

  if (colorid == TH_CAMERA_PATH) {
    finalColor = colorCameraPath;
  }
  else {
    finalColor = color;
    stipple_coord = stipple_start = screen_position(gl_Position);
  }
#endif

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(world_pos);
#endif
}