#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_image.h"
#include "../src/ll_vk_image.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_buffer.h"

#include "helpers.h"

#define DEFAULT_FMT VK_FORMAT_B8G8R8A8_UNORM

START_TEST (ut_image_create)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

        VkPhysicalDeviceMemoryProperties dev_mem_props;
        vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);
	
	struct Image image;
	image_create(device,
		     queue_fam,
		     dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     1920, 1080,
		     &image);

        // Try creating a framebuffer from it
	create_rpass(device, DEFAULT_FMT, &rpass);

	VkFramebuffer fb;
	create_framebuffer(device, 1920, 1080, rpass, image.view, &fb);

	ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_image_handle_create)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);
	
	VkImage image;
	image_handle_create(device,
			    queue_fam,
			    DEFAULT_FMT,
			    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			    1920, 1080,
			    &image);

	// Test by trying to create a view for the image
	VkImageView view;
	image_view_create(device,
			  DEFAULT_FMT, VK_IMAGE_ASPECT_COLOR_BIT,
			  image, &view);

	// We should get exactly 1 validation message: that no memory is bound	
	ck_assert(dbg_msg_ct == 1);
} END_TEST

START_TEST (ut_image_memory_bind)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkImage image;
	image_handle_create(device,
			    queue_fam,
			    DEFAULT_FMT,
			    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			    1920, 1080,
			    &image);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	VkDeviceMemory image_mem;
	image_memory_bind(device, dev_mem_props, 0, image, &image_mem);

	// Now there should be no errors if we try to create an image view
	VkImageView view;
	image_view_create(device,
			  DEFAULT_FMT, VK_IMAGE_ASPECT_COLOR_BIT,
			  image, &view);

	ck_assert(dbg_msg_ct == 0);	
} END_TEST

START_TEST (ut_find_memory_type)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	// VkMemoryRequirements includes a size an alignment, which we don't
	// care about, and memoryTypeBits, which is a bitfield where every valid
	// memory index is set (used as a mask for finding a suitable memory
	// type).
	// Since we're not actually creating a buffer, anything will do.
	uint32_t memoryTypeBits = 0b11111111111111111111111111111111;
	VkMemoryRequirements buf_reqs = {
		.size = 1,
		.alignment = 1,
		.memoryTypeBits = memoryTypeBits
	};

	VkMemoryPropertyFlags req_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	uint32_t mem_idx = find_memory_type(dev_mem_props, buf_reqs, req_props);

	// Make sure the returned index actually has the properties we want
	VkMemoryPropertyFlags real_props =
		dev_mem_props.memoryTypes[mem_idx].propertyFlags;

	ck_assert(real_props & req_props == req_props);
} END_TEST

START_TEST (ut_image_view_create)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkImage image;
	image_handle_create(device,
			    queue_fam,
			    DEFAULT_FMT,
			    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			    1920, 1080,
			    &image);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	VkDeviceMemory image_mem;
	image_memory_bind(device, dev_mem_props, 0, image, &image_mem);

	VkImageView view;
	image_view_create(device,
			  DEFAULT_FMT, VK_IMAGE_ASPECT_COLOR_BIT,
			  image, &view);

	// Try to create a framebuffer with the image and image view
	create_rpass(device, DEFAULT_FMT, &rpass);

	VkFramebuffer fb;
	create_framebuffer(device, 1920, 1080, rpass, view, &fb);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_image_copy_to_buffer)
{
	VK_OBJECTS;
	helper_get_queue(&gwin,
			 &dbg_msg_ct,
			 NULL,
			 &instance,
			 &phys_dev,
			 &queue_fam,
			 &device,
			 &queue);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);
	
	struct Image image;
	image_create(device, queue_fam, dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     1920, 1080,
		     &image);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);
	
	image_transition(device, queue, cpool,
			 image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// We should now be able to use it as a transfer source
	struct Buffer buf;
	buffer_create(device, dev_mem_props,
		      // Need 4 bytes for a single pixel
		      4,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		      0,
		      &buf);

	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 1;
	region.bufferImageHeight = 1;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = 1;
	region.imageExtent.height = 1;
	region.imageExtent.depth = 1;
	
	vkCmdCopyImageToBuffer(cbuf,
			       image.handle,
			       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			       buf.handle,
			       1, &region);

	VkResult res = vkEndCommandBuffer(cbuf);
	ck_assert(res == VK_SUCCESS);

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cbuf;

	res = vkQueueSubmit(queue, 1, &submit_info, NULL);
	ck_assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	ck_assert(res == VK_SUCCESS);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_image_transition)
{
	VK_OBJECTS;
	helper_get_queue(&gwin,
			 &dbg_msg_ct,
			 NULL,
			 &instance,
			 &phys_dev,
			 &queue_fam,
			 &device,
			 &queue);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);
	
	struct Image image;
	image_create(device, queue_fam, dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     1920, 1080,
		     &image);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);
	
	image_transition(device, queue, cpool,
			 image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	struct Buffer buf;
	buffer_create(device, dev_mem_props,
		      // Need 4 bytes for a single pixel
		      4,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &buf);

	// Write 255 to the first byte in the image so we can verify it changes
	// when we copy the image into it
	unsigned char data = 255;
	buffer_write(buf, 1, &data);

	// Copy and make sure the first byte is 0 again
	image_copy_to_buffer(device, queue, cpool,
			     VK_IMAGE_ASPECT_COLOR_BIT, 1, 1,
			     image.handle, buf.handle);

	void *mapped;
	VkResult res = vkMapMemory(buf.device, buf.memory, 0, 1, 0, &mapped);
        ck_assert(res == VK_SUCCESS);
	ck_assert(memcmp(mapped, "\0", 1) == 0);
	vkUnmapMemory(buf.device, buf.memory);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_image_suite(void)
{
	Suite *s;

	s = suite_create("Images");

	TCase *tc1 = tcase_create("Create image handle");
	tcase_add_test(tc1, ut_image_handle_create);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Bind image memory");
	tcase_add_test(tc2, ut_image_memory_bind);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Find memory type");
	tcase_add_test(tc3, ut_find_memory_type);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("Create image view");
	tcase_add_test(tc4, ut_image_view_create);
	suite_add_tcase(s, tc4);

	TCase *tc5 = tcase_create("Create Image");
	tcase_add_test(tc5, ut_image_create);
	suite_add_tcase(s, tc5);

	TCase *tc6 = tcase_create("Transition image layout");
	tcase_add_test(tc6, ut_image_transition);
	suite_add_tcase(s, tc6);

	return s;
}
