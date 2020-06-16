#include <assert.h>
#include <stdlib.h>

#include "vk_image.h"
#include "ll_vk_image.h"
#include "vk_cbuf.h"

void image_create(VkDevice device,
		  uint32_t queue_fam,
		  VkPhysicalDeviceMemoryProperties dev_mem_props,
		  VkFormat format,
		  VkImageUsageFlags usage,
		  VkMemoryPropertyFlags req_props,
		  VkImageAspectFlagBits aspect,
		  VkSampleCountFlagBits samples,
		  uint32_t width, uint32_t height,
		  struct Image *image)
{
	image_handle_create(device,
			    queue_fam,
			    format,
			    usage,
			    samples,
			    width, height,
			    &image->handle);

	image_memory_bind(device,
			  dev_mem_props,
			  req_props,
			  image->handle,
			  &image->memory);

	image_view_create(device,
			  format,
			  aspect,
			  image->handle,
			  &image->view);
}

void image_destroy(VkDevice device,
		   struct Image image)
{
	vkDestroyImage(device, image.handle, NULL);
	vkDestroyImageView(device, image.view, NULL);
	vkFreeMemory(device, image.memory, NULL);
}

void image_transition(VkDevice device,
		      VkQueue queue,
		      VkCommandPool cpool,
		      VkImage image,
		      VkImageAspectFlags aspect,
		      VkAccessFlags src_mask, VkAccessFlags dst_mask,
		      VkPipelineStageFlags src_stage,
		      VkPipelineStageFlags dst_stage,
		      VkImageLayout old_lt, VkImageLayout new_lt)
{
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_lt;
	barrier.newLayout = new_lt;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = src_mask;
	barrier.dstAccessMask = dst_mask;

	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);
	
	vkCmdPipelineBarrier(cbuf,
			     src_stage,
			     dst_stage,
			     0,
			     0, NULL,
			     0, NULL,
			     1, &barrier);

	VkResult res = vkEndCommandBuffer(cbuf);
	assert(res == VK_SUCCESS);

	VkSubmitInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cbuf;

	res = vkQueueSubmit(queue, 1, &info, NULL);
	assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	vkFreeCommandBuffers(device, cpool, 1, &cbuf);
}

void copy_image_buffer(VkDevice device,
		       VkQueue queue,
		       VkCommandPool cpool,
		       VkImageAspectFlags aspect,
		       uint32_t width, uint32_t height,
		       VkImage src, VkBuffer dest)
{
	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);
	
	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = width;
	region.bufferImageHeight = height;
	region.imageSubresource.aspectMask = aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
		
	vkCmdCopyImageToBuffer(cbuf,
			       src,
			       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			       dest,
			       1, &region);

	VkResult res = vkEndCommandBuffer(cbuf);
	assert(res == VK_SUCCESS);

	submit_syncless(device, queue, cpool, cbuf);
}

void vk_mem_to_string(VkDevice device,
		      uint32_t in_w, uint32_t in_h,
		      uint32_t out_w, uint32_t out_h,
		      VkDeviceMemory mem,
		      char *out)
{	
	void *mapped;
	VkResult res = vkMapMemory(device, mem, 0, 4 * in_w * in_h, 0, &mapped);
        assert(res == VK_SUCCESS);

	unsigned char (*pixels)[in_w] = malloc(in_w * in_h);

	for (int i = 0; i < in_w * in_h; i++) {
		unsigned char b = ((char *)mapped)[4 * i];
		unsigned char g = ((char *)mapped)[4 * i + 1];
		unsigned char r = ((char *)mapped)[4 * i + 2];
		unsigned char a = ((char *)mapped)[4 * i + 3];
		pixels[i / in_w][i % in_w] = (r + g + b) / 3;
	}
	vkUnmapMemory(device, mem);

	uint32_t scale_x = in_w / out_w;
	uint32_t scale_y = in_h / out_h;

	char *ptr = out;
	for (uint32_t y = 0; y < out_h; y++) {
		// Skip the last column to make room for newlines
		for (uint32_t x = 0; x < out_w - 1; x++) {
			if (pixels[y * scale_y][x * scale_x] > 0) *ptr++ = '#';
			else *ptr++ = ' ';
		}

		*ptr++ = '\n';
	}

	// Subtract 1 to avoid overrun
	*(ptr - 1) = '\0';
	// Re-do final newline
	*(ptr - 2) = '\n';

	free(pixels);
}

void image_handle_create(VkDevice device,
			 uint32_t queue_fam,
			 VkFormat format,
			 VkImageUsageFlags usage,
			 VkSampleCountFlagBits samples,
			 uint32_t width, uint32_t height,
			 VkImage *image)
{
	VkImageCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = {
			.width = width,
			.height = height,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = samples,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queue_fam,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	
	VkResult res = vkCreateImage(device, &info, NULL, image);
	assert(res == VK_SUCCESS);
}

void image_memory_bind(VkDevice device,
		       VkPhysicalDeviceMemoryProperties dev_mem_props,
		       VkMemoryPropertyFlags req_props,
		       VkImage image,
		       VkDeviceMemory *image_mem)
{
	VkResult res;

	VkMemoryRequirements img_reqs;
	vkGetImageMemoryRequirements(device, image, &img_reqs);

	uint32_t mem_type_idx = find_memory_type(dev_mem_props,
						 img_reqs,
						 req_props);

	// Allocate
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = img_reqs.size;
	alloc_info.memoryTypeIndex = mem_type_idx;

	res = vkAllocateMemory(device, &alloc_info, NULL, image_mem);
	assert(res == VK_SUCCESS);

	// Bind
	res = vkBindImageMemory(device, image, *image_mem, 0);
	assert(res == VK_SUCCESS);
}

void image_view_create(VkDevice device,
		       VkFormat format,
		       VkImageAspectFlagBits aspect,
		       VkImage image,
		       VkImageView *view)
{
	VkImageViewCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange.aspectMask = aspect,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};
	
	VkResult res = vkCreateImageView(device, &info, NULL, view);
	assert(res == VK_SUCCESS);
}

uint32_t find_memory_type(VkPhysicalDeviceMemoryProperties dev_props,
			  VkMemoryRequirements mem_reqs,
			  VkMemoryPropertyFlags req_props)
{
	uint32_t type_idx;
	int found = 0;
	for (uint32_t i = 0; i < dev_props.memoryTypeCount; i++) {
		uint32_t cur_props = dev_props.memoryTypes[i].propertyFlags;

		int suitable_for_buffer = mem_reqs.memoryTypeBits & (1 << i);
		int suitable_for_user = (cur_props & req_props) == req_props;

		if (suitable_for_buffer && suitable_for_user) {
			found = 1;
			type_idx = i;
			break;
		}
	}

	assert(found == 1);

	return type_idx;
}
