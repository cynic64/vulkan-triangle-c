#include "vk_window.h"
#include "vk_image.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

void window_create(GLFWwindow *gwin,
		   VkPhysicalDevice phys_dev,
		   VkInstance instance,
		   VkDevice device,
		   VkSurfaceKHR surface,
		   uint32_t queue_fam,
		   VkQueue queue,
		   VkRenderPass rpass,
		   uint32_t extra_view_ct, VkImageView *extra_views,
		   uint32_t swidth, uint32_t sheight,
		   struct Window *win)
{
	// Created resources are first assigned to local variables, then the
	// outputs are set later (because I find it more readable)

	// Swapchain
	VkSwapchainKHR swapchain;
	create_swapchain(phys_dev,
			 device,
			 queue_fam,
			 surface,
			 &swapchain,
			 swidth, sheight);

	// Image views
	uint32_t image_ct;
	create_swapchain_image_views(device, swapchain, &image_ct, NULL);

	VkImageView *views = malloc(sizeof(VkImageView) * image_ct);
	create_swapchain_image_views(device, swapchain, &image_ct, views);

	// Framebuffers
	uint32_t all_view_ct = 1 + extra_view_ct;

	VkFramebuffer *fbs = malloc(sizeof(VkFramebuffer) * image_ct);
	for (int i = 0; i < image_ct; i++) {
		VkImageView *all_views =
			concat_image_views(1, &views[i],
					   extra_view_ct, extra_views);
		
		create_framebuffer(device,
				   swidth, sheight,
				   rpass,
				   all_view_ct, all_views,
				   &fbs[i]);

		free(all_views);
	}

	// Assign
	win->gwin = gwin;
	win->phys_dev = phys_dev;
	win->instance = instance;
	win->device = device;
	win->surface = surface;
	win->queue_fam = queue_fam;
	win->queue = queue;
	win->rpass = rpass;

	win->swapchain = swapchain;
	win->image_ct = image_ct;
	win->views = views;
	win->fbs = fbs;
}

void window_recreate_swapchain(struct Window *win,
			       uint32_t extra_view_ct, VkImageView *extra_views,
			       uint32_t swidth, uint32_t sheight)
{
	vkDeviceWaitIdle(win->device);

	// Recreate swapchain
	create_swapchain(win->phys_dev,
			 win->device,
			 win->queue_fam,
			 win->surface,
			 &win->swapchain,
			 swidth, sheight);

	// Recreate swapchain image views
	// First destroy old image views
	assert(win->views != NULL);

	for (int i = 0; i < win->image_ct; i++) {
		vkDestroyImageView(win->device, win->views[i], NULL);
	}

	free(win->views);

	// Get new image count and allocate
	create_swapchain_image_views(win->device,
				     win->swapchain,
				     &win->image_ct,
				     NULL);
	win->views = malloc(sizeof(VkImageView) * win->image_ct);

	// Now actually create the views
	create_swapchain_image_views(win->device,
				     win->swapchain,
				     &win->image_ct,
				     win->views);

	// Recreate framebuffers
	// First destroy the old ones
	assert(win->fbs != NULL);

	for (int i = 0; i < win->image_ct; i++) {
		vkDestroyFramebuffer(win->device, win->fbs[i], NULL);
	}

	free(win->fbs);

	// Create new framebuffers
	uint32_t all_view_ct = 1 + extra_view_ct;
	
	win->fbs = malloc(sizeof(VkFramebuffer) * win->image_ct);

	for (int i = 0; i < win->image_ct; i++) {
		VkImageView *all_views
			= concat_image_views(1, &win->views[i],
					     extra_view_ct, extra_views);

		create_framebuffer(win->device,
				   swidth,
				   sheight,
				   win->rpass,
				   all_view_ct, all_views,
				   &win->fbs[i]);
		free(all_views);
	}
}

int window_acquire(struct Window *win,
		   VkSemaphore sem,
		   uint32_t *image_idx,
		   uint32_t extra_view_ct, VkImageView *extra_views,
		   VkFramebuffer *fb)

{
	int was_recreated = 0;

	while (1) {
		VkResult res = vkAcquireNextImageKHR(win->device,
						     win->swapchain,
						     UINT64_MAX,
						     sem,
						     NULL,
						     image_idx);

		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			printf("Recr\n");
			uint32_t swidth, sheight;
			get_dims(win->phys_dev, win->surface,
				 &swidth, &sheight);

			window_recreate_swapchain(win,
						  extra_view_ct, extra_views,
						  swidth, sheight);

			was_recreated = 1;

			continue;
		}

		assert(res == VK_SUCCESS);
		*fb = win->fbs[*image_idx];

		return was_recreated;
	}
}

void window_cleanup(struct Window *win)
{
	for (int i = 0; i < win->image_ct; i++) {
		vkDestroyFramebuffer(win->device, win->fbs[i], NULL);
		vkDestroyImageView(win->device, win->views[i], NULL);
	}

	vkDestroySwapchainKHR(win->device, win->swapchain, NULL);
}

void create_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface)
{
	VkResult res = glfwCreateWindowSurface(instance, window, NULL, surface);
	assert(res == VK_SUCCESS);
}

void create_swapchain(VkPhysicalDevice phys_dev,
		      VkDevice device,
		      uint32_t queue_fam,
		      VkSurfaceKHR surface,
		      VkSwapchainKHR *swapchain,
		      uint32_t width, uint32_t height)
{
	// Ensure surface has presentation support
	VkBool32 support = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev,
					     queue_fam,surface, &support);
	assert(support == VK_TRUE);

	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &caps);

	VkExtent2D extent;
	extent.width = width;
	extent.height = height;

	VkSwapchainCreateInfoKHR info = {0};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
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

void create_swapchain_image_views(VkDevice device,
				  VkSwapchainKHR swapchain,
				  uint32_t *image_view_ct,
				  VkImageView *image_views)
{
	VkResult res;

	// Query image count
	res = vkGetSwapchainImagesKHR(device, swapchain, image_view_ct, NULL);
	assert(res == VK_SUCCESS);

	if (image_views == NULL) return;

	// Get images
	VkImage *images = malloc(sizeof(VkImage) * *image_view_ct);
	res = vkGetSwapchainImagesKHR(device, swapchain, image_view_ct, images);
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

void create_framebuffer(VkDevice device,
			uint32_t width, uint32_t height,
			VkRenderPass rpass,
			uint32_t view_ct, VkImageView *views,
			VkFramebuffer *fb)
{
	VkFramebufferCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = rpass;
	info.attachmentCount = view_ct;
	info.pAttachments = views;
	info.width = width;
	info.height = height;
	info.layers = 1;

	VkResult res = vkCreateFramebuffer(device, &info, NULL, fb);
	assert(res == VK_SUCCESS);
}

void get_dims(VkPhysicalDevice phys_dev,
	      VkSurfaceKHR surface,
	      uint32_t *width, uint32_t *height)
{
	VkSurfaceCapabilitiesKHR caps;
	VkResult res =
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev,
							  surface, &caps);
	assert(res == VK_SUCCESS);

	*width = caps.currentExtent.width;
	*height = caps.currentExtent.height;
}

VkImageView *concat_image_views(uint32_t a_ct, VkImageView *a_views,
				uint32_t b_ct, VkImageView *b_views)
{
	uint32_t total_ct = a_ct + b_ct;
	VkImageView *out = malloc(sizeof(out[0]) * total_ct);

	for (int i = 0; i < a_ct; i++) {
		out[i] = a_views[i];
	}

	for (int i = 0; i < b_ct; i++) {
		out[i + a_ct] = b_views[i];
	}

	return out;
}






