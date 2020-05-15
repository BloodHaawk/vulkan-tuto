#include "application.h"

#include <iomanip>
#include <iostream>
#include <set>

const std::vector<const char *> validation_layers = {"VK_LAYER_KHRONOS_validation"};
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
    std::set<std::string> required_layers{validation_layers.cbegin(), validation_layers.cend()};
    for (const auto &layer_properties : vk::enumerateInstanceLayerProperties()) {
        required_layers.erase(layer_properties.layerName);
    }
    return required_layers.empty();
}

void Application::createInstance()
{
    auto app_info = vk::ApplicationInfo{
        nullptr,                  // *applicationName
        VK_MAKE_VERSION(1, 0, 0), // applicationVersion
        "No Engine",              // *engineName
        VK_MAKE_VERSION(1, 0, 0), // engineVersion
        VK_API_VERSION_1_0        // apiVersion
    };

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

    auto create_info = vk::InstanceCreateInfo{
        {},                                           // flags
        &app_info,                                    // *applicationInfo
        enabled_layer_count,                          // enabledLayerCount
        enabled_layer_names,                          // **enabledExtensionsNames
        static_cast<unsigned int>(extensions.size()), // enabledExtensionCount
        extensions.data(),                            // **enabledExtensionNames
    };

    instance = vk::createInstanceUnique(create_info);
    dldy.init(*instance, vkGetInstanceProcAddr);
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

QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device)
{
    QueueFamilyIndices indices;

    unsigned int i = 0;
    for (const auto &queue_family : device.getQueueFamilyProperties()) {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool isDeviceSuitable(const vk::PhysicalDevice &device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.is_complete();
}

void Application::pickPhysicalDevice()
{
    auto devices = instance->enumeratePhysicalDevices();
    auto it = std::find_if(
        devices.cbegin(), devices.cend(), [&](const auto &device) { return isDeviceSuitable(device); });
    if (it == devices.cend()) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    physcial_device = *it;
}

void Application::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physcial_device);

    float queue_priority = 1;
    auto queue_create_info = vk::DeviceQueueCreateInfo{
        {},                              // flags
        indices.graphics_family.value(), // queueFamilyIndex,
        1,                               // queueCount
        &queue_priority                  // *queuePriorities
    };

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

    auto device_create_info = vk::DeviceCreateInfo{
        {},                  // flags
        1,                   // queueCreateInfoCount
        &queue_create_info,  // *queueCreateInfos
        enabled_layer_count, // enabledLayerCountr
        enabled_layer_names, // **enabledLayerNames
    };

    device = physcial_device.createDeviceUnique(device_create_info);

    graphics_queue = device->getQueue(indices.graphics_family.value(), 0);
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