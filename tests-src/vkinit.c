#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vktools.h"
#include "../src/glfwtools.h"

START_TEST (ut_create_instance_before_glfw) {
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
} END_TEST

START_TEST (ut_create_instance) {
    init_glfw();

    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
    ck_assert(instance != NULL);

    // make sure the instance is actually usable too
    uint32_t api_version;
    VkResult res = vkEnumerateInstanceVersion(&api_version);
    ck_assert(res == VK_SUCCESS);
    ck_assert(api_version > VK_API_VERSION_1_0);
    ck_assert(api_version < VK_MAKE_VERSION(1, 3, 0));
}

START_TEST (ut_check_exts) {
    int res;

    char *bad_exts[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
        "potato"
    };
    res = check_exts(3, bad_exts);
    ck_assert(res == -1);

    char *good_exts[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
    };
    res = check_exts(2, good_exts);
    ck_assert(res == 0);
}

START_TEST (ut_check_layers) {
    int res;

    char *bad_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_VALVE_steam_fossilize_64",
        "potato"
    };
    res = check_layers(3, bad_layers);
    ck_assert(res == -1);

    char *good_layers[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_VALVE_steam_fossilize_64"
    };
    res = check_layers(2, good_layers);
    ck_assert(res == 0);
}

START_TEST (ut_get_physical_device) {
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);

    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    ck_assert(phys_dev != NULL);

    // make sure it's usable
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_dev, &props);

    int res = strcmp(props.deviceName, "GeForce GTX 1060");
    ck_assert(res == 0);
} END_TEST

START_TEST (ut_get_queue_fam) {
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);

    uint32_t queue_fam = get_queue_fam(phys_dev);

    // make sure that family has graphics capabilities
    uint32_t queue_fam_ct;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_fam_ct, NULL);
    VkQueueFamilyProperties *queue_fam_props =
        malloc(sizeof(VkQueueFamilyProperties) * queue_fam_ct);
    vkGetPhysicalDeviceQueueFamilyProperties(
        phys_dev, &queue_fam_ct, queue_fam_props);

    VkQueueFlagBits queue_fam_flags = queue_fam_props[queue_fam].queueFlags;
    ck_assert(VK_QUEUE_GRAPHICS_BIT & queue_fam_flags);
} END_TEST

START_TEST (ut_check_dev_exts) {
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);

    int res;

    char *bad_exts[] = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_VALVE_steam_fossilize_64",
        "potato"
    };
    res = check_dev_exts(phys_dev, 3, bad_exts);
    ck_assert(res == -1);

    char *good_exts[] = {
        "VK_KHR_swapchain",
        "VK_KHR_multiview"
    };
    res = check_dev_exts(phys_dev, 2, good_exts);
    ck_assert(res == 0);
}

START_TEST (ut_create_device) {
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);

    VkDevice device = NULL;
    create_device(&instance, phys_dev, queue_fam, &device);
    ck_assert(device != NULL);

    // make sure it's usable by trying to create a buffer with it
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = 1;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    VkResult res = vkCreateBuffer(device, &buffer_info, NULL, &buffer);
    ck_assert(res == VK_SUCCESS);
} END_TEST

START_TEST (ut_get_queue) {
    init_glfw();
    VkInstance instance;
    create_instance(&instance, default_debug_callback, NULL);
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);

    VkQueue queue = NULL;
    get_queue(device, queue_fam, &queue);
    ck_assert(queue != NULL);

    // make sure it's usable by trying to wait for it to become idle
    VkResult res = vkQueueWaitIdle(queue);
    ck_assert(res == VK_SUCCESS);
} END_TEST

START_TEST (ut_heap_2D) {
    char **strings;
    heap_2D(&strings, 8, 4);

    for (int i = 0; i < 8; i++) {
        strcpy(strings[i], "abc");
    }
} END_TEST

START_TEST(ut_init_debug) {
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

    // test by trying to create a 0-size buffer and making sure we receive
    // messages
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = 0;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    VkResult res = vkCreateBuffer(device, &buffer_info, NULL, &buffer);

    ck_assert(dbg_msg_ct == 1);
}

Suite *vkinit_suite(void) {
    Suite *s;

    s = suite_create("Vulkan Initialization");

    TCase *tc1 = tcase_create("Create instance before GLFW init");
    tcase_add_test_raise_signal(tc1, ut_create_instance_before_glfw, 6);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create instance");
    tcase_add_test(tc2, ut_create_instance);
    suite_add_tcase(s, tc2);

    TCase *tc3 = tcase_create("Check extensions");
    tcase_add_test(tc3, ut_check_exts);
    suite_add_tcase(s, tc3);

    TCase *tc4 = tcase_create("Check layers");
    tcase_add_test(tc4, ut_check_layers);
    suite_add_tcase(s, tc4);

    TCase *tc5 = tcase_create("Get physical device");
    tcase_add_test(tc5, ut_get_physical_device);
    suite_add_tcase(s, tc5);

    TCase *tc6 = tcase_create("Get queue family");
    tcase_add_test(tc6, ut_get_queue_fam);
    suite_add_tcase(s, tc6);

    TCase *tc7 = tcase_create("Create device");
    tcase_add_test(tc7, ut_create_device);
    suite_add_tcase(s, tc7);

    TCase *tc8 = tcase_create("Get queue");
    tcase_add_test(tc8, ut_get_queue);
    suite_add_tcase(s, tc8);

    TCase *tc9 = tcase_create("2D heap allocation");
    tcase_add_test(tc9, ut_heap_2D);
    suite_add_tcase(s, tc9);

    TCase *tc10 = tcase_create("Validation layers");
    tcase_add_test(tc10, ut_init_debug);
    suite_add_tcase(s, tc10);

    TCase *tc11 = tcase_create("Check device extensions");
    tcase_add_test(tc11, ut_check_dev_exts);
    suite_add_tcase(s, tc11);

    return s;
}

