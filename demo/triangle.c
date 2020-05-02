#include "../src/glfwtools.h"
#include "../src/vk_tools.h"
#include "../src/vk_window.h"
#include "../src/vk_pipe.h"
#include "../src/vk_cbuf.h"
#include "../src/vk_sync.h"

#include <stdlib.h>
#include <assert.h>
#include <time.h>

// returns the elapsed time in floating-point seconds
double get_elapsed(struct timespec *s_time);

int main() {
    // initialize GLFW
    GLFWwindow *gwin = init_glfw();

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

    // timing
    struct timespec s_time;
    clock_gettime(CLOCK_MONOTONIC, &s_time);
    int f_count = 0;

    // loop
    while (!glfwWindowShouldClose(gwin)) {
        VkResult res;
        glfwPollEvents();

        // acquire image
        uint32_t image_idx;
        VkFramebuffer fb;
        window_acquire(&win, image_avail_sem, &image_idx, &fb);

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
        present_info.pSwapchains = &win.swapchain;
        present_info.pImageIndices = &image_idx;

        res = VK_ERROR_UNKNOWN;
        res = vkQueuePresentKHR(queue, &present_info);

        // maybe recreate
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            get_dims(phys_dev, surface, &swidth, &sheight);
            window_recreate_swapchain(&win, swidth, sheight);
        } else {
            assert(res == VK_SUCCESS);
        }

        // wait idle
        res = VK_ERROR_UNKNOWN;
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
    vkDestroySemaphore(device, image_avail_sem, NULL);
    vkDestroySemaphore(device, render_done_sem, NULL);

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
