#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vktools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"

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
    // already more comprehensive tests for these functions in vkinit.c
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

    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    // make sure the surface is usable
    VkSurfaceCapabilitiesKHR surface_caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_caps);
    ck_assert(surface_caps.minImageCount == 2);
} END_TEST

START_TEST (ut_support) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    // make sure the physical device and queue family support presentation
    VkBool32 support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, queue_fam, surface, &support);
    ck_assert(support == VK_TRUE);
} END_TEST

START_TEST (ut_create_swapchain) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    uint32_t queue_fam;
    VkDevice device;
    VkQueue queue;
    setup(&window, &instance, &phys_dev, &queue_fam, &device, &queue);
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);

    VkSwapchainKHR swapchain;
    create_swapchain(
        phys_dev,
        device,
        queue_fam,
        surface,
        &swapchain,
        WIDTH,
        HEIGHT
    );

    // make sure it worked by getting images
    uint32_t sw_image_ct;
    vkGetSwapchainImagesKHR(device, swapchain, &sw_image_ct, NULL);
    ck_assert(sw_image_ct > 0);
    VkImage *sw_images = malloc(sizeof(VkImage) * sw_image_ct);
    VkResult res =
        vkGetSwapchainImagesKHR(device, swapchain, &sw_image_ct, sw_images);
    ck_assert(res == VK_SUCCESS);
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
    VkSwapchainKHR swapchain;
    create_swapchain(phys_dev, device, queue_fam, surface, &swapchain, WIDTH, HEIGHT);

    int dbg_msg_ct = 0;
    init_debug(&instance, default_debug_callback, &dbg_msg_ct);

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

    return s;
}

