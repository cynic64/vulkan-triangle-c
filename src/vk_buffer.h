#ifndef VK_BUFFER_H_
#define VK_BUFFER_H_

#include <vulkan/vulkan.h>

// This merely creates the handle, does not allocate or bind any memory to that
// handle.
void create_buffer_handle(
    VkDevice device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer *buffer
);

// Allocates a suitable chunk of memory on the GPU and binds it to buffer.
// NOTE: size
void create_buffer_memory(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties real_dev_props,
    VkBuffer buffer,
    VkMemoryPropertyFlags req_props,
    VkDeviceMemory *buffer_mem
);

void copy_buffer(
    VkDevice device,
    VkQueue queue,
    VkCommandPool cpool,
    VkDeviceSize size,
    VkBuffer src,
    VkBuffer dst
);

#endif // VK_BUFFER_H_
