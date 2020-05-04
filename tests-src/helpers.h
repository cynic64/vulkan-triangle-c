#ifndef T_HELPERS_H_
#define T_HELPERS_H_

#include <stdlib.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "../src/vk_window.h"

#define VK_OBJECTS \
    GLFWwindow *gwin = NULL; \
    int dbg_msg_ct = 0; \
    VkDebugUtilsMessengerEXT dbg_msgr; \
    VkInstance instance = NULL; \
    VkPhysicalDevice phys_dev = NULL; \
    uint32_t queue_fam = 999; \
    VkDevice device = NULL; \
    VkQueue queue = NULL; \
    VkSurfaceKHR surface = NULL; \
    uint32_t swidth; \
    uint32_t sheight; \
    struct Window win = {0}; \
    VkRenderPass rpass = NULL; \
    VkPipeline pipel = NULL;

// Initializes GLFW, creates an instance, and returns it in *instance.
//
// Uses the default debug callback with pUserData.
//
// If dbg_msgr is NULL, won't use it (because most tests don't bother cleaning
// up the messenger).
//
// If gwin is NULL, won't set it to the GLFW window (because a lot of tests
// don't need the window)
void helper_create_instance(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance
);

// Creates an instance and gets a physical device.
void helper_get_phys_dev(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev
);

// Gets a queue family.
// Also creates an instance and gets a physical device.
void helper_get_queue_fam(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam
);

// Creates a logical device.
// Also creates an instance, gets a physical device, and gets a queue family.
void helper_create_device(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam,
    VkDevice *device
);

// Gets a queue from a created logical device.
// Also creates an instance, gets a physical device, and gets a queue family
void helper_get_queue(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam,
    VkDevice *device,
    VkQueue *queue
);

// Creates a surface.
// Also does everthing up to queue creation.
void helper_create_surface(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam,
    VkDevice *device,
    VkQueue *queue,
    VkSurfaceKHR *surface
);

// Creates a Window.
// Also does everything up to queue creation.
void helper_window_create(
    GLFWwindow **gwin,
    void *pUserData,
    VkDebugUtilsMessengerEXT *dbg_msgr,
    VkInstance *instance,
    VkPhysicalDevice *phys_dev,
    uint32_t *queue_fam,
    VkDevice *device,
    VkQueue *queue,
    VkSurfaceKHR *surface,
    uint32_t *swidth,
    uint32_t *sheight,
    struct Window *win
);

// Creates a pipeline using the shaders in assets/testing.
// Doesn't create a device, user must provide it.
void helper_create_pipel(
    VkDevice device,
    VkRenderPass rpass,
    uint32_t binding_ct,
    VkVertexInputBindingDescription *binding_descs,
    uint32_t attr_ct,
    VkVertexInputAttributeDescription *attr_descs,
    char *vs_path,
    char *fs_path,
    VkPipeline *pipel
);

// Creates a shtage by loading the shader at the given path.
void helper_create_shtage(
    VkDevice device,
    const char *path,
    VkShaderStageFlagBits stage,
    VkPipelineShaderStageCreateInfo *shtage
);

// Creates a vertex buffer and an index bufer for a triangle
void helper_create_bufs(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    VkBuffer *vbuf,
    VkBuffer *ibuf
);

#endif // T_HELPERS_H_
