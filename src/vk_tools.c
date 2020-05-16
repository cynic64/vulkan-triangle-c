#include "vk_tools.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_EXTENSION_NAME_LEN 256

void get_queue(VkDevice device, uint32_t queue_fam, VkQueue *queue)
{
	vkGetDeviceQueue(device, queue_fam, 0, queue);
}

void create_device(VkPhysicalDevice phys_dev, uint32_t queue_fam, VkDevice *device)
{
	// Make sure swapchain extension is available
	char *exts[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	uint32_t ext_ct = 1;
	assert(check_dev_exts(phys_dev, ext_ct, exts) == 0);

	// VkDeviceQueueCreateInfo
	VkDeviceQueueCreateInfo queue_info = {0};
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = queue_fam;
	queue_info.queueCount = 1;
	float queue_priority = 1.0f;
	queue_info.pQueuePriorities = &queue_priority;

	// VkPhysicalDeviceFeatures
	VkPhysicalDeviceFeatures dev_features = {0};

	// VkDeviceCreateInfo
	VkDeviceCreateInfo device_info = {0};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.pEnabledFeatures = &dev_features;
	device_info.enabledExtensionCount = ext_ct;
	device_info.ppEnabledExtensionNames = (const char * const *) exts;
	device_info.enabledLayerCount = 0;

	VkResult res = vkCreateDevice(phys_dev, &device_info, NULL, device);
	assert(res == VK_SUCCESS);
}

uint32_t get_queue_fam(VkPhysicalDevice phys_dev)
{
	uint32_t queue_fam_ct;
	vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_fam_ct, NULL);

	VkQueueFamilyProperties *queue_fam_props =
		malloc(sizeof(VkQueueFamilyProperties) * queue_fam_ct);
	vkGetPhysicalDeviceQueueFamilyProperties(phys_dev,
						 &queue_fam_ct,
						 queue_fam_props);

	for (int i = 0; i < queue_fam_ct; i++) {
		if (queue_fam_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			free(queue_fam_props);
			return i;
		}
	}

	free(queue_fam_props);
	exit(1);
}

void get_physical_device(VkInstance instance, VkPhysicalDevice *phys_dev)
{
	uint32_t phys_dev_ct;
	vkEnumeratePhysicalDevices(instance, &phys_dev_ct, NULL);

	if (phys_dev_ct == 0) {
		printf("No usable devices!\n");
		exit(1);
	}

	VkPhysicalDevice *phys_devs = malloc(sizeof(VkPhysicalDevice) * phys_dev_ct);
	vkEnumeratePhysicalDevices(instance, &phys_dev_ct, phys_devs);
	VkPhysicalDeviceProperties *props =
		malloc(sizeof(VkPhysicalDeviceProperties) * phys_dev_ct);

	for (int i = 0; i < phys_dev_ct; i++) {
		vkGetPhysicalDeviceProperties(phys_devs[i], &props[i]);
	}

	// Use first device
	// printf("Using device: %s\n", props[0].deviceName);
	*phys_dev = phys_devs[0];
	free(phys_devs);
	free(props);
}

void init_debug(VkInstance *instance,
		DebugCallback dbg_cback,
		void *pUserData,
		VkDebugUtilsMessengerEXT *dbg_msgr)
{
	VkDebugUtilsMessengerCreateInfoEXT dbg_info = {0};
	populate_dbg_info(&dbg_info, dbg_cback, pUserData);

	create_dbg_msgr(*instance, &dbg_info, dbg_msgr);
}

void destroy_dbg_msgr(VkInstance instance,
		      VkDebugUtilsMessengerEXT *dbg_msgr)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func =
		(PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
				      "vkDestroyDebugUtilsMessengerEXT");
	assert(func != NULL);

	func(instance, *dbg_msgr, NULL);

	*dbg_msgr = NULL;
}

void create_dbg_msgr(VkInstance instance,
		     VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
		     VkDebugUtilsMessengerEXT *dbg_msgr)
{
	PFN_vkCreateDebugUtilsMessengerEXT func =
		(PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
				      "vkCreateDebugUtilsMessengerEXT");
	assert(func != NULL);

	VkResult res = func(instance, dbg_info, NULL, dbg_msgr);
	assert(res == VK_SUCCESS);
}

void populate_dbg_info(VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
		       DebugCallback dbg_cback,
		       void *pUserData)
{
	dbg_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dbg_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	dbg_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	dbg_info->pfnUserCallback = dbg_cback;
	dbg_info->pUserData = pUserData;
}

