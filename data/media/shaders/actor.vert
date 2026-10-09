#version 150

#pragma optimize(on)

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec4 position;
in vec3 texcoord;
out vec2 frag_texcoord;

void main(void) {
  frag_texcoord = texcoord.st;
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * position;
}
