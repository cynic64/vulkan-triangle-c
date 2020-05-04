#include <assert.h>

#include "vk_uniform.h"

void allocate_descriptor_set(
    VkDevice device,
    VkDescriptorPool dpool,
    VkDescriptorSetLayout layout,
    VkBuffer buffer,
    uint32_t location,
    uint32_t size,
    VkDescriptorSet *set
) {
    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = dpool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;

    VkResult res = vkAllocateDescriptorSets(device, &alloc_info, set);
    assert(res == VK_SUCCESS);

    // fuckin' magic
    VkDescriptorBufferInfo buffer_info = {0};
    buffer_info.buffer = buffer;
    buffer_info.offset = 0;
    buffer_info.range = size;

    VkWriteDescriptorSet desc_write = {0};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.dstSet = *set;
    desc_write.dstBinding = location;
    desc_write.dstArrayElement = 0;

    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_write.descriptorCount = 1;
    desc_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device, 1, &desc_write, 0, NULL);
}

void create_descriptor_pool(
    VkDevice device,
    uint32_t desc_cap,
    uint32_t set_cap,
    VkDescriptorPool *dpool
) {
    VkDescriptorPoolSize pool_size = {0};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = desc_cap;

    VkDescriptorPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &pool_size;
    info.maxSets = set_cap;

    VkResult res = vkCreateDescriptorPool(device, &info, NULL, dpool);
    assert(res == VK_SUCCESS);
}

void create_descriptor_binding(
    uint32_t location,
    VkShaderStageFlags stage,
    VkDescriptorSetLayoutBinding *binding
) {
    binding->binding = 0;
    binding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding->descriptorCount = 1;
    binding->stageFlags = stage;
    binding->pImmutableSamplers = NULL;
}

void create_descriptor_layout(
    VkDevice device,
    uint32_t binding_ct,
    VkDescriptorSetLayoutBinding *bindings,
    VkDescriptorSetLayout *layout
) {
    VkDescriptorSetLayoutCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = binding_ct;
    info.pBindings = bindings;

    VkResult res = vkCreateDescriptorSetLayout(device, &info, NULL, layout);
    assert(res == VK_SUCCESS);
}