void create_instance(DebugCallback dbg_cback,
		     void *user_data,
		     VkInstance *instance)
{
	const char * const exts[] = {"VK_KHR_surface",
				     "VK_KHR_xcb_surface",
				     "VK_EXT_debug_utils"};
	uint32_t ext_ct = ARRAY_SIZE(exts);

	// Ensure all required validation layers exist
	const char * const val_layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	uint32_t val_layer_ct = 1;
	assert(check_layers(val_layer_ct, val_layers) == 0);

	VkApplicationInfo app_info = {0};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Thingy";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Epic Triangle Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_info = {0};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = ext_ct;
	instance_info.ppEnabledExtensionNames = exts;
	instance_info.enabledLayerCount = val_layer_ct;
	instance_info.ppEnabledLayerNames = val_layers;

	// Enable debugging during instance creation/destruction
	VkDebugUtilsMessengerCreateInfoEXT dbg_info = {0};
	populate_dbg_info(&dbg_info, default_debug_callback, user_data);
	instance_info.pNext = &dbg_info;

	// Create
	VkResult result = vkCreateInstance(&instance_info, NULL, instance);
	assert(result == VK_SUCCESS);
}

VKAPI_ATTR VkBool32 VKAPI_CALL
default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		       VkDebugUtilsMessageTypeFlagsEXT messageType,
		       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		       void* pUserData)
{
	printf("Validation layer: %s\n", pCallbackData->pMessage);

	if (pUserData != NULL) (*(int*)pUserData)++;

	return VK_FALSE;
}

int check_layers(uint32_t req_layer_ct, const char * const *req_layers)
{
	uint32_t real_layer_ct;
	vkEnumerateInstanceLayerProperties(&real_layer_ct, NULL);

	VkLayerProperties *real_layers =
		malloc(sizeof(VkLayerProperties) * real_layer_ct);

	vkEnumerateInstanceLayerProperties(&real_layer_ct, real_layers);

	// Ensure all required layers exist
	for (int i = 0; i < req_layer_ct; i++) {
		const char *req_layer_name = req_layers[i];
		int found = 0;

		for (int j = 0; j < real_layer_ct; j++) {
			if (strcmp(real_layers[j].layerName, req_layer_name) == 0) {
				found = 1;
				break;
			}
		}

		if (!found) {
			free(real_layers);
			return -1;
		}
	}

	free(real_layers);
	return 0;
}

int check_exts(uint32_t req_ext_ct, char **req_exts)
{
	// Returns 0 if all extensions were found, -1 otherwise

	uint32_t real_ext_ct;
	vkEnumerateInstanceExtensionProperties(NULL, &real_ext_ct, NULL);

	VkExtensionProperties *real_ext =
		malloc(sizeof(VkExtensionProperties) * real_ext_ct);
	vkEnumerateInstanceExtensionProperties(NULL, &real_ext_ct, real_ext);

	// go through all required extensions and ensure they exist
	for (int i = 0; i < req_ext_ct; i++) {
		const char *req_name = req_exts[i];

		int found = 0;
		for (int j = 0; j < real_ext_ct; j++) {
			if (strcmp(req_name, real_ext[j].extensionName) == 0) {
				found = 1;
				break;
			}
		}

		if (!found) return -1;
	}

	free(real_ext);

	return 0;
}

int check_dev_exts(VkPhysicalDevice phys_dev,
		   uint32_t req_ext_ct,
		   char **req_exts)
{
	// Returns 0 if all extensions were found, -1 otherwise

	uint32_t real_ext_ct;
	vkEnumerateDeviceExtensionProperties(phys_dev, NULL, &real_ext_ct, NULL);

	VkExtensionProperties *real_ext =
		malloc(sizeof(VkExtensionProperties) * real_ext_ct);
	vkEnumerateDeviceExtensionProperties(
		phys_dev, NULL, &real_ext_ct, real_ext);

	// Go through all required extensions and ensure they exist
	for (int i = 0; i < req_ext_ct; i++) {
		const char *req_name = req_exts[i];

		int found = 0;
		for (int j = 0; j < real_ext_ct; j++) {
			if (strcmp(req_name, real_ext[j].extensionName) == 0) {
				found = 1;
				break;
			}
		}

		if (!found) return -1;
	}

	free(real_ext);

	return 0;
}
