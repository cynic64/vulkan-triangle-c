#include <assert.h>
#include <string.h>

#include "vk_buffer.h"
#include "vk_cbuf.h"

void buffer_create(VkDevice device,
		   VkPhysicalDeviceMemoryProperties dev_mem_props,
		   VkDeviceSize size,
		   VkBufferUsageFlags usage,
		   VkMemoryPropertyFlags props,
		   struct Buffer *buf)
{
	create_buffer_handle(device,
			     size,
			     usage,
			     &buf->handle);

	create_buffer_memory(device,
			     dev_mem_props,
			     buf->handle,
			     props,
			     &buf->memory);

	buf->device = device;
}

void buffer_write(struct Buffer buf,
		  uint32_t size,
		  void *data)
{
	void *mapped;
	VkResult res = vkMapMemory(buf.device, buf.memory, 0, size, 0, &mapped);
        assert(res == VK_SUCCESS);
        memcpy(mapped, data, (size_t) size);
	vkUnmapMemory(buf.device, buf.memory);
}

void buffer_destroy(struct Buffer buf)
{
	vkDestroyBuffer(buf.device, buf.handle, NULL);
	vkFreeMemory(buf.device, buf.memory, NULL);
}

void create_buffer_handle(VkDevice device,
			  VkDeviceSize size,
			  VkBufferUsageFlags usage,
			  VkBuffer *buffer)
{
	VkBufferCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(device, &info, NULL, buffer);
	assert(res == VK_SUCCESS);
}

void create_buffer_memory(VkDevice device,
			  VkPhysicalDeviceMemoryProperties real_dev_props,
			  VkBuffer buffer,
			  VkMemoryPropertyFlags req_props,
			  VkDeviceMemory *buffer_mem)
{
	// Get the type filter of the buffer we will bind to
	VkMemoryRequirements buf_reqs;
	vkGetBufferMemoryRequirements(device, buffer, &buf_reqs);

	// Find a usable memory type
	uint32_t mem_type_idx;
	int found_mem_type = 0;
	for (uint32_t i = 0; i < real_dev_props.memoryTypeCount; i++) {
		uint32_t mem_type_props = real_dev_props.memoryTypes[i].propertyFlags;

		int suitable_for_buffer = buf_reqs.memoryTypeBits & (1 << i);
		int suitable_for_user = (mem_type_props & req_props) == req_props;

		if (suitable_for_buffer && suitable_for_user) {
			found_mem_type = 1;
			mem_type_idx = i;
			break;
		}
	}

	assert(found_mem_type == 1);

	// Allocate
	VkMemoryAllocateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = buf_reqs.size;
	info.memoryTypeIndex = mem_type_idx;

	VkResult res = vkAllocateMemory(device, &info, NULL, buffer_mem);
	assert(res == VK_SUCCESS);

	// Bind
	res = vkBindBufferMemory(device, buffer, *buffer_mem, 0);
	assert(res == VK_SUCCESS);
}

void copy_buffer(VkDevice device, VkQueue queue,
		 VkCommandPool cpool,
		 VkDeviceSize size,
		 VkBuffer src, VkBuffer dst)
{
	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);
	
	// Record
	VkBufferCopy region = {0};
	region.size = size;
	vkCmdCopyBuffer(cbuf, src, dst, 1, &region);

	VkResult res = vkEndCommandBuffer(cbuf);
	assert(res == VK_SUCCESS);

	// Submit
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cbuf;

	res = vkQueueSubmit(queue, 1, &submit_info, NULL);
	assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	// Free
	vkFreeCommandBuffers(device, cpool, 1, &cbuf);
}
