#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"

#include "helpers.h"

START_TEST (ut_read_bin) {
    FILE *fp = fopen("assets/testing/bin_data", "rb");
    ck_assert(fp != NULL);

    size_t buf_size = 0;
    read_bin(fp, &buf_size, NULL);

    char *buf = malloc(buf_size);
    read_bin(fp, &buf_size, buf);

    char known_buf[] = {1, 2, 3, 4, 'a', 'b', 'c', 'd'};

    ck_assert(buf_size == 8);
    ck_assert(memcmp(buf, known_buf, buf_size) == 0);
} END_TEST

START_TEST (ut_create_shmod) {
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

    FILE *fp = fopen("assets/testing/shaders/empty.vert.spv", "rb");
    ck_assert(fp != NULL);
    size_t buf_size = 0;
    read_bin(fp, &buf_size, NULL);
    char *buf = malloc(buf_size);
    read_bin(fp, &buf_size, buf);

    VkShaderModule shmod = NULL;
    create_shmod(device, buf_size, buf, &shmod);

    // it's kinda difficult to test whether the shader modules are valid or not
    // without creating the entire pipeline.
    // this is all I have for now
    ck_assert(shmod != NULL);

    vkDestroyShaderModule(device, shmod, NULL);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST(ut_create_shtage) {
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

    VkPipelineShaderStageCreateInfo shtage = {0};
    helper_create_shtage(
        device,
        "assets/testing/shaders/empty.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT,
        &shtage
    );

    ck_assert(shtage.stage == VK_SHADER_STAGE_VERTEX_BIT);

    // it's just a struct, not a real Vulkan object on the GPU, so we can't
    // destroy it and see what happens
} END_TEST

START_TEST (ut_create_layout) {
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

    VkPipelineLayout layout = NULL;
    create_layout(device, &layout);

    // again, hard to test
    ck_assert(layout != NULL);

    vkDestroyPipelineLayout(device, layout, NULL);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_rpass) {
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

    create_rpass(device, SW_FORMAT, &rpass);

    ck_assert(rpass != NULL);

    vkDestroyRenderPass(device, rpass, NULL);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST(ut_create_pipel) {
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

    // load shaders
    VkPipelineShaderStageCreateInfo shtages[2];

    helper_create_shtage(
        device,
        "assets/testing/shaders/empty.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT,
        &shtages[0]
    );
    helper_create_shtage(
        device,
        "assets/testing/shaders/empty.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT,
        &shtages[1]
    );

    // render pass
    create_rpass(device, SW_FORMAT, &rpass);

    // layout
    VkPipelineLayout layout;
    create_layout(device, &layout);

    // pipeline!
    create_pipel(
        device,
        2,
        shtages,
        layout,
        0,
        NULL,
        0,
        NULL,
        rpass,
        &pipel
    );

    ck_assert(pipel != NULL);
    vkDestroyPipeline(device, pipel, NULL);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_pipe_suite(void) {
    Suite *s;

    s = suite_create("Vulkan Pipeline Creation");

    TCase *tc1 = tcase_create("Read binary file");
    tcase_add_test(tc1, ut_read_bin);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create shader module");
    tcase_add_test(tc2, ut_create_shmod);
    suite_add_tcase(s, tc2);

    TCase *tc3 = tcase_create("Create pipeline layout");
    tcase_add_test(tc3, ut_create_layout);
    suite_add_tcase(s, tc3);

    TCase *tc4 = tcase_create("Create render pass");
    tcase_add_test(tc4, ut_create_rpass);
    suite_add_tcase(s, tc4);

    TCase *tc5 = tcase_create("Create shader stage");
    tcase_add_test(tc5, ut_create_shtage);
    suite_add_tcase(s, tc5);

    TCase *tc6 = tcase_create("Create pipeline");
    tcase_add_test(tc6, ut_create_pipel);
    suite_add_tcase(s, tc6);

    return s;
}

