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
#include "../src/vk_image.h"
#include "../src/camera.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

/*
 * DEFINES
 */
#define MAX_FRAMES_IN_FLIGHT 4
#define DEPTH_FMT VK_FORMAT_D32_SFLOAT

/*
 * STRUCTS
 */

struct Mesh {
	uint32_t vertex_ct;
	struct Vertex3PosColor *vertices;
	uint32_t index_ct;
	uint32_t *indices;
};

// Indexed [z * width * height + y * width + x].
struct VoxelWorld {
	// x
	uint32_t width;
	// y
	uint32_t height;
	// z
	uint32_t depth;

	unsigned char *data;
};

/*
 * FUNCTIONS
 */

// Mallocs.
void voxel_world_to_mesh(struct VoxelWorld *world, struct Mesh *mesh);

// Example:
// "000|111\n"
// "010|111\n"
// "000|111\n"
//
// For a pyramid.
//
// Mallocs.
void voxel_world_from_string(char *str, struct VoxelWorld *world);

/*
 * First query how many vertices/indices will be produced by calling with either
 * vertices or indices being NULL. Call again after allocating memory for both
 * vertices and indices.
 */
void gen_cube_mesh(float x, float y, float z, uint32_t idx_off,
		   uint32_t *vertex_ct, uint32_t *index_ct,
		   struct Vertex3PosColor *vertices, uint32_t *indices);

// Returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

int main()
{
	// Used for error checking on VK functions throughout
	VkResult res;

	// Initialize GLFW
	GLFWwindow *gwin = init_glfw();
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
	rpass_with_depth(device, SW_FORMAT, VK_FORMAT_D32_SFLOAT, &rpass);

	// Depth buffer
	struct Image depth_image;
	image_create(device, queue_fam, mem_props,
		     DEPTH_FMT,
		     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_DEPTH_BIT,
		     VK_SAMPLE_COUNT_1_BIT,
		     swidth, sheight,
		     &depth_image);

	// Window
	struct Window win;
	window_create(gwin, phys_dev, instance, device,
		      surface,
		      queue_fam, queue,
		      rpass,
		      1, &depth_image.view,
		      swidth, sheight,
		      &win);

	// Command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// Buffers
	struct VoxelWorld world;
	/*
	voxel_world_from_string("000|111\n"
				"000|011\n"
				"010|111\n"
				"000|111\n", &world);
	*/

	world.width = 64;
	world.height = 64;
	world.depth = 64;
	world.data = malloc(world.width * world.height * world.depth);
	for (int i = 0; i < world.width * world.height * world.depth; i++) {
		world.data[i] = rand() % 2;
	}

	struct Mesh mesh;
	voxel_world_to_mesh(&world, &mesh);

	printf("Vertices: %d\n", mesh.vertex_ct);
	printf("Indices: %d\n", mesh.index_ct);

	VkDeviceSize vertices_size = mesh.vertex_ct * sizeof(mesh.vertices[0]);
	VkDeviceSize indices_size = mesh.index_ct * sizeof(mesh.indices[0]);

	// Staging
	struct Buffer staging_buf;
	buffer_create(device,
		      mem_props,
		      vertices_size > indices_size ? vertices_size : indices_size,
		      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &staging_buf);

	// Vertex
	buffer_write(staging_buf, vertices_size, (void *) mesh.vertices);

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
			   staging_buf.handle,vbuf.handle);

	// Index buffer
	buffer_write(staging_buf, indices_size, (void *) mesh.indices);

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
			   staging_buf.handle,ibuf.handle);

	// Uniform buffer
	double mouse_x, mouse_y;
	glfwGetCursorPos(gwin, &mouse_x, &mouse_y);
	struct FlyCamera cam = cam_fly_new(-16.0f, -16.0f, -16.0f,
					   0.0f, 0.0f,
					   mouse_x, mouse_y);
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
	VkFence *render_done_fences = malloc(sizeof(render_done_fences[0]) * MAX_FRAMES_IN_FLIGHT);
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
		     rpass, 1, VK_SAMPLE_COUNT_1_BIT,
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
	VkClearValue clears[] = {{0.0f, 0.0f, 0.0f, 0.0f},
				 {1.0f, 0}};	
	uint32_t clear_ct = ARRAY_SIZE(clears);

	// Timing
	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);
	
	struct timespec last_frame_time = s_time;
	
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
			image_create(device, queue_fam, mem_props,
				     DEPTH_FMT,
				     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				     VK_IMAGE_ASPECT_DEPTH_BIT,
				     VK_SAMPLE_COUNT_1_BIT,
				     swidth, sheight,
				     &depth_image);

			window_recreate_swapchain(&win,
						  1, &depth_image.view,
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
		double delta = get_elapsed(&last_frame_time);
		clock_gettime(CLOCK_MONOTONIC, &last_frame_time);
		
		glfwGetCursorPos(gwin, &mouse_x, &mouse_y);
		
		cam_fly_update(&cam, gwin, mouse_x, mouse_y, delta);
		cam_fly_mat(&cam, swidth, sheight, uniform_data);
		
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
			    mesh.index_ct,
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
	vkDestroyCommandPool(device, cpool, NULL);

	window_cleanup(&win);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	buffer_destroy(uniform_buf);

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

	vkDestroyRenderPass(device, rpass, NULL);

	vkDestroySurfaceKHR(instance, surface, NULL);

	vkDestroyDevice(device, NULL);
	destroy_dbg_msgr(instance, &dbg_msgr);
	vkDestroyInstance(instance, NULL);

	glfw_cleanup(gwin);

	return 0;
}

