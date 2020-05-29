#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_cbuf.h"
#include "../src/vk_tools.h"
#include "../src/vk_buffer.h"
#include "../src/vk_image.h"

#include "helpers.h"

START_TEST (ut_create_buffer_handle)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkDeviceSize byte_count = 128;
	VkBuffer buffer = NULL;
	create_buffer_handle(device,
			     byte_count,
			     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			     &buffer);

	ck_assert(buffer != NULL);

	vkDestroyBuffer(device, buffer, NULL);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_buffer_memory)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkDeviceSize buffer_size = 128;
	VkBuffer buffer;
	create_buffer_handle(device,
			     buffer_size,
			     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			     &buffer);

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	VkDeviceMemory buffer_mem = NULL;
	create_buffer_memory(device,
			     mem_props,
			     buffer,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &buffer_mem);

	ck_assert(buffer_mem != NULL);

	// try to map and write to it
	// exactly 32 values, which should be 128 bytes
	int source_data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
	void *buffer_data;

	VkResult res = vkMapMemory(device, buffer_mem, 0, buffer_size, 0, &buffer_data);
	ck_assert(res == VK_SUCCESS);
        memcpy(buffer_data, source_data, (size_t) buffer_size);
	vkUnmapMemory(device, buffer_mem);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_copy_memory)
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

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);
	VkDeviceSize buffer_size = 128;

	// create first buffer
	VkBuffer buf_first;
	create_buffer_handle(device,
			     buffer_size,
			     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     &buf_first);

	VkDeviceMemory buf_mem_first;
	create_buffer_memory(device,
			     mem_props,
			     buf_first,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &buf_mem_first);

	// write to it
	int source_data[] = {0, 1, 2};
	void *buffer_data;

	VkResult res = vkMapMemory(device, buf_mem_first, 0, buffer_size, 0, &buffer_data);
	ck_assert(res == VK_SUCCESS);
	memcpy(buffer_data, source_data, sizeof(source_data));
	vkUnmapMemory(device, buf_mem_first);

	// create a second buffer
	VkBuffer buf_second;
	create_buffer_handle(device,
			     buffer_size,
			     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			     &buf_second);

	VkDeviceMemory buf_mem_second;
	create_buffer_memory(device,
			     mem_props,
			     buf_second,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &buf_mem_second);

	// copy
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	copy_buffer_buffer(device, queue, cpool,
		    buffer_size,
		    buf_first, buf_second);

	// map the second buffer and make sure its contents are the same
	void *new_buffer_data;
	res = vkMapMemory(device, buf_mem_second, 0, buffer_size, 0, &new_buffer_data);
	ck_assert(res == VK_SUCCESS);
        ck_assert(memcmp(new_buffer_data, source_data, sizeof(int) * 3) == 0);
	vkUnmapMemory(device, buf_mem_second);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_buffer_create)
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

	uint32_t buf_size = 128;

	struct Buffer buf = {0};
	buffer_create(device,
		      dev_mem_props,
		      buf_size,
		      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &buf);

	// make sure we can map it
	void *mapped_buf;
	VkResult res = vkMapMemory(device, buf.memory, 0, buf_size, 0, &mapped_buf);
	ck_assert(res == VK_SUCCESS);

	vkDestroyBuffer(device, buf.handle, NULL);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_buffer_write)
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

	uint32_t buf_size = 128;

	struct Buffer buf = {0};
	buffer_create(device,
		      dev_mem_props,
		      buf_size,
		      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &buf);

	// write some bytes
	int source[] = {1, 2, 3};
	VkDeviceSize size = 3 * sizeof(int);

	buffer_write(buf, size, source);

	// read and make sure it matches the source
	void *mapped_buf;
	VkResult res = vkMapMemory(device, buf.memory, 0, buf_size, 0, &mapped_buf);
	ck_assert(res == VK_SUCCESS);
	ck_assert(memcmp(source, mapped_buf, size) == 0);
	vkUnmapMemory(device, buf.memory);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_buffer_destroy)
{
	VK_OBJECTS;
	helper_create_device(&gwin,
			     &dbg_msg_ct,
			     &dbg_msgr,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device);

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Buffer buf;
	buffer_create(device,
		      mem_props,
		      1,
		      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		      0,
		      &buf);

	// destroy and make sure no validation layers complain
	buffer_destroy(buf);

	vkDestroyDevice(device, NULL);
	destroy_dbg_msgr(instance, &dbg_msgr);
	vkDestroyInstance(instance, NULL);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_copy_buffer_image)
{
	VK_OBJECTS;
	helper_get_queue(&gwin,
			 &dbg_msg_ct, NULL,
			 &instance, &phys_dev, &queue_fam, &device,
			 &queue);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	// Fill a buffer with some data
	struct Buffer buf;
	unsigned char data[] = {0, 1, 2, 3};
	VkDeviceSize data_size = sizeof(data);
	
	helper_create_buffer_with_data(phys_dev, device, data_size, data, &buf);

	// Copy it to an image
	struct Image image;
	image_create(device, queue_fam, dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		     | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		     | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     1, 1,
		     &image);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	image_transition(device, queue, cpool,
			 image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 0,
			 VK_ACCESS_TRANSFER_WRITE_BIT,
			 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_image(device, queue, cpool,
			  VK_IMAGE_ASPECT_COLOR_BIT,
			  1, 1,
			  buf.handle, image.handle);

	// Check the image's contents
	void *mapped;
	VkResult res =
		vkMapMemory(device, image.memory, 0, data_size, 0, &mapped);
        ck_assert(res == VK_SUCCESS);
	
        ck_assert(memcmp(mapped, data, data_size) == 0);
	
	vkUnmapMemory(device, image.memory);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_buffer_suite(void)
{
	Suite *s;

	s = suite_create("Buffers");

	TCase *tc1 = tcase_create("Create buffer handle");
	tcase_add_test(tc1, ut_create_buffer_handle);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Create buffer memory");
	tcase_add_test(tc2, ut_create_buffer_memory);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Copy memory");
	tcase_add_test(tc3, ut_copy_memory);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("Buffer: Create");
	tcase_add_test(tc4, ut_buffer_create);
	suite_add_tcase(s, tc4);

	TCase *tc5 = tcase_create("Buffer: Write");
	tcase_add_test(tc5, ut_buffer_write);
	suite_add_tcase(s, tc5);

	TCase *tc6 = tcase_create("Buffer: Destroy");
	tcase_add_test(tc6, ut_buffer_destroy);
	suite_add_tcase(s, tc6);

	TCase *tc7 = tcase_create("Copy buffer to image");
	tcase_add_test(tc7, ut_copy_buffer_image);
	suite_add_tcase(s, tc7);

	return s;
}
