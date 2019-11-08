
in vec3 pos;
in vec3 dash_pos;   /* Start point of the stippling pattern. */
in float dash_with; /* Screen space width of the stippling. */
in vec4 color;
in int colorid; /* if equal 0 use color attrib. */

flat out vec4 finalColor;

void main()
{
  vec3 world_pos = point_object_to_world(pos);
  gl_Position = point_world_to_ndc(world_pos);

  if (colorid == TH_CAMERA_PATH) {
    finalColor = colorCameraPath;
  }
  else {
    finalColor = color;
  }

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(world_pos);
#endif
}