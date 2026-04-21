#include <GLFW/glfw3.h>
#include <cstdint>
#include <set>
#include <vk_mem_alloc.h>
#include <volk.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#include "utils.hxx"
#include "vk_fmt.hxx"

std::set<std::string> get_supported_instance_extensions() {
    uint32_t count = 0;
    assert_vk_success(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    auto extension_properties = std::vector<VkExtensionProperties>(count);
    assert_vk_success(
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data()));

    auto extensions = std::set<std::string>();
    for (const auto& extension : extension_properties) {
        extensions.insert(extension.extensionName);
    }

    return extensions;
}

std::set<std::string> get_supported_device_extensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    assert_vk_success(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));
    auto extension_properties = std::vector<VkExtensionProperties>(count);
    assert_vk_success(
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extension_properties.data()));

    auto extensions = std::set<std::string>();
    for (const auto& extension : extension_properties) {
        extensions.insert(extension.extensionName);
    }

    return extensions;
}

std::vector<VkSurfaceFormatKHR> get_supported_surface_formats(
    VkPhysicalDevice device,
    VkSurfaceKHR surface) {
    uint32_t count;
    assert_vk_success(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr));
    auto formats = std::vector<VkSurfaceFormatKHR>(count);
    assert_vk_success(
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data()));
    return formats;
}

const size_t DESIRED_FRAMES_IN_FLIGHT = 3;

