#ifndef VK_CBUF_H_
#define VK_CBUF_H_

#include <vulkan/vulkan.h>

void create_cpool(VkDevice device, uint32_t queue_fam, VkCommandPool *cpool);

void create_cbuf(
    VkDevice device,
    VkCommandPool cpool,
    VkRenderPass rpass,
    VkFramebuffer fb,
    uint32_t width,
    uint32_t height,
    VkPipeline pipel,
    VkBuffer vbuf,
    uint32_t vertex_ct,
    VkCommandBuffer *cbuf
);

#endif // VK_CBUF_H_
