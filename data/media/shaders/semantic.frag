#version 150

#pragma optimize(on)

uniform vec3 semanticColor;

out vec4 stdout;

void main(void) {
  stdout = vec4(semanticColor, 1.0);
}
