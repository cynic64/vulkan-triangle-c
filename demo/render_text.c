#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"
#include "../src/vk_window.h"

#include "../tests-src/helpers.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#define MAX_FRAMES_IN_FLIGHT 4

#define IMAGE_W 1920
#define IMAGE_H 1080

// Returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

int main()
{
	// Used for error checking on VK functions throughout
	VkResult res;

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
	create_rpass(device, VK_FORMAT_B8G8R8A8_UNORM, &rpass);

	// Image to render to
	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_UNORM,
		.extent = {
			.width = 1920,
			.height = 1080,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queue_fam,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkImage image;
	res = vkCreateImage(device, &image_info, NULL, &image);
	assert(res == VK_SUCCESS);

	// Image memory
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	VkMemoryRequirements buf_reqs;
	vkGetImageMemoryRequirements(device, image, &buf_reqs);

	VkMemoryPropertyFlags req_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// Find a usable memory type
	uint32_t mem_type_idx;
	int found_mem_type = 0;
	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		uint32_t mem_type_props = mem_props.memoryTypes[i].propertyFlags;

		int suitable_for_buffer = buf_reqs.memoryTypeBits & (1 << i);
		int suitable_for_user = (mem_type_props & req_props) == req_props;

		if (suitable_for_buffer && suitable_for_user) {
			found_mem_type = 1;
			mem_type_idx = i;
			break;
		}
	}

	assert(found_mem_type == 1);

	// Allocate
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = buf_reqs.size;
	alloc_info.memoryTypeIndex = mem_type_idx;

	VkDeviceMemory image_mem;
	res = vkAllocateMemory(device, &alloc_info, NULL, &image_mem);
	assert(res == VK_SUCCESS);

	// Bind
	res = vkBindImageMemory(device, image, image_mem, 0);
	assert(res == VK_SUCCESS);

	// Image view
	VkImageViewCreateInfo image_view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_UNORM,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};
	VkImageView image_view;
	res = vkCreateImageView(device, &image_view_info, NULL, &image_view);
	assert(res == VK_SUCCESS);

	// Create framebuffer
	VkFramebuffer fb;
	create_framebuffer(device, 1920, 1080, rpass, image_view, &fb);

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
	copy_buffer(device,
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
	copy_buffer(device,
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
		     rpass,
		     &pipel);

	// Cleanup shader modules
	vkDestroyShaderModule(device, vs_mod, NULL);
	vkDestroyShaderModule(device, fs_mod, NULL);

        // Create command buffer
        VkCommandBuffer cbuf;
        create_cbuf(device,
		    cpool,
		    rpass,
		    fb,
		    IMAGE_W, IMAGE_H,
		    layout,
		    pipel,
		    0, NULL,
		    vbuf.handle, ibuf.handle,
		    3,
		    &cbuf);

        // Submit
        VkSubmitInfo submit_info = {0};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cbuf;
        submit_info.signalSemaphoreCount = 0;

        res = vkQueueSubmit(queue, 1, &submit_info, NULL);
        assert(res == VK_SUCCESS);

        // Wait idle
        res = vkQueueWaitIdle(queue);
        assert(res == VK_SUCCESS);

	// Transition layout of image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	VkCommandBufferAllocateInfo transition_alloc_info = {0};
	transition_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transition_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transition_alloc_info.commandPool = cpool;
	transition_alloc_info.commandBufferCount = 1;

	VkCommandBuffer transition_cbuf;
	res = vkAllocateCommandBuffers(device, &transition_alloc_info, &transition_cbuf);
	assert(res == VK_SUCCESS);

	// Record
	VkCommandBufferBeginInfo transition_begin_info = {0};
	transition_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transition_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(transition_cbuf, &transition_begin_info);
	
	vkCmdPipelineBarrier(transition_cbuf,
			     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			     VK_PIPELINE_STAGE_TRANSFER_BIT,
			     0,
			     0, NULL,
			     0, NULL,
			     1, &barrier);

	res = vkEndCommandBuffer(transition_cbuf);
	assert(res == VK_SUCCESS);

	VkSubmitInfo transition_submit_info = {0};
	transition_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transition_submit_info.commandBufferCount = 1;
	transition_submit_info.pCommandBuffers = &transition_cbuf;

	res = vkQueueSubmit(queue, 1, &transition_submit_info, NULL);
	assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	// Create a host-visible buffer to copy the rendered image to
	struct Buffer dest_buf;
	buffer_create(device, mem_props, 4 * 1920 * 1080, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &dest_buf);

	// Copy it
	// Allocate a command buffer
	VkCommandBufferAllocateInfo cbuf_alloc_info = {0};
	cbuf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbuf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbuf_alloc_info.commandPool = cpool;
	cbuf_alloc_info.commandBufferCount = 1;

	VkCommandBuffer copy_cbuf;
	res = vkAllocateCommandBuffers(device, &cbuf_alloc_info, &copy_cbuf);
	assert(res == VK_SUCCESS);

	// Record
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(copy_cbuf, &begin_info);

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 1920;
	region.bufferImageHeight = 1080;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = 1920;
	region.imageExtent.height = 1080;
	region.imageExtent.depth = 1;
		
	vkCmdCopyImageToBuffer(copy_cbuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest_buf.handle, 1, &region);
	res = vkEndCommandBuffer(copy_cbuf);
	assert(res == VK_SUCCESS);

	// Submit
	VkSubmitInfo cbuf_submit_info = {0};
	cbuf_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	cbuf_submit_info.commandBufferCount = 1;
	cbuf_submit_info.pCommandBuffers = &copy_cbuf;

	res = vkQueueSubmit(queue, 1, &cbuf_submit_info, NULL);
	assert(res == VK_SUCCESS);
	res = vkQueueWaitIdle(queue);
	assert(res == VK_SUCCESS);

	// Map and print
	unsigned char pixels[1080][1920];
	
	void *mapped;
	res = vkMapMemory(device, dest_buf.memory, 0, 4 * 1920 * 1080, 0, &mapped);
        assert(res == VK_SUCCESS);
	for (int i = 0; i < 1920 * 1080; i++) {
		unsigned char b = ((char *)mapped)[4 * i];
		unsigned char g = ((char *)mapped)[4 * i + 1];
		unsigned char r = ((char *)mapped)[4 * i + 2];
		unsigned char a = ((char *)mapped)[4 * i + 3];
		pixels[i / 1920][i % 1920] = (r + g + b) / 3;
	}
	vkUnmapMemory(device, dest_buf.memory);

	for (int y = 0; y < 108; y++) {
		for (int x = 0; x < 192; x++) {
			if (pixels[y * 10][x * 10] > 0) printf("#");
			else printf(" ");
		}
		printf("\n");
	}

	vkFreeCommandBuffers(device, cpool, 1, &copy_cbuf);	
	
	vkDestroyImage(device, image, NULL);
	vkFreeMemory(device, image_mem, NULL);

	buffer_destroy(dest_buf);

	vkDestroyImageView(device, image_view, NULL);
	vkDestroyFramebuffer(device, fb, NULL);

        vkFreeCommandBuffers(device, cpool, 1, &cbuf);

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
