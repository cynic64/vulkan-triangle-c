#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_vertex.h"
#include "../src/obj.h"

#include "helpers.h"

/* Returns 1 if [a] and [b] match, 0 otherwise. */
int is_match(float a[3], float b[3]) {
    if (a[0] == b[0]
        && a[1] == b[1]
        && a[2] == b[2]) return 1;

    printf("Do not match [x1 y1 z1] [x2 y2 z2]: [%f %f %f] [%f %f %f]\n",
        a[0], a[1], a[2], b[0], b[1], b[2]);
 
   return 0;
}

START_TEST (ut_load_triangle) {
    // triangle.obj has just 3 vertices at:
    // (-1 0 1), (-1 0 -1), (1 0 -1)
    // Indices: 1, 2, 0
    FILE *fp = fopen("assets/models/triangle.obj", "r");
    ck_assert(fp != NULL);

    size_t vertex_ct = 0;
    size_t index_ct = 0;

    // Query how much memory will be needed to store vertices and indices
    load_obj(fp, &vertex_ct, &index_ct, NULL, NULL);

    ck_assert(vertex_ct == 3);
    ck_assert(index_ct == 3);

    // Load
    struct Vertex3PosNormal *vertices = malloc(sizeof(vertices[0]) * vertex_ct);
    uint32_t *indices = malloc(sizeof(indices[1]) * index_ct);

    load_obj(fp, &vertex_ct, &index_ct, vertices, indices);

    // Check
    float v1[] = {-1.0f, 0.0f, 1.0f};
    float v2[] = {-1.0f, 0.0f, -1.0f};
    float v3[] = {1.0f, 0.0f, -1.0f};
    ck_assert(is_match(vertices[0].pos, v1));
    ck_assert(is_match(vertices[1].pos, v2));
    ck_assert(is_match(vertices[2].pos, v3));

    float n[] = {0.0f, -1.0f, 0.0f};
    size_t n_sz = sizeof(n);
    ck_assert(memcmp(vertices[0].normal, n, n_sz) == 0);
    ck_assert(memcmp(vertices[1].normal, n, n_sz) == 0);
    ck_assert(memcmp(vertices[2].normal, n, n_sz) == 0);

    uint32_t true_indices[] = {1, 2, 0};
    ck_assert(memcmp(indices, true_indices, sizeof(true_indices)) == 0);
} END_TEST

Suite *vk_obj_suite(void) {
    Suite *s;

    s = suite_create("OBJ Loading");

    TCase *tc1 = tcase_create("Load Triangle");
    tcase_add_test(tc1, ut_load_triangle);
    suite_add_tcase(s, tc1);

    return s;
}
