#include "vk_window.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void window_create(
    GLFWwindow *gwin,
    VkPhysicalDevice phys_dev,
    VkDevice device,
    VkInstance instance,
    uint32_t queue_fam,
    VkQueue queue,
    VkRenderPass rpass,
    uint32_t swidth,
    uint32_t sheight,
    struct Window *window
) {
}

void window_recreate_swapchain(uint32_t swidth, uint32_t sheight) {
}

void window_acquire(VkSemaphore *sem, uint32_t *image_idx, VkFramebuffer *fb) {
}

void create_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface) {
    VkResult res = glfwCreateWindowSurface(instance, window, NULL, surface);
    assert(res == VK_SUCCESS);
}

void create_swapchain(
    VkPhysicalDevice phys_dev,
    VkDevice device,
    uint32_t queue_fam,
    VkSurfaceKHR surface,
    VkSwapchainKHR *swapchain,
    uint32_t width,
    uint32_t height
) {
    // ensure surface has presentation support
    VkBool32 support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, queue_fam, surface, &support);
    assert(support == VK_TRUE);

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &caps);

    VkExtent2D extent;
    extent.width = width;
    extent.height = height;

    VkSwapchainCreateInfoKHR info = {0};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    info.surface = surface;
    info.minImageCount = caps.minImageCount;
    info.imageFormat = SW_FORMAT;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices = &queue_fam;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;

    VkResult res = vkCreateSwapchainKHR(device, &info, NULL, swapchain);
    assert(res == VK_SUCCESS);
}

// If image_views is NULL, will only output image view count to image_view_ct.
// Otherwise, outputs to both image_view_ct and image_views.
void create_swapchain_image_views(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t *image_view_ct,
    VkImageView *image_views
) {
    VkResult res;

    // query image count
    res =
        vkGetSwapchainImagesKHR(device, swapchain, image_view_ct, NULL);
    assert(res == VK_SUCCESS);

    if (image_views == NULL) return;

    // get images
    VkImage *images = malloc(sizeof(VkImage) * *image_view_ct);
    res =
        vkGetSwapchainImagesKHR(device, swapchain, image_view_ct, images);
    assert(res == VK_SUCCESS);

    for (int i = 0; i < *image_view_ct; i++) {
        VkImageViewCreateInfo info = {0};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = images[i];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = SW_FORMAT;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        res = vkCreateImageView(device, &info, NULL, &image_views[i]);
        assert(res == VK_SUCCESS);
    }
}

void create_framebuffer(
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkRenderPass rpass,
    VkImageView image_view,
    VkFramebuffer *fb
) {
    VkFramebufferCreateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = rpass;
    info.attachmentCount = 1;
    info.pAttachments = &image_view;
    info.width = width;
    info.height = height;
    info.layers = 1;

    VkResult res = vkCreateFramebuffer(device, &info, NULL, fb);
    assert(res == VK_SUCCESS);
}

void get_dims(
    VkPhysicalDevice phys_dev,
    VkSurfaceKHR surface,
    uint32_t *width,
    uint32_t *height
) {
    VkSurfaceCapabilitiesKHR caps;
    VkResult res =
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &caps);
    assert(res == VK_SUCCESS);

    *width = caps.currentExtent.width;
    *height = caps.currentExtent.height;
}
