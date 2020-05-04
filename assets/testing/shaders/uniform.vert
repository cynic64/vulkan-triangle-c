#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Uniform {
    mat4 mtx;
} ubo;

void main() {
    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
}
