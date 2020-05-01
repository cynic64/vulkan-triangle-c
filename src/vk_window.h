#ifndef VK_WINDOW_H_
#define VK_WINDOW_H_

#define SW_FORMAT VK_FORMAT_B8G8R8A8_SRGB

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct Window {
    // provided
    GLFWwindow *gwin;
    VkPhysicalDevice phys_dev;
    VkInstance instance;
    VkDevice device;
    VkSurfaceKHR surface;
    uint32_t queue_fam;
    VkQueue queue;
    VkRenderPass rpass;

    // created
    VkSwapchainKHR swapchain;
    uint32_t image_ct;
    VkImageView *views;
    VkFramebuffer *fbs;
};

void window_create(
    GLFWwindow *gwin,
    VkPhysicalDevice phys_dev,
    VkInstance instance,
    VkDevice device,
    VkSurfaceKHR surface,
    uint32_t queue_fam,
    VkQueue queue,
    VkRenderPass rpass,
    uint32_t swidth,
    uint32_t sheight,
    struct Window *win
);

void window_recreate_swapchain(
    struct Window *win,
    uint32_t swidth,
    uint32_t sheight
);

void window_acquire(
    struct Window *win,
    VkSemaphore sem,
    uint32_t *image_idx,
    VkFramebuffer *fb
);

void create_surface(
    VkInstance instance,
    GLFWwindow *win,
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

void get_dims(
    VkPhysicalDevice phys_dev,
    VkSurfaceKHR surface,
    uint32_t *width,
    uint32_t *height
);

#endif // VK_WINDOW_H_
