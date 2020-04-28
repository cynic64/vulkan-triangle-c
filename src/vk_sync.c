#ifndef VK_CBUF_H_
#define VK_CBUF_H_

#include "vk_sync.h"

#include <assert.h>

void create_sem(VkDevice device, VkSemaphore *sem) {
    VkSemaphoreCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkResult res = vkCreateSemaphore(device, &info, NULL, sem);
    assert(res == VK_SUCCESS);
}

#endif // VK_CBUF_H_
