#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_uniform.h"
#include "../src/vk_rpass.h"
#include "../src/vk_sync_pool.h"
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

	uint32_t phys_dev_ct;
	vkEnumeratePhysicalDevices(instance, &phys_dev_ct, NULL);
	VkPhysicalDevice *phys_devs = malloc(sizeof(VkPhysicalDevice) * phys_dev_ct);
	vkEnumeratePhysicalDevices(instance, &phys_dev_ct, phys_devs);
	VkPhysicalDeviceProperties *props =
		malloc(sizeof(VkPhysicalDeviceProperties) * phys_dev_ct);

	for (int i = 0; i < phys_dev_ct; i++) {
		vkGetPhysicalDeviceProperties(phys_devs[i], &props[i]);
		printf("Found device: %s\n", props[i].deviceName);
	}

	printf("Using device: %s\n", props[0].deviceName);
	free(phys_devs);
	free(props);

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
	rpass_basic(device, SW_FORMAT, &rpass);

	// Window
	struct Window win;
	window_create(gwin, phys_dev, instance, device,
		      surface,
		      queue_fam, queue,
		      rpass,
		      0, NULL,
		      swidth, sheight,
		      &win);

	// Command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// Buffers
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Vertex3PosColor vertices[] = {
		// Top
		{ .pos = {-1.0, 1.0, -1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {1.0, 0.0, 0.0} },
		// Bottom
		{ .pos = {-1.0, -1.0, -1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, -1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {-1.0, -1.0, 1.0}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {0.0, 1.0, 0.0} },
		// Left
		{ .pos = {-1.0, -1.0, -1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, -1.0, 1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, -1.0}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {0.0, 0.0, 1.0} },
		// Right
		{ .pos = {1.0, -1.0, -1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {0.0, 1.0, 1.0} },
		// Front
		{ .pos = {-1.0, -1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {-1.0, 1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {1.0, -1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {1.0, 1.0, -1.0}, .color = {1.0, 0.0, 1.0} },
		// Back
		{ .pos = {-1.0, -1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {1.0, -1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {-1.0, 1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {1.0, 1.0, 1.0}, .color = {1.0, 1.0, 0.0} },
	};
	uint32_t vertex_count = ARRAY_SIZE(vertices);
	VkDeviceSize vertices_size = sizeof(vertices);

	uint32_t indices[] = {0, 3, 1, 2, 3, 0, 4, 5, 7, 7, 6, 4, 8, 9, 11, 11, 10, 8, 12, 13, 15, 15, 14, 12, 16, 17, 19, 19, 18, 16, 20, 21, 23, 23, 22, 20};
	uint32_t index_count = sizeof(indices) / sizeof(indices[0]);
	VkDeviceSize indices_size = sizeof(indices);

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
	VkSemaphore *image_avail_sems = malloc(sizeof(image_avail_sems[0]) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphore *render_done_sems = malloc(sizeof(render_done_sems[0]) * MAX_FRAMES_IN_FLIGHT);
	VkFence *swapchain_fences = malloc(sizeof(swapchain_fences[0]) * win.image_ct);

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
		     rpass, 0, VK_SAMPLE_COUNT_1_BIT,
		     &pipel);

	// Cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

	// Command buffers (one for every frame in flight)
	VkCommandBuffer *cbufs = malloc(sizeof(cbufs[0])
					* MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		cbufs[i] = NULL;
	}

	// Clear values
	VkClearValue clears[] = {{0.0f, 0.0f, 0.0f, 0.0f}};
	uint32_t clear_ct = ARRAY_SIZE(clears);

	// SyncPool struct
	struct SyncPool sync_pool;
	sync_pool_create(device, MAX_FRAMES_IN_FLIGHT, &sync_pool);

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
			get_dims(phys_dev, surface, &swidth, &sheight);
			window_recreate_swapchain(&win,
						  0, NULL, swidth, sheight);

			must_recreate_swapchain = 0;
		}
		
		glfwPollEvents();

		// Choose sync primitives
		VkFence render_done_fence;
		uint32_t sync_set_idx;
		sync_pool_acquire(device, &sync_pool,
				  &render_done_fence, &sync_set_idx);
		
		VkSemaphore image_avail_sem = image_avail_sems[sync_set_idx];
		VkSemaphore render_done_sem = render_done_sems[sync_set_idx];

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
			    rpass, clear_ct, clears,
			    fb,
			    swidth,
			    sheight,
			    layout,
			    pipel,
			    1,
			    &sets[sync_set_idx].handle,
			    vbuf.handle,
			    ibuf.handle,
			    index_count,
			    &cbuf);

		cbufs[sync_set_idx] = cbuf;

		// Submit
		submit_synced(queue, image_avail_sem, render_done_sem,
			      render_done_fence, cbuf);

		// Present
		VkPresentInfoKHR present_info = {0};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &render_done_sem;
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

	vkDestroyCommandPool(device, cpool, NULL);

	window_cleanup(&win);

	sync_pool_destroy(device, sync_pool);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	buffer_destroy(uniform_buf);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, image_avail_sems[i], NULL);
		vkDestroySemaphore(device, render_done_sems[i], NULL);

		set_destroy(device, sets[i]);
	}

	vkDestroyDescriptorPool(device, dpool, NULL);

	buffer_destroy(vbuf);
	buffer_destroy(ibuf);
	buffer_destroy(staging_buf);

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
