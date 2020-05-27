#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"
#include "../src/vk_image.h"
#include "../src/vk_vertex.h"
#include "../src/vk_buffer.h"

#include "helpers.h"

#define IM_WIDTH 1920
#define IM_HEIGHT 1080

START_TEST(ut_triangle_windowless)
{
	VK_OBJECTS;
	helper_get_queue(NULL,
			 &dbg_msg_ct,
			 NULL,
			 &instance,
			 &phys_dev,
			 &queue_fam,
			 &device,
			 &queue);

	VkPhysicalDeviceMemoryProperties dev_mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &dev_mem_props);

	struct Image image;
	image_create(device,
		     queue_fam,
		     dev_mem_props,
		     DEFAULT_FMT,
		     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     VK_IMAGE_ASPECT_COLOR_BIT,
		     IM_WIDTH, IM_HEIGHT,
		     &image);

	create_rpass(device, DEFAULT_FMT, &rpass);

	VkFramebuffer fb;
	create_framebuffer(device, IM_WIDTH, IM_HEIGHT, rpass, image.view, &fb);

	VkBuffer vbuf, ibuf;
	helper_create_bufs(phys_dev, device, &vbuf, &ibuf);

	VkPipelineShaderStageCreateInfo shader_stages[2];
	helper_create_shtage(device, "assets/testing/shaders/simple.vert.spv",
			     VK_SHADER_STAGE_VERTEX_BIT, &shader_stages[0]);
	helper_create_shtage(device, "assets/testing/shaders/simple.frag.spv",
			     VK_SHADER_STAGE_FRAGMENT_BIT, &shader_stages[1]);

	VkPipelineLayout layout;
	create_layout(device, 0, NULL, &layout);

	create_pipel(device, 2, shader_stages, layout,
		     VERTEX_3_POS_COLOR_BINDING_CT, VERTEX_3_POS_COLOR_BINDINGS,
		     VERTEX_3_POS_COLOR_ATTRIBUTE_CT, VERTEX_3_POS_COLOR_ATTRIBUTES,
		     rpass,
		     &pipel);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	VkCommandBuffer cbuf;
	create_cbuf(device, cpool, rpass, fb, IM_WIDTH, IM_HEIGHT, layout,
		    pipel, 0, NULL, vbuf, ibuf, 3, &cbuf);

	submit_syncless(device, queue, cpool, cbuf);

	// Copy to buffer
	image_transition(device, queue, cpool,
			 image.handle, VK_IMAGE_ASPECT_COLOR_BIT,
			 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			 VK_ACCESS_TRANSFER_READ_BIT,
			 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			 VK_PIPELINE_STAGE_TRANSFER_BIT,
			 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	struct Buffer buf;
	buffer_create(device, dev_mem_props,
		      4 * IM_WIDTH * IM_HEIGHT,
		      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		      &buf);

	image_copy_to_buffer(device, queue, cpool,
			     VK_IMAGE_ASPECT_COLOR_BIT, IM_WIDTH, IM_HEIGHT,
			     image.handle, buf.handle);

	// View
	const uint32_t printed_w = 80;
	const uint32_t printed_h = 24;
	char image_string[printed_w * printed_h];

	vk_mem_to_string(device,
			 1920, 1080,
			 printed_w, printed_h,
			 buf.memory,
			 image_string);

	char known_string[] =
		"                                                                               \n"
		"                                       ###                                     \n"
		"                                     #######                                   \n"
		"                                   ##########                                  \n"
		"                                  #############                                \n"
		"                                #################                              \n"
		"                              ####################                             \n"
		"                             #######################                           \n"
		"                           ###########################                         \n"
		"                         ##############################                        \n"
		"                        #################################                      \n"
		"                      #####################################                    \n"
		"                    ########################################                   \n"
		"                   ###########################################                 \n"
		"                 ###############################################               \n"
		"               ##################################################              \n"
		"              #####################################################            \n"
		"            #########################################################          \n"
		"          ############################################################         \n"
		"         ###############################################################       \n"
		"       ###################################################################     \n"
		"     ######################################################################    \n"
		"    #########################################################################  \n"
		"  ############################################################################\n";

	ck_assert(strcmp(image_string, known_string) == 0);

	ck_assert(dbg_msg_ct == 0);
}

Suite *vk_fullstack_suite(void) {
	Suite *s;

	s = suite_create("Full-stack tests");

	TCase *tc1 = tcase_create("Windowless triangle");
	tcase_add_test(tc1, ut_triangle_windowless);
	suite_add_tcase(s, tc1);

	return s;
}
