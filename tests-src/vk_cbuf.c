#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"

START_TEST (ut_create_cpool) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    VkDevice device;
    VkQueue queue;
    int dbg_msg_ct = 0;
    window = init_glfw();
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    create_device(&instance, phys_dev, queue_fam, &device);
    get_queue(device, queue_fam, &queue);
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);

    VkCommandPool cpool = NULL;
    create_cpool(device, queue_fam, &cpool);
    ck_assert(cpool != NULL);

    // test it by trying to create a command buffer
    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = cpool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VkCommandBuffer cbuf = NULL;
    VkResult res = vkAllocateCommandBuffers(device, &info, &cbuf);
    ck_assert(cbuf != NULL);
    ck_assert(res == VK_SUCCESS);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_cbuf) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    VkDevice device;
    VkQueue queue;
    int dbg_msg_ct = 0;
    window = init_glfw();
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    create_device(&instance, phys_dev, queue_fam, &device);
    get_queue(device, queue_fam, &queue);
    VkCommandPool cpool = NULL;
    create_cpool(device, queue_fam, &cpool);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);
    VkSwapchainKHR swapchain;
    create_swapchain(phys_dev, device, queue_fam, surface, &swapchain, swidth, sheight);
    uint32_t sw_image_view_ct = 0;
    create_swapchain_image_views(device, swapchain, &sw_image_view_ct, NULL);
    ck_assert(sw_image_view_ct > 0);
    VkImageView *sw_image_views = malloc(sizeof(VkImageView) * sw_image_view_ct);
    create_swapchain_image_views(
        device,
        swapchain,
        &sw_image_view_ct,
        sw_image_views
    );
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);
    VkFramebuffer *fbs = malloc(sizeof(VkFramebuffer) * sw_image_view_ct);
    for (int i = 0; i < sw_image_view_ct; i++) {
        VkFramebuffer fb = NULL;
        create_framebuffer(
            device,
            swidth,
            sheight,
            rpass,
            sw_image_views[i],
            &fbs[i]
        );
    }
    // layout
    VkPipelineLayout layout;
    create_layout(device, &layout);
    // pipeline
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
    VkPipeline pipel = NULL;
    create_pipel(
        device,
        2,
        shtages,
        layout,
        rpass,
        &pipel
    );

    VkCommandBuffer cbuf = NULL;
    create_cbuf(
        device,
        cpool,
        rpass,
        fbs[0],
        swidth,
        sheight,
        pipel,
        &cbuf
    );
    ck_assert(cbuf != NULL);

    // try submitting it
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cbuf;
    submit_info.signalSemaphoreCount = 0;

    VkResult res = vkQueueSubmit(queue, 1, &submit_info, NULL);
    ck_assert(res == VK_SUCCESS);

    vkQueueWaitIdle(queue);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_cbuf_suite(void) {
    Suite *s;

    s = suite_create("Command buffer creation");

    TCase *tc1 = tcase_create("Create command pool");
    tcase_add_test(tc1, ut_create_cpool);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create command buffer");
    tcase_add_test(tc2, ut_create_cbuf);
    suite_add_tcase(s, tc2);

    return s;
}
