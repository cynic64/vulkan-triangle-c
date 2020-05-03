#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_sync.h"
#include "../src/vk_cbuf.h"

#include "helpers.h"

START_TEST (ut_create_surface) {
    // create surface
    VK_OBJECTS;

    helper_create_surface(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface
    );

    ck_assert(surface != NULL);

    // make sure the surface is usable
    VkSurfaceCapabilitiesKHR surface_caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_caps);
    ck_assert(surface_caps.minImageCount == 2);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_support) {
    // create surface
    VK_OBJECTS;
    helper_create_surface(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface
    );

    // make sure the physical device and queue family support presentation
    VkBool32 support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, queue_fam, surface, &support);
    ck_assert(support == VK_TRUE);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_swapchain) {
    // create swapchain
    VK_OBJECTS;
    helper_create_surface(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface
    );

    get_dims(phys_dev, surface, &swidth, &sheight);

    VkSwapchainKHR swapchain;
    create_swapchain(
        phys_dev,
        device,
        queue_fam,
        surface,
        &swapchain,
        swidth,
        sheight
    );

    // make sure it worked by getting images
    uint32_t sw_image_ct;
    vkGetSwapchainImagesKHR(device, swapchain, &sw_image_ct, NULL);
    ck_assert(sw_image_ct > 0);
    VkImage *sw_images = malloc(sizeof(VkImage) * sw_image_ct);
    VkResult res =
        vkGetSwapchainImagesKHR(device, swapchain, &sw_image_ct, sw_images);
    ck_assert(res == VK_SUCCESS);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_image_views) {
    // create image views
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    uint32_t sw_image_view_ct = 0;
    create_swapchain_image_views(device, win.swapchain, &sw_image_view_ct, NULL);
    ck_assert(sw_image_view_ct > 0);
    VkImageView *sw_image_views = malloc(sizeof(VkImageView) * sw_image_view_ct);
    create_swapchain_image_views(
        device,
        win.swapchain,
        &sw_image_view_ct,
        sw_image_views
    );

    // make sure each image view is valid by destroying each one and making sure
    // no validation layers complain
    for (int i = 0; i < sw_image_view_ct; i++) {
        vkDestroyImageView(device, sw_image_views[i], NULL);
    }

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_framebuffer) {
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    for (int i = 0; i < win.image_ct; i++) {
        VkFramebuffer fb = NULL;
        create_framebuffer(
            device,
            swidth,
            sheight,
            win.rpass,
            win.views[i],
            &fb
        );

        ck_assert(fb != NULL);

        vkDestroyFramebuffer(device, fb, NULL);
    }

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_get_dims) {
    int dbg_msg_ct = 0;

    uint32_t true_width = 800;
    uint32_t true_height = 800;
    // we can't use init_glfw because we need to disable resizing
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(
        true_width,
        true_height,
        "Vulkan",
        NULL,
        NULL
    );
    VkInstance instance;
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);

    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    uint32_t swidth = 0;
    uint32_t sheight = 0;
    get_dims(phys_dev, surface, &swidth, &sheight);

    ck_assert(swidth == true_width);
    ck_assert(sheight == true_height);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_window_create) {
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    // verify it works by trying to acquire an image manually
    uint32_t image_idx;
    VkSemaphore sem;
    create_sem(device, &sem);
    VkResult res = vkAcquireNextImageKHR(
        win.device,
        win.swapchain,
        UINT64_MAX,
        sem,
        NULL,
        &image_idx
    );

    ck_assert(res == VK_SUCCESS);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_window_recreate) {
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    window_recreate_swapchain(&win, swidth, sheight);

    // verify by trying to acquire an image, again
    uint32_t image_idx;
    VkSemaphore sem;
    create_sem(device, &sem);
    VkResult res = vkAcquireNextImageKHR(
        win.device,
        win.swapchain,
        UINT64_MAX,
        sem,
        NULL,
        &image_idx
    );

    ck_assert(res == VK_SUCCESS);

    ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_window_acquire) {
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        NULL,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    // acquire
    VkFramebuffer fb;
    uint32_t image_idx;
    VkSemaphore sem;
    create_sem(device, &sem);

    window_acquire(&win, sem, &image_idx, &fb);

    // test by trying to use it
    // create pipeline
    helper_create_pipel(
        device,
        win.rpass,
        0,
        NULL,
        0,
        NULL,
        "assets/testing/shaders/empty.vert.spv",
        "assets/testing/shaders/empty.frag.spv",
        &pipel
    );

    // create command buffer
    VkCommandBuffer cbuf;
    VkCommandPool cpool;
    create_cpool(device, queue_fam, &cpool);
    create_cbuf(device, cpool, win.rpass, fb, swidth, sheight, pipel, &cbuf);

    // submit
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = &sem;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cbuf;
    submit_info.signalSemaphoreCount = 0;

    VkResult res = vkQueueSubmit(queue, 1, &submit_info, NULL);
    ck_assert(res == VK_SUCCESS);

    vkQueueWaitIdle(queue);
    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_window_cleanup) {
    VK_OBJECTS;
    helper_window_create(
        &gwin,
        &dbg_msg_ct,
        &dbg_msgr,
        &instance,
        &phys_dev,
        &queue_fam,
        &device,
        &queue,
        &surface,
        &swidth,
        &sheight,
        &win
    );

    window_cleanup(&win);

    vkDestroyRenderPass(device, win.rpass, NULL);

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyDevice(device, NULL);
    destroy_dbg_msgr(instance, &dbg_msgr);
    vkDestroyInstance(instance, NULL);

    ck_assert(dbg_msg_ct == 0);
}

Suite *vk_window_suite(void) {
    Suite *s;

    s = suite_create("Swapchain Initialization");

    TCase *tc2 = tcase_create("Create surface");
    tcase_add_test(tc2, ut_create_surface);
    suite_add_tcase(s, tc2);

    TCase *tc3 = tcase_create("Physical device/queue family support presentation");
    tcase_add_test(tc3, ut_support);
    suite_add_tcase(s, tc3);

    TCase *tc4 = tcase_create("Create swapchain");
    tcase_add_test(tc4, ut_create_swapchain);
    suite_add_tcase(s, tc4);

    TCase *tc5 = tcase_create("Create image views");
    tcase_add_test(tc5, ut_create_image_views);
    suite_add_tcase(s, tc5);

    TCase *tc6 = tcase_create("Create framebuffer");
    tcase_add_test(tc6, ut_create_framebuffer);
    suite_add_tcase(s, tc6);

    TCase *tc7 = tcase_create("Get dimensions");
    tcase_add_test(tc7, ut_get_dims);
    suite_add_tcase(s, tc7);

    TCase *tc8 = tcase_create("Window: Create");
    tcase_add_test(tc8, ut_window_create);
    suite_add_tcase(s, tc8);

    TCase *tc9 = tcase_create("Window: Recreate swapchain");
    tcase_add_test(tc9, ut_window_recreate);
    suite_add_tcase(s, tc9);

    TCase *tc10 = tcase_create("Window: Acquire image");
    tcase_add_test(tc10, ut_window_acquire);
    suite_add_tcase(s, tc10);

    TCase *tc11 = tcase_create("Window: Cleanup");
    tcase_add_test(tc11, ut_window_cleanup);
    suite_add_tcase(s, tc11);

    return s;
}
