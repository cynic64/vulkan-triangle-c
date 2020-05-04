#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

#include "../src/vk_pipe.h"
#include "../src/vk_uniform.h"
#include "../src/vk_buffer.h"
#include "../src/vk_cbuf.h"

#include "helpers.h"

START_TEST (ut_create_decriptor_layout) {
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

    VkDescriptorSetLayoutBinding binding = {0};
    create_descriptor_binding(0, VK_SHADER_STAGE_VERTEX_BIT, &binding);

    VkDescriptorSetLayout desc_layout = NULL;
    create_descriptor_layout(device, 1, &binding, &desc_layout);
    ck_assert(desc_layout != NULL);

    // try using it when creating a pipeline
    VkPipelineShaderStageCreateInfo shtages[2];
    helper_create_shtage(
        device,
        "assets/testing/shaders/uniform.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT,
        &shtages[0]
    );
    helper_create_shtage(
        device,
        "assets/testing/shaders/uniform.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT,
        &shtages[1]
    );

    // format doesn't matter since we don't use a swapchain
    create_rpass(device, VK_FORMAT_B8G8R8A8_UNORM, &rpass);

    create_layout(device, 1, &desc_layout, &pipe_layout);

    create_pipel(
        device,
        2,
        shtages,
        pipe_layout,
        0,
        NULL,
        0,
        NULL,
        rpass,
        &pipel
    );

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_descriptor_pool) {
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

    VkDescriptorSetLayoutBinding binding;
    create_descriptor_binding(0, VK_SHADER_STAGE_VERTEX_BIT, &binding);

    VkDescriptorSetLayout layout;
    create_descriptor_layout(device, 1, &binding, &layout);

    VkDescriptorPool dpool = NULL;
    create_descriptor_pool(device, 1, 1, &dpool);
    ck_assert(dpool != NULL);

    // try to allocate a descriptor set from the pool
    VkDescriptorSetAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = dpool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set;

    VkResult res = vkAllocateDescriptorSets(device, &info, &set);
    ck_assert(res == VK_SUCCESS);

    ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_allocate_descriptor_set) {
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

    VkDescriptorSetLayoutBinding binding;
    create_descriptor_binding(0, VK_SHADER_STAGE_VERTEX_BIT, &binding);

    VkDescriptorSetLayout desc_layout;
    create_descriptor_layout(device, 1, &binding, &desc_layout);

    VkDescriptorPool dpool;
    create_descriptor_pool(device, 1, 1, &dpool);

    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

    uint32_t uniform_size = sizeof(mat4);

    struct Buffer buf;
    buffer_create(
        device,
        mem_props,
        uniform_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        0,
        &buf
    );

    VkDescriptorSet set = NULL;
    allocate_descriptor_set(
        device,
        dpool,
        desc_layout,
        buf.handle,
        0,
        uniform_size,
        &set
    );

    // we should now be able to bind that set in a command buffer without errors
    VkCommandPool cpool;
    create_cpool(device, queue_fam, &cpool);

    create_layout(device, 1, &desc_layout, &pipe_layout);
    VkPipelineShaderStageCreateInfo shtage;
    helper_create_shtage(
        device,
        "assets/testing/shaders/uniform.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT,
        &shtage
    );

    create_rpass(device, VK_FORMAT_B8G8R8A8_UNORM, &rpass);

    create_pipel(
        device,
        1,
        &shtage,
        pipe_layout,
        0,
        NULL,
        0,
        NULL,
        rpass,
        &pipel
    );

    // start recording command buffer
    VkCommandBuffer cbuf;
    VkResult res;

    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = cpool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    res = vkAllocateCommandBuffers(device, &alloc_info, &cbuf);
    ck_assert(res == VK_SUCCESS);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    res = vkBeginCommandBuffer(cbuf, &begin_info);
    ck_assert(res == VK_SUCCESS);

    vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipel);

    // finally
    vkCmdBindDescriptorSets(
        cbuf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipe_layout,
        0,
        1,
        &set,
        0,
        NULL
    );

    ck_assert(dbg_msg_ct == 0);
}

Suite *vk_uniform_suite(void) {
    Suite *s;

    s = suite_create("Uniforms");

    TCase *tc1 = tcase_create("Create descriptor layout");
    tcase_add_test(tc1, ut_create_decriptor_layout);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create descriptor pool");
    tcase_add_test(tc2, ut_create_descriptor_pool);
    suite_add_tcase(s, tc2);

    TCase *tc3 = tcase_create("Allocate descriptor set");
    tcase_add_test(tc3, ut_allocate_descriptor_set);
    suite_add_tcase(s, tc3);

    return s;
}
