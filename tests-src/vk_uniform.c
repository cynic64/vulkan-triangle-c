#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

#include "../src/vk_tools.h"
#include "../src/vk_pipe.h"
#include "../src/vk_uniform.h"
#include "../src/vk_buffer.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_vertex.h"

#include "helpers.h"

START_TEST (ut_create_descriptor_layout)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkDescriptorSetLayoutBinding binding = {0};
	create_descriptor_binding(0,
				  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				  VK_SHADER_STAGE_VERTEX_BIT,
				  &binding);

	VkDescriptorSetLayout desc_layout = NULL;
	create_descriptor_layout(device, 1, &binding, &desc_layout);
	ck_assert(desc_layout != NULL);

	// Try using it when creating a pipeline
	VkPipelineShaderStageCreateInfo shtages[2];
	helper_create_shtage(device,
			     "assets/testing/shaders/uniform.vert.spv",
			     VK_SHADER_STAGE_VERTEX_BIT,
			     &shtages[0]);
	helper_create_shtage(device,
			     "assets/testing/shaders/uniform.frag.spv",
			     VK_SHADER_STAGE_FRAGMENT_BIT,
			     &shtages[1]);

	// format doesn't matter since we don't use a swapchain
	create_rpass(device, DEFAULT_FMT, &rpass);

	create_layout(device, 1, &desc_layout, &pipe_layout);

	create_pipel(device,
		     2,
		     shtages,
		     pipe_layout,
		     0, NULL,
		     0, NULL,
		     rpass,
		     &pipel);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_descriptor_pool)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkDescriptorSetLayoutBinding bindings[2];
	create_descriptor_binding(0,
				  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				  VK_SHADER_STAGE_VERTEX_BIT,
				  &bindings[0]);
	create_descriptor_binding(1,
				  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				  VK_SHADER_STAGE_FRAGMENT_BIT,
				  &bindings[1]);

	VkDescriptorSetLayout layout;
	create_descriptor_layout(device, 2, bindings, &layout);

	VkDescriptorPool dpool = NULL;
	create_descriptor_pool(device, 1, 2, &dpool);
	ck_assert(dpool != NULL);

	// Try to allocate a uniform and image sampler descriptor set from the
	// pool
	VkDescriptorSetAllocateInfo uniform_info = {0};
	uniform_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	uniform_info.descriptorPool = dpool;
	uniform_info.descriptorSetCount = 1;
	uniform_info.pSetLayouts = &layout;

	VkDescriptorSet sets[2];

	VkResult res = vkAllocateDescriptorSets(device, &uniform_info, sets);
	ck_assert(res == VK_SUCCESS);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_allocate_descriptor_set)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkDescriptorSetLayoutBinding binding;
	create_descriptor_binding(0,
				  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				  VK_SHADER_STAGE_VERTEX_BIT,
				  &binding);

	VkDescriptorSetLayout desc_layout;
	create_descriptor_layout(device, 1, &binding, &desc_layout);

	VkDescriptorPool dpool;
	create_descriptor_pool(device, 1, 1, &dpool);

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	uint32_t uniform_size = sizeof(mat4);

	struct Buffer buf;
	buffer_create(device,
		      mem_props,
		      uniform_size,
		      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		      0,
		      &buf);

	VkDescriptorSet set = NULL;
	allocate_descriptor_set(device,
				dpool,
				desc_layout,
				&set);

	// we should now be able to bind that set in a command buffer without errors
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	create_layout(device, 1, &desc_layout, &pipe_layout);
	VkPipelineShaderStageCreateInfo shtage;
	helper_create_shtage(device,
			     "assets/testing/shaders/uniform.vert.spv",
			     VK_SHADER_STAGE_VERTEX_BIT,
			     &shtage);

	create_rpass(device, VK_FORMAT_B8G8R8A8_UNORM, &rpass);

	create_pipel(device,
		     1,
		     &shtage,
		     pipe_layout,
		     0,
		     NULL,
		     0,
		     NULL,
		     rpass,
		     &pipel);

	// start recording command buffer
	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);

	vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipel);

	// finally
	vkCmdBindDescriptorSets(cbuf,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipe_layout,
				0,
				1,
				&set,
				0,
				NULL);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST(ut_set_create)
{
	VK_OBJECTS;
	helper_get_queue(&gwin,
			 &dbg_msg_ct, &dbg_msgr,
			 &instance, &phys_dev, &queue_fam, &device, &queue);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// There's no easy way to test this apart from actually rendering a
	// frame and seeing if all the sets worked out correctly.
	//
	// So, we use a shader that will combine a total of four uniforms to
	// determine the color of a single pixel, and then check that the
	// rendered pixel is the color we expect it to be.
	//
	// The layout is:
	// (set = 0, binding = 0) vec4: (0.626) =~ 0xa0
	// (set = 0, binding = 1) A single-pixel image, #b0000000
	// (set = 1, binding = 0) A single-pixel image, #c0000000
	// (set = 1, binding = 1) vec4: (0.815) =~ 0xd0
	//
	// The final pixel should be of color #d0a0b0c0, because the alpha gets
	// flipped around from a vec4 to B8G8R8A8_UNORM.
	float buf_data[2][4] = {{0.626, 0.626, 0.626, 0.626},
				{0.815, 0.815, 0.815, 0.815}};
	uint32_t buf_data_size = sizeof(buf_data[0]);
	
	unsigned char img_data[2][4] = {{0xb0, 0xb0, 0xb0, 0xb0},
					{0xc0, 0xc0, 0xc0, 0xc0}};

	// Create buffers
	struct Buffer bufs[2];

	for (int i = 0; i < ARRAY_SIZE(bufs); i++) {
		helper_create_buffer_with_data(phys_dev, device,
					       buf_data_size, &buf_data[i], &bufs[i]);
	}

	// Create images
	struct Image imgs[2];

	for (int i = 0; i < ARRAY_SIZE(imgs); i++) {
		helper_create_image_with_data(phys_dev, device,
					      queue_fam, queue, cpool,
					      DEFAULT_FMT,
					      VK_IMAGE_USAGE_SAMPLED_BIT,
					      VK_IMAGE_ASPECT_COLOR_BIT,
					      1, 1,
					      4, &img_data[i], &imgs[i]);
		
		image_transition(device, queue, cpool,
				 imgs[i].handle, VK_IMAGE_ASPECT_COLOR_BIT,
				 VK_ACCESS_TRANSFER_WRITE_BIT,
				 VK_ACCESS_SHADER_READ_BIT,
				 VK_PIPELINE_STAGE_TRANSFER_BIT,
				 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// Sampler
	VkSamplerCreateInfo sampler_info = {0};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler sampler;
	VkResult res = vkCreateSampler(device, &sampler_info, NULL, &sampler);
	ck_assert(res == VK_SUCCESS);

	// Setup
	VkDescriptorType desc_types_1[2] =
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
	VkDescriptorType desc_types_2[2] = 
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
	
	VkDescriptorImageInfo image_infos_1[2];
	image_infos_1[1].sampler = sampler;
	image_infos_1[1].imageView = imgs[0].view;
	image_infos_1[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo image_infos_2[2];
	image_infos_2[0].sampler = sampler;
	image_infos_2[0].imageView = imgs[1].view;
	image_infos_2[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo buffer_infos_1[2];
	buffer_infos_1[0].buffer = bufs[0].handle;
	buffer_infos_1[0].offset = 0;
	buffer_infos_1[0].range = buf_data_size;

	VkDescriptorBufferInfo buffer_infos_2[2];
	buffer_infos_2[1].buffer = bufs[1].handle;
	buffer_infos_2[1].offset = 0;
	buffer_infos_2[1].range = buf_data_size;

	// Same for both sets
	VkShaderStageFlags stages[2] = {VK_SHADER_STAGE_FRAGMENT_BIT,
					VK_SHADER_STAGE_FRAGMENT_BIT};

	// Create sets
	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	VkDescriptorPool dpool;
	// 2 sets, 4 bindings total
	create_descriptor_pool(device, 4, 2, &dpool);

	struct Set set1;
	set_create(device, dpool,
		   2, desc_types_1, buffer_infos_1, image_infos_1, stages,
		   &set1);

	struct Set set2;
	set_create(device, dpool,
		   2, desc_types_2, buffer_infos_2, image_infos_2, stages,
		   &set2);

	struct Image image;
	image_create(device, queue_fam, dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     1, 1,
		     &image);

	create_rpass(device, DEFAULT_FMT, &rpass);

	VkFramebuffer fb;
	create_framebuffer(device, 1, 1, rpass, image.view, &fb);

	// Square vbuf/ibuf
	struct Vertex2PosColor vertices[] = {
		{.pos = {-1.0, -1.0}, .color = {1.0, 0.0, 1.0}},
		{.pos = {1.0, -1.0}, .color = {1.0, 0.0, 1.0}},
		{.pos = {-1.0, 1.0}, .color = {1.0, 0.0, 1.0}},
		{.pos = {1.0, 1.0}, .color = {1.0, 0.0, 1.0}}
	};
	uint32_t vertex_ct = ARRAY_SIZE(vertices);

	uint32_t indices[] = {0, 1, 2, 1, 2, 3};
	uint32_t index_ct = ARRAY_SIZE(indices);

	struct Buffer vbuf;
	helper_create_buffer_with_data(phys_dev, device,
				       sizeof(vertices), vertices,
				       &vbuf);

	struct Buffer ibuf;
	helper_create_buffer_with_data(phys_dev, device,
				       sizeof(indices), indices,
				       &ibuf);

	// Shaders
	VkPipelineShaderStageCreateInfo shader_stages[2];
	helper_create_shtage(device, "assets/testing/shaders/sets.vert.spv",
			     VK_SHADER_STAGE_VERTEX_BIT, &shader_stages[0]);
	helper_create_shtage(device, "assets/testing/shaders/sets.frag.spv",
			     VK_SHADER_STAGE_FRAGMENT_BIT, &shader_stages[1]);

	VkDescriptorSetLayout set_layouts[] = {set1.layout, set2.layout};
	uint32_t set_layout_ct = ARRAY_SIZE(set_layouts);

	VkDescriptorSet sets[] = {set1.handle, set2.handle};
	uint32_t set_ct = ARRAY_SIZE(sets);
	
	VkPipelineLayout layout;
	create_layout(device, set_layout_ct, set_layouts, &layout);

	create_pipel(device, 2, shader_stages, layout,
		     VERTEX_2_POS_COLOR_BINDING_CT, VERTEX_2_POS_COLOR_BINDINGS,
		     VERTEX_2_POS_COLOR_ATTRIBUTE_CT, VERTEX_2_POS_COLOR_ATTRIBUTES,
		     rpass,
		     &pipel);

	VkCommandBuffer cbuf;
	create_cbuf(device, cpool, rpass, fb, 1, 1, layout, pipel,
		    set_ct, sets, vbuf.handle, ibuf.handle, index_ct,
		    &cbuf);

	submit_syncless(device, queue, cpool, cbuf);

	// Copy image out
	struct Buffer buf;
	buffer_create(device, dev_mem_props,
		      4, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &buf);

	image_transition(device, queue, cpool,
			 image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	copy_image_buffer(device, queue, cpool,
			  VK_IMAGE_ASPECT_COLOR_BIT, 1, 1,
			  image.handle, buf.handle);

	void *mapped;
	uint32_t pixel;
	res = vkMapMemory(buf.device, buf.memory, 0, 4, 0, &mapped);
        ck_assert(res == VK_SUCCESS);
	memcpy(&pixel, mapped, 4);
	vkUnmapMemory(buf.device, buf.memory);

	// Byte order: ARGB
	uint32_t true_value = 0xd0a0b0c0;
	ck_assert(memcmp(&pixel, &true_value, sizeof(true_value)) == 0);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_uniform_suite(void)
{
	Suite *s;

	s = suite_create("Uniforms");

	TCase *tc1 = tcase_create("Create descriptor layout");
	tcase_add_test(tc1, ut_create_descriptor_layout);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Create descriptor pool");
	tcase_add_test(tc2, ut_create_descriptor_pool);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Allocate descriptor set");
	tcase_add_test(tc3, ut_allocate_descriptor_set);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("Set: Create");
	tcase_add_test(tc4, ut_set_create);
	suite_add_tcase(s, tc4);

	return s;
}
