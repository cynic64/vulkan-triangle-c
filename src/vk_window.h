#ifndef VK_WINDOW_H_
#define VK_WINDOW_H_

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

void create_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface);

#endif // VK_WINDOW_H_
