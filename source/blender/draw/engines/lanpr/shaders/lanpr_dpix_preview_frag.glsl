in vec4 color_out;

void main()
{
  gl_FragData[0] = vec4(color_out.rgb, 1);
}
