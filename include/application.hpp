#ifndef APPLICATION_H
#define APPLICATION_H

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <optional>

struct QueueFamilyIndices {
    std::optional<unsigned int> graphics_family;
    std::optional<unsigned int> present_family;

    QueueFamilyIndices(const vk::PhysicalDevice &physical_device, const vk::SurfaceKHR &surface)
    {
        unsigned int i = 0;
        for (const auto &queue_family : physical_device.getQueueFamilyProperties()) {
            if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphics_family = i;
            }
            if (physical_device.getSurfaceSupportKHR(i, surface)) {
                present_family = i;
            }

            if (this->is_complete()) {
                break;
            }

            i++;
        }
    }

    bool is_complete() { return graphics_family.has_value() && present_family.has_value(); }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilitites;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;

    SwapChainSupportDetails(const vk::PhysicalDevice &physical_device, const vk::SurfaceKHR &surface)
    {
        capabilitites = physical_device.getSurfaceCapabilitiesKHR(surface);
        formats = physical_device.getSurfaceFormatsKHR(surface);
        present_modes = physical_device.getSurfacePresentModesKHR(surface);
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(vk::Format requested_format, vk::ColorSpaceKHR requested_color_space)
    {
        for (const auto &available_format : formats) {
            if (available_format.format == requested_format && available_format.colorSpace == requested_color_space) {
                return available_format;
            }
        }
        return formats[0];
    }

    vk::PresentModeKHR choosePresentMode(vk::PresentModeKHR requested_present_mode)
    {
        for (const auto &available_present_mode : present_modes) {
            if (available_present_mode == requested_present_mode) {
                return available_present_mode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(GLFWwindow *window)
    {
        if (capabilitites.currentExtent.width != UINT32_MAX) {
            return capabilitites.currentExtent;
        } else {
            int window_width = 0, window_height = 0;
            glfwGetFramebufferSize(window, &window_width, &window_height);
            vk::Extent2D actual_extent = {(uint32_t)window_width, (uint32_t)window_height};

            actual_extent.width = std::max(capabilitites.minImageExtent.width, std::min(capabilitites.maxImageExtent.width, actual_extent.width));
            actual_extent.height = std::max(capabilitites.minImageExtent.height, std::min(capabilitites.maxImageExtent.height, actual_extent.height));

            return actual_extent;
        }
    }
};

class Application
{
  public:
    Application()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
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
    const unsigned int max_frames_in_flight = 2;
    GLFWwindow *window;

    vk::UniqueInstance instance;
    vk::DispatchLoaderDynamic dldy;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debug_messenger;
    vk::UniqueSurfaceKHR surface;

    vk::PhysicalDevice physcial_device;
    vk::UniqueDevice device;

    vk::Queue graphics_queue;
    vk::Queue present_queue;

    vk::UniqueSwapchainKHR swap_chain;
    std::vector<vk::Image> swap_chain_images;
    vk::Format swap_chain_image_format;
    vk::Extent2D swap_chain_extent;
    std::vector<vk::UniqueImageView> swap_chain_image_views;
    std::vector<vk::UniqueFramebuffer> swap_chain_framebuffers;

    vk::UniqueRenderPass render_pass;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniquePipeline graphics_pipeline;

    vk::UniqueCommandPool command_pool;
    std::vector<vk::CommandBuffer> command_buffers;

    std::vector<vk::UniqueSemaphore> image_available_semaphores;
    std::vector<vk::UniqueSemaphore> render_finished_semaphores;
    std::vector<vk::UniqueFence> in_flight_fences;
    std::vector<vk::Fence> images_in_flight;
    size_t current_frame = 0;
    bool framebuffer_resized = false;

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void drawFrame();
    void recreateSwapChain();

    void mainLoop();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }
};

#endif
