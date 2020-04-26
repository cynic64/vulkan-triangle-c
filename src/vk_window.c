#include "vk_window.h"

#include <stdio.h>
#include <assert.h>

void create_surface(VkInstance instance, GLFWwindow *window, VkSurfaceKHR *surface) {
    VkResult res = glfwCreateWindowSurface(instance, window, NULL, surface);
    assert(res == VK_SUCCESS);
}
