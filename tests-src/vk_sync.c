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

#include "helpers.h"

START_TEST (ut_create_sem) {
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

    VkSemaphore sem = NULL;
    create_sem(device, &sem);
    ck_assert(sem != NULL);

    vkDestroySemaphore(device, sem, NULL);
    ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_create_fence) {
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

    VkFence fence = NULL;
    create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT, &fence);
    ck_assert(fence != NULL);

    // try waiting on it
    VkResult res = vkWaitForFences(device, 1, &fence, VK_TRUE, 100000);
    ck_assert(res == VK_SUCCESS);

    vkDestroyFence(device, fence, NULL);
    ck_assert(dbg_msg_ct == 0);
}

Suite *vk_sync_suite(void) {
    Suite *s;

    s = suite_create("Synchronization");

    TCase *tc1 = tcase_create("Create semaphore");
    tcase_add_test(tc1, ut_create_sem);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("Create fence");
    tcase_add_test(tc2, ut_create_fence);
    suite_add_tcase(s, tc2);

    return s;
}
