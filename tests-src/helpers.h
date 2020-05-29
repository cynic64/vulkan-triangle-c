#ifndef T_HELPERS_H_
#define T_HELPERS_H_

#include <stdlib.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "../src/vk_window.h"
#include "../src/vk_buffer.h"
#include "../src/vk_image.h"

#define DEFAULT_FMT VK_FORMAT_B8G8R8A8_UNORM

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
    VkPipelineLayout pipe_layout = NULL; \
    VkPipeline pipel = NULL;

/*
 * Creates an instance, and returns it in *instance, initializing GLFW in the
 * process if gwin is not NULL.
 *
 * Uses the default debug callback, passing user_data
 *
 * If dbg_msgr is NULL, won't use it (because most tests don't need a handle on
 * it).
 */
void helper_create_instance(GLFWwindow **gwin,
			    void *user_data,
			    VkDebugUtilsMessengerEXT *dbg_msgr,
			    VkInstance *instance);

/*
 * Creates an instance and gets a physical device.
 */
void helper_get_phys_dev(GLFWwindow **gwin,
			 void *pUserData,
			 VkDebugUtilsMessengerEXT *dbg_msgr,
			 VkInstance *instance,
			 VkPhysicalDevice *phys_dev);

/* 
 * Gets a queue family.
 * Also creates an instance and gets a physical device.
 */
void helper_get_queue_fam(GLFWwindow **gwin,
			  void *pUserData,
			  VkDebugUtilsMessengerEXT *dbg_msgr,
			  VkInstance *instance,
			  VkPhysicalDevice *phys_dev,
			  uint32_t *queue_fam);

/* 
 * Creates a logical device.
 * Also creates an instance, gets a physical device, and gets a queue family.
 */
void helper_create_device(GLFWwindow **gwin,
			  void *pUserData,
			  VkDebugUtilsMessengerEXT *dbg_msgr,
			  VkInstance *instance,
			  VkPhysicalDevice *phys_dev,
			  uint32_t *queue_fam,
			  VkDevice *device);

/*
 * Gets a queue from a created logical device.
 * Also creates an instance, gets a physical device, and gets a queue family
 */
void helper_get_queue(GLFWwindow **gwin,
		      void *pUserData,
		      VkDebugUtilsMessengerEXT *dbg_msgr,
		      VkInstance *instance,
		      VkPhysicalDevice *phys_dev,
		      uint32_t *queue_fam,
		      VkDevice *device,
		      VkQueue *queue);

/*
 * Creates a surface.
 * Also does everthing up to queue creation.
 */
void helper_create_surface(GLFWwindow **gwin,
			   void *pUserData,
			   VkDebugUtilsMessengerEXT *dbg_msgr,
			   VkInstance *instance,
			   VkPhysicalDevice *phys_dev,
			   uint32_t *queue_fam,
			   VkDevice *device,
			   VkQueue *queue,
			   VkSurfaceKHR *surface);

/*
 * Creates a Window.
 * Also does everything up to queue creation.
 */
void helper_window_create(GLFWwindow **gwin,
			  void *pUserData,
			  VkDebugUtilsMessengerEXT *dbg_msgr,
			  VkInstance *instance,
			  VkPhysicalDevice *phys_dev,
			  uint32_t *queue_fam,
			  VkDevice *device,
			  VkQueue *queue,
			  VkSurfaceKHR *surface,
			  uint32_t *swidth, uint32_t *sheight,
			  struct Window *win);

/*
 * Creates a pipeline using the shaders in assets/testing.
 * Doesn't create a device, user must provide it.
 */
void helper_create_pipel(VkDevice device,
			 VkRenderPass rpass,
			 uint32_t binding_ct,
			 VkVertexInputBindingDescription *binding_descs,
			 uint32_t attr_ct,
			 VkVertexInputAttributeDescription *attr_descs,
			 char *vs_path, char *fs_path,
			 VkPipelineLayout *layout,
			 VkPipeline *pipel);

/*
 * Creates a shtage by loading the shader at the given path.
 */
void helper_create_shtage(VkDevice device,
			  const char *path,
			  VkShaderStageFlagBits stage,
			  VkPipelineShaderStageCreateInfo *shtage);

/*
 * Creates a vertex buffer and an index buffer for a triangle.
 */
void helper_create_bufs(VkPhysicalDevice phys_dev, VkDevice device,
			VkBuffer *vbuf, VkBuffer *ibuf);

/*
 * Initializes a Buffer struct.
 *
 * Usages: TRANSFER_SRC, VERTEX_BUFFER, INDEX_BUFFER, UNIFORM_BUFFER
 * Properties: HOST_VISIBLE, HOST_COHERENT
 */
void helper_create_buffer_with_data(VkPhysicalDevice phys_dev, VkDevice device,
				    VkDeviceSize size, void *data,
				    struct Buffer *buf);

/*
 * Initializes an Image struct with some data.
 * 
 * Is host-readable and host-writable.
 *
 * Will be left as VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
 */
void helper_create_image_with_data(VkPhysicalDevice phys_dev,
				   VkDevice device, uint32_t queue_fam, VkQueue queue,
				   VkCommandPool cpool,
				   VkFormat format, VkImageUsageFlags usage,
				   VkImageAspectFlagBits aspect,
				   uint32_t width, uint32_t height,
				   VkDeviceSize size, void *data,
				   struct Image *image);

#endif // T_HELPERS_H_
