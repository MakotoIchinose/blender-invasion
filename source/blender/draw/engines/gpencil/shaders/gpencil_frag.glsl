
uniform sampler2D gpFillTexture;
uniform sampler2D gpStrokeTexture;

in vec4 finalColor;
in vec2 finalUvs;
flat in int matFlag;

out vec4 fragColor;

void main()
{
  if (GP_FLAG_TEST(matFlag, GP_STROKE_TEXTURE_USE)) {
    bool premul = GP_FLAG_TEST(matFlag, GP_STROKE_TEXTURE_PREMUL);
    fragColor = texture_read_as_srgb(gpStrokeTexture, premul, finalUvs);
  }
  else if (GP_FLAG_TEST(matFlag, GP_FILL_TEXTURE_USE)) {
    bool use_clip = GP_FLAG_TEST(matFlag, GP_FILL_TEXTURE_CLIP);
    vec2 uvs = (use_clip) ? clamp(finalUvs, 0.0, 1.0) : finalUvs;
    bool premul = GP_FLAG_TEST(matFlag, GP_FILL_TEXTURE_PREMUL);
    fragColor = texture_read_as_srgb(gpFillTexture, premul, uvs) * finalColor;
  }
  else {
    fragColor = finalColor;
  }
}
