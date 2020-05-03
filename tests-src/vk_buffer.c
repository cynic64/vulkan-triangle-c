#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_buffer.h"

#include "helpers.h"

START_TEST (ut_dummy) {
    ck_assert(buffer_dummy() == 1);
}

Suite *vk_buffer_suite(void) {
    Suite *s;

    s = suite_create("Buffers");

    TCase *tc1 = tcase_create("Dummy");
    tcase_add_test(tc1, ut_dummy);
    suite_add_tcase(s, tc1);

    return s;
}
