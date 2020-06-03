#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_window.h"
#include "../src/vk_image.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#define MAX_FRAMES_IN_FLIGHT 4

#define IMAGE_W 1920
#define IMAGE_H 1080

/*
 * End recording, submit and wait for completion of a given command buffer.
 * Doesn't do any kind of synchronization, so not that useful outside of testing
 * purposes.
 *
 * Also frees the command buffer.
 */
void cbuf_finish_one_time(VkDevice device,
			  VkQueue queue,
			  VkCommandPool cpool,
			  VkCommandBuffer cbuf);

int main()
{
	// Used for error checking on VK functions throughout
	VkResult res;

	// Initialize GLFW
	GLFWwindow *gwin = init_glfw();

	// Create instance
	VkInstance instance;
	// NULL is pUserData
	create_instance(default_debug_callback, NULL, &instance);

	// Set up debug messenger (again, NULL is pUserData)
	VkDebugUtilsMessengerEXT dbg_msgr;
	init_debug(&instance, default_debug_callback, NULL, &dbg_msgr);

	// Get physical device
	VkPhysicalDevice phys_dev;
	get_physical_device(instance, &phys_dev);

	// Get queue family
	uint32_t queue_fam = get_queue_fam(phys_dev);

	// Create device
	VkDevice device;
	create_device(phys_dev, queue_fam, &device);

	// Get queue
	VkQueue queue;
	get_queue(device, queue_fam, &queue);

	// Render pass
	VkRenderPass rpass;
	create_rpass(device, SW_FORMAT, &rpass);

	// Render target
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Image image;
	image_create(device,
		     queue_fam,
		     mem_props,
		     VK_FORMAT_B8G8R8A8_SRGB,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     IMAGE_W, IMAGE_H,
		     &image);
	
	// Create framebuffer
	VkFramebuffer fb;
	create_framebuffer(device,
			   IMAGE_W, IMAGE_H, rpass, 1, &image.view,
			   &fb);

	// Command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// Buffers
	struct Vertex2PosColor vertices[] = {
		{ .pos = {0.0, -1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {-1.0, 1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, 1.0}, .color = {0.0, 0.0, 1.0} }
	};
	uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
	VkDeviceSize vertices_size = sizeof(vertices);

	uint32_t indices[] = {0, 1, 2};
	uint32_t index_count = sizeof(indices) / sizeof(indices[0]);
	VkDeviceSize indices_size = sizeof(indices);

	// Staging
	struct Buffer staging_buf;
	buffer_create(device,
		      mem_props,
		      MAX(vertices_size, indices_size),
		      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &staging_buf);

	// Vertex
	buffer_write(staging_buf, vertices_size, (void *) vertices);

	struct Buffer vbuf;
	buffer_create(device,
		      mem_props,
		      vertices_size,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		      &vbuf);

	// Copy staging to vertex
	copy_buffer_buffer(device,
			   queue,
			   cpool,
			   vertices_size,
			   staging_buf.handle,
			   vbuf.handle);

	// Index buffer
	buffer_write(staging_buf, indices_size, (void *) indices);

	struct Buffer ibuf;
	buffer_create(device,
		      mem_props,
		      vertices_size,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		      &ibuf);

	// Copy staging to index
	copy_buffer_buffer(device,
			   queue,
			   cpool,
			   indices_size,
			   staging_buf.handle,
			   ibuf.handle);

	// Pipeline layout
	VkPipelineLayout layout;
	create_layout(device, 0, NULL, &layout);

	// Shaders
	FILE *fp;
	size_t vs_size, fs_size;
	char *vs_buf, *fs_buf;

	// Vertex shader
	fp = fopen("assets/shaders/triangle/main.vert.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &vs_size, NULL);
	vs_buf = malloc(vs_size);
	read_bin(fp, &vs_size, vs_buf);
	fclose(fp);

	VkShaderModule vs_mod;
	create_shmod(device, vs_size, vs_buf, &vs_mod);

	// Fragment shader
	fp = NULL;
	fp = fopen("assets/shaders/triangle/main.frag.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &fs_size, NULL);
	fs_buf = malloc(fs_size);
	read_bin(fp, &fs_size, fs_buf);
	fclose(fp);

	VkShaderModule fs_mod;
	create_shmod(device, fs_size, fs_buf, &fs_mod);

	// Shader Stages
	VkPipelineShaderStageCreateInfo vs_stage;
	VkPipelineShaderStageCreateInfo fs_stage;
	create_shtage(vs_mod, VK_SHADER_STAGE_VERTEX_BIT, &vs_stage);
	create_shtage(fs_mod, VK_SHADER_STAGE_FRAGMENT_BIT, &fs_stage);
	VkPipelineShaderStageCreateInfo shtages[] = {vs_stage, fs_stage};

	// Pipeline
	VkPipeline pipel = NULL;
	create_pipel(device,
		     2, shtages,
		     layout,
		     VERTEX_2_POS_COLOR_BINDING_CT, VERTEX_2_POS_COLOR_BINDINGS,
		     VERTEX_2_POS_COLOR_ATTRIBUTE_CT, VERTEX_2_POS_COLOR_ATTRIBUTES,
		     rpass, 0,
		     &pipel);

	// Cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

	// Clear values
	VkClearValue clears[] = {{0.0f, 0.0f, 0.0f, 0.0f}};
	uint32_t clear_ct = ARRAY_SIZE(clears);

        // Create command buffer
        VkCommandBuffer cbuf;
        create_cbuf(device, cpool,
		    rpass, clear_ct, clears,
		    fb,
		    IMAGE_W, IMAGE_H,
		    layout,
		    pipel,
		    0, NULL,
		    vbuf.handle, ibuf.handle,
		    3,
		    &cbuf);

        // Submit
	submit_syncless(device, queue, cpool, cbuf);

        // Wait idle
        res = vkQueueWaitIdle(queue);
        assert(res == VK_SUCCESS);

	// Transition layout of image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	image_transition(device, queue, cpool, image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Create a host-visible buffer to copy the rendered image to
	struct Buffer dest_buf;
	buffer_create(device, mem_props,
		      4 * IMAGE_W * IMAGE_H,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &dest_buf);

	// Copy it
	copy_image_buffer(device,
			  queue,
			  cpool,
			  VK_IMAGE_ASPECT_COLOR_BIT,
			  IMAGE_W,
			  IMAGE_H,
			  image.handle, dest_buf.handle);

	// Print
	// Should these be #defines? I have no idea.
	const uint32_t printed_w = 80;
	const uint32_t printed_h = 24;
	char image_string[printed_w * printed_h];

	vk_mem_to_string(device,
			 IMAGE_W, IMAGE_H,
			 printed_w, printed_h,
			 dest_buf.memory,
			 image_string);

	printf("%s", image_string);

	image_destroy(device, image);
	
	buffer_destroy(dest_buf);

	vkDestroyFramebuffer(device, fb, NULL);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	buffer_destroy(vbuf);
	buffer_destroy(ibuf);
	buffer_destroy(staging_buf);

	vkDestroyCommandPool(device, cpool, NULL);
	vkDestroyRenderPass(device, rpass, NULL);

	vkDestroyDevice(device, NULL);
	destroy_dbg_msgr(instance, &dbg_msgr);
	vkDestroyInstance(instance, NULL);

	return 0;
}

void cbuf_finish_one_time(VkDevice device,
			  VkQueue queue,
			  VkCommandPool cpool,
			  VkCommandBuffer cbuf)
{
	VkResult res;
	
	res = vkEndCommandBuffer(cbuf);
	assert(res == VK_SUCCESS);

	VkSubmitInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cbuf;

	res = vkQueueSubmit(queue, 1, &info, NULL);
	assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	vkFreeCommandBuffers(device, cpool, 1, &cbuf);
}

