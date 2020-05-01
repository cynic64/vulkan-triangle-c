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

static void setup(
    GLFWwindow **window,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam,
    VkDevice *device,
    VkQueue *queue
) {
    *window = init_glfw();
    create_instance(instance, default_debug_callback, NULL);
    get_physical_device(*instance, phys_dev);
    *queue_fam = get_queue_fam(*phys_dev);
    create_device(instance, *phys_dev, *queue_fam, device);
    get_queue(*device, *queue_fam, queue);
}

START_TEST (ut_setup) {
    // this is not as thoroughly checked as it could be because there are
    // already more comprehensive tests for these functions in vk_init.c
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;

    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);

    // try to wait on the queue becoming idle to test that it works
    VkResult res = vkQueueWaitIdle(queue);
    ck_assert(res == VK_SUCCESS);
} END_TEST

START_TEST (ut_create_surface) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);

    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    // make sure the surface is usable
    VkSurfaceCapabilitiesKHR surface_caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_caps);
    ck_assert(surface_caps.minImageCount == 2);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_support) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    // make sure the physical device and queue family support presentation
    VkBool32 support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, queue_fam, surface, &support);
    ck_assert(support == VK_TRUE);

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_swapchain) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    uint32_t swidth, sheight;
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
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);
    VkSwapchainKHR swapchain;
    create_swapchain(phys_dev, device, queue_fam, surface, &swapchain, swidth, sheight);

    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);

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

    // make sure each image view is valid by destroying each one and making sure
    // no validation layers complain
    for (int i = 0; i < sw_image_view_ct; i++) {
        vkDestroyImageView(device, sw_image_views[i], NULL);
    }

    ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_framebuffer) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
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

    for (int i = 0; i < sw_image_view_ct; i++) {
        VkFramebuffer fb = NULL;
        create_framebuffer(
            device,
            swidth,
            sheight,
            rpass,
            sw_image_views[i],
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
    GLFWwindow *gwin;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&gwin, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    VkSurfaceKHR surface;
    create_surface(instance, gwin, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);

    struct Window win = {0};
    window_create(
        gwin,
        phys_dev,
        instance,
        device,
        surface,
        queue_fam,
        queue,
        rpass,
        swidth,
        sheight,
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
    GLFWwindow *gwin;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&gwin, &instance, &phys_dev, &queue_fam, &device, &queue);
    int dbg_msg_ct = 0;
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    VkSurfaceKHR surface;
    create_surface(instance, gwin, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);
    struct Window win = {0};
    window_create(gwin, phys_dev, instance, device, surface, queue_fam, queue, rpass, swidth, sheight, &win);

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
    GLFWwindow *gwin;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    VkDevice device;
    VkQueue queue;
    int dbg_msg_ct = 0;
    gwin = init_glfw();
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
    create_surface(instance, gwin, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);
    struct Window win;
    window_create(gwin, phys_dev, instance, device, surface, queue_fam, queue, rpass, swidth, sheight, &win);

    // acquire
    VkFramebuffer fb;
    uint32_t image_idx;
    VkSemaphore sem;
    create_sem(device, &sem);

    window_acquire(&win, sem, &image_idx, &fb);

    // test by trying to use it
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
    VkPipeline pipel;
    create_pipel(device, 2, shtages, layout, rpass, &pipel);
    VkCommandBuffer cbuf;
    create_cbuf(device, cpool, rpass, fb, swidth, sheight, pipel, &cbuf);
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

Suite *vk_window_suite(void) {
    Suite *s;

    s = suite_create("Swapchain Initialization");

    TCase *tc1 = tcase_create("Setup device, queue, etc. for other tests");
    tcase_add_test(tc1, ut_setup);
    suite_add_tcase(s, tc1);

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

    return s;
}
