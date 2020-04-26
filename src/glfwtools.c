#include "glfwtools.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

GLFWwindow *init_glfw() {
    glfwInit();

    // don't create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

void glfw_cleanup(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

