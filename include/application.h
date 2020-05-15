#ifndef APPLICATION_H
#define APPLICATION_H

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <optional>

struct QueueFamilyIndices {
    std::optional<unsigned int> graphics_family;

    bool is_complete() { return graphics_family.has_value(); }
};

class Application
{
  public:
    Application()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr);
    }

    void run()
    {
        initVulkan();
        mainLoop();
    }

    ~Application()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

  private:
    const unsigned int width = 800;
    const unsigned int height = 600;
    GLFWwindow *window;

    vk::UniqueInstance instance;
    vk::DispatchLoaderDynamic dldy;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debug_messenger;

    vk::PhysicalDevice physcial_device;
    vk::UniqueDevice device;

    vk::Queue graphics_queue;

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void mainLoop();
};

#endif
