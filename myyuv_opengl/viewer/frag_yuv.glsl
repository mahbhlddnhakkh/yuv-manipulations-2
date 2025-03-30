R""(

#version 330
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D YTex;
uniform sampler2D UTex;
uniform sampler2D VTex;

void main() {
  float nx, ny, r, g, b, y, u, v;

  nx = TexCoord.x;
  ny = 1.0f - TexCoord.y; // flip

  y = texture(YTex, vec2(nx, ny)).x;
  u = texture(UTex, vec2(nx, ny)).x - 0.5f;
  v = texture(VTex, vec2(nx, ny)).x - 0.5f;

  r = y + 1.403f * v;
  g = y - 0.714f * v - 0.344f * u;
  b = y + 1.773f * u;

  FragColor = vec4(r, g, b, 1.0f);
}

)""
