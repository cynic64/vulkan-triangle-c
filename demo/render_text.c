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

/*
 * Initialize everything necessary to render to an image. All arguments are
 * outputs.
 */
void helper_text_render_initialize(VkInstance *instance,
				   VkDebugUtilsMessengerEXT *dbg_msgr,
				   VkPhysicalDevice *phys_dev,
				   uint32_t *queue_fam,
				   VkDevice *device,
				   VkQueue *queue,
				   VkRenderPass *rpass);

/*
 * Create an image, bind some memory to it, and create an image view.
 */
void create_image(VkDevice device,
		  uint32_t queue_fam,
		  VkPhysicalDeviceMemoryProperties dev_mem_props,
		  VkImageUsageFlagBits usage,
		  VkFormat format,
		  VkMemoryPropertyFlags req_mem_props,
		  uint32_t width, uint32_t height,
		  VkImage *image, VkImageView *image_view,
		  VkDeviceMemory *image_mem);


/*
 * Transitions an image's layout.
 *
 * aspect: Image aspect being transitioned, like VK_IMAGE_ASPECT_COLOR_BIT
 * old_lt: Old layout
 * new_lt: New layout
 * src_mask: What accesses to wait on, like VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
 * dst_mask: What accesses should wait, like VK_ACCESS_TRANSFER_READ_BIT
 * src_stage: What pipeline stage to wait on
 * dst_stage: What pipeline stage should wait
 */
void transition_image(VkDevice device,
		      VkQueue queue,
		      VkCommandPool cpool,
		      VkImage image,
		      VkImageAspectFlags aspect,
		      VkAccessFlags src_mask, VkAccessFlags dst_mask,
		      VkPipelineStageFlags src_stage,
		      VkPipelineStageFlags dst_stage,
		      VkImageLayout old_lt, VkImageLayout new_lt);

/*
 * Find a suitable memory type given memory requirements and properties.
 *
 * mem_reqs: Whatever vkGet[Buffer|Image]MemoryRequirements says
 * req_mem_props: Properties like DEVICE_LOCAL, HOST_COHERENT, etc.
 */
uint32_t find_memory_type(VkPhysicalDeviceMemoryProperties dev_mem_props,
			  VkMemoryRequirements mem_reqs,
			  VkMemoryPropertyFlags req_mem_props);

/*
 * Copy an given image to a given buffer.
 *
 * The image must have layout IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
 *
 * The buffer must:
 * - Be big enough
 * - Have usage TRANSFER_DST
 * - Be HOST_VISIBLE
 * And HOST_COHERENT is a good idea too.
 */
void copy_image_to_buffer(VkDevice device,
			  VkQueue queue,
			  VkCommandPool cpool,
			  VkImageAspectFlags aspect,
			  uint32_t width, uint32_t height,
			  VkImage src, VkBuffer dest);

/*
 * Format some Vulkan memory as a string.
 * The point is to ASCII-fy images rendered by Vulkan.
 * The image should be copied to some host-visible memory first.
 *
 * Assumes image was B8G8R8A8_UNORM.
 *
 * Out should be pre-allocated to size 4 * out_w * out_h
 *
 * in_w, in_h: Width and height image was, in pixels
 * out_w, out_h: Width and height output should be, in characters
 * out: Char array to output to
 */
void vk_mem_to_string(VkDevice device,
		      uint32_t in_w, uint32_t in_h,
		      uint32_t out_w, uint32_t out_h,
		      VkDeviceMemory mem,
		      char *out);

/*
 * Allocate a command buffer for one-time use and begin recording.
 */
