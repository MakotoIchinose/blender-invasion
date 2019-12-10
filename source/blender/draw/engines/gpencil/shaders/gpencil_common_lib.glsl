
/* Must match C declaration. */
struct gpMaterial {
  vec4 stroke_color;
  vec4 fill_color;
};

layout(std140) uniform gpMaterialBlock
{
  gpMaterial materials[GPENCIL_MATERIAL_BUFFER_LEN];
};
