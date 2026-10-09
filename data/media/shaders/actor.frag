#version 150

#pragma optimize(on)

uniform vec3 semanticColor;
uniform sampler2D map_albedo;
in vec2 frag_texcoord;

out vec4 stdout;

void main(void) {
  // In the Luca-compatible profile every non-actor is assigned black.
  // Discarding it preserves the black background while keeping the original
  // player texture, from which the CPU edge stage extracts internal contours.
  if (dot(semanticColor, semanticColor) < 0.000001) discard;
  vec4 base = texture(map_albedo, frag_texcoord);
  if (base.a < 0.12) discard;
  stdout = vec4(base.rgb, 1.0);
}
