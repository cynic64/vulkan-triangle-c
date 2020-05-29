#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "helpers.h"

#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_buffer.h"
#include "../src/vk_pipe.h"
#include "../src/vk_vertex.h"

void helper_create_instance(GLFWwindow **gwin,
			    void *user_data,
			    VkDebugUtilsMessengerEXT *dbg_msgr,
			    VkInstance *instance)
{
	if (gwin != NULL) {
		*gwin = init_glfw();
	}

	create_instance(default_debug_callback, user_data, instance);

	// Don't bother returning a handle on the messenger if it's NULL
	if (dbg_msgr == NULL) {
		VkDebugUtilsMessengerEXT tmp;
		init_debug(instance, default_debug_callback, user_data, &tmp);
	} else {
		init_debug(instance, default_debug_callback, user_data, dbg_msgr);
	}
}

void helper_get_phys_dev(GLFWwindow **gwin,
			 void *pUserData,
			 VkDebugUtilsMessengerEXT *dbg_msgr,
			 VkInstance *instance,
			 VkPhysicalDevice *phys_dev)
{
	helper_create_instance(gwin, pUserData, dbg_msgr, instance);

	get_physical_device(*instance, phys_dev);
}

void helper_get_queue_fam(GLFWwindow **gwin,
			  void *pUserData,
			  VkDebugUtilsMessengerEXT *dbg_msgr,
			  VkInstance *instance,
			  VkPhysicalDevice *phys_dev,
			  uint32_t *queue_fam)
{
	helper_get_phys_dev(gwin, pUserData, dbg_msgr, instance, phys_dev);

	*queue_fam = get_queue_fam(*phys_dev);
}

void helper_create_device(GLFWwindow **gwin,
			  void *pUserData,
			  VkDebugUtilsMessengerEXT *dbg_msgr,
			  VkInstance *instance,
			  VkPhysicalDevice *phys_dev,
			  uint32_t *queue_fam,
			  VkDevice *device)
{
	helper_get_queue_fam(gwin, pUserData, dbg_msgr, instance, phys_dev, queue_fam);

	create_device(*phys_dev, *queue_fam, device);
}

void helper_get_queue(GLFWwindow **gwin,
		      void *pUserData,
		      VkDebugUtilsMessengerEXT *dbg_msgr,
		      VkInstance *instance,
		      VkPhysicalDevice *phys_dev,
		      uint32_t *queue_fam,
		      VkDevice *device,
		      VkQueue *queue)
{
	helper_create_device(gwin,
			     pUserData,
			     dbg_msgr,
			     instance,
			     phys_dev,
			     queue_fam,
			     device);

	get_queue(*device, *queue_fam, queue);
}

void helper_create_surface(GLFWwindow **gwin,
			   void *pUserData,
			   VkDebugUtilsMessengerEXT *dbg_msgr,
			   VkInstance *instance,
			   VkPhysicalDevice *phys_dev,
			   uint32_t *queue_fam,
			   VkDevice *device,
			   VkQueue *queue,
			   VkSurfaceKHR *surface)
{
	// we need the GLFW window to create the surface, so if it's NULL use our
	// own
	if (gwin == NULL) {
		gwin = malloc(sizeof(gwin));
		*gwin = malloc(sizeof(*gwin));
	}

	helper_get_queue(gwin,
			 pUserData,
			 dbg_msgr,
			 instance,
			 phys_dev,
			 queue_fam,
			 device,
			 queue);

	create_surface(*instance, *gwin, surface);
}

void helper_window_create(GLFWwindow **gwin,
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
			  struct Window *win)
{
	// we need the GLFW window to create a Window from, so if it's NULL use our
	// own
	if (gwin == NULL) gwin = malloc(sizeof(gwin));

	helper_create_surface(gwin,
			      pUserData,
			      dbg_msgr,
			      instance,
			      phys_dev,
			      queue_fam,
			      device,
			      queue,
			      surface);

	get_dims(*phys_dev, *surface, swidth, sheight);

	VkRenderPass rpass;
	create_rpass(*device, SW_FORMAT, &rpass);

	window_create(*gwin,
		      *phys_dev,
		      *instance,
		      *device,
		      *surface,
		      *queue_fam,
		      *queue,
		      rpass,
		      *swidth,
		      *sheight,
		      win);
}

