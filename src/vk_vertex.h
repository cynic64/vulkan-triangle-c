#ifndef VK_VERTEX_H_
#define VK_VERTEX_H_

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

/* Contains functions to get the binding and attribute descriptions for the
   different, pre-defined vertex types. */

/* Vertex3PosColor */
struct Vertex3PosColor {
	vec3 pos;
	vec3 color;
};

static VkVertexInputBindingDescription VERTEX_3_POS_COLOR_BINDINGS[] = {
	{
		.binding = 0,
		.stride = sizeof(struct Vertex3PosColor),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}
};

static uint32_t VERTEX_3_POS_COLOR_BINDING_CT = 1;

static VkVertexInputAttributeDescription VERTEX_3_POS_COLOR_ATTRIBUTES[] = {
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosColor, pos)
	},
	{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosColor, color)
	}
};

static uint32_t VERTEX_3_POS_COLOR_ATTRIBUTE_CT = 2;

/* Vertex3PosNormal */
struct Vertex3PosNormal {
	vec3 pos;
	vec3 normal;
};

static VkVertexInputBindingDescription VERTEX_3_POS_NORMAL_BINDINGS[] = {
	{
		.binding = 0,
		.stride = sizeof(struct Vertex3PosNormal),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}
};

static uint32_t VERTEX_3_POS_NORMAL_BINDING_CT = 1;

static VkVertexInputAttributeDescription VERTEX_3_POS_NORMAL_ATTRIBUTES[] = {
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosNormal, pos)
	},
	{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosNormal, normal)
	}
};

static uint32_t VERTEX_3_POS_NORMAL_ATTRIBUTE_CT = 2;

/* Vertex3PosNormalTex */
struct Vertex3PosNormalTex {
	vec3 pos;
	vec3 normal;
	vec2 tex;
};

static VkVertexInputBindingDescription VERTEX_3_POS_NORMAL_TEX_BINDINGS[] = {
	{
		.binding = 0,
		.stride = sizeof(struct Vertex3PosNormalTex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}
};

static uint32_t VERTEX_3_POS_NORMAL_TEX_BINDING_CT = 1;

static VkVertexInputAttributeDescription VERTEX_3_POS_NORMAL_TEX_ATTRIBUTES[] = {
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosNormalTex, pos)
	},
	{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex3PosNormalTex, normal)
	},
	{
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(struct Vertex3PosNormalTex, tex)
	}
};

static uint32_t VERTEX_3_POS_NORMAL_TEX_ATTRIBUTE_CT = 3;

/* Vertex2PosColor */

struct Vertex2PosColor {
	vec2 pos;
	vec3 color;
};

static VkVertexInputBindingDescription VERTEX_2_POS_COLOR_BINDINGS[] = {
	{
		.binding = 0,
		.stride = sizeof(struct Vertex2PosColor),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}
};

static uint32_t VERTEX_2_POS_COLOR_BINDING_CT = 1;

static VkVertexInputAttributeDescription VERTEX_2_POS_COLOR_ATTRIBUTES[] = {
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(struct Vertex2PosColor, pos)
	},
	{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct Vertex2PosColor, color)
	}
};

static uint32_t VERTEX_2_POS_COLOR_ATTRIBUTE_CT = 2;

/* Vertex2PosTex */
struct Vertex2PosTex {
	vec2 pos;
	vec2 texc;
};

static VkVertexInputBindingDescription VERTEX_2_POS_TEX_BINDINGS[] = {
	{
		.binding = 0,
		.stride = sizeof(struct Vertex2PosTex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}
};

static uint32_t VERTEX_2_POS_TEX_BINDING_CT = 1;

static VkVertexInputAttributeDescription VERTEX_2_POS_TEX_ATTRIBUTES[] = {
	{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(struct Vertex2PosTex, pos)
	},
	{
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(struct Vertex2PosTex, texc)
	}
};

static uint32_t VERTEX_2_POS_TEX_ATTRIBUTE_CT = 2;

#endif // VK_VERTEX_H_
