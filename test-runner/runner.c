#include "../tests-src/vk_init.h"
#include "../tests-src/vk_window.h"
#include "../tests-src/vk_pipe.h"
#include "../tests-src/vk_cbuf.h"
#include "../tests-src/vk_sync.h"
#include "../tests-src/vk_vertex.h"
#include "../tests-src/vk_buffer.h"
#include "../tests-src/vk_uniform.h"
#include "../tests-src/vk_image.h"

#include "../tests-src/obj.h"
#include "../tests-src/camera.h"
#include "../tests-src/fullstack.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int suite_count = 12;
    Suite **suites = malloc(sizeof(suites[0]) * suite_count);

    int suite_idx = 0;

    // Create suites
    suites[suite_idx++] = vk_init_suite();
    suites[suite_idx++] = vk_window_suite();
    suites[suite_idx++] = vk_pipe_suite();
    suites[suite_idx++] = vk_cbuf_suite();
    suites[suite_idx++] = vk_sync_suite();
    suites[suite_idx++] = vk_vertex_suite();
    suites[suite_idx++] = vk_buffer_suite();
    suites[suite_idx++] = vk_uniform_suite();
    suites[suite_idx++] = vk_camera_suite();
    suites[suite_idx++] = vk_obj_suite();
    suites[suite_idx++] = vk_image_suite();
    suites[suite_idx++] = vk_fullstack_suite();

    // If we got a command-line argument, only run that suite
    if (argc == 2) {
        int selected_idx = atoi(argv[1]);
        SRunner *runner = srunner_create(suites[selected_idx]);
        srunner_run_all(runner, CK_NORMAL);
        srunner_free(runner);

        return 0;
    } else if (argc > 2) {
        printf("Usage: runner [suite]\n");
        exit(1);
    }

    // Otherwise, run all tests
    for (int i = 0; i < suite_idx; i++) {
        SRunner *runner = srunner_create(suites[i]);
        
        printf("--------------------------------------------------------------------------------\n");
        srunner_run_all(runner, CK_NORMAL);
        
        srunner_free(runner);
    }
}
