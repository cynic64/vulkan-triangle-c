#include "../tests-src/vk_init.h"
#include "../tests-src/vk_window.h"
#include "../tests-src/vk_pipe.h"
#include "../tests-src/vk_cbuf.h"
#include "../tests-src/vk_sync.h"
#include "../tests-src/vk_vertex.h"
#include "../tests-src/vk_buffer.h"
#include "../tests-src/vk_uniform.h"

#include "../tests-src/camera.h"

#include <stdlib.h>
#include <stdio.h>

int main() {
    Suite *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
    SRunner *sr1, *sr2, *sr3, *sr4, *sr5, *sr6, *sr7, *sr8, *sr9;

    s1 = vk_init_suite();
    sr1 = srunner_create(s1);

    s2 = vk_window_suite();
    sr2 = srunner_create(s2);

    s3 = vk_pipe_suite();
    sr3 = srunner_create(s3);

    s4 = vk_cbuf_suite();
    sr4 = srunner_create(s4);

    s5 = vk_sync_suite();
    sr5 = srunner_create(s5);

    s6 = vk_vertex_suite();
    sr6 = srunner_create(s6);

    s7 = vk_buffer_suite();
    sr7 = srunner_create(s7);

    s8 = vk_uniform_suite();
    sr8 = srunner_create(s8);

    s9 = vk_camera_suite();
    sr9 = srunner_create(s9);

    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr1, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr2, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr3, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr4, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr5, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr6, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr7, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr8, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");
    srunner_run_all(sr9, CK_NORMAL);
    printf("--------------------------------------------------------------------------------\n");

    srunner_free(sr1);
    srunner_free(sr2);
    srunner_free(sr3);
    srunner_free(sr4);
    srunner_free(sr5);
    srunner_free(sr6);
    srunner_free(sr7);
    srunner_free(sr8);
    srunner_free(sr9);
}
