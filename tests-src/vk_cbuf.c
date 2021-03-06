#include <stdlib.h>
#include <stdio.h>

#include <check.h>
#include <vulkan/vulkan.h>

#include "../src/vk_tools.h"
#include "../src/glfwtools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_buffer.h"
#include "../src/vk_vertex.h"

#include "helpers.h"

START_TEST (ut_create_cpool)
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

	VkCommandPool cpool = NULL;
	create_cpool(device, queue_fam, &cpool);
	ck_assert(cpool != NULL);

	// test it by trying to create a command buffer
	VkCommandBufferAllocateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = cpool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;

	VkCommandBuffer cbuf = NULL;
	VkResult res = vkAllocateCommandBuffers(device, &info, &cbuf);
	ck_assert(cbuf != NULL);
	ck_assert(res == VK_SUCCESS);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_create_cbuf)
{
	VK_OBJECTS;
	helper_window_create(&gwin,
			     &dbg_msg_ct, NULL,
			     &instance,
			     &phys_dev,
			     &queue_fam,
			     &device,
			     &queue,
			     &surface,
			     &swidth, &sheight,
			     &win);

	helper_create_pipel(device,
			    win.rpass,
			    VERTEX_3_POS_COLOR_BINDING_CT, VERTEX_3_POS_COLOR_BINDINGS,
			    VERTEX_3_POS_COLOR_ATTRIBUTE_CT, VERTEX_3_POS_COLOR_ATTRIBUTES,
			    "assets/testing/shaders/simple.vert.spv",
			    "assets/testing/shaders/simple.frag.spv",
			    &pipe_layout, &pipel);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);

	VkBuffer vbuf, ibuf;
	helper_create_bufs(phys_dev, device, &vbuf, &ibuf);

	VkCommandBuffer cbuf = NULL;
	create_cbuf(device,
		    cpool,
		    win.rpass,
		    win.fbs[0],
		    swidth, sheight,
		    NULL, pipel,
		    0, NULL,
		    vbuf, ibuf, 3,
		    &cbuf);
	ck_assert(cbuf != NULL);

	// Try submitting it
	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = NULL;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cbuf;
	submit_info.signalSemaphoreCount = 0;

	VkResult res = vkQueueSubmit(queue, 1, &submit_info, NULL);
	ck_assert(res == VK_SUCCESS);

	vkQueueWaitIdle(queue);
	ck_assert(dbg_msg_ct == 0);
} END_TEST

START_TEST (ut_cbuf_begin_one_time)
{
	VK_OBJECTS;
	helper_get_queue(&gwin,
			 &dbg_msg_ct,
			 NULL,
			 &instance,
			 &phys_dev,
			 &queue_fam,
			 &device,
			 &queue);

	VkCommandPool cpool;
	create_cpool(device, queue_fam, &cpool);
	
	VkCommandBuffer cbuf;
	cbuf_begin_one_time(device, cpool, &cbuf);

	// We should now be able to end the command buffer without any problems
	VkResult res = vkEndCommandBuffer(cbuf);
	ck_assert(res == VK_SUCCESS);

	// And also free it without problems
	res = vkQueueWaitIdle(queue);
	ck_assert(res == VK_SUCCESS);

	vkFreeCommandBuffers(device, cpool, 1, &cbuf);

	ck_assert(dbg_msg_ct == 0);
} END_TEST

Suite *vk_cbuf_suite(void) {
	Suite *s;

	s = suite_create("Command buffer creation");

	TCase *tc1 = tcase_create("Create command pool");
	tcase_add_test(tc1, ut_create_cpool);
	suite_add_tcase(s, tc1);

	TCase *tc2 = tcase_create("Create command buffer");
	tcase_add_test(tc2, ut_create_cbuf);
	suite_add_tcase(s, tc2);

	TCase *tc3 = tcase_create("Begin one-time command buffer");
	tcase_add_test(tc3, ut_cbuf_begin_one_time);
	suite_add_tcase(s, tc3);

	return s;
}
