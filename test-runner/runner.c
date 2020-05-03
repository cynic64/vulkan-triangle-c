#include "../tests-src/vk_init.h"
#include "../tests-src/vk_window.h"
#include "../tests-src/vk_pipe.h"
#include "../tests-src/vk_cbuf.h"
#include "../tests-src/vk_sync.h"
#include "../tests-src/vk_vertex.h"

#include <stdlib.h>
#include <stdio.h>

int main() {
    Suite *s1, *s2, *s3, *s4, *s5, *s6;
    SRunner *sr1, *sr2, *sr3, *sr4, *sr5, *sr6;

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

    srunner_free(sr1);
    srunner_free(sr2);
    srunner_free(sr3);
    srunner_free(sr4);
    srunner_free(sr5);
    srunner_free(sr6);
}
