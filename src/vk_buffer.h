#ifndef VK_BUFFER_H_
#define VK_BUFFER_H_

#include <vulkan/vulkan.h>

struct Buffer {
	VkDevice device;
	VkBuffer handle;
	VkDeviceMemory memory;
};

void buffer_create(VkDevice device,
		   VkPhysicalDeviceMemoryProperties dev_mem_props,
		   VkDeviceSize size,
		   VkBufferUsageFlags usage,
		   VkMemoryPropertyFlags props,
		   struct Buffer *buf);

/*
 * Note: buffer must be host-visible, and probably should be host-coherent.
 */
void buffer_write(struct Buffer buf,
		  uint32_t size,
		  void *data);

void buffer_destroy(struct Buffer buf);

/*
 * This merely creates the handle, does not allocate or bind any memory to that
 * handle.
 */
void create_buffer_handle(VkDevice device,
			  VkDeviceSize size,
			  VkBufferUsageFlags usage,
			  VkBuffer *buffer);

/*
 * Allocates a suitable chunk of memory on the GPU and binds it to buffer.
 */
void create_buffer_memory(VkDevice device,
			  VkPhysicalDeviceMemoryProperties real_dev_props,
			  VkBuffer buffer,
			  VkMemoryPropertyFlags req_props,
			  VkDeviceMemory *buffer_mem);

/*
 * Copies one buffer to another.
 */
void copy_buffer_buffer(VkDevice device, VkQueue queue, VkCommandPool cpool,
			VkDeviceSize size,
			VkBuffer src, VkBuffer dst);

/*
 * Copies a buffer to an image.
 * Image must already be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
 */
void copy_buffer_image(VkDevice device, VkQueue queue, VkCommandPool cpool,
		       VkImageAspectFlagBits aspect,
		       uint32_t width, uint32_t height,
		       VkBuffer src, VkImage dst);
#endif // VK_BUFFER_H_
