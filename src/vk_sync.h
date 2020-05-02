#ifndef VK_SYNC_H_
#define VK_SYNC_H_

#include <vulkan/vulkan.h>

void create_sem(VkDevice device, VkSemaphore *sem);

void create_fence(VkDevice device, VkFenceCreateFlags flags, VkFence *fence);

#endif // VK_SYNC_H_
