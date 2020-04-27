#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vktools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"

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
    int dbg_msg_ct = 0;
    init_glfw();
    VkInstance instance = NULL;
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    VkPhysicalDevice phys_dev = NULL;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device = NULL;
    create_device(&instance, phys_dev, queue_fam, &device);
    init_debug(&instance, default_debug_callback, &dbg_msg_ct);

    FILE *fp = fopen("assets/testing/test.vert.spv", "rb");
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
    init_glfw();
    VkInstance instance = NULL;
    create_instance(&instance, default_debug_callback, NULL);
    VkPhysicalDevice phys_dev = NULL;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device = NULL;
    create_device(&instance, phys_dev, queue_fam, &device);
    init_debug(&instance, default_debug_callback, NULL);
    FILE *fp = fopen("assets/testing/test.vert.spv", "rb");
    ck_assert(fp != NULL);
    size_t buf_size = 0;
    read_bin(fp, &buf_size, NULL);
    char *buf = malloc(buf_size);
    read_bin(fp, &buf_size, buf);
    VkShaderModule shmod = NULL;
    create_shmod(device, buf_size, buf, &shmod);

    VkPipelineShaderStageCreateInfo shtage = {0};
    create_shtage(shmod, VK_SHADER_STAGE_VERTEX_BIT, &shtage);

    ck_assert(shtage.stage == VK_SHADER_STAGE_VERTEX_BIT);

    // it's just a struct, not a real Vulkan object on the GPU, so we can't
    // destroy it and see what happens
} END_TEST

START_TEST (ut_create_layout) {
    int dbg_msg_ct = 0;
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);
    init_debug(&instance, default_debug_callback, &dbg_msg_ct);

    VkPipelineLayout layout = NULL;
    create_layout(device, &layout);

    // again, hard to test
    ck_assert(layout != NULL);

    vkDestroyPipelineLayout(device, layout, NULL);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_rpass) {
    int dbg_msg_ct = 0;
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);
    init_debug(&instance, default_debug_callback, &dbg_msg_ct);

    VkRenderPass rpass = NULL;
    create_rpass(device, SW_FORMAT, &rpass);

    ck_assert(rpass != NULL);

    vkDestroyRenderPass(device, rpass, NULL);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST(ut_create_pipel) {
    int dbg_msg_ct = 0;
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);
    init_debug(&instance, default_debug_callback, &dbg_msg_ct);

    // load shaders
    FILE *fp;
    size_t vs_size, fs_size;
    char *vs_buf, *fs_buf;

    fp = fopen("assets/testing/test.vert.spv", "rb");
    ck_assert(fp != NULL);
    read_bin(fp, &vs_size, NULL);
    vs_buf = malloc(vs_size);
    read_bin(fp, &vs_size, vs_buf);
    fclose(fp);
    VkShaderModule vs_mod;
    create_shmod(device, vs_size, vs_buf, &vs_mod);

    fp = fopen("assets/testing/test.frag.spv", "rb");
    ck_assert(fp != NULL);
    read_bin(fp, &fs_size, NULL);
    fs_buf = malloc(fs_size);
    read_bin(fp, &fs_size, fs_buf);
    fclose(fp);
    VkShaderModule fs_mod;
    create_shmod(device, fs_size, fs_buf, &fs_mod);

    VkPipelineShaderStageCreateInfo vs_stage;
    VkPipelineShaderStageCreateInfo fs_stage;
    create_shtage(vs_mod, VK_SHADER_STAGE_VERTEX_BIT, &vs_stage);
    create_shtage(fs_mod, VK_SHADER_STAGE_FRAGMENT_BIT, &fs_stage);
    VkPipelineShaderStageCreateInfo shtages[] = {vs_stage, fs_stage};

    // render pass
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);

    // layout
    VkPipelineLayout layout;
    create_layout(device, &layout);

    // pipeline!
    VkPipeline pipel = NULL;
    create_pipel(
        device,
        2,
        shtages,
        layout,
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

