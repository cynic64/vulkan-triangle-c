#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"

#include <stdlib.h>
#include <assert.h>

void recreate_swapchain(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    uint32_t queue_fam,
    VkSurfaceKHR surface,
    VkRenderPass rpass,
    VkSwapchainKHR *swapchain,
    uint32_t *sw_image_view_ct,
    VkImageView **sw_image_views,
    VkFramebuffer **fbs
);

int main() {
    // initialize GLFW
    GLFWwindow *window = init_glfw();

    // create instance
    VkInstance instance;
    // NULL is pUserData
    int n = 0;
    create_instance(&instance, default_debug_callback, NULL);

    // set up debug messenger (again, NULL is pUserData)
    VkDebugUtilsMessengerEXT dbg_msgr;
    init_debug(&instance, default_debug_callback, NULL, &dbg_msgr);

    // get physical device
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);

    // get queue family
    uint32_t queue_fam = get_queue_fam(phys_dev);

    // create device
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);

    // get queue
    VkQueue queue;
    get_queue(device, queue_fam, &queue);

    // surface
    VkSurfaceKHR surface;
    create_surface(instance, window, &surface);
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);

    // swapchain
    VkSwapchainKHR swapchain;
    create_swapchain(phys_dev, device, queue_fam, surface, &swapchain, swidth, sheight);

    uint32_t sw_image_view_ct = 0;
    create_swapchain_image_views(device, swapchain, &sw_image_view_ct, NULL);

    printf("Swapchain image count: %d\n", sw_image_view_ct);

    VkImageView *sw_image_views = malloc(sizeof(VkImageView) * sw_image_view_ct);
    create_swapchain_image_views(
        device,
        swapchain,
        &sw_image_view_ct,
        sw_image_views
    );

    // command pool
    VkCommandPool cpool;
    create_cpool(device, queue_fam, &cpool);

    // render pass
    VkRenderPass rpass;
    create_rpass(device, SW_FORMAT, &rpass);

    // framebuffers
    VkFramebuffer *fbs = malloc(sizeof(VkFramebuffer) * sw_image_view_ct);
    for (int i = 0; i < sw_image_view_ct; i++) {
        VkFramebuffer fb = NULL;
        create_framebuffer(
            device,
            swidth,
            sheight,
            rpass,
            sw_image_views[i],
            &fbs[i]
        );
    }

    // pipeline layout
    VkPipelineLayout layout;
    create_layout(device, &layout);

    // shaders
    FILE *fp;
    size_t vs_size, fs_size;
    char *vs_buf, *fs_buf;

    // vs
    fp = fopen("assets/shaders/triangle/main.vert.spv", "rb");
    assert(fp != NULL);

    read_bin(fp, &vs_size, NULL);
    vs_buf = malloc(vs_size);
    read_bin(fp, &vs_size, vs_buf);
    fclose(fp);

    VkShaderModule vs_mod;
    create_shmod(device, vs_size, vs_buf, &vs_mod);

    // fs
    fp = NULL;
    fp = fopen("assets/shaders/triangle/main.frag.spv", "rb");
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
    create_pipel(
        device,
        2,
        shtages,
        layout,
        rpass,
        &pipel
    );

    // cleanup shader modules
    vkDestroyShaderModule(device, vs_mod, NULL);
    vkDestroyShaderModule(device, fs_mod, NULL);

    // semaphores
    VkSemaphore image_avail_sem;
    VkSemaphore render_done_sem;
    create_sem(device, &image_avail_sem);
    create_sem(device, &render_done_sem);

    // loop
    while (!glfwWindowShouldClose(window)) {
        VkResult res;
        glfwPollEvents();

        // acquire image
        uint32_t image_idx;
        res = vkAcquireNextImageKHR(
            device,
            swapchain,
            UINT64_MAX,
            image_avail_sem,
            NULL,
            &image_idx
        );

        // choose framebuffer
        VkFramebuffer fb = fbs[image_idx];

        // create command buffer
        VkCommandBuffer cbuf;
        create_cbuf(
            device,
            cpool,
            rpass,
            fb,
            swidth,
            sheight,
            pipel,
            &cbuf
        );

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

        res = VK_ERROR_UNKNOWN;
        res = vkQueueSubmit(queue, 1, &submit_info, NULL);
        assert(res == VK_SUCCESS);

        // present
        VkPresentInfoKHR present_info = {0};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_sems;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_idx;

        res = VK_ERROR_UNKNOWN;
        res = vkQueuePresentKHR(queue, &present_info);

        // maybe recreate
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(
                phys_dev,
                device,
                queue_fam,
                surface,
                rpass,
                &swapchain,
                &sw_image_view_ct,
                &sw_image_views,
                &fbs
            );

            get_dims(phys_dev, surface, &swidth, &sheight);
        } else assert(res == VK_SUCCESS);

        // wait idle
        res = VK_ERROR_UNKNOWN;
        res = vkQueueWaitIdle(queue);
        assert(res == VK_SUCCESS);

        // free command buffer
        vkFreeCommandBuffers(device, cpool, 1, &cbuf);
    }

    // cleanup
    for (int i = 0; i < sw_image_view_ct; i++) {
        vkDestroyImageView(device, sw_image_views[i], NULL);
        vkDestroyFramebuffer(device, fbs[i], NULL);
    }

    vkDestroyPipeline(device, pipel, NULL);
    vkDestroyPipelineLayout(device, layout, NULL);
    vkDestroySemaphore(device, image_avail_sem, NULL);
    vkDestroySemaphore(device, render_done_sem, NULL);

    vkDestroyCommandPool(device, cpool, NULL);
    vkDestroyRenderPass(device, rpass, NULL);

    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);

    vkDestroyDevice(device, NULL);
    destroy_dbg_msgr(instance, &dbg_msgr);
    vkDestroyInstance(instance, NULL);

    glfw_cleanup(window);

    return 0;
}

