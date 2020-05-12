#include <stdio.h>
#include <assert.h>

#include "vk_pipe.h"

void read_bin(FILE *fp, size_t *buf_size, char *buf)
{
	// get size
	fseek(fp, 0, SEEK_END);
	long end = ftell(fp);
	rewind(fp);
	*buf_size = end - ftell(fp);
	assert(buf_size > 0);

	if (buf == NULL) return;

	size_t res = fread(buf, 1, *buf_size, fp);
	assert(*buf_size == res);
}

void create_shmod(
	VkDevice device,
	size_t code_size,
	char *code,
	VkShaderModule *shmod
	)
{
	VkShaderModuleCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = code_size;
	info.pCode = (const uint32_t*) code;

	VkResult res = vkCreateShaderModule(device, &info, NULL, shmod);
	assert(res == VK_SUCCESS);
}

void create_shtage(
	VkShaderModule shmod,
	VkShaderStageFlagBits stage,
	VkPipelineShaderStageCreateInfo *shtage
	)
{
	shtage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shtage->pNext = NULL;
	shtage->flags = 0;
	shtage->stage = stage;
	shtage->module = shmod;
	shtage->pName = "main";
	shtage->pSpecializationInfo = NULL;
}

void create_layout(
	VkDevice device,
	uint32_t desc_layout_ct,
	VkDescriptorSetLayout *desc_layouts,
	VkPipelineLayout *layout
	)
{
	VkPipelineLayoutCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.setLayoutCount = desc_layout_ct;
	info.pSetLayouts = desc_layouts;

	VkResult res = vkCreatePipelineLayout(device, &info, NULL, layout);
	assert(res == VK_SUCCESS);
}

void create_rpass(VkDevice device, VkFormat format, VkRenderPass *rpass)
{
	VkAttachmentDescription color_attachment = {0};
	color_attachment.format = format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attach_ref = {0};
	color_attach_ref.attachment = 0;
	color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attach_ref;

	VkSubpassDependency dep = {0};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcAccessMask = 0;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &color_attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dep;

	VkResult res = vkCreateRenderPass(device, &info, NULL, rpass);
	assert(res == VK_SUCCESS);
}

void create_pipel(
	VkDevice device,
	uint32_t shtage_ct,
	VkPipelineShaderStageCreateInfo *shtages,
	VkPipelineLayout layout,
	uint32_t binding_ct,
	VkVertexInputBindingDescription *binding_descs,
	uint32_t attr_ct,
	VkVertexInputAttributeDescription *attr_descs,
	VkRenderPass rpass,
	VkPipeline *pipel
	)
{
	VkPipelineVertexInputStateCreateInfo vertex_input = {0};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = binding_ct;
	vertex_input.pVertexBindingDescriptions = binding_descs;
	vertex_input.vertexAttributeDescriptionCount = attr_ct;
	vertex_input.pVertexAttributeDescriptions = attr_descs;

	VkPipelineInputAssemblyStateCreateInfo input_ass = {0};
	input_ass.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_ass.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_ass.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blend_attach = {0};
	blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		|  VK_COLOR_COMPONENT_G_BIT
		|  VK_COLOR_COMPONENT_B_BIT
		|  VK_COLOR_COMPONENT_A_BIT;
	blend_attach.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo blend = {0};
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.logicOpEnable = VK_FALSE;
	blend.attachmentCount = 1;
	blend.pAttachments = &blend_attach;

	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	// Dummy values, we use dynamic states
	viewport.width = 100.0f;
	viewport.height = 100.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	VkOffset2D scissor_offset = {0};
	scissor_offset.x = 0;
	scissor_offset.y = 0;
	VkExtent2D scissor_extent = {0};
	scissor_extent.width = 100;
	scissor_extent.height = 100;
	scissor.offset = scissor_offset;
	scissor.extent = scissor_extent;

	VkPipelineViewportStateCreateInfo viewport_state = {0};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkDynamicState dyn_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dyn_state = {0};
	dyn_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dyn_state.dynamicStateCount = 2;
	dyn_state.pDynamicStates = dyn_states;

	VkGraphicsPipelineCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = shtage_ct;
	info.pStages = shtages;
	info.pVertexInputState = &vertex_input;
	info.pInputAssemblyState = &input_ass;
	info.pViewportState = &viewport_state;
	info.pRasterizationState = &rasterizer;
	info.pMultisampleState = &multisampling;
	info.pDepthStencilState = NULL;
	info.pColorBlendState = &blend;
	info.pDynamicState = &dyn_state;
	info.layout = layout;
	info.renderPass = rpass;
	info.subpass = 0;

	VkResult res = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&info,
		NULL,
		pipel
		);
	assert(res == VK_SUCCESS);
}
