#include <assert.h>

#include "vk_uniform.h"

struct Uniform uniform_create(VkDevice device,
			      VkDescriptorPool dpool,
			      VkPhysicalDeviceMemoryProperties mem_props,
			      VkShaderStageFlags stage,
			      VkDeviceSize size)
{
	struct Uniform u = {0};
	u.size = size;
	u.device = device;

	// Create buffer
	buffer_create(device,
		      mem_props,
		      size,
		      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &u.buffer);

	// Create descriptor set
	VkDescriptorSetLayoutBinding u_desc_binding;
	create_descriptor_binding(0,
				  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				  stage,
				  &u_desc_binding);

	create_descriptor_layout(device, 1, &u_desc_binding, &u.layout);

	allocate_descriptor_set(device,
				dpool,
				u.layout,
				u.buffer.handle,
				0,
				size,
				&u.set);

	return u;
}

void uniform_write(struct Uniform u,
		   void *data)
{
	buffer_write(u.buffer, u.size, data);
}

void uniform_destroy(struct Uniform u)
{
	vkDestroyDescriptorSetLayout(u.device, u.layout, NULL);
	buffer_destroy(u.buffer);
}

void allocate_descriptor_set(VkDevice device,
			     VkDescriptorPool dpool,
			     VkDescriptorSetLayout layout,
			     VkBuffer buffer,
			     uint32_t location,
			     uint32_t size,
			     VkDescriptorSet *set)
{
	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = dpool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkResult res = vkAllocateDescriptorSets(device, &alloc_info, set);
	assert(res == VK_SUCCESS);

	// Fuckin' magic
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

void create_descriptor_pool(VkDevice device,
			    uint32_t desc_cap,
			    uint32_t set_cap,
			    VkDescriptorPool *dpool)
{
	VkDescriptorPoolSize pool_sizes[2] = {0};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = desc_cap;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = desc_cap;

	VkDescriptorPoolCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = 2;
	info.pPoolSizes = pool_sizes;
	info.maxSets = set_cap;

	VkResult res = vkCreateDescriptorPool(device, &info, NULL, dpool);
	assert(res == VK_SUCCESS);
}

void create_descriptor_binding(uint32_t location,
			       VkDescriptorType type,
			       VkShaderStageFlags stage,
			       VkDescriptorSetLayoutBinding *binding)
{
	binding->binding = location;
	binding->descriptorType = type;
	binding->descriptorCount = 1;
	binding->stageFlags = stage;
	binding->pImmutableSamplers = NULL;
}

void create_descriptor_layout(VkDevice device,
			      uint32_t binding_ct,
			      VkDescriptorSetLayoutBinding *bindings,
			      VkDescriptorSetLayout *layout)
{
	VkDescriptorSetLayoutCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = binding_ct;
	info.pBindings = bindings;

	VkResult res = vkCreateDescriptorSetLayout(device, &info, NULL, layout);
	assert(res == VK_SUCCESS);
}
