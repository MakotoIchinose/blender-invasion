
/* Copied from BLI_bitmap.h */
/* 2^5 = 32 (bits) */
#define _BITMAP_POWER 5u
/* 0b11111 */
#define _BITMAP_MASK 31u

/* number of blocks needed to hold '_tot' bits */
#define _BITMAP_NUM_BLOCKS(_tot) (((_tot) >> _BITMAP_POWER) + 1)

uniform int offset;
uniform int tot_index;
uniform sampler2D depthBuffer;

in vec3 pos;
in uint color;

flat out uint bit;

float normalize_factor = 2.0 / _BITMAP_NUM_BLOCKS(tot_index);

void main()
{
  vec3 world_pos = point_object_to_world(pos);
  vec4 ndc_pos = point_world_to_ndc(world_pos);

  float depth = texture(depthBuffer, ndc_pos.xy).x;
  float r_pos;

  if (false && (depth * ndc_pos.w) > ndc_pos.z) {
    bit = 0u;
    r_pos = -1.0;
  }
  else {
    uint index = floatBitsToUint(intBitsToFloat(offset)) + color;
    bit = 1u << (index & _BITMAP_MASK);
    r_pos = (index >> _BITMAP_POWER) * normalize_factor - 1.0;
  }

  gl_Position = vec4(r_pos, 0.0, 0.0, 1.0);
  gl_PointSize = 1.0;

#ifdef USE_WORLD_CLIP_PLANES
  world_clip_planes_calc_clip_distance(world_pos);
#endif
}
