#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Uniform {
    mat4 mtx;
} ubo;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texc;

layout(location = 0) out vec2 out_texc;

void main() {
    gl_Position = ubo.mtx * vec4(in_pos, 1.0);

    out_texc = in_texc;
}
