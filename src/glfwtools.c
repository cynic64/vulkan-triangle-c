#include "glfwtools.h"

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

GLFWwindow *init_glfw() {
    glfwInit();
    glfwSetErrorCallback(glfw_error_callback);

    // don't create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

void glfw_cleanup(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void glfw_error_callback(int code, const char *string) {
    printf("GLFW Error: %s\n", string);
}