int32_t main() {
    if (!glfwInit()) {
        eprintln("GLFW initialize error");
        exit(1);
    }

    assert_vk_success(volkInitialize());

    /* === Instance === */

    auto appInfo = VkApplicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Minimal",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName = "Vulkan Minimal Engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };
    uint32_t instance_extensions_count = 0;
    const char** glfw_instance_extensions =
        glfwGetRequiredInstanceExtensions(&instance_extensions_count);
    auto instance_ci = VkInstanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instance_extensions_count,
        .ppEnabledExtensionNames = glfw_instance_extensions,
    };
    VkInstance instance;
    assert_vk_success(vkCreateInstance(&instance_ci, nullptr, &instance));
    volkLoadInstance(instance);

    /* === Physical devices === */

    uint32_t physical_devices_count = 0;
    assert_vk_success(vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr));
    auto physical_devices = std::vector<VkPhysicalDevice>(physical_devices_count);
    assert_vk_success(
        vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data()));
    if (physical_devices_count == 0) {
        eprintln("[ERROR] vulkan reported 0 physical devices, exiting");
        exit(1);
    }
    println("found {} device(s)", physical_devices_count);
    for (const auto& [i, device] : enumerate(physical_devices)) {
        auto driver_properties = VkPhysicalDeviceDriverProperties {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        };
        auto device_properties2 = VkPhysicalDeviceProperties2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &driver_properties,
        };
        vkGetPhysicalDeviceProperties2(device, &device_properties2);
        const auto& properties = device_properties2.properties;
        println("[device #{}]", i);
        println("ID:                  {}", properties.deviceID);
        println("name:                {}", (const char*)properties.deviceName);
        println("type:                {}", fmt_vk(properties.deviceType));
        println("vendor ID:           {}", properties.vendorID);
        println("API version:         {}", fmt_vk_api_version(properties.apiVersion));
        println("driver name:         {}", driver_properties.driverName);
        println("driver version:      {}", fmt_vk_version(properties.driverVersion));
        println("driver info:         {}", driver_properties.driverInfo);
        println(
            "conformance version: {}.{}.{}.{}",
            driver_properties.conformanceVersion.major,
            driver_properties.conformanceVersion.minor,
            driver_properties.conformanceVersion.patch,
            driver_properties.conformanceVersion.subminor);
    }
    uint32_t physical_device_index = 0;
    println(
        "selecting physical device #{} (FIXME: select the most suitable)",
        physical_device_index);
    auto physical_device = physical_devices[physical_device_index];

    /* === Queue family === */

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_family_count, nullptr);
    auto queue_family_properties = std::vector<VkQueueFamilyProperties2>(queue_family_count);
    std::fill(
        queue_family_properties.begin(),
        queue_family_properties.end(),
        VkQueueFamilyProperties2 {.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(
        physical_device,
        &queue_family_count,
        queue_family_properties.data());
    auto queue_family_index_ = std::optional<uint32_t>(0);
    for (const auto& [i, properties2] : enumerate(queue_family_properties)) {
        uint32_t index = (uint32_t)i;
        const auto& properties = properties2.queueFamilyProperties;
        auto flags = properties.queueFlags;
        bool presentation_support =
            glfwGetPhysicalDevicePresentationSupport(instance, physical_device, index) == GLFW_TRUE;
        println("[queue family #{}]", index);
        println("queue count:  {}", properties.queueCount);
        println("graphics:     {}", (flags & VK_QUEUE_GRAPHICS_BIT) != 0 ? "YES" : "NO");
        println("compute:      {}", (flags & VK_QUEUE_COMPUTE_BIT) != 0 ? "YES" : "NO");
        println("presentation: {}", presentation_support ? "YES" : "NO");
        if (!queue_family_index_.has_value() && (flags & VK_QUEUE_COMPUTE_BIT) != 0 &&
            presentation_support) {
            queue_family_index_ = index;
        }
    }
    if (queue_family_index_.has_value()) {
        println("selected queue family #{}", queue_family_index_.value());
    } else {
        eprintln("[ERROR] no graphics-capable & presentation-capable queue families");
        exit(1);
    }
    uint32_t queue_family_index = queue_family_index_.value();

    /* === Logical device and queue === */

    auto queue_priorities = std::array {1.0f};
    auto queue_ci = VkDeviceQueueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
        .queueCount = queue_priorities.size(),
        .pQueuePriorities = queue_priorities.data(),
    };
    auto enabled_vk12_features = VkPhysicalDeviceVulkan12Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true,
    };
    auto enabled_vk13_features = VkPhysicalDeviceVulkan13Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabled_vk12_features,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    auto enabled_vk10_features = VkPhysicalDeviceFeatures {
        .samplerAnisotropy = VK_TRUE,
    };
    const auto device_extensions = std::array {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
        // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        // VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        // // Dynamic rendering.
        // VK_KHR_MULTIVIEW_EXTENSION_NAME,
        // VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
        // VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        // VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        // VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };
    auto supported_device_extensions = get_supported_device_extensions(physical_device);
    size_t unsupported_device_extension_count = 0;
    for (const auto& extension_name : device_extensions) {
        if (!supported_device_extensions.contains(extension_name)) {
            eprintln("device extension {} not supported", extension_name);
            unsupported_device_extension_count += 1;
        }
    }
    if (unsupported_device_extension_count != 0) {
        eprintln(
            "exiting because the above {} device extension(s) are not supported",
            unsupported_device_extension_count);
        exit(1);
    }
    auto device_ci = VkDeviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_vk13_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_ci,
        .enabledExtensionCount = (uint32_t)device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &enabled_vk10_features,
    };
    VkDevice device;
    assert_vk_success(vkCreateDevice(physical_device, &device_ci, nullptr, &device));
    VkQueue queue;
    vkGetDeviceQueue(device, queue_family_index, 0, &queue);

    /* === VMA === */

    auto vk_functions = VmaVulkanFunctions {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage,
    };
    auto allocator_ci = VmaAllocatorCreateInfo {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vk_functions,
        .instance = instance,
    };
    VmaAllocator allocator;
    assert_vk_success(vmaCreateAllocator(&allocator_ci, &allocator));

    /* === Window and Surface === */

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no OpenGL
    GLFWwindow* window = glfwCreateWindow(960, 720, "Vulkan Minimal", nullptr, nullptr);
    if (!window) {
        eprintln("[ERROR] cannot GLFW create window");
        glfwTerminate();
        exit(1);
    }
    uint32_t window_width;
    uint32_t window_height;
    glfwGetFramebufferSize(window, (int32_t*)&window_width, (int32_t*)&window_height);
    VkSurfaceKHR surface;
    assert_vk_success(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    /* === Swapchain === */

    VkSurfaceCapabilitiesKHR surface_capabilities;
    assert_vk_success(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));
    auto swapchain_extent = VkExtent2D {
        .width = window_width,
        .height = window_height,
    };
    auto supported_surface_formats = get_supported_surface_formats(physical_device, surface);
    auto surface_format_ = std::optional<VkSurfaceFormatKHR>();
    println("found {} supported surface formats", supported_surface_formats.size());
    for (const auto& format : supported_surface_formats) {
        if (surface_format_.has_value())
            continue;
        switch (format.format) {
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB: {
            surface_format_ = format;
        } break;
        default: break;
        }
    }
    if (surface_format_.has_value()) {
        println(
            "selected surface format {}, colorspace {}",
            fmt_vk(surface_format_->format),
            fmt_vk(surface_format_->colorSpace));
    } else {
        eprintln("[ERROR] no suitable surface format");
        exit(1);
    }
    auto surface_format = surface_format_.value();
    auto swapchain_ci = VkSwapchainCreateInfoKHR {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capabilities.minImageCount,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    };
    VkSwapchainKHR swapchain;
    assert_vk_success(vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain));
    uint32_t swapchain_image_count;
    assert_vk_success(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr));
    auto swapchain_images = std::vector<VkImage>(swapchain_image_count);
    assert_vk_success(vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &swapchain_image_count,
        swapchain_images.data()));
    auto swapchain_image_views = std::vector<VkImageView>(swapchain_image_count);
    for (const auto& [i, image] : enumerate(swapchain_images)) {
        auto ci = VkImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        assert_vk_success(vkCreateImageView(device, &ci, nullptr, &swapchain_image_views[i]));
    }

    /* === Depth texture === */

    auto preferred_depth_formats = std::array {
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    for (VkFormat& format : preferred_depth_formats) {
        auto format_properties =
            VkFormatProperties2 {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(physical_device, format, &format_properties);
        if (format_properties.formatProperties.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depth_format = format;
            break;
        }
    }
    if (depth_format != VK_FORMAT_UNDEFINED) {
        println("selected depth texture format {}", fmt_vk(depth_format));
    } else {
        eprintln("no suitable depth texture format");
        exit(1);
    }
    auto depth_image_ci = VkImageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent {
            .width = window_width,
            .height = window_height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    auto alloc_ci = VmaAllocationCreateInfo {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    VkImage depth_image;
    VmaAllocation depth_image_allocation;
    assert_vk_success(vmaCreateImage(
        allocator,
        &depth_image_ci,
        &alloc_ci,
        &depth_image,
        &depth_image_allocation,
        nullptr));
    auto depth_view_ci = VkImageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    VkImageView depth_image_view;
    assert_vk_success(vkCreateImageView(device, &depth_view_ci, nullptr, &depth_image_view));

    /* Frame Semaphores & Fences */
    auto semaphore_ci = VkSemaphoreCreateInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    auto fence_ci = VkFenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    auto frames_in_flight = std::min<size_t>(DESIRED_FRAMES_IN_FLIGHT, swapchain_images.size());
    println("frames in flight: {}", frames_in_flight);
    auto frame_fences = std::vector<VkFence>(frames_in_flight);
    auto present_semaphores = std::vector<VkSemaphore>(frames_in_flight);
    auto render_semaphores = std::vector<VkSemaphore>(swapchain_images.size());
    for (size_t i = 0; i < frames_in_flight; i++) {
        assert_vk_success(vkCreateFence(device, &fence_ci, nullptr, &frame_fences[i]));
        assert_vk_success(
            vkCreateSemaphore(device, &semaphore_ci, nullptr, &present_semaphores[i]));
    }
    for (auto& semaphore : render_semaphores) {
        assert_vk_success(vkCreateSemaphore(device, &semaphore_ci, nullptr, &semaphore));
    }

    /* === Command Buffer === */

    auto command_pool_ci = VkCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index,
    };
    VkCommandPool command_pool;
    auto command_buffers = std::vector<VkCommandBuffer>(frames_in_flight);
    assert_vk_success(vkCreateCommandPool(device, &command_pool_ci, nullptr, &command_pool));
    auto command_buffer_alloc_info = VkCommandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .commandBufferCount = (uint32_t)command_buffers.size(),
    };
    assert_vk_success(
        vkAllocateCommandBuffers(device, &command_buffer_alloc_info, command_buffers.data()));

    /* ==== Graphics Pipeline === */

    auto shader_spv_code = read_file_to_bytes("assets/shaders/the_shader.slang.spv");
    auto shader_module_ci = VkShaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_spv_code.size(),
        .pCode = (uint32_t*)shader_spv_code.data(),
    };
    VkShaderModule shader_module;
    assert_vk_success(vkCreateShaderModule(device, &shader_module_ci, nullptr, &shader_module));

    auto pipeline_layout_ci = VkPipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    VkPipelineLayout pipeline_layout;
    assert_vk_success(
        vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr, &pipeline_layout));

    auto pipeline_shader_stages = std::array {
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader_module,
            .pName = "main",
        },
    };
    auto pipeline_vertex_input_state = VkPipelineVertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };
    auto pipeline_input_assembly_state = VkPipelineInputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };
    auto pipeline_viewport_state = VkPipelineViewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    auto pipeline_depth_stencil_state = VkPipelineDepthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };
    auto pipeline_rendering_ci = VkPipelineRenderingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &surface_format.format,
        .depthAttachmentFormat = depth_format,
    };
    auto pipeline_rasterization_state = VkPipelineRasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f,
    };
    auto pipeline_multisample_state = VkPipelineMultisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    auto pipeline_blend_attachment = VkPipelineColorBlendAttachmentState {
        .colorWriteMask = 0xF,
    };
    auto pipeline_color_blend_state = VkPipelineColorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &pipeline_blend_attachment,
    };
    auto pipeline_dynamic_states_array = std::array {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    auto pipeline_dynamic_state = VkPipelineDynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = pipeline_dynamic_states_array.size(),
        .pDynamicStates = pipeline_dynamic_states_array.data(),
    };
    auto graphics_pipeline_ci = VkGraphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_ci,
        .stageCount = pipeline_shader_stages.size(),
        .pStages = pipeline_shader_stages.data(),
        .pVertexInputState = &pipeline_vertex_input_state,
        .pInputAssemblyState = &pipeline_input_assembly_state,
        .pViewportState = &pipeline_viewport_state,
        .pRasterizationState = &pipeline_rasterization_state,
        .pMultisampleState = &pipeline_multisample_state,
        .pDepthStencilState = &pipeline_depth_stencil_state,
        .pColorBlendState = &pipeline_color_blend_state,
        .pDynamicState = &pipeline_dynamic_state,
        .layout = pipeline_layout,
    };
    VkPipeline pipeline;
    assert_vk_success(
        vkCreateGraphicsPipelines(device, nullptr, 1, &graphics_pipeline_ci, nullptr, &pipeline));
    vkDestroyShaderModule(device, shader_module, nullptr);

    /* === Render Loop === */

    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    bool swapchain_needs_update = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (swapchain_needs_update) {
            glfwGetFramebufferSize(window, (int32_t*)&window_width, (int32_t*)&window_height);
            println("window size: {}x{}", window_width, window_height);
            swapchain_needs_update = false;
            assert_vk_success(vkDeviceWaitIdle(device));
            assert_vk_success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physical_device,
                surface,
                &surface_capabilities));
            swapchain_ci.oldSwapchain = swapchain;
            swapchain_ci.imageExtent = {
                .width = window_width,
                .height = window_height,
            };
            assert_vk_success(vkCreateSwapchainKHR(device, &swapchain_ci, nullptr, &swapchain));
            for (const auto& image_view : swapchain_image_views) {
                vkDestroyImageView(device, image_view, nullptr);
            }
            assert_vk_success(
                vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr));
            swapchain_images.resize(swapchain_image_count);
            assert_vk_success(vkGetSwapchainImagesKHR(
                device,
                swapchain,
                &swapchain_image_count,
                swapchain_images.data()));
            swapchain_image_views.resize(swapchain_image_count);
            for (const auto& [i, image] : enumerate(swapchain_images)) {
                auto image_view_ci = VkImageViewCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = surface_format.format,
                    .subresourceRange =
                        {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
                };
                assert_vk_success(
                    vkCreateImageView(device, &image_view_ci, nullptr, &swapchain_image_views[i]));
            }
            vkDestroySwapchainKHR(device, swapchain_ci.oldSwapchain, nullptr);
            vmaDestroyImage(allocator, depth_image, depth_image_allocation);
            vkDestroyImageView(device, depth_image_view, nullptr);
            depth_image_ci.extent = {
                .width = window_width,
                .height = window_height,
                .depth = 1,
            };
            auto alloc_ci = VmaAllocationCreateInfo {
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
            };
            assert_vk_success(vmaCreateImage(
                allocator,
                &depth_image_ci,
                &alloc_ci,
                &depth_image,
                &depth_image_allocation,
                nullptr));
            auto viewCI = VkImageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = depth_image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depth_format,
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .levelCount = 1,
                        .layerCount = 1,
                    },
            };
            assert_vk_success(vkCreateImageView(device, &viewCI, nullptr, &depth_image_view));
        }

        assert_vk_success(vkWaitForFences(device, 1, &frame_fences[frame_index], true, UINT64_MAX));
        assert_vk_success(vkResetFences(device, 1, &frame_fences[frame_index]));
        VkResult acquire_next_image_result = vkAcquireNextImageKHR(
            device,
            swapchain,
            UINT64_MAX,
            present_semaphores[frame_index],
            VK_NULL_HANDLE,
            &image_index);
        switch (acquire_next_image_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR: {
            swapchain_needs_update = true;
        } break;
        default:
            eprintln(
                "error acquiring swapchain image for a frame: {}",
                fmt_vk(acquire_next_image_result));
        }
        auto command_buffer = command_buffers[frame_index];
        assert_vk_success(vkResetCommandBuffer(command_buffer, 0));
        VkCommandBufferBeginInfo command_buffer_begin_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
        assert_vk_success(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
        auto output_barriers = std::array {
            VkImageMemoryBarrier2 {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .image = swapchain_images[image_index],
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            },
            VkImageMemoryBarrier2 {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                .image = depth_image,
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            },
        };
        auto barrier_dependency_info = VkDependencyInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = output_barriers.size(),
            .pImageMemoryBarriers = output_barriers.data(),
        };
        vkCmdPipelineBarrier2(command_buffer, &barrier_dependency_info);
        auto color_attachment_info = VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain_image_views[image_index],
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue =
                VkClearValue {
                    .color =
                        {{0.006995410187265387f,
                          0.006995410187265387f,
                          0.006995410187265387f,
                          1.0f}},
                },
        };
        auto depth_attachment_info = VkRenderingAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = depth_image_view,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = VkClearValue {.depthStencil = {1.0f, 0}},
        };
        auto rendering_info = VkRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea {
                .extent {
                    .width = window_width,
                    .height = window_height,
                },
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = &depth_attachment_info,
        };
        vkCmdBeginRendering(command_buffer, &rendering_info);
        auto viewport = VkViewport {
            .width = (float)window_width,
            .height = (float)window_height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        auto scissor = VkRect2D {
            .extent =
                VkExtent2D {
                    .width = window_width,
                    .height = window_height,
                },
        };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(
            command_buffer, // commandBuffer,
            6,              // vertexCount,
            1,              // instanceCount,
            0,              // firstVertex,
            0);             // firstInstance

        vkCmdEndRendering(command_buffer);
        auto barrierPresent = VkImageMemoryBarrier2 {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = swapchain_images[image_index],
            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        auto barrier_present_dependency_info = VkDependencyInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrierPresent,
        };
        vkCmdPipelineBarrier2(command_buffer, &barrier_present_dependency_info);

        assert_vk_success(vkEndCommandBuffer(command_buffer));
        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        auto submitInfo = VkSubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphores[frame_index],
            .pWaitDstStageMask = &waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphores[image_index],
        };
        assert_vk_success(vkQueueSubmit(queue, 1, &submitInfo, frame_fences[frame_index]));
        frame_index = (frame_index + 1) % frames_in_flight;
        auto present_info = VkPresentInfoKHR {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphores[image_index],
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index,
        };
        VkResult present_result = vkQueuePresentKHR(queue, &present_info);
        switch (present_result) {
        case VK_SUCCESS: break;
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR: {
            swapchain_needs_update = true;
        } break;
        default:
            eprintln("error presenting a swapchain image for a frame: {}", fmt_vk(present_result));
        }
    }

    /* === Cleanup === */

    vkDeviceWaitIdle(device);

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyImageView(device, depth_image_view, nullptr);
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
    for (const auto& fence : frame_fences) {
        vkDestroyFence(device, fence, nullptr);
    }
    for (const auto& semaphore : render_semaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (const auto& semaphore : present_semaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (const auto& image_view : swapchain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
