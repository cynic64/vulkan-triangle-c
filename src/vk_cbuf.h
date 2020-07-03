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

/*
 * Submits a command buffer, waiting on a semaphore and signalling a fence and a
 * semaphore.
 *
 * Does not free command buffer afterwards. Wait stage is always
 * COLOR_ATTACHMENT_OUTPUT_BIT.
 *
 * s_wait: Semaphore to wait on before submission (usually image available
 * semaphore)
 * s_signal: Semaphore to signal (usually used later for submission to wait on)
 * f_signal: Fence to signal
 */
void submit_synced(VkQueue queue,
		   VkSemaphore s_wait, VkSemaphore s_signal, VkFence f_signal,
		   VkCommandBuffer cbuf);
		   

#endif // VK_CBUF_H_
