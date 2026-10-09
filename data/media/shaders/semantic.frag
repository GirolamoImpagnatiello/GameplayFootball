#version 150

#pragma optimize(on)

uniform vec3 semanticColor;
uniform sampler2D map_albedo;
uniform int isPitchSurface;
uniform int isCrowdSurface;
in vec2 frag_texcoord;

out vec4 stdout;

void main(void) {
  vec3 color = semanticColor;
  if (isPitchSurface != 0) {
    // Painted markings are baked into the pitch albedo at match creation.
    // Inspect the base texture across the pixel footprint. Using the regular
    // filtered mip level made thin distant markings disappear in alternating
    // fragments, which looked like depth clipping in the semantic mask.
    vec2 footprint = max(fwidth(frag_texcoord) * 0.55,
                         1.0 / vec2(textureSize(map_albedo, 0)));
    vec3 a0 = textureLod(map_albedo, frag_texcoord, 0.0).rgb;
    vec3 ax0 = textureLod(map_albedo, frag_texcoord - vec2(footprint.x, 0.0), 0.0).rgb;
    vec3 ax1 = textureLod(map_albedo, frag_texcoord + vec2(footprint.x, 0.0), 0.0).rgb;
    vec3 ay0 = textureLod(map_albedo, frag_texcoord - vec2(0.0, footprint.y), 0.0).rgb;
    vec3 ay1 = textureLod(map_albedo, frag_texcoord + vec2(0.0, footprint.y), 0.0).rgb;
    float paint = max(min(min(a0.r, a0.g), a0.b),
                  max(min(min(ax0.r, ax0.g), ax0.b),
                  max(min(min(ax1.r, ax1.g), ax1.b),
                  max(min(min(ay0.r, ay0.g), ay0.b),
                      min(min(ay1.r, ay1.g), ay1.b)))));
    if (paint > 0.075)
      color = vec3(1.0, 1.0, 1.0);
  }
  if (isCrowdSurface != 0) {
    // The stadium crowd is rendered on RGBA billboards. The old semantic
    // pass colored each complete quad as crowd, making the seating bowl look
    // fully occupied even where the texture is transparent. Preserve the
    // people silhouettes and label the remaining billboard area as seats.
    float crowdAlpha = texture(map_albedo, frag_texcoord).a;
    if (crowdAlpha < 0.50)
      color = vec3(50.0 / 255.0, 50.0 / 255.0, 50.0 / 255.0);
  }
  stdout = vec4(color, 1.0);
}
