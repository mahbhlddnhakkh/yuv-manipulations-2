R""(

#version 330
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D tex;

void main() {
  FragColor = vec4(texture(tex, TexCoord).bgr, 1.0f); // because bgra format is not supported on my GPU for some reason
}

)""
