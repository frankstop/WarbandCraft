#version 150
uniform sampler2D world;
uniform float strength;
in vec2 uv;
out vec4 color;
void main() {
    vec4 c = texture(world, uv);
    // Work on unassociated color and preserve world coverage exactly.
    vec3 rgb = c.a > 0.00001 ? c.rgb / c.a : vec3(0.0);
    float luma = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 graded = mix(vec3(luma), rgb, 1.12);
    graded = (graded - 0.5) * 1.07 + 0.5;
    graded *= vec3(1.02, 1.0, 0.98);
    color = vec4(mix(rgb, clamp(graded, 0.0, 1.0), strength) * c.a, c.a);
}
