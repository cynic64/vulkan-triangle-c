#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_vertex.h"

#include "helpers.h"

START_TEST (ut_vertex2_pos_color) {
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

    // try to create a pipeline with the Vertex2PosColor descriptions, and make
    // sure no validation layers complain
    create_rpass(device, SW_FORMAT, &rpass);

    helper_create_pipel(
        device,
        rpass,
        VERTEX_2_POS_COLOR_BINDING_CT,
        VERTEX_2_POS_COLOR_BINDINGS,
        VERTEX_2_POS_COLOR_ATTRIBUTE_CT,
        VERTEX_2_POS_COLOR_ATTRIBUTES,
        "assets/testing/shaders/simple.vert.spv",
        "assets/testing/shaders/simple.frag.spv",
        &pipel
    );

    ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_vertex_suite(void) {
    Suite *s;

    s = suite_create("Vertex types");

    TCase *tc1 = tcase_create("Vertex2PosColor");
    tcase_add_test(tc1, ut_vertex2_pos_color);
    suite_add_tcase(s, tc1);

    return s;
}
