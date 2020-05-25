#ifndef VK_UNIFORM_H_
#define VK_UNIFORM_H_

#include <vulkan/vulkan.h>

#include "vk_buffer.h"

/* 
 * Helper for creating uniforms.
 * Creates a descriptor set as well as the underlying buffer.
 * .buffer and .set are the only fields that should be publicly accessed.
 */
struct Uniform {
	VkDevice device;
	struct Buffer buffer;
	VkDeviceSize size;
	VkDescriptorSet set;
	VkDescriptorSetLayout layout;
};

struct Uniform uniform_create(VkDevice device,
			      VkDescriptorPool dpool,
			      VkPhysicalDeviceMemoryProperties mem_props,
			      VkShaderStageFlags stage,
			      VkDeviceSize size);

/* Writes to the buffer inside the given Uniform.
 * The size of the write is determined by the size given to the Uniform upon
 * creation.
 */
void uniform_write(struct Uniform u,
		   void *data);

void uniform_destroy(struct Uniform u);

/*
 * Allocates a single descriptor set from a descriptor pool.
 */
void allocate_descriptor_set(VkDevice device,
			     VkDescriptorPool dpool,
			     VkDescriptorSetLayout layout,
			     VkBuffer buffer,
			     uint32_t location,
			     uint32_t size,
			     VkDescriptorSet *set);

/*
 * Creates a descriptor pool.
 *
 * desc_cap: Maximum number of individual descriptors that can be allocated
 * set_cap: Maximum number of sets that can be allocated
 *
 * The pool is able to create both uniforms and combined image samplers. The
 * descriptor cap is separate between uniforms and image samplers, but the set
 * cap is shared between both.
 */
void create_descriptor_pool(VkDevice device,
			    uint32_t desc_cap,
			    uint32_t set_cap,
			    VkDescriptorPool *dpool);

void create_descriptor_binding(uint32_t location,
			       VkDescriptorType type,
			       VkShaderStageFlags stage,
			       VkDescriptorSetLayoutBinding *binding);

void create_descriptor_layout(VkDevice device,
			      uint32_t binding_ct,
			      VkDescriptorSetLayoutBinding *bindings,
			      VkDescriptorSetLayout *layout);

#endif // VK_UNIFORM_H_
