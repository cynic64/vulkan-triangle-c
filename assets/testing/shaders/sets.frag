#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform Uniform1 {
	vec4 data;
} buf1;
layout(set = 0, binding = 1) uniform sampler2D tex1;

layout(set = 1, binding = 0) uniform sampler2D tex2;
layout(set = 1, binding = 1) uniform Uniform2 {
	vec4 data;
} buf2;



layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color;

void main() {
	float red = buf1.data.r;
	float green = texture(tex1, vec2(0.5, 0.5)).g;
	float blue = texture(tex2, vec2(0.5, 0.5)).b;
	float alpha = buf2.data.a;
	
	out_color = vec4(red, green, blue, alpha);
}