void cbuf_begin_one_time(VkDevice device,
			 VkCommandPool cpool,
			 VkCommandBuffer *cbuf);

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

	VkInstance instance;
	VkDebugUtilsMessengerEXT dbg_msgr;
	VkPhysicalDevice phys_dev;
	uint32_t queue_fam;
	VkDevice device;
	VkQueue queue;
	VkRenderPass rpass;
	helper_text_render_initialize(&instance,
				      &dbg_msgr,
				      &phys_dev,
				      &queue_fam,
				      &device,
				      &queue,
				      &rpass);

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	VkImage image;
	VkImageView image_view;
	VkDeviceMemory image_mem;
	create_image(device,
		     queue_fam,
		     mem_props,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		     VK_FORMAT_B8G8R8A8_UNORM,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     1920, 1080,
		     &image,
		     &image_view,
		     &image_mem);

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
	transition_image(device, queue, cpool, image, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Create a host-visible buffer to copy the rendered image to
	struct Buffer dest_buf;
	buffer_create(device, mem_props, 4 * 1920 * 1080, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &dest_buf);

	// Copy it
	copy_image_to_buffer(device,
			     queue,
			     cpool,
			     VK_IMAGE_ASPECT_COLOR_BIT,
			     1920,
			     1080,
			     image, dest_buf.handle);

	// Print
	// Should these be #defines? I have no idea.
	const uint32_t printed_w = 80;
	const uint32_t printed_h = 24;
	char image_string[printed_w * printed_h];

	vk_mem_to_string(device,
			 1920, 1080,
			 printed_w, printed_h,
			 dest_buf.memory,
			 image_string);

	printf(image_string);

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

void helper_text_render_initialize(VkInstance *instance,
				   VkDebugUtilsMessengerEXT *dbg_msgr,
				   VkPhysicalDevice *phys_dev,
				   uint32_t *queue_fam,
				   VkDevice *device,
				   VkQueue *queue,
				   VkRenderPass *rpass)
{
	helper_get_queue(NULL,
			 NULL, dbg_msgr,
			 instance,
			 phys_dev,
			 queue_fam,
			 device,
			 queue);

	create_rpass(*device, VK_FORMAT_B8G8R8A8_UNORM, rpass);
}

void create_image(VkDevice device,
		  uint32_t queue_fam,
		  VkPhysicalDeviceMemoryProperties dev_mem_props,
		  VkImageUsageFlagBits usage,
		  VkFormat format,
		  VkMemoryPropertyFlags req_mem_props,
		  uint32_t width, uint32_t height,
		  VkImage *image, VkImageView *image_view,
		  VkDeviceMemory *image_mem)
{
	// Image handle
	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = {
			.width = width,
			.height = height,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.usage = usage,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queue_fam,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VkResult res = vkCreateImage(device, &image_info, NULL, image);
	assert(res == VK_SUCCESS);

	// Image memory
	VkMemoryRequirements buf_reqs;
	vkGetImageMemoryRequirements(device, *image, &buf_reqs);

	uint32_t mem_type_idx = find_memory_type(dev_mem_props,
						 buf_reqs,
						 req_mem_props);

	// Allocate
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = buf_reqs.size;
	alloc_info.memoryTypeIndex = mem_type_idx;

	res = vkAllocateMemory(device, &alloc_info, NULL, image_mem);
	assert(res == VK_SUCCESS);

	// Bind
	res = vkBindImageMemory(device, *image, *image_mem, 0);
	assert(res == VK_SUCCESS);

	// Image view
	VkImageViewCreateInfo image_view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = *image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
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
	
	res = vkCreateImageView(device, &image_view_info, NULL, image_view);
	assert(res == VK_SUCCESS);
}

void vk_mem_to_string(VkDevice device,
		      uint32_t in_w, uint32_t in_h,
		      uint32_t out_w, uint32_t out_h,
		      VkDeviceMemory mem,
		      char *out)
{	
	void *mapped;
	VkResult res = vkMapMemory(device, mem, 0, 4 * in_w * in_h, 0, &mapped);
        assert(res == VK_SUCCESS);

	unsigned char (*pixels)[in_w] = malloc(in_w * in_h);

	for (int i = 0; i < in_w * in_h; i++) {
		unsigned char b = ((char *)mapped)[4 * i];
		unsigned char g = ((char *)mapped)[4 * i + 1];
		unsigned char r = ((char *)mapped)[4 * i + 2];
		unsigned char a = ((char *)mapped)[4 * i + 3];
		pixels[i / in_w][i % in_w] = (r + g + b) / 3;
	}
	vkUnmapMemory(device, mem);

	uint32_t scale_x = in_w / out_w;
	uint32_t scale_y = in_h / out_h;

	char *ptr = out;
	for (uint32_t y = 0; y < out_h; y++) {
		// Skip the last column to make room for newlines
		for (uint32_t x = 0; x < out_w - 1; x++) {
			if (pixels[y * scale_y][x * scale_x] > 0) *ptr++ = '#';
			else *ptr++ = ' ';
		}

		*ptr++ = '\n';
	}

	// Subtract 1 to avoid overrun
	*(ptr - 1) = '\0';
	// Re-do final newline
	*(ptr - 2) = '\n';

	free(pixels);
}

void copy_image_to_buffer(VkDevice device,
			  VkQueue queue,
			  VkCommandPool cpool,
			  VkImageAspectFlags aspect,
			  uint32_t width, uint32_t height,
			  VkImage src, VkBuffer dest)
{
	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);
	
	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = width;
	region.bufferImageHeight = height;
	region.imageSubresource.aspectMask = aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset.x = 0;
	region.imageOffset.y = 0;
	region.imageOffset.z = 0;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
		
	vkCmdCopyImageToBuffer(cbuf,
			       src,
			       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			       dest,
			       1, &region);

	cbuf_finish_one_time(device, queue, cpool, cbuf);
}

void transition_image(VkDevice device,
		      VkQueue queue,
		      VkCommandPool cpool,
		      VkImage image,
		      VkImageAspectFlags aspect,
		      VkAccessFlags src_mask, VkAccessFlags dst_mask,
		      VkPipelineStageFlags src_stage,
		      VkPipelineStageFlags dst_stage,
		      VkImageLayout old_lt, VkImageLayout new_lt)
{
	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_lt;
	barrier.newLayout = new_lt;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = src_mask;
	barrier.dstAccessMask = dst_mask;

	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);
	
	vkCmdPipelineBarrier(cbuf,
			     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			     VK_PIPELINE_STAGE_TRANSFER_BIT,
			     0,
			     0, NULL,
			     0, NULL,
			     1, &barrier);

	cbuf_finish_one_time(device, queue, cpool, cbuf);
}

void cbuf_begin_one_time(VkDevice device,
			 VkCommandPool cpool,
			 VkCommandBuffer *cbuf)
{
	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = cpool;
	alloc_info.commandBufferCount = 1;

	VkResult res = vkAllocateCommandBuffers(device, &alloc_info, cbuf);
	assert(res == VK_SUCCESS);
	
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(*cbuf, &begin_info);	
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

uint32_t find_memory_type(VkPhysicalDeviceMemoryProperties dev_props,
			  VkMemoryRequirements mem_reqs,
			  VkMemoryPropertyFlags req_props) {
	uint32_t type_idx;
	int found = 0;
	for (uint32_t i = 0; i < dev_props.memoryTypeCount; i++) {
		uint32_t cur_props = dev_props.memoryTypes[i].propertyFlags;

		int suitable_for_buffer = mem_reqs.memoryTypeBits & (1 << i);
		int suitable_for_user = (cur_props & req_props) == req_props;

		if (suitable_for_buffer && suitable_for_user) {
			found = 1;
			type_idx = i;
			break;
		}
	}

	assert(found == 1);

	return type_idx;
}
