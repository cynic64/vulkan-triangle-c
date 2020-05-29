#ifndef VK_UNIFORM_H_
#define VK_UNIFORM_H_

#include <vulkan/vulkan.h>

#include "vk_buffer.h"

struct Set {
	VkDescriptorSet handle;
	VkDescriptorSetLayout layout;
};

/*
 * Create a VkDescriptorSet.
 *
 * desc_ct: How may descriptors this set has
 * desc_types: What type each descriptor is (buffer or sampled image)
 * buffer_infos: Buffer info or NULL if descriptor is not a buffer
 * image_infos: Image info or NULL if descriptor is not an image
 * stages: What shader stages each descriptor will be used in
 */
void set_create(VkDevice device, VkDescriptorPool dpool,
		uint32_t desc_ct,
		VkDescriptorType *desc_types,
		VkDescriptorBufferInfo *buffer_infos,
		VkDescriptorImageInfo *image_infos,
		VkShaderStageFlags *stages,
		struct Set *set);

/*
 * Destroys a Set struct.
 *
 * Really only destroys the layout, the set itself should be freed by the user
 * as part of the descriptor pool.
 */
void set_destroy(VkDevice device, struct Set set);

/*
 * Allocates a single descriptor set from a descriptor pool.
 */
void allocate_descriptor_set(VkDevice device,
			     VkDescriptorPool dpool, VkDescriptorSetLayout layout,
			     VkDescriptorSet *set);

/*
 * Write any kind of descriptor (image or buffer).
 *
 * location: Descriptor index (within set)
 * type: Descriptor type, also determines what other arguments are used
 * buf: If type == UNIFORM_BUFFER, buffer to write
 * buf_size: If type == UNIFORM_BUFFER, buffer size
 * img_sampler: If type == COMBINED_IMAGE_SAMPLER, image sampler
 * img_view: If type == COMBINED_IMAGE_SAMPLER, image view
 * img_layout: If type == COMBINED_IMAGE_SAMPLER, image layout
 *
 * If an argument is not relevant, leave it as NULL.
 */
void write_descriptor_general(VkDevice device,
			      VkDescriptorSet set, uint32_t location,
			      VkDescriptorType type,
			      VkBuffer buf, VkDeviceSize buf_size,
			      VkSampler img_sampler, VkImageView img_view,
			      VkImageLayout img_layout);

/*
 * Write a buffer to a specific descriptor within a set.
 *
 * location: Descriptor index (so within set)
 */
void write_descriptor_buffer(VkDevice device,
			     VkDescriptorSet set, uint32_t location,
			     VkBuffer buffer, VkDeviceSize size);

/*
 * Write an image to a descriptor within a set.
 */
void write_descriptor_image(VkDevice device,
			    VkDescriptorSet set, uint32_t location,
			    VkSampler sampler,
			    VkImageView view, VkImageLayout layout);
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
