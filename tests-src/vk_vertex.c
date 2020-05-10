#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_vertex.h"
#include "../src/vk_pipe.h"

#include "helpers.h"

START_TEST (ut_vertex_types) {
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

    // Try to create a pipeline with different vertex descriptions and see if
    // any validation layers complain.
    // Note that this doesn't really work all that well, and I'd have to really
    // fuck up the definitions for anything to be noticed. So that's nice!
    create_rpass(device, SW_FORMAT, &rpass);

    // Vertex3PosColor
    helper_create_pipel(
        device,
        rpass,
        VERTEX_3_POS_COLOR_BINDING_CT,
        VERTEX_3_POS_COLOR_BINDINGS,
        VERTEX_3_POS_COLOR_ATTRIBUTE_CT,
        VERTEX_3_POS_COLOR_ATTRIBUTES,
        "assets/testing/shaders/simple.vert.spv",
        "assets/testing/shaders/simple.frag.spv",
        &pipe_layout,
        &pipel
    );

    // Vertex2PosColor
    helper_create_pipel(
        device,
        rpass,
        VERTEX_2_POS_COLOR_BINDING_CT,
        VERTEX_2_POS_COLOR_BINDINGS,
        VERTEX_2_POS_COLOR_ATTRIBUTE_CT,
        VERTEX_2_POS_COLOR_ATTRIBUTES,
        "assets/testing/shaders/simple.vert.spv",
        "assets/testing/shaders/simple.frag.spv",
        &pipe_layout,
        &pipel
    );

    ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_vertex_suite(void) {
    Suite *s;

    s = suite_create("Vertex types");

    TCase *tc1 = tcase_create("All vertex types");
    tcase_add_test(tc1, ut_vertex_types);
    suite_add_tcase(s, tc1);

    return s;
}
