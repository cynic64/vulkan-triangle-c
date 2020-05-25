#ifndef VK_IMAGE_H_
#define VK_IMAGE_H_

#include <vulkan/vulkan.h>

/*
 * Wrapper around all the essential things needed to create and use an image in
 * Vulkan.
 */
struct Image {
	VkImage handle;
	VkDeviceMemory memory;
	VkImageView view;
};

void image_create(VkDevice device,
		  uint32_t queue_fam,
		  VkPhysicalDeviceMemoryProperties dev_mem_props,
		  VkFormat format,
		  VkImageUsageFlags usage,
		  VkMemoryPropertyFlags req_props,
		  VkImageAspectFlagBits aspect,
		  uint32_t width, uint32_t height,
		  struct Image *image);

/*
 * Transitions an image's layout.
 *
 * aspect: Image aspect being transitioned, like VK_IMAGE_ASPECT_COLOR_BIT
 * old_lt: Old layout
 * new_lt: New layout
 * src_mask: What accesses to wait on, like VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
 * dst_mask: What accesses should wait, like VK_ACCESS_TRANSFER_READ_BIT
 * src_stage: What pipeline stage to wait on
 * dst_stage: What pipeline stage should wait
 */
void image_transition(VkDevice device,
		      VkQueue queue,
		      VkCommandPool cpool,
		      VkImage image,
		      VkImageAspectFlags aspect,
		      VkAccessFlags src_mask, VkAccessFlags dst_mask,
		      VkPipelineStageFlags src_stage,
		      VkPipelineStageFlags dst_stage,
		      VkImageLayout old_lt, VkImageLayout new_lt);

/*
 * Copy an given image to a given buffer.
 *
 * The image must have layout IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
 *
 * The buffer must:
 * - Be big enough
 * - Have usage TRANSFER_DST
 */
void image_copy_to_buffer(VkDevice device,
			  VkQueue queue,
			  VkCommandPool cpool,
			  VkImageAspectFlags aspect,
			  uint32_t width, uint32_t height,
			  VkImage src, VkBuffer dest);

/*
 * Format some Vulkan memory as a string.
 * The point is to ASCII-fy images rendered by Vulkan.
 * The image should be copied to some host-visible memory first.
 *
 * Assumes image was B8G8R8A8_UNORM.
 *
 * Out should be pre-allocated to size 4 * out_w * out_h
 *
 * in_w, in_h: Width and height image was, in pixels
 * out_w, out_h: Width and height output should be, in characters
 * out: Char array to output to
 */
void vk_mem_to_string(VkDevice device,
		      uint32_t in_w, uint32_t in_h,
		      uint32_t out_w, uint32_t out_h,
		      VkDeviceMemory mem,
		      char *out);

/*
 * Creates an image handle. Does not bind memory to it or create an image view.
 */
void image_handle_create(VkDevice device,
			 uint32_t queue_fam,
			 VkFormat format,
			 VkImageUsageFlags usage,
			 uint32_t width, uint32_t height,
			 VkImage *image);

/*
 * Allocates and binds memory for an image.
 */
void image_memory_bind(VkDevice device,
		       VkPhysicalDeviceMemoryProperties dev_mem_props,
		       VkMemoryPropertyFlags req_props,
		       VkImage image,
		       VkDeviceMemory *image_mem);

/*
 * Creates an image view given an image.
 */
void image_view_create(VkDevice device,
		       VkFormat format,
		       VkImageAspectFlagBits aspect,
		       VkImage image,
		       VkImageView *view);

/*
 * Destroys an Image struct and all associated resources
 */
void image_destroy(VkDevice device,
		   struct Image image);

#endif // VK_IMAGE_H_
