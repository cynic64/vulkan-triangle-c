#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "vk_uniform.h"

void set_create(VkDevice device, VkDescriptorPool dpool,
		uint32_t desc_ct,
		VkDescriptorType *desc_types,
		VkDescriptorBufferInfo *buffer_infos,
		VkDescriptorImageInfo *image_infos,
		VkShaderStageFlags *stages,
		struct Set *set)
{
	// Create descriptors
	VkDescriptorSetLayoutBinding *descs = malloc(sizeof(descs[0]) * desc_ct);

	for (int i = 0; i < desc_ct; i++) {
		create_descriptor_binding(i, desc_types[i], stages[i],
					  &descs[i]);
	}

	// Create set layout
	create_descriptor_layout(device, desc_ct, descs, &set->layout);

	// Allocate set
	allocate_descriptor_set(device, dpool, set->layout, &set->handle);

	// Write
	for (int i = 0; i < desc_ct; i++) {
		write_descriptor_general(device, set->handle,
					 i, desc_types[i],
					 buffer_infos[i].buffer,
					 buffer_infos[i].range,
					 image_infos[i].sampler,
					 image_infos[i].imageView,
					 image_infos[i].imageLayout);
	}
}

void write_descriptor_general(VkDevice device,
			      VkDescriptorSet set, uint32_t location,
			      VkDescriptorType type,
			      VkBuffer buf, VkDeviceSize buf_size,
			      VkSampler img_sampler, VkImageView img_view,
			      VkImageLayout img_layout)
{
	if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
		assert(buf != NULL);
		assert(buf_size > 0);
		write_descriptor_buffer(device, set, location, buf, buf_size);
	} else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
		assert(img_sampler != NULL);
		assert(img_view != NULL);
		write_descriptor_image(device, set, location,
				       img_sampler, img_view, img_layout);
	} else {
		printf("(write_descriptor_general) Bad type: %d\n", type);
		exit(1);
	}
}

void allocate_descriptor_set(VkDevice device,
			     VkDescriptorPool dpool, VkDescriptorSetLayout layout,
			     VkDescriptorSet *set)
{
	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = dpool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkResult res = vkAllocateDescriptorSets(device, &alloc_info, set);
	assert(res == VK_SUCCESS);
}

void write_descriptor_buffer(VkDevice device,
			     VkDescriptorSet set, uint32_t location,
			     VkBuffer buffer, VkDeviceSize size)
{
	VkDescriptorBufferInfo buffer_info = {0};
	buffer_info.buffer = buffer;
	buffer_info.offset = 0;
	buffer_info.range = size;

	VkWriteDescriptorSet desc_write = {0};
	desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_write.dstSet = set;
	desc_write.dstBinding = location;
	desc_write.dstArrayElement = 0;

	desc_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	desc_write.descriptorCount = 1;
	desc_write.pBufferInfo = &buffer_info;

	vkUpdateDescriptorSets(device, 1, &desc_write, 0, NULL);
}

void write_descriptor_image(VkDevice device,
			    VkDescriptorSet set, uint32_t location,
			    VkSampler sampler,
			    VkImageView view, VkImageLayout layout)
{
	VkDescriptorImageInfo image_info = {0};
	image_info.sampler = sampler;
	image_info.imageView = view;
	image_info.imageLayout = layout;

	VkWriteDescriptorSet desc_write = {0};
	desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_write.dstSet = set;
	desc_write.dstBinding = location;
	desc_write.dstArrayElement = 0;

	desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_write.descriptorCount = 1;
	desc_write.pImageInfo = &image_info;

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
