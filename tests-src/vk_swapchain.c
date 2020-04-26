#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vktools.h"
#include "../src/glfwtools.h"

START_TEST (ut_setup) {
    // this is not as thoroughly checked as it could be because there are
    // already more comprehensive tests for these functions in vkinit.c

    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);

    ck_assert(device != NULL);
} END_TEST

Suite *vk_swapchain_suite(void) {
    Suite *s;

    s = suite_create("Swapchain Initialization");

    TCase *tc1 = tcase_create("Setup device, queue, etc. for other tests");
    tcase_add_test(tc1, ut_setup);
    suite_add_tcase(s, tc1);

    return s;
}

