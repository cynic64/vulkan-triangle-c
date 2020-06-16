#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_uniform.h"
#include "../src/vk_image.h"
#include "../src/vk_rpass.h"
#include "../src/camera.h"
#include "../src/obj.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_FRAMES_IN_FLIGHT 4
#define DEPTH_FMT VK_FORMAT_D32_SFLOAT
#define SWAPCHAIN_FMT VK_FORMAT_B8G8R8A8_SRGB
#define SAMPLES VK_SAMPLE_COUNT_8_BIT

double mouse_x = 0;
double mouse_y = 0;

// Returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

static void glfw_cursor_callback(GLFWwindow *win, double x, double y) {
	mouse_x = x;
	mouse_y = y;
}

int main() {
	// Used for error checking on VK functions throughout
	VkResult res;

	// Initialize GLFW
	GLFWwindow *gwin = init_glfw();

	// Mouse settings
	glfwSetCursorPosCallback(gwin, glfw_cursor_callback);
	glfwSetInputMode(gwin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	// Get queue family
	uint32_t queue_fam = get_queue_fam(phys_dev);

	// Create device
	VkDevice device;
	create_device(phys_dev, queue_fam, &device);

	// Get queue
	VkQueue queue;
	get_queue(device, queue_fam, &queue);

	// Surface
	VkSurfaceKHR surface;
	create_surface(instance, gwin, &surface);
	uint32_t swidth, sheight;
	get_dims(phys_dev, surface, &swidth, &sheight);

	// Render pass
	VkRenderPass rpass;
	rpass_multisampled_with_depth(device,
				      SWAPCHAIN_FMT,
				      VK_FORMAT_D32_SFLOAT,
				      SAMPLES,
				      &rpass);

	// Depth buffer
	struct Image depth_image;
	image_create(device, queue_fam, mem_props,
		     DEPTH_FMT,
		     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_DEPTH_BIT,
		     SAMPLES,
		     swidth, sheight,
		     &depth_image);

	// Color target (multisampled, will be resolved to swapchain image)
	struct Image color_image;
	image_create(device, queue_fam, mem_props,
		     SWAPCHAIN_FMT,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     SAMPLES,
		     swidth, sheight,
		     &color_image);

	// Window
	VkImageView extra_views[] = {color_image.view, depth_image.view};
	struct Window win;
	window_create(gwin, phys_dev, instance, device,
		      surface,
		      queue_fam, queue,
		      rpass,
		      ARRAY_SIZE(extra_views), extra_views,
		      swidth, sheight,
		      &win);

	// Command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// Read OBJ
	FILE *obj_fp = fopen("assets/models/bunny.obj", "r");
	assert(obj_fp != NULL);
    
	size_t vertex_ct, index_ct;
	obj_load(obj_fp, &vertex_ct, &index_ct, NULL, NULL);

	printf("Vertex, index count: [%lu, %lu]\n", vertex_ct, index_ct);

	uint32_t *indices = malloc(sizeof(indices[0]) * index_ct);
	struct ObjVertex *obj_vtxs = malloc(sizeof(obj_vtxs[0]) * vertex_ct);
	struct Vertex3PosNormal *vertices = malloc(sizeof(vertices[0]) * vertex_ct);

	obj_load(obj_fp, &vertex_ct, &index_ct, obj_vtxs, indices);

	fclose(obj_fp);

	obj_vertex_to_vertex_3_pos_normal_list(vertices, obj_vtxs, vertex_ct);

	// Buffers
	VkDeviceSize vertices_size = sizeof(vertices[0]) * vertex_ct;

	VkDeviceSize indices_size = sizeof(indices[0]) * index_ct;

	// Staging
	struct Buffer staging_buf;
	buffer_create(device,
		      mem_props,
		      vertices_size > indices_size ? vertices_size : indices_size,
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

	// Uniform buffer
	struct OrbitCamera cam = cam_orbit_new(0.0f, 0.0f);
	mat4 uniform_data = {0};
	uint32_t uniform_size = sizeof(uniform_data);
	struct Buffer uniform_buf;
	buffer_create(device, mem_props, uniform_size,
		      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &uniform_buf);

	// Descriptor pool
	VkDescriptorPool dpool;
	create_descriptor_pool(device, MAX_FRAMES_IN_FLIGHT, MAX_FRAMES_IN_FLIGHT, &dpool);

	// Synchronization primitives
	VkSemaphore *image_avail_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphore *render_done_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkFence *render_done_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	VkFence *swapchain_fences = malloc(sizeof(VkFence) * win.image_ct);

	// Sets (one for each frame in flight)
	uint32_t desc_ct = 1;
	VkDescriptorType desc_types[] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
	VkDescriptorBufferInfo desc_buffers[] = {{.buffer = uniform_buf.handle,
						  .offset = 0,
						  .range = uniform_size}};
	VkDescriptorImageInfo desc_images[] = {NULL};
	VkShaderStageFlags desc_stages[] = {VK_SHADER_STAGE_VERTEX_BIT};
	struct Set *sets = malloc(sizeof(sets[0]) * MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		create_sem(device, &image_avail_sems[i]);
		create_sem(device, &render_done_sems[i]);
		create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT, &render_done_fences[i]);

		set_create(device, dpool,
			   desc_ct, desc_types,
			   desc_buffers, desc_images, desc_stages,
			   &sets[i]);
	}

	for (int i = 0; i < win.image_ct; i++) {
		swapchain_fences[i] = NULL;
	}

	// Pipeline layout
	VkPipelineLayout layout;
	create_layout(device, 1, &sets[0].layout, &layout);

	// Shaders
	FILE *fp;
	size_t vs_size, fs_size;
	char *vs_buf, *fs_buf;

	// Vertex shader
	fp = fopen("assets/shaders/cube/main.vert.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &vs_size, NULL);
	vs_buf = malloc(vs_size);
	read_bin(fp, &vs_size, vs_buf);
	fclose(fp);

	VkShaderModule vs_mod;
	create_shmod(device, vs_size, vs_buf, &vs_mod);

	// Fragment shader
	fp = NULL;
	fp = fopen("assets/shaders/cube/main.frag.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &fs_size, NULL);
	fs_buf = malloc(fs_size);
	read_bin(fp, &fs_size, fs_buf);
	fclose(fp);

	VkShaderModule fs_mod;
	create_shmod(device, fs_size, fs_buf, &fs_mod);

	// Shtages
	VkPipelineShaderStageCreateInfo vs_stage;
	VkPipelineShaderStageCreateInfo fs_stage;
	create_shtage(vs_mod, VK_SHADER_STAGE_VERTEX_BIT, &vs_stage);
	create_shtage(fs_mod, VK_SHADER_STAGE_FRAGMENT_BIT, &fs_stage);
	VkPipelineShaderStageCreateInfo shtages[] = {vs_stage, fs_stage};

	// Pipeline
	VkPipeline pipel = NULL;
	create_pipel(device,
		     2,
		     shtages,
		     layout,
		     VERTEX_3_POS_COLOR_BINDING_CT,
		     VERTEX_3_POS_COLOR_BINDINGS,
		     VERTEX_3_POS_COLOR_ATTRIBUTE_CT,
		     VERTEX_3_POS_COLOR_ATTRIBUTES,
		     rpass, 1, SAMPLES,
		     &pipel);

	// Clear values
	VkClearValue clears[] = {{0.0f, 0.0f, 0.0f, 0.0f},
				 {0.0f, 0.0f, 0.0f, 0.0f},
				 {1.0f, 0}};	
	uint32_t clear_ct = ARRAY_SIZE(clears);

	// Cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

	// Command buffers (one for every frame in flight)
	VkCommandBuffer *cbufs = malloc(sizeof(cbufs[0])
					* MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		cbufs[i] = NULL;
	}

	// Timing
	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);
	int f_count = 0;

	// Swapchain
	int must_recreate_swapchain = 0;

	// Loop
	while (!glfwWindowShouldClose(gwin)) {
		// Maybe recreate
		if (must_recreate_swapchain) {
			res = vkQueueWaitIdle(queue);
			assert(res == VK_SUCCESS);
			
			get_dims(phys_dev, surface, &swidth, &sheight);

			image_destroy(device, depth_image);
			image_destroy(device, color_image);
			image_create(device, queue_fam, mem_props,
				     DEPTH_FMT,
				     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				     VK_IMAGE_ASPECT_DEPTH_BIT,
				     SAMPLES,
				     swidth, sheight,
				     &depth_image);
			image_create(device, queue_fam, mem_props,
				     SWAPCHAIN_FMT,
				     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				     VK_IMAGE_ASPECT_COLOR_BIT,
				     SAMPLES,
				     swidth, sheight,
				     &color_image);

			extra_views[0] = color_image.view;
			extra_views[1] = depth_image.view;
			window_recreate_swapchain(&win,
						  ARRAY_SIZE(extra_views),
						  extra_views,
						  swidth, sheight);

			must_recreate_swapchain = 0;
		}
		
		glfwPollEvents();

		// Choose sync primitives
		int sync_set_idx = f_count % MAX_FRAMES_IN_FLIGHT;
		VkSemaphore image_avail_sem = image_avail_sems[sync_set_idx];
		VkSemaphore render_done_sem = render_done_sems[sync_set_idx];
		VkFence render_done_fence = render_done_fences[sync_set_idx];

		// Wait for previous frame using this sync set to complete
		res = vkWaitForFences(device, 1, &render_done_fence, VK_TRUE, UINT64_MAX);
		assert(res == VK_SUCCESS);

		// Update uniform buffer
		cam_orbit_mat(&cam, swidth, sheight, mouse_x, mouse_y, uniform_data);
		buffer_write(uniform_buf, uniform_size, uniform_data);

		// Acquire image
		uint32_t image_idx;
		VkFramebuffer fb;
		int ac_res = window_acquire(&win, image_avail_sem, &image_idx, &fb);

		if (ac_res != 0) {
			must_recreate_swapchain = 1;
			continue;
		}

		// Free previously used command buffer
		VkCommandBuffer cbuf = cbufs[sync_set_idx];
		if (cbuf != NULL) {
			vkFreeCommandBuffers(device, cpool, 1, &cbuf);
		}

		// Wait for swapchain fence
		VkFence swapchain_fence = swapchain_fences[image_idx];
		if (swapchain_fence != NULL) {
			res = vkWaitForFences(device, 1, &swapchain_fence, VK_TRUE, UINT64_MAX);
			assert(res == VK_SUCCESS);
		}

		res = vkResetFences(device, 1, &render_done_fence);
		assert(res == VK_SUCCESS);

		// Set swapchain fence
		swapchain_fences[image_idx] = render_done_fence;

		// Create command buffer
		create_cbuf(device,
			    cpool,
			    rpass,
			    clear_ct, clears,
			    fb,
			    swidth, sheight,
			    layout, pipel,
			    1, &sets[sync_set_idx].handle,
			    vbuf.handle,
			    ibuf.handle,
			    index_ct,
			    &cbuf);

		cbufs[sync_set_idx] = cbuf;

		// Submit
		VkSemaphore wait_sems[] = {image_avail_sem};
		VkSemaphore signal_sems[] = {render_done_sem};
		VkPipelineStageFlags wait_stages[] =
			{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

		VkSubmitInfo submit_info = {0};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_sems;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &cbuf;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sems;

		res = vkQueueSubmit(queue, 1, &submit_info, render_done_fence);
		assert(res == VK_SUCCESS);

		// Present
		VkPresentInfoKHR present_info = {0};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_sems;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &win.swapchain;
		present_info.pImageIndices = &image_idx;

		res = vkQueuePresentKHR(queue, &present_info);

		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			must_recreate_swapchain = 1;
		} else {
			assert(res == VK_SUCCESS);
		}

		f_count++;
	}

	// Calculate delta / FPS
	double elapsed = get_elapsed(&s_time);
	printf("%d frames in %.4f secs --> %.4f FPS\n", f_count, elapsed, (double) f_count / elapsed);
	printf("Avg. delta: %.4f ms\n", elapsed / (double) f_count * 1000.0f);

	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	image_destroy(device, depth_image);
	image_destroy(device, color_image);
	
	vkDestroyCommandPool(device, cpool, NULL);
	window_cleanup(&win);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, image_avail_sems[i], NULL);
		vkDestroySemaphore(device, render_done_sems[i], NULL);
		vkDestroyFence(device, render_done_fences[i], NULL);

		set_destroy(device, sets[i]);
	}

	vkDestroyDescriptorPool(device, dpool, NULL);

	buffer_destroy(vbuf);
	buffer_destroy(ibuf);
	buffer_destroy(staging_buf);
	buffer_destroy(uniform_buf);

	vkDestroyRenderPass(device, rpass, NULL);

	vkDestroySurfaceKHR(instance, surface, NULL);

	vkDestroyDevice(device, NULL);
	destroy_dbg_msgr(instance, &dbg_msgr);
	vkDestroyInstance(instance, NULL);

	glfw_cleanup(gwin);

	return 0;
}

double get_elapsed(struct timespec *s_time) {
	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);

	double secs = e_time.tv_sec - s_time->tv_sec;
	double subsec = (e_time.tv_nsec - s_time->tv_nsec) / 1000000000.0f;

	return secs + subsec;
}
