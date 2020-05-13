#ifndef VK_WINDOW_H_
#define VK_WINDOW_H_

#define SW_FORMAT VK_FORMAT_B8G8R8A8_SRGB

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

/*
 * A struct wrapping a GLFW window and the swapchain associated with it.
 *
 * Handles image acquisition, framebuffer creation, and swapchain recreation.
 */
struct Window {
	// User-provided
	GLFWwindow *gwin;
	VkPhysicalDevice phys_dev;
	VkInstance instance;
	VkDevice device;
	VkSurfaceKHR surface;
	uint32_t queue_fam;
	VkQueue queue;
	VkRenderPass rpass;

	// Created
	VkSwapchainKHR swapchain;
	uint32_t image_ct;
	VkImageView *views;
	VkFramebuffer *fbs;
};

/*
 * Create a Window struct.
 *
 * Creates a swapchain, swapchain image views, and framebuffers (which are
 * stored in the Window struct).
 */
void window_create(GLFWwindow *gwin,
		   VkPhysicalDevice phys_dev,
		   VkInstance instance,
		   VkDevice device,
		   VkSurfaceKHR surface,
		   uint32_t queue_fam,
		   VkQueue queue,
		   VkRenderPass rpass,
		   uint32_t swidth, uint32_t sheight,
		   struct Window *win);

/*
 * Recreates the swapchain stored in the Window struct, using swidth and sheight
 * as the new dimensions
 */
void window_recreate_swapchain(struct Window *win,
			       uint32_t swidth, uint32_t sheight);

/*
 * Acquire a swapchain image.
 *
 * win: Pointer to an existing Window struct (will be modified)
 * sem: Semaphore to signal on image acquisition
 * image_idx: Will store the acquired image index
 * fb: Will store the acquired image's framebuffer
 */
void window_acquire(struct Window *win,
		    VkSemaphore sem,
		    uint32_t *image_idx,
		    VkFramebuffer *fb);

/*
 * Destroys a Window struct and created resources(swapchain, image views,
 * framebuffers)
 */
void window_cleanup(struct Window *win);

/*
 * Helper functions
 */


/*
 * Create a surface.
 */
void create_surface(VkInstance instance,
		    GLFWwindow *win,
		    VkSurfaceKHR *surface);

/*
 * Create a swapchain.
 */
void create_swapchain(VkPhysicalDevice phys_dev,
		      VkDevice device,
		      uint32_t queue_fam,
		      VkSurfaceKHR surface,
		      VkSwapchainKHR *swapchain,
		      uint32_t width, uint32_t height);

/*
 * Create the image views for a swapchain.
 *
 * Does not allocate its own memory.
 * First, call with image_views as NULL, which will set image_view_ct.
 * Then call with a properly sizes image_views.
 */
void create_swapchain_image_views(VkDevice device,
				  VkSwapchainKHR swapchain,
				  uint32_t *image_view_ct,
				  VkImageView *image_views);

/*
 * Create a framebuffer given a render pass and image view.
 */
void create_framebuffer(VkDevice device,
			uint32_t width, uint32_t height,
			VkRenderPass rpass,
			VkImageView image_view,
			VkFramebuffer *fb);

/*
 * Get the dimensions of a surface.
 */
void get_dims(VkPhysicalDevice phys_dev,
	      VkSurfaceKHR surface,
	      uint32_t *width, uint32_t *height);

#endif // VK_WINDOW_H_
