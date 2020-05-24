#include "application.hpp"

#include <fstream>
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

bool checkDeviceExtensionSupport(const vk::PhysicalDevice &physical_device)
{
    auto required_extensions = std::set<std::string>(device_extensions.cbegin(), device_extensions.cend());
    for (const auto &extension : physical_device.enumerateDeviceExtensionProperties()) {
        required_extensions.erase(extension.extensionName);
    }
    return required_extensions.empty();
}

bool isDeviceSuitable(const vk::PhysicalDevice &physical_device, const vk::SurfaceKHR &surface)
{
    auto indices = QueueFamilyIndices(physical_device, surface);

    if (indices.is_complete() && checkDeviceExtensionSupport(physical_device)) {
        auto swap_chain_support = SwapChainSupportDetails(physical_device, surface);
        return !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    return false;
}

void Application::pickPhysicalDevice()
{
    auto devices = instance->enumeratePhysicalDevices();
    auto it = std::find_if(
        devices.cbegin(), devices.cend(), [&](const auto &device) { return isDeviceSuitable(device, *surface); });
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

void Application::createImageViews()
{
    swap_chain_image_views.resize(swap_chain_images.size());
    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        auto image_view_create_info = vk::ImageViewCreateInfo(
            {},                      // flags
            swap_chain_images[i],    // image
            vk::ImageViewType::e2D,  // viewType
            swap_chain_image_format, // format
            vk::ComponentMapping(),  // components
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor, // aspectMask
                0,                               // baseMipLevel
                1,                               // levelCount
                0,                               // baseArrayLayer
                1                                // layerCount
                )                                // subresourceRange
        );

        swap_chain_image_views[i] = device->createImageViewUnique(image_view_create_info);
    }
}

void Application::createRenderPass()
{
    auto color_attachment = vk::AttachmentDescription(
        {},                               // flags
        swap_chain_image_format,          // format
        vk::SampleCountFlagBits::e1,      // samples
        vk::AttachmentLoadOp::eClear,     // loadOp
        vk::AttachmentStoreOp::eStore,    // storeOp
        vk::AttachmentLoadOp::eDontCare,  // stencilLoadOp
        vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
        vk::ImageLayout::eUndefined,      // initialLayout
        vk::ImageLayout::ePresentSrcKHR   // finalLayout
    );

    auto color_attachment_ref = vk::AttachmentReference(
        0,                                       // attachment
        vk::ImageLayout::eColorAttachmentOptimal // layout
    );

    auto subpass = vk::SubpassDescription(
        {},                               // flags
        vk::PipelineBindPoint::eGraphics, // pipilineBindPoint
        0,                                // inputAttachmentCount
        nullptr,                          // *inputAttachments
        1,                                // colorAttachmentCount
        &color_attachment_ref             // *colorAttachments
    );

    auto subpass_dependency = vk::SubpassDependency(
        VK_SUBPASS_EXTERNAL,                               // srcSubpass
        0,                                                 // dstSubpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // dstStageMask
        {},                                                // srcAccessMask
        vk::AccessFlagBits::eColorAttachmentWrite          // dstAccessMask
    );

    auto render_pass_create_info = vk::RenderPassCreateInfo(
        {},                 // flags
        1,                  // attachmentCount
        &color_attachment,  // *attachments
        1,                  // subpassCount
        &subpass,           // *subpasses
        1,                  // dependencyCount
        &subpass_dependency // *dependencies
    );

    render_pass = device->createRenderPassUnique(render_pass_create_info);
}

