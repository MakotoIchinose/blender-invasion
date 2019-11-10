/* Need dedicated obmat since we use instancing attribs
 * (we cannot let have baseinstance mess them). */
uniform vec4 editModelMat[4];

/* ---- Instantiated Attrs ---- */
in vec2 pos;

/* ---- Per instance Attrs ---- */
in float size;
in vec3 local_pos;

flat out vec4 finalColor;

void main()
{
  mat4 obmat = mat4(editModelMat[0], editModelMat[1], editModelMat[2], editModelMat[3]);
  /* Could be optimized... but it is only for a handful of verts so not a priority. */
  mat3 imat = inverse(mat3(obmat));
  vec3 right = normalize(imat * screenVecs[0].xyz);
  vec3 up = normalize(imat * screenVecs[1].xyz);
  vec3 screen_pos = (right * pos.x + up * pos.y) * size;
  vec4 pos_4d = obmat * vec4(local_pos + screen_pos, 1.0);
  gl_Position = ViewProjectionMatrix * pos_4d;
  finalColor = colorSkinRoot;

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(pos_4d.xyz);
#endif
}
