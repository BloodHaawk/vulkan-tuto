#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);

    std::cout << "Instance extensions:" << std::endl;
    for (auto const &property : vk::enumerateInstanceExtensionProperties()) {
        std::cout << property.extensionName << ":" << std::endl;
        std::cout << "\tVersion: " << property.specVersion << std::endl;
        std::cout << std::endl;
    }

    glm::mat4 matrix;
    glm::vec4 vector;
    auto test = matrix * vector;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
