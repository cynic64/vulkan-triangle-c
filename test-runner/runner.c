#include "../tests-src/vkinit.h"
#include "../tests-src/vk_swapchain.h"

#include <stdlib.h>
#include <stdio.h>

int main() {
    Suite *s1, *s2;
    SRunner *sr1, *sr2;

    s1 = vkinit_suite();
    sr1 = srunner_create(s1);

    s2 = vk_swapchain_suite();
    sr2 = srunner_create(s2);

    srunner_run_all(sr1, CK_NORMAL);
    srunner_run_all(sr2, CK_NORMAL);

    srunner_free(sr1);
    srunner_free(sr2);
}
