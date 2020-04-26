#include "../tests-src/vkinit.h"

#include <stdlib.h>
#include <stdio.h>

int main() {
    Suite *s;
    SRunner *sr;

    s = suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);

    srunner_free(sr);
}