vk::UniqueShaderModule createShadermodule(const vk::UniqueDevice &device, const std::string &filename)
{
    auto file = std::ifstream(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Failed to open '") + filename + "'!");
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    auto create_info = vk::ShaderModuleCreateInfo(
        {},                                               // flags
        buffer.size(),                                    // codeSize
        reinterpret_cast<const uint32_t *>(buffer.data()) // *code
    );

    return device->createShaderModuleUnique(create_info);
}

void Application::createGraphicsPipeline()
{
    auto vert_shader_module = createShadermodule(device, "shaders/vertex.spv");
    auto frag_shader_module = createShadermodule(device, "shaders/fragment.spv");

    auto vert_shader_stage_info = vk::PipelineShaderStageCreateInfo(
        {},                               // flags
        vk::ShaderStageFlagBits::eVertex, // stage
        *vert_shader_module,              // module
        "main"                            // *name
    );
    auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo(
        {},                                 // flags
        vk::ShaderStageFlagBits::eFragment, // stage
        *frag_shader_module,                // module
        "main"                              // *name
    );

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo(
        {},      // flags
        0,       // vertexBindingDescriptionCount
        nullptr, // *vertexBindingDescriptions
        0,       // vertexAttributeDescriptionCount
        nullptr  // *vertexAttributeDesscriptions
    );

    auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo(
        {},                                   // flags
        vk::PrimitiveTopology::eTriangleList, // topology
        VK_FALSE                              // primitiveRestartEnable
    );

    auto viewport = vk::Viewport(
        0.0f,                                        // x
        0.0f,                                        // y
        static_cast<float>(swap_chain_extent.width), // width
        static_cast<float>(swap_chain_extent.height) // height
    );

    auto scissor = vk::Rect2D(
        vk::Offset2D(0, 0), // offset
        swap_chain_extent   // extent
    );

    auto viewport_state = vk::PipelineViewportStateCreateInfo(
        {},        // flags
        1,         // viewportCount
        &viewport, // *viewports
        1,         // scissorCount
        &scissor   // *scissors
    );

    auto rasterizer = vk::PipelineRasterizationStateCreateInfo(
        {},                          // flags
        VK_FALSE,                    // depthClampEnable
        VK_FALSE,                    // rasterizerDiscardEnable
        vk::PolygonMode::eFill,      // polygonMode
        vk::CullModeFlagBits::eBack, // cullMode
        vk::FrontFace::eClockwise,   // frontFace
        VK_FALSE,                    // depthBiasEnable
        0.0f,                        // depthBiasCosntantFactor
        0.0f,                        // depthBiasClamp
        0.0f,                        // depthBiasSlopeFactor
        1.0f                         // lineWidth
    );

    auto multisampling = vk::PipelineMultisampleStateCreateInfo(
        {},                          // flags
        vk::SampleCountFlagBits::e1, // rasterizationSamples
        VK_FALSE                     // sampleShadingEnable
    );

    auto color_blend_attachment = vk::PipelineColorBlendAttachmentState(
        VK_TRUE,                // blendEnable
        vk::BlendFactor::eOne,  // srcColorBlendFactor
        vk::BlendFactor::eZero, // dstColorBlendFactor
        vk::BlendOp::eAdd,      // colorBlendOp
        vk::BlendFactor::eOne,  // srcAlphaBlendFactor
        vk::BlendFactor::eZero, // dstAlphaBlendFactor
        vk::BlendOp::eAdd,      // alphaBlendOp
        vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA // colorWriteMask
    );

    auto color_blending = vk::PipelineColorBlendStateCreateInfo(
        {},                     // flags
        VK_FALSE,               // logicOpEnable
        vk::LogicOp::eCopy,     // logicOp
        1,                      // attachmentCount
        &color_blend_attachment // *attachments
    );

    auto pipeline_layout_info = vk::PipelineLayoutCreateInfo(
        {},      // flags
        0,       // setLayoutCount
        nullptr, // *setLayouts
        0,       // pushConstantRangeCount
        nullptr  // *pushConstantRanges
    );

    pipeline_layout = device->createPipelineLayoutUnique(pipeline_layout_info);

    auto graphics_pipeline_create_info = vk::GraphicsPipelineCreateInfo(
        {},                 // flags
        2,                  // stageCount
        shader_stages,      // *stages
        &vertex_input_info, // *vertexInoutState
        &input_assembly,    // *inputAssemblyState
        nullptr,            // *tesselationState
        &viewport_state,    // *viewportState
        &rasterizer,        // *rasterizationState
        &multisampling,     // *multisampleState
        nullptr,            // *depthStencilState
        &color_blending,    // *colorBlendState
        nullptr,            // *dynamicState
        *pipeline_layout,   // layout
        *render_pass        // renderPass
    );

    graphics_pipeline = device->createGraphicsPipelineUnique(nullptr, graphics_pipeline_create_info);
}

void Application::createFramebuffers()
{
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        vk::ImageView attachments[] = {*swap_chain_image_views[i]};

        auto framebuffer_create_info = vk::FramebufferCreateInfo(
            {},                       //flags
            *render_pass,             // renderPass
            1,                        // attachmentCount
            attachments,              // *attachments
            swap_chain_extent.width,  // width
            swap_chain_extent.height, // height
            1                         // layers
        );

        swap_chain_framebuffers[i] = device->createFramebufferUnique(framebuffer_create_info);
    }
}

