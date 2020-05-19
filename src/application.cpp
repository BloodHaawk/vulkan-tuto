#include "application.hpp"

#include <iomanip>
#include <iostream>
#include <set>

const std::vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef NDEBUG
const bool enabled_validation_layers = false;
#else
const bool enabled_validation_layers = true;
#endif

std::vector<const char *> getRequiredExtensions()
{
    unsigned int glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool checkValidationLayerSupport()
{
    auto required_layers = std::set<std::string>(validation_layers.cbegin(), validation_layers.cend());
    for (const auto &layer_properties : vk::enumerateInstanceLayerProperties()) {
        required_layers.erase(layer_properties.layerName);
    }
    return required_layers.empty();
}

void Application::createInstance()
{
    auto app_info = vk::ApplicationInfo(
        nullptr,                  // *applicationName
        VK_MAKE_VERSION(1, 0, 0), // applicationVersion
        "No Engine",              // *engineName
        VK_MAKE_VERSION(1, 0, 0), // engineVersion
        VK_API_VERSION_1_0        // apiVersion
    );

    if (enabled_validation_layers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    unsigned int enabled_layer_count = 0;
    const char *const *enabled_layer_names;
    if (enabled_validation_layers) {
        enabled_layer_count = validation_layers.size();
        enabled_layer_names = validation_layers.data();
    } else {
        enabled_layer_count = 0;
        enabled_layer_names = nullptr;
    }

    auto extensions = getRequiredExtensions();

    auto create_info = vk::InstanceCreateInfo(
        {},                                           // flags
        &app_info,                                    // *applicationInfo
        enabled_layer_count,                          // enabledLayerCount
        enabled_layer_names,                          // **enabledExtensionsNames
        static_cast<unsigned int>(extensions.size()), // enabledExtensionCount
        extensions.data()                             // **enabledExtensionNames
    );

    instance = vk::createInstanceUnique(create_info);
    dldy.init(*instance);
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
              const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void * /*pUserData*/)
{
    std::cerr << "validation layer: (severity: 0x" << std::setfill('0') << std::setw(4) << std::hex
              << message_severity << ") " << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

void Application::setupDebugMessenger()
{
    if (enabled_validation_layers) {
        auto message_severity =
            // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

        auto message_type =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

        auto create_info = vk::DebugUtilsMessengerCreateInfoEXT{
            {},               // flags
            message_severity, // messageSeverity
            message_type,     // messageType
            debugCallback     // debugCallback
        };

        debug_messenger = instance->createDebugUtilsMessengerEXTUnique(create_info, nullptr, dldy);
    }
}

void Application::createSurface()
{
    VkSurfaceKHR surface_tmp;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface_tmp) != VK_SUCCESS) {
        std::runtime_error("Could not create window surface!");
    }
    // Set the instance as the allocator for correct order of destruction
    surface = vk::UniqueSurfaceKHR(surface_tmp, *instance);
}

bool checkDeviceExtensionSupport(const vk::PhysicalDevice &device)
{
    auto required_extensions = std::set<std::string>(device_extensions.cbegin(), device_extensions.cend());
    for (const auto &extension : device.enumerateDeviceExtensionProperties()) {
        required_extensions.erase(extension.extensionName);
    }
    return required_extensions.empty();
}

bool isDeviceSuitable(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    auto indices = QueueFamilyIndices(device, surface);

    if (indices.is_complete() && checkDeviceExtensionSupport(device)) {
        auto swap_chain_support = SwapChainSupportDetails(device, surface);
        return !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    return false;
}

void Application::pickPhysicalDevice(const vk::SurfaceKHR &surface)
{
    auto devices = instance->enumeratePhysicalDevices();
    auto it = std::find_if(
        devices.cbegin(), devices.cend(), [&](const auto &device) { return isDeviceSuitable(device, surface); });
    if (it == devices.cend()) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    physcial_device = *it;
}

void Application::createLogicalDevice()
{
    auto indices = QueueFamilyIndices(physcial_device, *surface);

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::set<unsigned int> unique_queue_families = {indices.graphics_family.value(),
                                                    indices.present_family.value()};

    float queue_priority = 1;
    queue_create_infos.reserve(unique_queue_families.size());

    for (auto queue_family : unique_queue_families) {
        auto queue_create_info = vk::DeviceQueueCreateInfo(
            {},             // flags
            queue_family,   // queueFamilyIndex
            1,              // queueCount
            &queue_priority // *queuePriority
        );
        queue_create_infos.push_back(queue_create_info);
    }

    unsigned int enabled_layer_count = 0;
    const char *const *enabled_layer_names;
    if (enabled_validation_layers) {
        enabled_layer_count = validation_layers.size();
        enabled_layer_names = validation_layers.data();
    } else {
        enabled_layer_count = 0;
        enabled_layer_names = nullptr;
    }

    auto extensions = getRequiredExtensions();

    auto device_create_info = vk::DeviceCreateInfo(
        {},                                                   // flags
        static_cast<unsigned int>(queue_create_infos.size()), // queueCreateInfoCount
        queue_create_infos.data(),                            // *queueCreateInfos
        enabled_layer_count,                                  // enabledLayerCount
        enabled_layer_names,                                  // **enabledLayerNames
        static_cast<unsigned int>(device_extensions.size()),  // enabledExtensionCount
        device_extensions.data()                              // **enabledExtensionNames
    );

    device = physcial_device.createDeviceUnique(device_create_info);

    graphics_queue = device->getQueue(indices.graphics_family.value(), 0);
    present_queue = device->getQueue(indices.present_family.value(), 0);
}

void Application::createSwapChain()
{
    auto swap_chain_support = SwapChainSupportDetails(physcial_device, *surface);
    auto surface_format = swap_chain_support.chooseSwapSurfaceFormat(
        vk::Format::eB8G8R8Srgb,          // requested_format
        vk::ColorSpaceKHR::eSrgbNonlinear // requested_color_space
    );
    auto present_mode = swap_chain_support.choosePresentMode(
        vk::PresentModeKHR::eMailbox // requested_present_mode
    );
    auto extent = swap_chain_support.chooseSwapExtent(width, height);

    unsigned int image_count = swap_chain_support.capabilitites.minImageCount + 1;
    if (swap_chain_support.capabilitites.maxImageCount > 0 && image_count > swap_chain_support.capabilitites.maxImageCount) {
        image_count = swap_chain_support.capabilitites.maxImageCount;
    }

    auto indices = QueueFamilyIndices(physcial_device, *surface);
    auto image_sharing_mode = vk::SharingMode::eExclusive;
    uint32_t queue_family_index_count = 0;
    uint32_t *queue_family_indices = nullptr;
    uint32_t family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family != indices.present_family) {
        image_sharing_mode = vk::SharingMode::eConcurrent;
        queue_family_index_count = 2;
        queue_family_indices = family_indices;
    }

    auto swap_chain_create_info = vk::SwapchainCreateInfoKHR(
        {},                                                // flags
        *surface,                                          // surface
        image_count,                                       // minImageCount
        surface_format.format,                             // imageFormat
        surface_format.colorSpace,                         // imageColorSpace
        extent,                                            // imageExtent
        1,                                                 // imageArrayLayers
        vk::ImageUsageFlagBits::eColorAttachment,          // imageUsage
        image_sharing_mode,                                // imageSharingMode
        queue_family_index_count,                          // queueFamilyIndexCount
        queue_family_indices,                              // *queueFamilyIndices
        swap_chain_support.capabilitites.currentTransform, // preTransform
        vk::CompositeAlphaFlagBitsKHR::eOpaque,            // compositeAlpha
        present_mode,                                      // presentMode
        VK_TRUE,                                           // clipped
        nullptr                                            // oldSwapChain
    );

    swap_chain = device->createSwapchainKHRUnique(swap_chain_create_info);
    swap_chain_images = device->getSwapchainImagesKHR(*swap_chain);
    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
}

void Application::mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
        glfwPollEvents();
    }
}
