#ifndef VKTOOLS_H_
#define VKTOOLS_H_

#include <vulkan/vulkan.h>

typedef VkBool32 (*DebugCallback) (
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *,
    void *
);

typedef VkResult (*DebugLoader) (
    VkInstance *,
    const VkDebugUtilsMessengerCreateInfoEXT *,
    const VkAllocationCallbacks *,
    VkDebugUtilsMessengerEXT **
);

// Returns 0 if all extensions were found, -1 otherwise
int check_exts(uint32_t req_ext_ct, char **req_exts);

// Same, but for device extensions
int check_dev_exts(
    VkPhysicalDevice phys_dev,
    uint32_t req_ext_ct,
    char **req_exts
);

void create_instance(VkInstance *instance, DebugCallback dbg_cback);

int check_layers(uint32_t req_layer_ct, char **req_layers);

void init_debug(VkInstance *instance, DebugCallback dbg_cback);

void get_physical_device(VkInstance instance, VkPhysicalDevice *phys_dev);

uint32_t get_queue_fam(VkPhysicalDevice phys_dev);

void create_device(
    VkInstance *instance,
    VkPhysicalDevice phys_dev,
    uint32_t queue_fam,
    VkDevice *device
);

void get_queue(VkDevice device, uint32_t queue_fam, VkQueue *queue);

void get_extensions(uint32_t *extension_ct, char **extensions);

void populate_dbg_info(
    VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
    DebugCallback dbg_cback
);

void create_dbg_msgr(
    VkInstance instance,
    VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
    VkDebugUtilsMessengerEXT *dbg_msgr
);

VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);

void heap_2D(char ***ppp, int major, int minor);

#endif // VKTOOLS_H_