void helper_create_pipel(VkDevice device,
			 VkRenderPass rpass,
			 uint32_t binding_ct,
			 VkVertexInputBindingDescription *binding_descs,
			 uint32_t attr_ct,
			 VkVertexInputAttributeDescription *attr_descs,
			 char *vs_path, char *fs_path,
			 VkPipelineLayout *layout, VkPipeline *pipel)
{
	// Layout
	create_layout(device, 0, NULL, layout);

	// Shaders
	FILE *fp;
	size_t vs_size, fs_size;
	char *vs_buf, *fs_buf;

	fp = fopen(vs_path, "rb");
	assert(fp != NULL);
	read_bin(fp, &vs_size, NULL);
	vs_buf = malloc(vs_size);
	read_bin(fp, &vs_size, vs_buf);
	fclose(fp);
	VkShaderModule vs_mod;
	create_shmod(device, vs_size, vs_buf, &vs_mod);

	fp = fopen(fs_path, "rb");
	assert(fp != NULL);
	read_bin(fp, &fs_size, NULL);
	fs_buf = malloc(fs_size);
	read_bin(fp, &fs_size, fs_buf);
	fclose(fp);
	VkShaderModule fs_mod;

	// Shader module
	create_shmod(device, fs_size, fs_buf, &fs_mod);
	VkPipelineShaderStageCreateInfo vs_stage;
	VkPipelineShaderStageCreateInfo fs_stage;
	// Shader stages
	create_shtage(vs_mod, VK_SHADER_STAGE_VERTEX_BIT, &vs_stage);
	create_shtage(fs_mod, VK_SHADER_STAGE_FRAGMENT_BIT, &fs_stage);
	VkPipelineShaderStageCreateInfo shtages[] = {vs_stage, fs_stage};

	// pipeline!
	create_pipel(device,
		     2, shtages,
		     *layout,
		     binding_ct, binding_descs,
		     attr_ct, attr_descs,
		     rpass,
		     pipel);
}

void helper_create_shtage(VkDevice device,
			  const char *path,
			  VkShaderStageFlagBits stage,
			  VkPipelineShaderStageCreateInfo *shtage)
{
	FILE *fp = fopen(path, "rb");
	assert(fp != NULL);

	size_t buf_size = 0;
	read_bin(fp, &buf_size, NULL);

	char *buf = malloc(buf_size);
	read_bin(fp, &buf_size, buf);

	VkShaderModule shmod = NULL;
	create_shmod(device, buf_size, buf, &shmod);

	create_shtage(shmod, stage, shtage);
}

void helper_create_bufs(VkPhysicalDevice phys_dev,
			VkDevice device,
			VkBuffer *vbuf,
			VkBuffer *ibuf)
{
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Vertex3PosColor vertices[] = {
		{ .pos = {0.0, -1.0, 0.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {-1.0, 1.0, 0.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, 1.0, 0.0}, .color = {0.0, 0.0, 1.0} }
	};
	uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
	VkDeviceSize vertices_size = sizeof(vertices);

	uint32_t indices[] = {0, 1, 2};
	uint32_t index_count = sizeof(indices) / sizeof(indices[0]);
	VkDeviceSize indices_size = sizeof(indices);

	// vbuf
	struct Buffer vbuf_complete;
	buffer_create(device,
		      mem_props,
		      vertices_size,
		      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &vbuf_complete);

	buffer_write(vbuf_complete, vertices_size, (void *) vertices);

	// ibuf
	struct Buffer ibuf_complete;
	buffer_create(device,
		      mem_props,
		      indices_size,
		      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &ibuf_complete);

	buffer_write(ibuf_complete, indices_size, (void *) indices);

	// set
	*vbuf = vbuf_complete.handle;
	*ibuf = ibuf_complete.handle;
}

void helper_create_buffer_with_data(VkPhysicalDevice phys_dev, VkDevice device,
				    VkDeviceSize size, void *data,
				    struct Buffer *buf)
{
	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	buffer_create(device, dev_mem_props,
		      size,
		      VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		      | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		      | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		      | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      buf);

	buffer_write(*buf, size, data);
}

void helper_create_image_with_data(VkPhysicalDevice phys_dev, VkDevice device,
				   uint32_t queue_fam, VkQueue queue,
				   VkCommandPool cpool,
				   VkFormat format, VkImageUsageFlags usage,
				   VkImageAspectFlagBits aspect,
				   uint32_t width, uint32_t height,
				   VkDeviceSize size, void *data,
				   struct Image *image)
{
	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	struct Buffer staging;
	helper_create_buffer_with_data(phys_dev, device, size, data, &staging);

	image_create(device, queue_fam, dev_mem_props,
		     format,
		     usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     aspect,
		     width, height,
		     image);

	image_transition(device, queue, cpool,
			 image->handle, aspect,
			 0,
			 VK_ACCESS_TRANSFER_WRITE_BIT,
			 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_image(device, queue, cpool, aspect, width, height,
			  staging.handle, image->handle);
}
