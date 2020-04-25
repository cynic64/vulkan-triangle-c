#include "vktools.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void get_queue(VkDevice device, uint32_t queue_fam, VkQueue *queue) {
    vkGetDeviceQueue(device, queue_fam, 0, queue);
}

void create_device(VkInstance *instance, VkPhysicalDevice phys_dev, uint32_t queue_fam, VkDevice *device) {
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
    device_info.enabledExtensionCount = 0;
    device_info.enabledLayerCount = 0;

    VkResult res = vkCreateDevice(phys_dev, &device_info, NULL, device);
    assert(res == VK_SUCCESS);
}

uint32_t get_queue_fam(VkPhysicalDevice phys_dev) {
    uint32_t queue_fam_ct;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_fam_ct, NULL);

    VkQueueFamilyProperties *queue_fam_props =
        malloc(sizeof(VkQueueFamilyProperties) * queue_fam_ct);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_fam_ct, queue_fam_props);

    for (int i = 0; i < queue_fam_ct; i++) {
        if (queue_fam_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            printf("Using queue family: %u\n", i);
            free(queue_fam_props);
            return i;
        }
    }

    printf("Could not find a queue family that supports graphics\n");
    free(queue_fam_props);
    exit(1);
}

void get_physical_device(VkInstance instance, VkPhysicalDevice *phys_dev) {
    uint32_t phys_dev_ct;
    vkEnumeratePhysicalDevices(instance, &phys_dev_ct, NULL);

    VkPhysicalDevice *phys_devs = malloc(sizeof(VkPhysicalDevice) * phys_dev_ct);
    vkEnumeratePhysicalDevices(instance, &phys_dev_ct, phys_devs);

    for (int i = 0; i < phys_dev_ct; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(phys_devs[i], &props);
        printf("Found device: %s\n", props.deviceName);
    }

    // use first device
    *phys_dev = phys_devs[0];
    free(phys_devs);
}

void init_debug(VkInstance *instance, DebugCallback dbg_cback) {
    VkDebugUtilsMessengerCreateInfoEXT dbg_info = {0};
    populate_dbg_info(&dbg_info, dbg_cback);

    VkDebugUtilsMessengerEXT dbg_msgr;
    create_dbg_msgr(*instance, &dbg_info, &dbg_msgr);
}

void create_dbg_msgr(
    VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
    VkDebugUtilsMessengerEXT *dbg_msgr)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"
        );
    assert(func != NULL);

    VkResult res = func(instance, dbg_info, NULL, dbg_msgr);
    assert(res == VK_SUCCESS);
}

void populate_dbg_info(
    VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
    DebugCallback dbg_cback
) {
    dbg_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_info->pfnUserCallback = dbg_cback;
}

void create_instance(VkInstance *instance, DebugCallback dbg_cback) {
    // get required instance extensions, and ensure they exist
    uint32_t extension_ct;
    // maximum 16 extensions, 255 characters each
    char **extensions = NULL;
    heap_2D(&extensions, 16, 255);

    get_extensions(&extension_ct, extensions);

    for (int i = 0; i < extension_ct; i++) {
        printf("Extension: %s\n", extensions[i]);
    }

    // ensure all required validation layers exist
    char *val_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    uint32_t val_layer_ct = 1;
    assert(check_validation(val_layers, val_layer_ct) == 0);

    // VkApplicationInfo
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    app_info.pApplicationName = "Thingy";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Epic Triangle Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    app_info.apiVersion = VK_API_VERSION_1_0;

    // VkInstanceCreateInfo
    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = extension_ct;
    instance_info.ppEnabledExtensionNames = (const char * const *) extensions;
    instance_info.ppEnabledLayerNames = (const char * const *) val_layers;
    instance_info.enabledLayerCount = val_layer_ct;

    // enable debugging during instance creation/destruction
    VkDebugUtilsMessengerCreateInfoEXT dbg_info;
    populate_dbg_info(&dbg_info, dbg_cback);
    instance_info.pNext = &dbg_info;

    // create
    VkResult result = vkCreateInstance(&instance_info, NULL, instance);
    assert(result == VK_SUCCESS);

    for (int i = 0; i < extension_ct; i++) {
        free(extensions[i]);
    }
    free(extensions);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    printf("Validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

int check_validation(char **req_layers, uint32_t req_layer_ct) {
    // Ensures all instance validation layers named in <req_layers> are present.
    // Returns 0 if they are, -1 on failure to find a layer.

    uint32_t real_layer_ct;
    vkEnumerateInstanceLayerProperties(&real_layer_ct, NULL);

    VkLayerProperties *real_layers =
        malloc(sizeof(VkLayerProperties) * real_layer_ct);

    vkEnumerateInstanceLayerProperties(&real_layer_ct, real_layers);

    // ensure all required layers exist
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

        printf("Found required layer %s\n", req_layer_name);
    }

    free(real_layers);
    return 0;
}

int check_req_exts(uint32_t req_ext_ct, char **req_exts) {
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
                printf("Found required extension %s\n", req_name);
                found = 1;
                break;
            }
        }

        if (!found) return -1;
    }

    free(real_ext);

    return 0;
}

void get_extensions(uint32_t *extension_ct, char **extensions) {
    // get list of extensions GLFW requires, and ensure they exist
    uint32_t glfw_ext_ct;
    const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_ct);
    assert(check_req_exts(glfw_ext_ct, (char **) glfw_exts) == 0);

    // ensure the other extensions we want exist too
    char *our_exts[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    uint32_t our_ext_ct = sizeof(our_exts) / sizeof(our_exts[0]);
    assert(check_req_exts(our_ext_ct, our_exts) == 0);

    // write both into <extensions>
    for (int i = 0; i < glfw_ext_ct; i++) {
        strcpy(extensions[i], glfw_exts[i]);
    }

    for (int i = 0; i < our_ext_ct; i++) {
        strcpy(extensions[i + glfw_ext_ct], our_exts[i]);
    }

    *extension_ct = glfw_ext_ct + our_ext_ct;
}

void heap_2D(char ***ppp, int major, int minor) {
    // Allocates [major][minor] char array onto the heap
    *ppp = malloc(sizeof(char *) * major);
    assert(*ppp != NULL);

    for (int i = 0; i < major; i++) {
        char *p = malloc(minor);
        assert(p != NULL);

        (*ppp)[i] = p;
    }
}
