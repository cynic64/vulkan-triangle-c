#ifndef VK_UNIFORM_H_
#define VK_UNIFORM_H_

#include <vulkan/vulkan.h>

void allocate_descriptor_set(
    VkDevice device,
    VkDescriptorPool dpool,
    VkDescriptorSetLayout layout,
    VkBuffer buffer,
    uint32_t location,
    uint32_t size,
    VkDescriptorSet *set
);

void create_descriptor_pool(
    VkDevice device,
    uint32_t desc_cap,
    uint32_t set_cap,
    VkDescriptorPool *dpool
);

void create_descriptor_binding(
    uint32_t location,
    VkShaderStageFlags stage,
    VkDescriptorSetLayoutBinding *binding
);

void create_descriptor_layout(
    VkDevice device,
    uint32_t binding_ct,
    VkDescriptorSetLayoutBinding *bindings,
    VkDescriptorSetLayout *layout
);

#endif // VK_UNIFORM_H_
