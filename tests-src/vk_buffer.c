#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_cbuf.h"
#include "../src/vk_buffer.h"

#include "helpers.h"

START_TEST (ut_create_buffer_handle) {
    VK_OBJECTS;
    helper_create_device(
       &gwin,
       &dbg_msg_ct,
       NULL,
       &instance,
       &phys_dev,
       &queue_fam,
       &device
    );

    VkDeviceSize byte_count = 128;
    VkBuffer buffer = NULL;
    create_buffer_handle(
        device,
        byte_count,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &buffer
    );

    ck_assert(buffer != NULL);

    vkDestroyBuffer(device, buffer, NULL);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_buffer_memory) {
    VK_OBJECTS;
    helper_create_device(
       &gwin,
       &dbg_msg_ct,
       NULL,
       &instance,
       &phys_dev,
       &queue_fam,
       &device
    );

    VkDeviceSize buffer_size = 128;
    VkBuffer buffer;
    create_buffer_handle(
        device,
        buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        &buffer
    );

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

    VkDeviceMemory buffer_mem = NULL;
    create_buffer_memory(
        device,
        mem_props,
        buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &buffer_mem
    );

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

START_TEST (ut_copy_memory) {
    VK_OBJECTS;
    helper_get_queue(
       &gwin,
       &dbg_msg_ct,
       NULL,
       &instance,
       &phys_dev,
       &queue_fam,
       &device,
       &queue
    );

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);
    VkDeviceSize buffer_size = 128;

    // create first buffer
    VkBuffer buf_first;
    create_buffer_handle(
        device,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        &buf_first
    );

    VkDeviceMemory buf_mem_first;
    create_buffer_memory(
        device,
        mem_props,
        buf_first,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &buf_mem_first
    );

    // write to it
    int source_data[] = {0, 1, 2};
    void *buffer_data;

    VkResult res = vkMapMemory(device, buf_mem_first, 0, buffer_size, 0, &buffer_data);
    ck_assert(res == VK_SUCCESS);
        memcpy(buffer_data, source_data, (size_t) buffer_size);
    vkUnmapMemory(device, buf_mem_first);

    // create a second buffer
    VkBuffer buf_second;
    create_buffer_handle(
        device,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        &buf_second
    );

    VkDeviceMemory buf_mem_second;
    create_buffer_memory(
        device,
        mem_props,
        buf_second,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &buf_mem_second
    );

    // copy
    VkCommandPool cpool;
    create_cpool(device, queue_fam, &cpool);

    copy_buffer(
        device,
        queue,
        cpool,
        buffer_size,
        buf_first,
        buf_second
    );

    // map the second buffer and make sure its contents are the same
    void *new_buffer_data;
    res = vkMapMemory(device, buf_mem_second, 0, buffer_size, 0, &new_buffer_data);
    ck_assert(res == VK_SUCCESS);
        ck_assert(memcmp(new_buffer_data, source_data, sizeof(int) * 3) == 0);
    vkUnmapMemory(device, buf_mem_second);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_buffer_create) {
    VK_OBJECTS;
    helper_create_device(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device
   );

   VkPhysicalDeviceMemoryProperties dev_mem_props;
   vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

   uint32_t buf_size = 128;

   struct Buffer buf = {0};
   buffer_create(
       device,
       dev_mem_props,
       buf_size,
       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
       &buf
   );

   // make sure we can map it
   void *mapped_buf;
   VkResult res = vkMapMemory(device, buf.memory, 0, buf_size, 0, &mapped_buf);
   ck_assert(res == VK_SUCCESS);

   vkDestroyBuffer(device, buf.handle, NULL);

   ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_buffer_write) {
    VK_OBJECTS;
    helper_create_device(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device
   );

   VkPhysicalDeviceMemoryProperties dev_mem_props;
   vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

   uint32_t buf_size = 128;

   struct Buffer buf = {0};
   buffer_create(
       device,
       dev_mem_props,
       buf_size,
       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
       &buf
   );

   // write some bytes
   int source[] = {1, 2, 3};
   VkDeviceSize size = 3 * sizeof(int);

   buffer_write(device, buf, size, source);

   // read and make sure it matches the source
   void *mapped_buf;
   VkResult res = vkMapMemory(device, buf.memory, 0, buf_size, 0, &mapped_buf);
       ck_assert(res == VK_SUCCESS);
       ck_assert(memcmp(source, mapped_buf, size) == 0);
   vkUnmapMemory(device, buf.memory);

   ck_assert(dbg_msg_ct == 0);
}

Suite *vk_buffer_suite(void) {
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

    TCase *tc4 = tcase_create("Buffer: create");
    tcase_add_test(tc4, ut_buffer_create);
    suite_add_tcase(s, tc4);

    TCase *tc5 = tcase_create("Buffer: write");
    tcase_add_test(tc5, ut_buffer_write);
    suite_add_tcase(s, tc5);

    return s;
}
