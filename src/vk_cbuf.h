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
    VkPipelineLayout layout,
    VkPipeline pipel,
    uint32_t desc_set_ct,
    VkDescriptorSet *desc_sets,
    VkBuffer vbuf,
    VkBuffer ibuf,
    uint32_t index_ct,
    VkCommandBuffer *cbuf
);

#endif // VK_CBUF_H_
