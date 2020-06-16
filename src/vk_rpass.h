#ifndef VK_RPASS_H_
#define VK_RPASS_H_

#include <vulkan/vulkan.h>

/*
 * Creates a render pass with only a single color attachment.
 */
void rpass_basic(VkDevice device, VkFormat format, VkRenderPass *rpass);

/*
 * Creates a renderpass with color and depth attachments.
 *
 * c_format: Format for the color attachment
 * d_format: Format for the depth attachment
 */
void rpass_with_depth(VkDevice device,
		      VkFormat c_format, VkFormat d_format,
		      VkRenderPass *rpass);

/*
 * Creates a multisampled renderpass with color and depth attachments, resolving
 * to the swapchain image.
 *
 * c_format: Format for the color and resolve attachments
 * d_format: Format for the depth attachment
 * samples: Sample count
 */
void rpass_multisampled_with_depth(VkDevice device,
				   VkFormat c_format, VkFormat d_format,
				   VkSampleCountFlagBits samples,
				   VkRenderPass *rpass);

#endif // VK_RPASS_H_
