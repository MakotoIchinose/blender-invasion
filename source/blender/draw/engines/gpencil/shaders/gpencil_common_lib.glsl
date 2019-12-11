
/* Must match C declaration. */
struct gpMaterial {
  vec4 stroke_color;
  vec4 fill_color;
  vec4 fill_uv_rot_scale;
  vec4 fill_uv_offset;
  /* Put float/int at the end to avoid padding error */
  float stroke_uv_factor;
  float fill_texture_mix;
  float fill_uv_y_scale;
  int flag;
  /* Please ensure 16 byte alignment (multiple of vec4). */
};

/* flag */
#define GP_STROKE_ALIGNMENT_STROKE 1
#define GP_STROKE_ALIGNMENT_OBJECT 2
#define GP_STROKE_ALIGNMENT_FIXED 3
#define GP_STROKE_ALIGNMENT 0x3
#define GP_STROKE_OVERLAP (1 << 2)
#define GP_STROKE_TEXTURE_USE (1 << 3)
#define GP_STROKE_TEXTURE_STENCIL (1 << 4)
#define GP_STROKE_TEXTURE_PREMUL (1 << 5)
#define GP_FILL_TEXTURE_USE (1 << 10)
#define GP_FILL_TEXTURE_PREMUL (1 << 11)
#define GP_FILL_TEXTURE_CLIP (1 << 12)

#define GP_FILL_FLAGS (GP_FILL_TEXTURE_USE | GP_FILL_TEXTURE_PREMUL | GP_FILL_TEXTURE_CLIP)

#define GP_FLAG_TEST(flag, val) (((flag) & (val)) != 0)

layout(std140) uniform gpMaterialBlock
{
  gpMaterial materials[GPENCIL_MATERIAL_BUFFER_LEN];
};
