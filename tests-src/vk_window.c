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
    create_instance(instance, default_debug_callback);
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
    printf("Min images: %u\n", surface_caps.minImageCount);
}

Suite *vk_window_suite(void) {
    Suite *s;

    s = suite_create("Swapchain Initialization");

    TCase *tc1 = tcase_create("Setup device, queue, etc. for other tests");
    tcase_add_test(tc1, ut_setup);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create surface");
    tcase_add_test(tc2, ut_create_surface);
    suite_add_tcase(s, tc2);

    return s;
}

