
in vec3 pos;
in vec3 dash_pos; /* Start point of the stippling pattern. */
in vec4 color;
in int colorid; /* if equal 0 (i.e: Not specified) use color attrib and stippling. */

noperspective out vec4 finalColor;

vec2 screen_position(vec4 p)
{
  vec2 s = p.xy / p.w;
  return s * sizeViewport.xy * 0.5;
}

void main()
{
  vec3 world_pos = point_object_to_world(pos);
  gl_Position = point_world_to_ndc(world_pos);

  if (colorid == TH_CAMERA_PATH) {
    finalColor = colorCameraPath;
  }
  else {
    finalColor = color;

    world_pos = point_object_to_world(dash_pos);

    vec2 s0 = screen_position(gl_Position);

    vec4 p = point_world_to_ndc(world_pos);
    vec2 s1 = screen_position(p);
    /* Put Stipple distance in alpha. */
    finalColor.a = -distance(s0, s1);
  }

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(world_pos);
#endif
}