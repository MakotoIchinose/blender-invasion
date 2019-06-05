
/* Needed to recover the full call resource handle.
 * Here used to have the unique ID of the drawcall. */
// uniform int resPoolID;

in vec3 pos;

#ifdef USE_GEOM
out vec3 vPos;
out int objectId_g;
#  define objectId objectId_g
#else

flat out int objectId;
#endif

void main()
{
  vec3 world_pos = point_object_to_world(pos);
#ifdef USE_GEOM
  vPos = point_world_to_view(world_pos);
#endif
  gl_Position = point_world_to_ndc(world_pos);
  /* Small bias to always be on top of the geom. */
  gl_Position.z -= 1e-3;

  /* ID 0 is nothing (background) */
  objectId = resource_id + 1;

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(world_pos);
#endif
}
