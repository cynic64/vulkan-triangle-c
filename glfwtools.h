#ifndef GLFWTOOLS_H_
#define GLFWTOOLS_H_

#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

GLFWwindow *init_glfw();

void glfw_cleanup(GLFWwindow *window);

#endif // GLFWTOOLS_H_
