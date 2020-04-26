#include "vk_window.h"

#include <stdio.h>
#include <assert.h>

void create_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface) {
    VkResult res = glfwCreateWindowSurface(instance, window, NULL, surface);
    assert(res == VK_SUCCESS);
}

void create_swapchain(
    VkDevice device,
    VkSwapchainKHR *swapchain,
    uint32_t width,
    uint32_t height
) {
}

// call once to query swapchain image view count, allocate image_views, then
// call again
void create_swapchain_image_views(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t *image_view_ct,
    VkImageView *image_views
) {
}