void Application::createCommandPool()
{
    auto indices = QueueFamilyIndices(physcial_device, *surface);

    auto pool_create_info = vk::CommandPoolCreateInfo(
        {},                             // flags
        indices.graphics_family.value() // queueFamilyIndex
    );
    command_pool = device->createCommandPoolUnique(pool_create_info);
}

void Application::createCommandBuffers()
{
    auto alloc_info = vk::CommandBufferAllocateInfo(
        *command_pool,                           // commandPool
        vk::CommandBufferLevel::ePrimary,        // level
        (uint32_t)swap_chain_framebuffers.size() // commandBufferCount
    );

    command_buffers = device->allocateCommandBuffersUnique(alloc_info);

    auto command_buffer_begin_info = vk::CommandBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eSimultaneousUse, // flags
        nullptr                                           // *inheritanceInfo
    );
    for (size_t i = 0; i < command_buffers.size(); i++) {
        command_buffers[i]->begin(command_buffer_begin_info);

        auto clear_color = vk::ClearValue(std::array{0.0f, 0.0f, 0.0f, 1.0f});
        auto render_pass_begin_info = vk::RenderPassBeginInfo(
            *render_pass,                // renderPass
            *swap_chain_framebuffers[i], // framebuffer
            vk::Rect2D(                  // renderArea
                {0, 0},                  // offset
                swap_chain_extent        // extent
                ),
            1,           // clearValueCount
            &clear_color // *clearValues
        );
        command_buffers[i]->beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        command_buffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
        command_buffers[i]->draw(
            3, // vertexCount
            1, //  instanceCount
            0, // firstVertex
            0  // firstInstance
        );
        command_buffers[i]->endRenderPass();
        command_buffers[i]->end();
    }
}

void Application::createSyncObjects()
{
    image_available_semaphores.resize(max_frames_in_flight);
    render_finished_semaphores.resize(max_frames_in_flight);
    in_flight_fences.resize(max_frames_in_flight);
    images_in_flight.resize(swap_chain_images.size());

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        image_available_semaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
        render_finished_semaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
        in_flight_fences[i] = device->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void Application::drawFrame()
{
    device->waitForFences(*in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index = device->acquireNextImageKHR(*swap_chain, UINT64_MAX, *image_available_semaphores[current_frame], nullptr).value;
    // Check if a previous frame is using this image
    if (images_in_flight[image_index] != vk::Fence(nullptr)) {
        device->waitForFences(images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being used by this frame
    images_in_flight[image_index] = *in_flight_fences[current_frame];

    vk::Semaphore wait_semaphores[] = {*image_available_semaphores[current_frame]};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signal_semaphores[] = {*render_finished_semaphores[current_frame]};
    auto submit_info = vk::SubmitInfo(
        1,                              // waitSemaphroeCount
        wait_semaphores,                // *waitSemaphores
        wait_stages,                    // *waitDstStageMask
        1,                              // commandBufferCount
        &*command_buffers[image_index], // *commandBuffers
        1,                              // signalSemaphoreCount
        signal_semaphores               // *signalSemaphores
    );

    device->resetFences(*in_flight_fences[current_frame]);
    graphics_queue.submit(submit_info, *in_flight_fences[current_frame]);

    auto present_info = vk::PresentInfoKHR(
        1,                 // waitSemaphoreCount
        signal_semaphores, // *waitSemaphores
        1,                 // swapchainCount
        &*swap_chain,      // *swapchains
        &image_index,      // *imageIndices
        nullptr            // *results
    );

    present_queue.presentKHR(present_info);
    current_frame = (current_frame + 1) % max_frames_in_flight;
}

void Application::mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }
        glfwPollEvents();
        drawFrame();
    }
    device->waitIdle();
}
