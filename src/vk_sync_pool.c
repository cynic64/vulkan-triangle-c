#include "vk_sync.h"

#include "vk_sync_pool.h"

#include <stdlib.h>
#include <assert.h>

#include <vulkan/vulkan.h>

void sync_pool_create(VkDevice device, uint32_t ct,
		      struct SyncPool *sync_pool)
{
	sync_pool->ct = ct;
	sync_pool->cur = 0;
	sync_pool->fences = malloc(sizeof(sync_pool->fences[0]) * ct);

	for (int i = 0; i < ct; i++) {
		create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT,
			     &sync_pool->fences[i]);
	}
}

void sync_pool_acquire(VkDevice device, struct SyncPool *sync_pool,
		       VkFence *fence, uint32_t *idx)
{
	VkFence f = sync_pool->fences[sync_pool->cur];

        VkResult res = vkWaitForFences(device,
				       1, &f,
				       VK_TRUE, UINT64_MAX);
	assert(res == VK_SUCCESS);

	res = vkResetFences(device, 1, &f);
	assert(res == VK_SUCCESS);

	*fence = f;

	if (idx != NULL) {
		*idx = sync_pool->cur;
	}

	sync_pool->cur = (sync_pool->cur + 1) % sync_pool->ct;
}

void sync_pool_destroy(VkDevice device, struct SyncPool sync_pool)
{
	for (int i = 0; i < sync_pool.ct; i++) {
		vkDestroyFence(device, sync_pool.fences[i], NULL);
	}
}