void voxel_world_to_mesh(struct VoxelWorld *world, struct Mesh *mesh)
{
	int w = world->width;
	int h = world->height;
	int d = world->depth;

	// Allocate
	int cell_ct = 0;
	for (int i = 0; i < w * h * d; i++) {
		if (world->data[i] != 0) cell_ct++;
	}
	printf("Cell count: %d\n", cell_ct);

	int cube_vtx_ct;
	int cube_idx_ct;
	gen_cube_mesh(0.0f, 0.0f, 0.0f, 0,
		      &cube_vtx_ct, &cube_idx_ct, NULL, NULL);

	mesh->vertex_ct = cell_ct * cube_vtx_ct;
	mesh->vertices = malloc(mesh->vertex_ct * sizeof(mesh->vertices[0]));
	mesh->index_ct = cell_ct * cube_idx_ct;
	mesh->indices = malloc(mesh->index_ct * sizeof(mesh->indices[0]));

	// Fill
	int vertex_idx = 0;
	int index_idx = 0;
	for (int z = 0; z < d; z++) {
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {				
				if (world->data[z * w * h + y * w + x] == 0)
					continue;

				gen_cube_mesh(x, y, z, vertex_idx, NULL, NULL,
					      &mesh->vertices[vertex_idx],
					      &mesh->indices[index_idx]);
				vertex_idx += cube_vtx_ct;
				index_idx += cube_idx_ct;
			}
		}
	}
}

void voxel_world_from_string(char *str, struct VoxelWorld *world)
{
	int char_idx = 0;
	int width = -1;
	int height = -1;
	int depth = 0;
	char c;

	// First pass to see how much to allocate
	int size = 0;
	while ((c = str[char_idx++]) != '\0') {
		if (c != '|' && c != '\n') size++;
	}

	printf("Size: %d\n", size);

	// Fill data
	unsigned char *data = malloc(size * sizeof(data[0]));
	assert(data != NULL);

	int data_idx = 0;
	char_idx = 0;
	while ((c = str[char_idx++]) != '\0') {
		if (c == '|') {
			// Set width if it hasn't been set yet
			if (width != -1) continue;
			width = data_idx;
		} else if (c == '\n') {
			// Set height if it hasn't been set yet
			if (height != -1) continue;
			
			assert(width > 0);
			height = data_idx / width;
		} else {
			// Set data
			data[data_idx++] = c - '0';
		}
	}

	world->data = data;
	world->width = width;
	world->height = height;
	world->depth = data_idx / width / height;
}

