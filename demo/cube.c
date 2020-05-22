#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_uniform.h"
#include "../src/camera.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MAX_FRAMES_IN_FLIGHT 4

double mouse_x = 0;
double mouse_y = 0;

// Returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

static void glfw_cursor_callback(GLFWwindow *win, double x, double y)
{
	mouse_x = x;
	mouse_y = y;
}

int main()
{
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

	// Get queue family
	uint32_t queue_fam = get_queue_fam(phys_dev);

	// create device
	VkDevice device;
	create_device(phys_dev, queue_fam, &device);

	// get queue
	VkQueue queue;
	get_queue(device, queue_fam, &queue);

	// surface
	VkSurfaceKHR surface;
	create_surface(instance, gwin, &surface);
	uint32_t swidth, sheight;
	get_dims(phys_dev, surface, &swidth, &sheight);

	// render pass
	VkRenderPass rpass;
	create_rpass(device, SW_FORMAT, &rpass);

	// window
	struct Window win;
	window_create(
		gwin,
		phys_dev,
		instance,
		device,
		surface,
		queue_fam,
		queue,
		rpass,
		swidth,
		sheight,
		&win
		);

	// command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// buffers
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Vertex3PosColor vertices[] = {
		// top
		{ .pos = {-1.0, 1.0, -1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {1.0, 0.0, 0.0} },
		// bottom
		{ .pos = {-1.0, -1.0, -1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, -1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {-1.0, -1.0, 1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {0.0, 1.0, 0.0} },
		// left
		{ .pos = {-1.0, -1.0, -1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, -1.0, 1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, -1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {0.0, 0.0, 1.0} },
		// right
		{ .pos = {1.0, -1.0, -1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {0.0, 1.0, 1.0} },
		// front
		{ .pos = {-1.0, -1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {1.0, -1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		// back
		{ .pos = {-1.0, -1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
	};
	uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
	VkDeviceSize vertices_size = sizeof(vertices);

	uint32_t indices[] = {0, 3, 1, 2, 3, 0, 4, 5, 7, 7, 6, 4, 8, 9, 11, 11, 10, 8, 12, 13, 15, 15, 14, 12, 16, 17, 19, 19, 18, 16, 20, 21, 23, 23, 22, 20};
	uint32_t index_count = sizeof(indices) / sizeof(indices[0]);
	VkDeviceSize indices_size = sizeof(indices);

	// staging
	struct Buffer staging_buf;
	buffer_create(device,
		      mem_props,
		      vertices_size > indices_size ? vertices_size : indices_size,
		      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &staging_buf);

	// vertex
	buffer_write(staging_buf, vertices_size, (void *) vertices);

	struct Buffer vbuf;
	buffer_create(device,
		      mem_props,
		      vertices_size,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		      &vbuf);

	// copy staging to vertex
	copy_buffer(device,
		    queue,
		    cpool,
		    vertices_size,
		    staging_buf.handle,
		    vbuf.handle);

	// index buffer
	buffer_write(staging_buf, indices_size, (void *) indices);

	struct Buffer ibuf;
	buffer_create(device,
		      mem_props,
		      vertices_size,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		      &ibuf);

	// copy staging to index
	copy_buffer(device,
		    queue,
		    cpool,
		    indices_size,
		    staging_buf.handle,
		    ibuf.handle);

	// uniform data
	struct OrbitCamera cam = cam_orbit_new(0.0f, 0.0f);
	mat4 uniform_data = {0};
	uint32_t uniform_size = sizeof(uniform_data);

	// descriptor pool
	VkDescriptorPool desc_set_pool;
	create_descriptor_pool(device, MAX_FRAMES_IN_FLIGHT, MAX_FRAMES_IN_FLIGHT, &desc_set_pool);

	// synchronization primitives
	VkSemaphore *image_avail_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphore *render_done_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkFence *render_done_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	VkFence *swapchain_fences = malloc(sizeof(VkFence) * win.image_ct);

	// uniforms (also one for each frame in flight)
	struct Uniform *uniforms = malloc(sizeof(struct Uniform) * MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		create_sem(device, &image_avail_sems[i]);
		create_sem(device, &render_done_sems[i]);
		create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT, &render_done_fences[i]);

		uniforms[] = uniform_create(device,
					    desc_set_pool,
					    mem_props,
					    VK_SHADER_STAGE_VERTEX_BIT,
					    uniform_size);
	}

	for (int i = 0; i < win.image_ct; i++) {
		swapchain_fences[i] = NULL;
	}

	// pipeline layout
	VkPipelineLayout layout;
	create_layout(device, 1, &uniforms[0].layout, &layout);

	// shaders
	FILE *fp;
	size_t vs_size, fs_size;
	char *vs_buf, *fs_buf;

	// vs
	fp = fopen("assets/shaders/cube/main.vert.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &vs_size, NULL);
	vs_buf = malloc(vs_size);
	read_bin(fp, &vs_size, vs_buf);
	fclose(fp);

	VkShaderModule vs_mod;
	create_shmod(device, vs_size, vs_buf, &vs_mod);

	// fs
	fp = NULL;
	fp = fopen("assets/shaders/cube/main.frag.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &fs_size, NULL);
	fs_buf = malloc(fs_size);
	read_bin(fp, &fs_size, fs_buf);
	fclose(fp);

	VkShaderModule fs_mod;
	create_shmod(device, fs_size, fs_buf, &fs_mod);

	// shtages
	VkPipelineShaderStageCreateInfo vs_stage;
	VkPipelineShaderStageCreateInfo fs_stage;
	create_shtage(vs_mod, VK_SHADER_STAGE_VERTEX_BIT, &vs_stage);
	create_shtage(fs_mod, VK_SHADER_STAGE_FRAGMENT_BIT, &fs_stage);
	VkPipelineShaderStageCreateInfo shtages[] = {vs_stage, fs_stage};

	// pipeline
	VkPipeline pipel = NULL;
	create_pipel(device,
		     2,
		     shtages,
		     layout,
		     VERTEX_3_POS_COLOR_BINDING_CT,
		     VERTEX_3_POS_COLOR_BINDINGS,
		     VERTEX_3_POS_COLOR_ATTRIBUTE_CT,
		     VERTEX_3_POS_COLOR_ATTRIBUTES,
		     rpass,
		     &pipel);

	// cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

	// timing
	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);
	int f_count = 0;

	// loop
	while (!glfwWindowShouldClose(gwin)) {
		glfwPollEvents();

		// choose sync primitives
		int sync_set_idx = f_count % MAX_FRAMES_IN_FLIGHT;
		VkSemaphore image_avail_sem = image_avail_sems[sync_set_idx];
		VkSemaphore render_done_sem = render_done_sems[sync_set_idx];
		VkFence render_done_fence = render_done_fences[sync_set_idx];

		// wait for previous frame using this sync set to complete
		res = vkWaitForFences(device, 1, &render_done_fence, VK_TRUE, UINT64_MAX);
		assert(res == VK_SUCCESS);

		res = vkResetFences(device, 1, &render_done_fence);
		assert(res == VK_SUCCESS);

		// update uniform buffer
		struct Uniform uniform = uniforms[sync_set_idx];
		cam_orbit_mat(&cam, swidth, sheight, mouse_x, mouse_y, uniform_data);
		uniform_write(uniform, uniform_data);

		// acquire image
		uint32_t image_idx;
		VkFramebuffer fb;
		window_acquire(&win, image_avail_sem, &image_idx, &fb);

		// wait for swapchain fence
		VkFence swapchain_fence = swapchain_fences[image_idx];
		if (swapchain_fence != NULL) {
			res = vkWaitForFences(device, 1, &swapchain_fence, VK_TRUE, UINT64_MAX);
			assert(res == VK_SUCCESS);
		}

		// set swapchain fence
		swapchain_fences[image_idx] = render_done_fence;

		// create command buffer
		VkCommandBuffer cbuf;
		create_cbuf(device,
			    cpool,
			    rpass,
			    fb,
			    swidth,
			    sheight,
			    layout,
			    pipel,
			    1,
			    &uniform.set,
			    vbuf.handle,
			    ibuf.handle,
			    index_count,
			    &cbuf);

		// submit
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

		// present
		VkPresentInfoKHR present_info = {0};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_sems;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &win.swapchain;
		present_info.pImageIndices = &image_idx;

		res = vkQueuePresentKHR(queue, &present_info);

		// maybe recreate
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			get_dims(phys_dev, surface, &swidth, &sheight);
			window_recreate_swapchain(&win, swidth, sheight);
		} else {
			assert(res == VK_SUCCESS);
		}

		// wait idle
		res = vkQueueWaitIdle(queue);
		assert(res == VK_SUCCESS);

		// free command buffer
		vkFreeCommandBuffers(device, cpool, 1, &cbuf);

		f_count++;
	}

	// calculate delta / FPS
	double elapsed = get_elapsed(&s_time);
	printf("%d frames in %.4f secs --> %.4f FPS\n", f_count, elapsed, (double) f_count / elapsed);
	printf("Avg. delta: %.4f ms\n", elapsed / (double) f_count * 1000.0f);

	window_cleanup(&win);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, image_avail_sems[i], NULL);
		vkDestroySemaphore(device, render_done_sems[i], NULL);
		vkDestroyFence(device, render_done_fences[i], NULL);

		uniform_destroy(uniforms[i]);
	}

	vkDestroyDescriptorPool(device, desc_set_pool, NULL);

	buffer_destroy(vbuf);
	buffer_destroy(ibuf);
	buffer_destroy(staging_buf);

	vkDestroyCommandPool(device, cpool, NULL);
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
