#include <assert.h>

#include "vk_rpass.h"
#include "vk_tools.h"

// Does not yet have the format set
static VkAttachmentDescription default_color_attachment = {
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

// Does not yet have the format set
static VkAttachmentDescription default_depth_attachment = {
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
};

void rpass_basic(VkDevice device, VkFormat format, VkRenderPass *rpass)
{
	VkAttachmentDescription color_attachment = default_color_attachment;
	color_attachment.format = format;

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

void rpass_with_depth(VkDevice device,
		      VkFormat c_format, VkFormat d_format,
		      VkRenderPass *rpass)
{
	VkAttachmentDescription color_attachment = default_color_attachment;
	color_attachment.format = c_format;

	VkAttachmentDescription depth_attachment = default_depth_attachment;
	depth_attachment.format = d_format;
 
	VkAttachmentDescription attachments[] = {color_attachment,
						 depth_attachment};

	VkAttachmentReference color_attach_ref = {0};
	color_attach_ref.attachment = 0;
	color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attach_ref = {0};
	depth_attach_ref.attachment = 1;
	depth_attach_ref.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attach_ref;
	subpass.pDepthStencilAttachment = &depth_attach_ref;

	VkSubpassDependency dep = {0};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcAccessMask = 0;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = ARRAY_SIZE(attachments);
	info.pAttachments = attachments;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dep;

	VkResult res = vkCreateRenderPass(device, &info, NULL, rpass);
	assert(res == VK_SUCCESS);
}

void rpass_multisampled_with_depth(VkDevice device,
				   VkFormat c_format, VkFormat d_format,
				   VkSampleCountFlagBits samples,
				   VkRenderPass *rpass)
{
	VkAttachmentDescription color_attachment = default_color_attachment;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color_attachment.format = c_format;
	color_attachment.samples = samples;

	VkAttachmentDescription depth_attachment = default_depth_attachment;
	depth_attachment.format = d_format;
 	depth_attachment.samples = samples;

	VkAttachmentDescription resolve_attachment = default_color_attachment;
	resolve_attachment.format = c_format;

	VkAttachmentDescription attachments[] = {resolve_attachment,
						 color_attachment,
						 depth_attachment};

	VkAttachmentReference color_attach_ref = {0};
	color_attach_ref.attachment = 1;
	color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attach_ref = {0};
	depth_attach_ref.attachment = 2;
	depth_attach_ref.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolve_attach_ref = {0};
	resolve_attach_ref.attachment = 0;
	resolve_attach_ref.layout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attach_ref;
	subpass.pDepthStencilAttachment = &depth_attach_ref;
	subpass.pResolveAttachments = &resolve_attach_ref;

	VkSubpassDependency dep = {0};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.srcAccessMask = 0;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = ARRAY_SIZE(attachments);
	info.pAttachments = attachments;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dep;

	VkResult res = vkCreateRenderPass(device, &info, NULL, rpass);
	assert(res == VK_SUCCESS);
}