void gen_cube_mesh(float x, float y, float z, uint32_t idx_off,
		   uint32_t *vertex_ct, uint32_t *index_ct,
		   struct Vertex3PosColor *vertices, uint32_t *indices)
{
	const struct Vertex3PosColor base_vertices[] = {
		// Top
		{ .pos = {-0.5, 0.5, -0.5}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {0.5, 0.5, -0.5}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {-0.5, 0.5, 0.5}, .color = {1.0, 0.0, 0.0} },
		{ .pos = {0.5, 0.5, 0.5}, .color = {1.0, 0.0, 0.0} },
		// Bottom
		{ .pos = {-0.5, -0.5, -0.5}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {0.5, -0.5, -0.5}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {-0.5, -0.5, 0.5}, .color = {0.0, 1.0, 0.0} },
		{ .pos = {0.5, -0.5, 0.5}, .color = {0.0, 1.0, 0.0} },
		// Left
		{ .pos = {-0.5, -0.5, -0.5}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-0.5, -0.5, 0.5}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-0.5, 0.5, -0.5}, .color = {0.0, 0.0, 1.0} },
		{ .pos = {-0.5, 0.5, 0.5}, .color = {0.0, 0.0, 1.0} },
		// Right
		{ .pos = {0.5, -0.5, -0.5}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {0.5, 0.5, -0.5}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {0.5, -0.5, 0.5}, .color = {0.0, 1.0, 1.0} },
		{ .pos = {0.5, 0.5, 0.5}, .color = {0.0, 1.0, 1.0} },
		// Front
		{ .pos = {-0.5, -0.5, -0.5}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {-0.5, 0.5, -0.5}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {0.5, -0.5, -0.5}, .color = {1.0, 0.0, 1.0} },
		{ .pos = {0.5, 0.5, -0.5}, .color = {1.0, 0.0, 1.0} },
		// Back
		{ .pos = {-0.5, -0.5, 0.5}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {0.5, -0.5, 0.5}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {-0.5, 0.5, 0.5}, .color = {1.0, 1.0, 0.0} },
		{ .pos = {0.5, 0.5, 0.5}, .color = {1.0, 1.0, 0.0} },
	};
	
	uint32_t base_indices[] = {0, 3, 1, 2, 3, 0, 4, 5, 7, 7, 6, 4, 8, 9, 11,
				   11, 10, 8, 12, 13, 15, 15, 14, 12, 16, 17,
				   19, 19, 18, 16, 20, 21, 23, 23, 22, 20};

	// Maybe only return counts
	if (vertices == NULL || indices == NULL) {
		*vertex_ct = ARRAY_SIZE(base_vertices);
		*index_ct = ARRAY_SIZE(base_indices);
		return;
	}

	// Otherwise set vertices
	for (int i = 0; i < ARRAY_SIZE(base_vertices); i++) {
		struct Vertex3PosColor old = base_vertices[i];
		vertices[i].pos[0] = old.pos[0] + x;
		vertices[i].pos[1] = old.pos[1] + y;
		vertices[i].pos[2] = old.pos[2] + z;
		vertices[i].color[0] = old.color[0];
		vertices[i].color[1] = old.color[1];
		vertices[i].color[2] = old.color[2];
	}

	// And indices
	for (int i = 0; i < ARRAY_SIZE(base_indices); i++) {
		indices[i] = base_indices[i] + idx_off;
	}
}

double get_elapsed(struct timespec *s_time) {
	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);

	double secs = e_time.tv_sec - s_time->tv_sec;
	double subsec = (e_time.tv_nsec - s_time->tv_nsec) / 1000000000.0f;

	return secs + subsec;
}
