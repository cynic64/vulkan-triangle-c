#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "helpers.h"

START_TEST (ut_create_instance_before_glfw)
{
	// create_instance should work without GLFW, so test that
	VK_OBJECTS;

	create_instance(default_debug_callback,  &dbg_msg_ct, &instance);

	ck_assert(instance != NULL);
	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_instance)
{
	VK_OBJECTS;

	init_glfw();
	create_instance(default_debug_callback, &dbg_msg_ct, &instance);

	ck_assert(dbg_msg_ct == 0);
}

START_TEST (ut_check_exts)
{
	int res;

	char *bad_exts[] = {
		"VK_KHR_surface",
		"VK_KHR_xcb_surface",
		"potato"
	};
	res = check_exts(3, bad_exts);
	ck_assert(res == -1);

	char *good_exts[] = {
		"VK_KHR_surface",
		"VK_KHR_xcb_surface",
	};
	res = check_exts(2, good_exts);
	ck_assert(res == 0);
}

START_TEST (ut_check_layers)
{
	int res;

	const char * const bad_layers[] = {
		"VK_LAYER_KHRONOS_validation",
		"potato"
	};
	uint32_t bad_layer_ct = sizeof(bad_layers) / sizeof(bad_layers[0]);
	res = check_layers(bad_layer_ct, bad_layers);
	ck_assert(res == -1);

	const char * const good_layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	uint32_t good_layer_ct = sizeof(bad_layers) / sizeof(bad_layers[0]);
	res = check_layers(good_layer_ct, good_layers);
	ck_assert(res == 0);
}

START_TEST (ut_get_physical_device)
{
	// get physical device
	VK_OBJECTS;

	helper_get_phys_dev(NULL, &dbg_msg_ct, NULL, &instance, &phys_dev);
	ck_assert(phys_dev != NULL);

	// test
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(phys_dev, &props);

	int res = strcmp(props.deviceName, "GeForce GTX 1060");
	ck_assert(res == 0);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_get_queue_fam)
{
	// get queue family
	VK_OBJECTS;

	helper_get_queue_fam(NULL, &dbg_msg_ct, NULL, &instance, &phys_dev, &queue_fam);

	ck_assert(queue_fam != 999);

	// make sure that family has graphics capabilities
	uint32_t queue_fam_ct;
	vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_fam_ct, NULL);
	VkQueueFamilyProperties *queue_fam_props =
		malloc(sizeof(VkQueueFamilyProperties) * queue_fam_ct);
	vkGetPhysicalDeviceQueueFamilyProperties(
		phys_dev, &queue_fam_ct, queue_fam_props);

	VkQueueFlagBits queue_fam_flags = queue_fam_props[queue_fam].queueFlags;
	ck_assert(VK_QUEUE_GRAPHICS_BIT & queue_fam_flags);
	ck_assert(VK_QUEUE_TRANSFER_BIT & queue_fam_flags);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_check_dev_exts)
{
	// create physical device
	VK_OBJECTS;

	helper_get_phys_dev(NULL, &dbg_msg_ct, NULL, &instance, &phys_dev);

	// test
	int res = 0;

	char *bad_exts[] = {
		"VK_LAYER_KHRONOS_validation",
		"VK_LAYER_VALVE_steam_fossilize_64",
		"potato"
	};
	res = check_dev_exts(phys_dev, 3, bad_exts);
	ck_assert(res == -1);

	char *good_exts[] = {
		"VK_KHR_swapchain",
		"VK_KHR_multiview"
	};
	res = check_dev_exts(phys_dev, 2, good_exts);
	ck_assert(res == 0);
}

START_TEST (ut_create_device)
{
	// create device
	VK_OBJECTS;

	helper_create_device(
		NULL,
		&dbg_msg_ct,
		NULL,
		&instance,
		&phys_dev,
		&queue_fam,
		&device
		);

	ck_assert(device != NULL);

	// make sure it's usable by trying to create a buffer with it
	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = 1;
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VkResult res = vkCreateBuffer(device, &buffer_info, NULL, &buffer);
	ck_assert(res == VK_SUCCESS);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_get_queue)
{
	// create queue
	VK_OBJECTS;

	helper_get_queue(
		NULL,
		&dbg_msg_ct,
		NULL,
		&instance,
		&phys_dev,
		&queue_fam,
		&device,
		&queue
		);

	ck_assert(queue != NULL);

	// make sure it's usable by trying to wait for it to become idle
	VkResult res = vkQueueWaitIdle(queue);
	ck_assert(res == VK_SUCCESS);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST(ut_init_debug)
{
	// create device
	int dbg_msg_ct = 0;
	VkInstance instance;
	VkPhysicalDevice phys_dev;
	uint32_t queue_fam;
	VkDevice device;

	helper_create_device(
		NULL,
		&dbg_msg_ct,
		NULL,
		&instance,
		&phys_dev,
		&queue_fam,
		&device
		);

	// test by trying to create a 0-size buffer and making sure we receive
	// messages
	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = 0;
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VkResult res = vkCreateBuffer(device, &buffer_info, NULL, &buffer);

	ck_assert(dbg_msg_ct == 1);
} END_TEST

START_TEST (ut_destroy_dbg_msgr)
{
	// Create instance and debug messenger
	int dbg_msg_ct = 0;
	VkInstance instance;
	VkDebugUtilsMessengerEXT dbg_msgr;

	helper_create_instance(NULL, &dbg_msg_ct, &dbg_msgr, &instance);

	// Destroy debug messenger
	destroy_dbg_msgr(instance, &dbg_msgr);

	// There should now be no validation layers complaining that the debug
	// messenger wasn't destroyed before the instance
	vkDestroyInstance(instance, NULL);
	ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_init_suite(void)
{
	Suite *s;

	s = suite_create("Vulkan Initialization");

	TCase *tc1 = tcase_create("Create instance before GLFW init");
	tcase_add_test(tc1, ut_create_instance_before_glfw);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Create instance");
	tcase_add_test(tc2, ut_create_instance);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Check extensions");
	tcase_add_test(tc3, ut_check_exts);
	suite_add_tcase(s, tc3);

	TCase *tc4 = tcase_create("Check layers");
	tcase_add_test(tc4, ut_check_layers);
	suite_add_tcase(s, tc4);

	TCase *tc5 = tcase_create("Get physical device");
	tcase_add_test(tc5, ut_get_physical_device);
	suite_add_tcase(s, tc5);

	TCase *tc6 = tcase_create("Get queue family");
	tcase_add_test(tc6, ut_get_queue_fam);
	suite_add_tcase(s, tc6);

	TCase *tc7 = tcase_create("Create device");
	tcase_add_test(tc7, ut_create_device);
	suite_add_tcase(s, tc7);

	TCase *tc8 = tcase_create("Get queue");
	tcase_add_test(tc8, ut_get_queue);
	suite_add_tcase(s, tc8);

	TCase *tc10 = tcase_create("Validation layers");
	tcase_add_test(tc10, ut_init_debug);
	suite_add_tcase(s, tc10);

	TCase *tc11 = tcase_create("Check device extensions");
	tcase_add_test(tc11, ut_check_dev_exts);
	suite_add_tcase(s, tc11);

	TCase *tc12 = tcase_create("Destroy debug messenger");
	tcase_add_test(tc12, ut_destroy_dbg_msgr);
	suite_add_tcase(s, tc12);

	return s;
}

