#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"

START_TEST (ut_create_sem) {
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice phys_dev;
    VkDevice device;
    VkQueue queue;
    VkDebugUtilsMessengerEXT dbg_msgr;
    int dbg_msg_ct = 0;
    window = init_glfw();
    create_instance(&instance, default_debug_callback, &dbg_msg_ct);
    init_debug(&instance, default_debug_callback, &dbg_msg_ct, &dbg_msgr);
    get_physical_device(instance, &phys_dev);
    uint32_t queue_fam = get_queue_fam(phys_dev);
    create_device(&instance, phys_dev, queue_fam, &device);
    get_queue(device, queue_fam, &queue);

    VkSemaphore sem = NULL;
    create_sem(device, &sem);
    ck_assert(sem != NULL);

    vkDestroySemaphore(device, sem, NULL);
    ck_assert(dbg_msg_ct == 0);
}

Suite *vk_sync_suite(void) {
    Suite *s;

    s = suite_create("Synchronization");

    TCase *tc1 = tcase_create("Create semaphore");
    tcase_add_test(tc1, ut_create_sem);
    suite_add_tcase(s, tc1);

    return s;
}
