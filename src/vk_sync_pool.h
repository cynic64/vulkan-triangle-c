#ifndef VK_SYNC_POOL_H_
#define VK_SYNC_POOL_H_

#include <vulkan/vulkan.h>

struct SyncPool {
	uint32_t ct;
	VkFence *fences;
	void **extra;
	uint32_t cur;
};

/*
 * Create a SyncPool struct. Mallocs for sync_pool.fences.
 *
 * ct: How many fences to create
 */

void sync_pool_create(VkDevice device, uint32_t ct,
		      struct SyncPool *sync_pool);

/*
 * Acquires a fence from a SyncPool struct, waiting on it.
 *
 * idx: If not NULL, will be set to the index of the selected fence
 */
void sync_pool_acquire(VkDevice device, struct SyncPool *sync_pool,
		       VkFence *fence, uint32_t *idx);

void sync_pool_destroy(VkDevice device, struct SyncPool sync_pool);

#endif // VK_SYNC_POOL_H_
