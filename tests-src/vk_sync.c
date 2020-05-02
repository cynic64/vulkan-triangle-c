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

Suite *vk_sync_suite(void) {
    Suite *s;

    s = suite_create("Synchronization");

    TCase *tc1 = tcase_create("Create semaphore");
    tcase_add_test(tc1, ut_create_sem);
    suite_add_tcase(s, tc1);

    return s;
}
