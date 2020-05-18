#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"

#include "helpers.h"

START_TEST(ut_fail)
{
	ck_assert(0);
}

Suite *vk_fullstack_suite(void) {
	Suite *s;

	s = suite_create("Full-stack tests");

	TCase *tc1 = tcase_create("Windowless triangle");
	tcase_add_test(tc1, ut_fail);
	suite_add_tcase(s, tc1);

	return s;
}
