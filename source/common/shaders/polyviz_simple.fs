#version 330

in  vec3 barycentric; // Barycentric coordinates passed from vertex shader
out vec4 out_color;

uniform vec4  color;  // Color for the current mesh

void main() {
    vec4 final = vec4(color.xyz, 0.5);
    out_color = final;
}
