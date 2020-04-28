#include <stdio.h>
#include <assert.h>

#include "vk_cbuf.h"

void create_cpool(VkDevice device, uint32_t queue_fam, VkCommandPool *cpool) {
    VkCommandPoolCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = queue_fam;

    VkResult res = vkCreateCommandPool(device, &info, NULL, cpool);
    assert(res == VK_SUCCESS);
}

void create_cbuf(
    VkDevice device,
    VkCommandPool cpool,
    VkRenderPass rpass,
    VkFramebuffer fb,
    uint32_t width,
    uint32_t height,
    VkPipeline pipel,
    VkCommandBuffer *cbuf
) {
    VkResult res;

    // allocate
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = cpool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    res = vkAllocateCommandBuffers(device, &alloc_info, cbuf);
    assert(res == VK_SUCCESS);

    // begin
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // make sure we don't get fooled by res being previously set to VK_SUCCESS
    res = VK_ERROR_UNKNOWN;
    res = vkBeginCommandBuffer(*cbuf, &begin_info);
    assert(res == VK_SUCCESS);

    // enter render pass
    VkClearValue clear = {0.0f, 0.0f, 0.0f, 0.0f};

    VkOffset2D render_area_offset = {0};
    render_area_offset.x = 0;
    render_area_offset.y = 0;
    VkExtent2D render_area_extent = {0};
    render_area_extent.width = width;
    render_area_extent.height = height;

    VkRenderPassBeginInfo rpass_info = {0};
    rpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpass_info.renderPass = rpass;
    rpass_info.framebuffer = fb;
    rpass_info.renderArea.offset = render_area_offset;
    rpass_info.renderArea.extent = render_area_extent;
    rpass_info.clearValueCount = 1;
    rpass_info.pClearValues = &clear;

    vkCmdBeginRenderPass(*cbuf, &rpass_info, VK_SUBPASS_CONTENTS_INLINE);

    // set scissors and viewport
    VkViewport viewport = {0};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkOffset2D scissor_offset = {0};
    scissor_offset.x = 0;
    scissor_offset.y = 0;
    VkExtent2D scissor_extent = {0};
    scissor_extent.width = width;
    scissor_extent.height = height;
    VkRect2D scissor = {0};
    scissor.offset = scissor_offset;
    scissor.extent = scissor_extent;

    vkCmdSetViewport(*cbuf, 0, 1, &viewport);
    vkCmdSetScissor(*cbuf, 0, 1, &scissor);

    // draw! :)
    vkCmdBindPipeline(*cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipel);
    vkCmdDraw(*cbuf, 3, 1, 0, 0);

    // finish
    vkCmdEndRenderPass(*cbuf);

    res = VK_ERROR_UNKNOWN;
    res = vkEndCommandBuffer(*cbuf);
    assert(res == VK_SUCCESS);
}
