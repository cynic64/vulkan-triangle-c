#ifndef VK_WINDOW_H_
#define VK_WINDOW_H_

#define SW_FORMAT VK_FORMAT_B8G8R8A8_SRGB

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

void create_surface(
    VkInstance instance,
    GLFWwindow *window,
    VkSurfaceKHR *surface
);

void create_swapchain(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    uint32_t queue_fam,
    VkSurfaceKHR surface,
    VkSwapchainKHR *swapchain,
    uint32_t width,
    uint32_t height
);

// call once to query swapchain image view count, allocate image_views, then
// call again
void create_swapchain_image_views(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t *image_view_ct,
    VkImageView *image_views
);

void create_framebuffer(
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkRenderPass rpass,
    VkImageView image_view,
    VkFramebuffer *fb
);

#endif // VK_WINDOW_H_
