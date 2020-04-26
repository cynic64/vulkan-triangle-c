#include "../src/glfwtools.h"
#include "../src/vktools.h"

int main() {
    // initialize GLFW
    GLFWwindow *window = init_glfw();

    // create instance
    VkInstance instance;
    create_instance(&instance, default_debug_callback);

    // set up debug messenger
    init_debug(&instance, default_debug_callback);

    // get physical device
    VkPhysicalDevice phys_dev;
    get_physical_device(instance, &phys_dev);

    // get queue family
    uint32_t queue_fam = get_queue_fam(phys_dev);

    // create device
    VkDevice device;
    create_device(&instance, phys_dev, queue_fam, &device);

    // get queue
    VkQueue queue;
    get_queue(device, queue_fam, &queue);

    // loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroyInstance(instance, NULL);
    glfw_cleanup(window);

    return 0;
}