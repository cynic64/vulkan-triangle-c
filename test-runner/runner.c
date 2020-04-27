#include "../tests-src/vkinit.h"
#include "../tests-src/vk_window.h"
#include "../tests-src/vk_pipe.h"

#include <stdlib.h>
#include <stdio.h>

int main() {
    Suite *s1, *s2, *s3;
    SRunner *sr1, *sr2, *sr3;

    s1 = vkinit_suite();
    sr1 = srunner_create(s1);

    s2 = vk_window_suite();
    sr2 = srunner_create(s2);

    s3 = vk_pipe_suite();
    sr3 = srunner_create(s3);

    srunner_run_all(sr1, CK_NORMAL);
    srunner_run_all(sr2, CK_NORMAL);
    srunner_run_all(sr3, CK_NORMAL);

    srunner_free(sr1);
    srunner_free(sr2);
    srunner_free(sr3);
}
