#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_texc;

layout(location = 0) out vec2 out_texc;

void main() {
    gl_Position = vec4(in_pos, 1.0, 1.0);

    out_texc = in_texc;
}
