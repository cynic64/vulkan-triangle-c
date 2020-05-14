#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_vertex.h"
#include "../src/obj.h"
#include "../src/ll_obj.h"

#include "helpers.h"

// Returns 1 if [a] and [b] match, 0 otherwise.
int is_match(float a[3], float b[3])
{
	if (a[0] == b[0]
	    && a[1] == b[1]
	    && a[2] == b[2]) return 1;

	printf("Do not match [x1 y1 z1] [x2 y2 z2]: [%f %f %f] [%f %f %f]\n",
	       a[0], a[1], a[2], b[0], b[1], b[2]);

	return 0;
}

// Returns 0.0 .. 1.0
float frand()
{
	return (float)rand() / (float)RAND_MAX;
}

START_TEST (ut_load_triangle)
{
	/*
	 * triangle.obj has just 3 vertices at:
	 * (-1 0 1), (-1 0 -1), (1 0 -1)
	 * Indices: 1, 2, 0
	 */
	FILE *fp = fopen("assets/models/triangle.obj", "r");
	ck_assert(fp != NULL);

	size_t vertex_ct = 0;
	size_t index_ct = 0;

	// Query how much memory will be needed to store vertices and indices
	obj_load(fp, &vertex_ct, &index_ct, NULL, NULL);

	ck_assert(vertex_ct == 3);
	ck_assert(index_ct == 3);

	// Load
	struct ObjVertex *vertices = malloc(sizeof(vertices[0]) * vertex_ct);
	uint32_t *indices = malloc(sizeof(indices[1]) * index_ct);

	obj_load(fp, &vertex_ct, &index_ct, vertices, indices);

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

START_TEST (ut_convert_single)
{
	struct ObjVertex obj;
	memcpy(obj.pos, (vec3){0.1f, 0.2f, 0.3f}, sizeof(obj.pos));
	memcpy(obj.normal, (vec3){1.0f, 0.9f, 0.8f}, sizeof(obj.normal));

	struct Vertex3PosNormal vtx;
	obj_vertex_to_vertex_3_pos_normal(&vtx, &obj);

	ck_assert(is_match(obj.pos, vtx.pos));
	ck_assert(is_match(obj.normal, vtx.normal));
} END_TEST

START_TEST (ut_convert_multiple)
{
	// Generate a bunch of random vertices
	int VERTEX_CT = 100;
	struct ObjVertex *objs = malloc(sizeof(objs[0]) * VERTEX_CT);
	struct Vertex3PosNormal *vtxs = malloc(sizeof(vtxs[0]) * VERTEX_CT);

	for (int i = 0; i < VERTEX_CT; i++) {
		memcpy(objs[i].pos, (vec3){frand(), frand(), frand()},
		       sizeof(objs[0].pos));
		memcpy(objs[i].normal, (vec3){frand(), frand(), frand()},
		       sizeof(objs[0].normal));
	}

	// Convert
	obj_vertex_to_vertex_3_pos_normal_list(vtxs, objs, VERTEX_CT);

	// Make sure they match
	for (int i = 0; i < VERTEX_CT; i++) {
		ck_assert(is_match(objs[i].pos, vtxs[i].pos));
		ck_assert(is_match(objs[i].normal, vtxs[i].normal));
	}
} END_TEST

/*
 * Helper function to test parse_triplet with different formats.
 *
 * Given a format string and a point, returns 1 if parsing the formatted string
 * matches the original point, 0 otherwise.
 *
 * Format string should have 3 %f's in it.
 */
int test_parse_triplet(float point[3], const char *format)
{
	char string[256];
	float parsed[3];

	sprintf(string, format, point[0], point[1], point[2]);
	parse_triplet(string, parsed);

	return is_match(point, parsed);
} 

START_TEST (ut_parse_triplet)
{
	float point[3] = {0.3, -0.4, 0.5};

	char string[256];
	float parsed[3];

	ck_assert(test_parse_triplet(point, "v %f %f %f") == 1);
	ck_assert(test_parse_triplet(point, "vn %f %f %f") == 1);
	ck_assert(test_parse_triplet(point, "v %f %f %f\n") == 1);
	ck_assert(test_parse_triplet(point, "vn %f %f %f\n") == 1);
} END_TEST

/*
 * Helper function to test parse_face with different formats.
 */
int test_parse_face(size_t pos_idxs[3],
		    size_t normal_idxs[3],
		    const char *format)
{
	char string[256];
	size_t parsed_pos_idxs[3];
	size_t parsed_normal_idxs[3];

	sprintf(string, format, pos_idxs[0], normal_idxs[0],
		pos_idxs[1], normal_idxs[1], pos_idxs[2], normal_idxs[2]);
	parse_face(string, parsed_pos_idxs, parsed_normal_idxs);

	int is_match = pos_idxs[0] == parsed_pos_idxs[0]
		&& pos_idxs[1] == parsed_pos_idxs[1]
		&& pos_idxs[2] == parsed_pos_idxs[2]
		&& normal_idxs[0] == parsed_normal_idxs[0]
		&& normal_idxs[1] == parsed_normal_idxs[1]
		&& normal_idxs[2] == parsed_normal_idxs[2];

	if (!is_match)
		printf("Do not match: [%d %d %d] [%d %d %d] != [%d %d %d] [%d %d %d]\n",
		       pos_idxs[0], pos_idxs[1], pos_idxs[2],
		       normal_idxs[0], normal_idxs[1], normal_idxs[2],
		       parsed_pos_idxs[0], parsed_pos_idxs[1], parsed_pos_idxs[2],
		       parsed_normal_idxs[0], parsed_normal_idxs[1], parsed_normal_idxs[2]);

	return is_match;
}

START_TEST (ut_parse_face)
{
	size_t pos_idxs[3] = {3, 6, 4};
	size_t normal_idxs[3] = {10, 2, 1};

	ck_assert(test_parse_face(pos_idxs, normal_idxs,
				  "f %d/1/%d %d/1/%d %d/1/%d") == 1);
	ck_assert(test_parse_face(pos_idxs, normal_idxs,	
				  "f %d/1/%d %d/1/%d %d/1/%d\n") == 1);
} END_TEST

Suite *vk_obj_suite(void)
{
	Suite *s;

	s = suite_create("OBJ Loading");

	TCase *tc1 = tcase_create("Load triangle");
	tcase_add_test(tc1, ut_load_triangle);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Convert single vertex");
	tcase_add_test(tc2, ut_convert_single);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Convert vertex array");
	tcase_add_test(tc3, ut_convert_multiple);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("(Internals) Parse triplet");
	tcase_add_test(tc4, ut_parse_triplet);
	suite_add_tcase(s, tc4);

	TCase *tc5 = tcase_create("(Internals) Parse face");
	tcase_add_test(tc5, ut_parse_face);
	suite_add_tcase(s, tc5);

	return s;
}
