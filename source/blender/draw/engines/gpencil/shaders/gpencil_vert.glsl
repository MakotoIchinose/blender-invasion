
/* TODO UBO */
uniform vec2 sizeViewport;
uniform vec2 sizeViewportInv;

in int ma1;
in int ma2;
in vec3 pos;  /* Prev adj vert */
in vec3 pos1; /* Current edge */
in vec3 pos2; /* Current edge */
in vec3 pos3; /* Next adj vert */

out vec4 finalColor;

void discard_vert()
{
  /* We set the vertex at the camera origin to generate 0 fragments. */
  gl_Position = vec4(0.0, 0.0, -3e36, 0.0);
}

vec2 project_to_screenspace(vec4 v)
{
  return ((v.xy / v.w) * 0.5 + 0.5) * sizeViewport;
}

void main()
{
  /* Trick to detect if a drawcall is stroke or fill. */
  bool is_fill = (gl_InstanceID == 0);

  if (!is_fill) {
    /* Enpoints, we discard the vertices. */
    if (ma1 == -1 || ma2 == -1) {
      discard_vert();
      return;
    }

    /* Avoid using a vertex attrib for quad positioning. */
    float x = float((gl_VertexID & 1));
    float y = float((gl_VertexID & 2) >> 1);

    vec4 ndc1 = point_world_to_ndc(pos1);
    vec4 ndc2 = point_world_to_ndc(pos2);

    /* TODO case where ndc2 is behind camera */
    vec2 ss1 = project_to_screenspace(ndc1);
    vec2 ss2 = project_to_screenspace(ndc2);
    vec2 perp = normalize(ss2 - ss1);
    perp = vec2(-perp.y, perp.x);

    gl_Position = (x == 0.0) ? ndc1 : ndc2;
    gl_Position.xy += perp * (y - 0.5) * sizeViewportInv.xy * gl_Position.w * 10.0;

    finalColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
  else {
    gl_Position = point_world_to_ndc(pos1);

    finalColor = vec4(1.0);
  }
}
