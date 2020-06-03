#include <stdio.h>
#include <assert.h>

#include "vk_cbuf.h"

void create_cpool(VkDevice device, uint32_t queue_fam, VkCommandPool *cpool)
{
	VkCommandPoolCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.queueFamilyIndex = queue_fam;

	VkResult res = vkCreateCommandPool(device, &info, NULL, cpool);
	assert(res == VK_SUCCESS);
}

void create_cbuf(VkDevice device,
		 VkCommandPool cpool,
		 VkRenderPass rpass,
		 uint32_t clear_ct, VkClearValue *clears,
		 VkFramebuffer fb,
		 uint32_t width, uint32_t height,
		 VkPipelineLayout layout,
		 VkPipeline pipel,
		 uint32_t desc_set_ct, VkDescriptorSet *desc_sets,
		 VkBuffer vbuf, VkBuffer ibuf,
		 uint32_t index_ct,
		 VkCommandBuffer *cbuf)
{
	VkResult res;

	// Allocate
	cbuf_begin_one_time(device, cpool, cbuf);

	// Enter render pass
	VkOffset2D render_area_offset = {0};
	render_area_offset.x = 0;
	render_area_offset.y = 0;
	VkExtent2D render_area_extent = {0};
	render_area_extent.width = width;
	render_area_extent.height = height;

	VkRenderPassBeginInfo rpass_info = {0};
	rpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpass_info.renderPass = rpass;
	rpass_info.framebuffer = fb;
	rpass_info.renderArea.offset = render_area_offset;
	rpass_info.renderArea.extent = render_area_extent;
	rpass_info.clearValueCount = clear_ct;
	rpass_info.pClearValues = clears;

	vkCmdBeginRenderPass(*cbuf, &rpass_info, VK_SUBPASS_CONTENTS_INLINE);

	// Set scissors and viewport
	VkViewport viewport = {0};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkOffset2D scissor_offset = {0};
	scissor_offset.x = 0;
	scissor_offset.y = 0;
	VkExtent2D scissor_extent = {0};
	scissor_extent.width = width;
	scissor_extent.height = height;
	VkRect2D scissor = {0};
	scissor.offset = scissor_offset;
	scissor.extent = scissor_extent;

	vkCmdSetViewport(*cbuf, 0, 1, &viewport);
	vkCmdSetScissor(*cbuf, 0, 1, &scissor);

	// Bind vertex buffer
	VkBuffer vertex_buffers[] = {vbuf};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(*cbuf, 0, 1, vertex_buffers, offsets);

	// Bind pipeline
	vkCmdBindPipeline(*cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipel);

	// Bind descriptor sets, if any
	if (desc_set_ct > 0) {
		assert(layout != NULL);
		
		vkCmdBindDescriptorSets(*cbuf,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					layout,
					0,
					desc_set_ct, desc_sets,
					0, NULL);
	}

	// Draw! :)
	vkCmdBindIndexBuffer(*cbuf, ibuf, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(*cbuf, index_ct, 1, 0, 0, 0);

	// Finish
	vkCmdEndRenderPass(*cbuf);

	res = vkEndCommandBuffer(*cbuf);
	assert(res == VK_SUCCESS);
}

void cbuf_begin_one_time(VkDevice device,
			 VkCommandPool cpool,
			 VkCommandBuffer *cbuf)
{
	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = cpool;
	alloc_info.commandBufferCount = 1;

	VkResult res = vkAllocateCommandBuffers(device, &alloc_info, cbuf);
	assert(res == VK_SUCCESS);
	
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(*cbuf, &begin_info);	
}

void submit_syncless(VkDevice device, VkQueue queue,
		     VkCommandPool cpool, VkCommandBuffer cbuf)
{
        VkSubmitInfo info = {0};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 0;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cbuf;
        info.signalSemaphoreCount = 0;

        VkResult res = vkQueueSubmit(queue, 1, &info, NULL);
        assert(res == VK_SUCCESS);

	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	vkFreeCommandBuffers(device, cpool, 1, &cbuf);
}
