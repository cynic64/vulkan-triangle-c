#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_image.h"
#include "../src/vk_uniform.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

// Initialize STB image loader
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TEXTURE_W 1024
#define TEXTURE_H 1024
#define MAX_FRAMES_IN_FLIGHT 4

// Returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

int main() {
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

	// Surface
	VkSurfaceKHR surface;
	create_surface(instance, gwin, &surface);
	uint32_t swidth, sheight;
	get_dims(phys_dev, surface, &swidth, &sheight);

	// Render pass
	VkRenderPass rpass;
	create_rpass(device, SW_FORMAT, &rpass);

	// Window
	struct Window win;
	window_create(gwin,
		      phys_dev,
		      instance,
		      device,
		      surface,
		      queue_fam,
		      queue,
		      rpass, 0, NULL,
		      swidth, sheight,
		      &win);

	// Command pool
	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	// Buffers
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	struct Vertex2PosTex vertices[] = {
		{ .pos = {-1.0, -1.0}, .texc = {0.0, 0.0} },
		{ .pos = {1.0, -1.0}, .texc = {1.0, 0.0} },
		{ .pos = {-1.0, 1.0}, .texc = {0.0, 1.0} },
		{ .pos = {1.0, 1.0}, .texc = {1.0, 1.0} }
	};
	uint32_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
	VkDeviceSize vertices_size = sizeof(vertices);

	uint32_t indices[] = {0, 2, 1, 1, 2, 3};
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

	// Load texture
	int texture_w, texture_h, texture_channels;
	unsigned char *texture_data = stbi_load("assets/images/texture.png",
					      &texture_w, &texture_h,
					      &texture_channels, 0);
	assert(texture_w == TEXTURE_W);
	assert(texture_h == TEXTURE_H);
	assert(texture_channels == 4);
	printf("Width, height, channels: %d, %d, %d\n",
	       texture_w, texture_h, texture_channels);
	uint32_t texture_size = texture_w * texture_h * texture_channels;

	// Copy to staging buffer for texture
	struct Buffer texture_staging;
	buffer_create(device, mem_props,
		      texture_size,
		      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &texture_staging);

	buffer_write(texture_staging, texture_size, texture_data);

	// Texture
	struct Image texture;
	image_create(device, queue_fam, mem_props,
		     VK_FORMAT_R8G8B8A8_SRGB,
		     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     TEXTURE_W, TEXTURE_H,
		     &texture);

	image_transition(device, queue, cpool,
			 texture.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 0,
			 VK_ACCESS_TRANSFER_WRITE_BIT,
			 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_image(device, queue, cpool,
			  VK_IMAGE_ASPECT_COLOR_BIT, TEXTURE_W, TEXTURE_H,
			  texture_staging.handle, texture.handle);

	image_transition(device, queue, cpool,
			 texture.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_TRANSFER_WRITE_BIT,
			 VK_ACCESS_SHADER_READ_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Sampler
	VkSamplerCreateInfo sampler_info = {0};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	VkSampler sampler;
	res = vkCreateSampler(device, &sampler_info, NULL, &sampler);
	assert(res == VK_SUCCESS);

	// Set
	VkDescriptorPool dpool;
	create_descriptor_pool(device, 1, 1, &dpool);
	
	struct Set set;
	VkDescriptorType desc_types[] =
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
	VkDescriptorBufferInfo desc_buffers[] = {NULL};
	VkDescriptorImageInfo desc_images[] =
		{{.sampler = sampler,
		  .imageView = texture.view,
		  .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
	VkShaderStageFlags desc_stages[] = {VK_SHADER_STAGE_FRAGMENT_BIT};

	set_create(device, dpool,
		   1, desc_types, desc_buffers, desc_images, desc_stages,
		   &set);
		
	// Pipeline layout
	VkPipelineLayout layout;
	create_layout(device, 1, &set.layout, &layout);

	// Shaders
	FILE *fp;
	size_t vs_size, fs_size;
	char *vs_buf, *fs_buf;

	// Vertex shader
	fp = fopen("assets/shaders/texture/main.vert.spv", "rb");
	assert(fp != NULL);

	read_bin(fp, &vs_size, NULL);
	vs_buf = malloc(vs_size);
	read_bin(fp, &vs_size, vs_buf);
	fclose(fp);

	VkShaderModule vs_mod;
	create_shmod(device, vs_size, vs_buf, &vs_mod);

	// Fragment shader
	fp = NULL;
	fp = fopen("assets/shaders/texture/main.frag.spv", "rb");
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
		     VERTEX_2_POS_TEX_BINDING_CT,
		     VERTEX_2_POS_TEX_BINDINGS,
		     VERTEX_2_POS_TEX_ATTRIBUTE_CT,
		     VERTEX_2_POS_TEX_ATTRIBUTES,
		     rpass, 0,
		     &pipel);

	// Cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

	// Synchronization primitives
	VkSemaphore *image_avail_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphore *render_done_sems = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	VkFence *render_done_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	VkFence *swapchain_fences = malloc(sizeof(VkFence) * win.image_ct);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		create_sem(device, &image_avail_sems[i]);
		create_sem(device, &render_done_sems[i]);
		create_fence(device, VK_FENCE_CREATE_SIGNALED_BIT, &render_done_fences[i]);
	}

	for (int i = 0; i < win.image_ct; i++) {
		swapchain_fences[i] = NULL;
	}

	// Command buffers (one for every frame in flight)
	VkCommandBuffer *cbufs = malloc(sizeof(cbufs[0])
					* MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		cbufs[i] = NULL;
	}

	// Clear values
	VkClearValue clears[] = {{0.0f, 0.0f, 0.0f, 0.0f}};
	uint32_t clear_ct = ARRAY_SIZE(clears);

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
		int sync_set_idx = f_count % MAX_FRAMES_IN_FLIGHT;
		VkSemaphore image_avail_sem = image_avail_sems[sync_set_idx];
		VkSemaphore render_done_sem = render_done_sems[sync_set_idx];
		VkFence render_done_fence = render_done_fences[sync_set_idx];

		// Wait for previous frame using this sync set to complete
		res = vkWaitForFences(device, 1, &render_done_fence, VK_TRUE, UINT64_MAX);
		assert(res == VK_SUCCESS);

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
		create_cbuf(device, cpool,
			    rpass, clear_ct, clears,
			    fb,
			    swidth, sheight,
			    layout, pipel,
			    1, &set.handle,
			    vbuf.handle, ibuf.handle, index_count,
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

	vkDestroyCommandPool(device, cpool, NULL);
	window_cleanup(&win);

	vkDestroyPipeline(device, pipel, NULL);
	vkDestroyPipelineLayout(device, layout, NULL);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, image_avail_sems[i], NULL);
		vkDestroySemaphore(device, render_done_sems[i], NULL);
		vkDestroyFence(device, render_done_fences[i], NULL);
	}

	set_destroy(device, set);

	vkDestroySampler(device, sampler, NULL);
	image_destroy(device, texture);

	vkDestroyDescriptorPool(device, dpool, NULL);
    
	buffer_destroy(vbuf);
	buffer_destroy(ibuf);
	buffer_destroy(staging_buf);
	buffer_destroy(texture_staging);

	vkDestroyRenderPass(device, rpass, NULL);

	vkDestroySurfaceKHR(instance, surface, NULL);

	vkDestroyDevice(device, NULL);
	destroy_dbg_msgr(instance, &dbg_msgr);
	vkDestroyInstance(instance, NULL);

	glfw_cleanup(gwin);

	stbi_image_free(texture_data);

	return 0;
}

double get_elapsed(struct timespec *s_time) {
	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);

	double secs = e_time.tv_sec - s_time->tv_sec;
	double subsec = (e_time.tv_nsec - s_time->tv_nsec) / 1000000000.0f;

	return secs + subsec;
}
