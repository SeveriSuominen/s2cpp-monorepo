#version 330

layout (location = 0) in vec3 position; // Vertex position
layout (location = 1) in vec3 normal;   // Vertex normal
layout (location = 2) in vec2 texCoord; // Texture coordinates

uniform mat4 mvp; // Model-View-Projection matrix
uniform mat4 model_mtx;
void main() {
    gl_Position = mvp * vec4(position * 0.995, 1.0);
}
