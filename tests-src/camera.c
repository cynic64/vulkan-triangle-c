#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <check.h>

#include <cglm/cglm.h>

#include "../src/camera.h"

void print_mat4(mat4 m)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%.2f ", m[i][j]);
		}

		printf("\n");
	}
}

// Returns 1 if the sum of the given matrix's elements is over a certain
// threshold
int is_nonzero(mat4 m)
{
	float thresh = 0.1;

	float sum = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			sum += m[i][j];
		}
	}

	if (sum > thresh) return 1;
	return 0;
}

START_TEST (ut_looker)
{
	vec3 eye = {1.0f, 1.0f, 1.0f};
	vec3 center = {0.0f, 0.0f, 0.0f};
	mat4 view;
	cam_looker(eye, center, view);

	ck_assert(is_nonzero(view));

	// if we multiple the eye with the view, we should end up with something
	// right in the center of the screen
	vec4 point = {eye[0], eye[1], eye[2], 1.0f};
	vec4 dest;
	glm_mat4_mulv(view, point, dest);

	ck_assert(dest[0] == 0.0);
	ck_assert(dest[1] == 0.0);
	ck_assert(dest[2] == 0.0);
	ck_assert(dest[3] == 1.0);
} END_TEST

START_TEST (ut_projector)
{
	uint32_t swidth = 1920;
	uint32_t sheight = 1080;

	mat4 proj = {0};
	cam_projector(swidth, sheight, proj);

	// idk how to do it better :p
	ck_assert(is_nonzero(proj));
} END_TEST

START_TEST(ut_tracker)
{
	struct MouseTracker tracker = {0.0f, 0.0f};

	uint32_t swidth = 1920;
	uint32_t sheight = 1080;
	double x = 0.0f;
	double y = 0.0f;

	cam_mouse_diff(
		&tracker,
		(double) swidth * 0.5, (double) sheight * 0.75,
		&x, &y
		);

	ck_assert(x > 0.0f);
	ck_assert(y > 0.0f);
} END_TEST

START_TEST(ut_orbit)
{
	struct OrbitCamera cam = cam_orbit_new(0.0f, 0.0f);

	mat4 view_proj = {0};
	cam_orbit_mat(&cam, 1920, 1080, 0.0f, 0.0f, view_proj);

	// again... idk
	ck_assert(is_nonzero(view_proj));
} END_TEST

START_TEST(ut_dir_vec)
{
	float yaw = M_PI * 0.25f;
	float pitch = M_PI * 0.25f;

	vec3 dir = {0};
	cam_get_dir_vec(yaw, pitch, dir);

	float mag = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
	ck_assert(mag > 0.99 && mag < 1.01);
}

Suite *vk_camera_suite(void)
{
	Suite *s;

	s = suite_create("Camera");

	TCase *tc1 = tcase_create("Looker");
	tcase_add_test(tc1, ut_looker);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Projector");
	tcase_add_test(tc2, ut_projector);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Tracker");
	tcase_add_test(tc3, ut_tracker);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("Orbit Camera");
	tcase_add_test(tc4, ut_orbit);
	suite_add_tcase(s, tc4);

	TCase *tc5 = tcase_create("Get direction vector");
	tcase_add_test(tc5, ut_dir_vec);
	suite_add_tcase(s, tc5);

	return s;
}
