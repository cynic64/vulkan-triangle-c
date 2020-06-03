#ifndef VK_CBUF_H_
#define VK_CBUF_H_

#include <vulkan/vulkan.h>

void create_cpool(VkDevice device, uint32_t queue_fam, VkCommandPool *cpool);

/*
 * Creates a command buffer, allocating it from a command pool.
 *
 * Layout is only required if descriptor sets are used, and can be NULL
 * otherwise.
 * If desc_set_ct is 0, desc_sets can also be NULL.
 */
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
		 VkCommandBuffer *cbuf);

/*
 * Allocate a command buffer for one-time use and begin recording.
 */
void cbuf_begin_one_time(VkDevice device,
			 VkCommandPool cpool,
			 VkCommandBuffer *cbuf);

/*
 * Submits a command buffer without waiting on any semaphores, or signalling any
 * semaphores.
 *
 * Frees command buffer afterwards.
 */
void submit_syncless(VkDevice device, VkQueue queue,
		     VkCommandPool cpool, VkCommandBuffer cbuf);

#endif // VK_CBUF_H_