void recreate_swapchain(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    uint32_t queue_fam,
    VkSurfaceKHR surface,
    VkRenderPass rpass,
    VkSwapchainKHR *swapchain,
    uint32_t *sw_image_view_ct,
    VkImageView **sw_image_views,
    VkFramebuffer **fbs
) {
    vkDeviceWaitIdle(device);

    // get new dimensions
    uint32_t swidth, sheight;
    get_dims(phys_dev, surface, &swidth, &sheight);

    // recreate swapchain
    create_swapchain(
        phys_dev,
        device,
        queue_fam,
        surface,
        swapchain,
        swidth,
        sheight
    );

    // recreate swapchain image views
    // first destroy old image views
    assert(*sw_image_views != NULL);

    for (int i = 0; i < *sw_image_view_ct; i++)
        vkDestroyImageView(device, (*sw_image_views)[i], NULL);

    // get new image count and allocate
    create_swapchain_image_views(device, *swapchain, sw_image_view_ct, NULL);
    free(*sw_image_views);
    *sw_image_views = malloc(sizeof(VkImageView) * *sw_image_view_ct);

    // now actually create the IVs
    create_swapchain_image_views(
        device,
        *swapchain,
        sw_image_view_ct,
        *sw_image_views
    );

    // recreate framebuffers
    // first destroy the old ones
    assert(*fbs != NULL);
    for (int i = 0; i < *sw_image_view_ct; i++)
        vkDestroyFramebuffer(device, (*fbs)[i], NULL);

    free(*fbs);
    *fbs = malloc(sizeof(VkFramebuffer) * *sw_image_view_ct);

    for (int i = 0; i < *sw_image_view_ct; i++) {
        create_framebuffer(
            device,
            swidth,
            sheight,
            rpass,
            (*sw_image_views)[i],
            &(*fbs)[i]
        );
    }
}
