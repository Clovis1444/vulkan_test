#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>

// NOLINTNEXTLINE
static std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers{false};
#else
// NOLINTNEXTLINE
const bool enableValidationLayers{true};
#endif  // NDEBUG

// NOLINTNEXTLINE
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // NOLINTNEXTLINE
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    // NOLINTNEXTLINE
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
// NOLINTNEXTLINE
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    // NOLINTNEXTLINE
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication {
   public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

   private:
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(kWidth_, kHeight_, "My Vulkan test app",
                                   nullptr, nullptr);
    };
    void initVulkan() {
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
        CreateCommandBuffer();
        createSyncObjects();
    }
    void mainLoop() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device_);
    }
    void cleanup() {
        vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
        vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
        vkDestroyFence(device_, inFlightFence_, nullptr);

        vkDestroyCommandPool(device_, commandPool_, nullptr);

        for (auto* framebuffer : swapChainFramebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }

        vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        vkDestroyRenderPass(device_, renderPass_, nullptr);

        for (auto* image_view : swapChainImageViews_) {
            vkDestroyImageView(device_, image_view, nullptr);
        }

        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
        vkDestroyDevice(device_, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        }
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyInstance(instance_, nullptr);

        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error(
                "validation layers requested, but not available!");
        }

        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Hello triangle";
        app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        app_info.pEngineName = "No engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        std::vector<const char*> extensions{getRequiredExtensions()};
        create_info.enabledExtensionCount =
            static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        if (enableValidationLayers) {
            create_info.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
            create_info.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debug_create_info);
            create_info.pNext = &debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;

            create_info.pNext = nullptr;
        }

        // printExtensionSupport(true);

        if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
            throw std::runtime_error{"Failed to create instance"};
        };
    }
    static void printExtensionSupport(bool verbose) {
        // Supported extension list
        uint32_t extension_count{};
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                               nullptr);
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                               extensions.data());

        std::cout << "Total supported extension count: " << extension_count
                  << '\n';
        if (verbose) {
            for (const auto& i : extensions) {
                std::cout << '\t' << i.extensionName << '\n';
            }
        }

        // GLFW extension list
        uint32_t glfw_extension_count{};
        const char** glfw_extensions{
            glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

        std::cout << "GLFW required extension count: " << glfw_extension_count
                  << '\n';
        if (verbose) {
            for (int i{0}; i < glfw_extension_count; ++i) {
                std::cout << '\t' << glfw_extensions[i] << '\n';
            }
        }
    }
    static std::vector<const char*> getRequiredExtensions() {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions{
            glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

        std::vector<const char*> extensions(
            glfw_extensions, glfw_extensions + glfw_extension_count);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        populateDebugMessengerCreateInfo(create_info);

        if (CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr,
                                         &debugMessenger_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }
    static void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info = {};
        create_info.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debugCallback;
    }

    static bool checkValidationLayerSupport() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count,
                                           available_layers.data());

        for (const char* layer_name : validationLayers) {
            bool layer_found = false;

            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                return false;
            }
        }

        return true;
    }

    // NOLINTBEGIN
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage
                  << std::endl;

        return VK_FALSE;
    }
    // NOLINTEND

    void pickPhysicalDevice() {
        uint32_t device_count{};

        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

        if (device_count == 0) {
            throw std::runtime_error{
                "Failed to find GPUs with Vulkan support!"};
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

        physicalDevice_ = VK_NULL_HANDLE;
        for (const VkPhysicalDevice& i : devices) {
            if (isDeviceSuitable(i)) {
                physicalDevice_ = i;
                break;
            }
        }

        if (physicalDevice_ == VK_NULL_HANDLE) {
            throw std::runtime_error{"Failed to find a suitable GPU!"};
        }
    }
    // Check if GPU suitable for this app
    bool isDeviceSuitable(const VkPhysicalDevice& device) {
        bool extensions_supported{checkDeviceExtensionSupport(device)};

        QueueFamilyIndices indices{findQueueFamilyIndices(device)};

        bool swap_chain_adequate{};
        if (extensions_supported) {
            SwapChainSupportDetails swap_chain_support{
                querySwapChainSupport(device)};
            swap_chain_adequate = !swap_chain_support.formats.empty() &&
                                  !swap_chain_support.presentModes.empty();
        }

        return indices.isComplete() && extensions_supported &&
               swap_chain_adequate;
    }

    // Check if a GPU supports all extension required for this program
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) {
        uint32_t extension_count{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                             nullptr);

        // All GPU supported extensions
        std::vector<VkExtensionProperties> available_extensions(
            extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                             available_extensions.data());

        // Extensions required for the current program
        std::set<std::string> required_extensions{deviceExtensions_.begin(),
                                                  deviceExtensions_.end()};

        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
        void reset() {
            graphicsFamily.reset();
            presentFamily.reset();
        }
    };
    QueueFamilyIndices findQueueFamilyIndices(const VkPhysicalDevice& device) {
        QueueFamilyIndices indices{};

        uint32_t queue_family_count{};

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                                 queue_families.data());

        int i{};

        for (const auto& queue_family : queue_families) {
            // indices.reset();

            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 present_support{};
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_,
                                                 &present_support);
            if (present_support) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            ++i;
        }

        return indices;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices{findQueueFamilyIndices(physicalDevice_)};

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
        std::set<uint32_t> unique_queue_families{indices.graphicsFamily.value(),
                                                 indices.presentFamily.value()};

        float queue_priority{1.0};

        for (uint32_t queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features{};

        VkDeviceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount =
            static_cast<uint32_t>(queue_create_infos.size());
        create_info.pEnabledFeatures = &device_features;

        create_info.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions_.size());
        create_info.ppEnabledExtensionNames = deviceExtensions_.data();

        if (enableValidationLayers) {
            create_info.enabledLayerCount =
                static_cast<uint32_t>(validationLayers.size());
            create_info.ppEnabledLayerNames = validationLayers.data();
        } else {
            create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice_, &create_info, nullptr, &device_) !=
            VK_SUCCESS) {
            throw std::runtime_error{"Failed to create logical device!"};
        }

        vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0,
                         &graphicsQueue_);
        vkGetDeviceQueue(device_, indices.presentFamily.value(), 0,
                         &presentQueue_);
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) !=
            VK_SUCCESS) {
            throw std::runtime_error{"Failed to create window surface!"};
        }
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    // Check if swapchain is compatible with window surface
    SwapChainSupportDetails querySwapChainSupport(
        const VkPhysicalDevice& device) {
        SwapChainSupportDetails details{};

        // Capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_,
                                                  &details.capabilities);

        // Formats
        uint32_t format_count{};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count,
                                             nullptr);
        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, surface_, &format_count, details.formats.data());
        }

        // Present modes
        uint32_t present_mode_count{};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_,
                                                  &present_mode_count, nullptr);
        if (present_mode_count != 0) {
            details.presentModes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface_, &present_mode_count,
                details.presentModes.data());
        }

        return details;
    }

    // Choose "best" available format
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& available_formats) {
        // Returns "best" format or the first one in the list
        for (const auto& format : available_formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }

        return available_formats[0];
    }
    // Choose "best" presentation mode
    static VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& available_present_modes) {
        // Returns VK_PRESENT_MODE_MAILBOX_KHR if available.
        // Otherwise returns VK_PRESENT_MODE_FIFO_KHR
        for (const auto& present_mode : available_present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
    // Choose "best" resolution
    // For more info see:
    // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html#_swap_extent
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        int width{};
        int height{};
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actual_extent{static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};

        actual_extent.width =
            std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        actual_extent.height =
            std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);

        return actual_extent;
    }

    void createSwapChain() {
        SwapChainSupportDetails swap_chain_details{
            querySwapChainSupport(physicalDevice_)};

        VkSurfaceFormatKHR surface_format =
            chooseSwapSurfaceFormat(swap_chain_details.formats);
        VkPresentModeKHR present_mode =
            chooseSwapPresentMode(swap_chain_details.presentModes);
        VkExtent2D extent = chooseSwapExtent(swap_chain_details.capabilities);

        uint32_t image_count{swap_chain_details.capabilities.minImageCount + 1};
        if (swap_chain_details.capabilities.maxImageCount > 0 &&
            image_count > swap_chain_details.capabilities.maxImageCount) {
            image_count = swap_chain_details.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilyIndices(physicalDevice_);
        uint32_t queue_family_indices[] = {indices.graphicsFamily.value(),
                                           indices.presentFamily.value()};

        // If graphics queue and presentation queue are NOT the same family
        if (indices.graphicsFamily != indices.presentFamily) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        }
        // If graphics queue and presentation queue ARE the same family
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;      // Optional
            create_info.pQueueFamilyIndices = nullptr;  // Optional
        }

        create_info.preTransform =
            swap_chain_details.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapChain_) !=
            VK_SUCCESS) {
            throw std::runtime_error{"Failed to create swap chain!"};
        }

        vkGetSwapchainImagesKHR(device_, swapChain_, &image_count, nullptr);
        swapChainImages_.resize(image_count);
        vkGetSwapchainImagesKHR(device_, swapChain_, &image_count,
                                swapChainImages_.data());

        swapChainImageFormat_ = surface_format.format;
        swapChainExtent_ = extent;
    }

    void createImageViews() {
        swapChainImageViews_.resize(swapChainImages_.size());

        for (size_t i{}; i < swapChainImages_.size(); ++i) {
            VkImageViewCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = swapChainImages_[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = swapChainImageFormat_;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device_, &create_info, nullptr,
                                  &swapChainImageViews_[i]) != VK_SUCCESS) {
                throw std::runtime_error{
                    "Failed to create swap chain image views!"};
            }
        }
    }

    void createGraphicsPipeline() {
        std::vector<char> vert_shader_code{readFile("shaders/vert.spv")};
        std::vector<char> frag_shader_code{readFile("shaders/frag.spv")};

        VkShaderModule vert_shader_module{createShaderModule(vert_shader_code)};
        VkShaderModule frag_shader_module{createShaderModule(frag_shader_code)};

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages[]{vert_shader_stage_info,
                                                        frag_shader_stage_info};

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = 0;
        vertex_input_info.pVertexBindingDescriptions = nullptr;
        vertex_input_info.vertexAttributeDescriptionCount = 0;
        vertex_input_info.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0F;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0F;  // Optional
        rasterizer.depthBiasClamp = 0.0F;           // Optional
        rasterizer.depthBiasSlopeFactor = 0.0F;     // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0F;           // Optional
        multisampling.pSampleMask = nullptr;             // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
        multisampling.alphaToOneEnable = VK_FALSE;       // Optional

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor =
            VK_BLEND_FACTOR_ONE;  // Optional
        color_blend_attachment.dstColorBlendFactor =
            VK_BLEND_FACTOR_ZERO;                               // Optional
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;  // Optional
        color_blend_attachment.srcAlphaBlendFactor =
            VK_BLEND_FACTOR_ONE;  // Optional
        color_blend_attachment.dstAlphaBlendFactor =
            VK_BLEND_FACTOR_ZERO;                               // Optional
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;  // Optional
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0F;  // Optional
        color_blending.blendConstants[1] = 0.0F;  // Optional
        color_blending.blendConstants[2] = 0.0F;  // Optional
        color_blending.blendConstants[3] = 0.0F;  // Optional

        std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                      VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount =
            static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 0;             // Optional
        pipeline_layout_info.pSetLayouts = nullptr;          // Optional
        pipeline_layout_info.pushConstantRangeCount = 0;     // Optional
        pipeline_layout_info.pPushConstantRanges = nullptr;  // Optional

        if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr,
                                   &pipelineLayout_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shader_stages;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = nullptr;  // Optional
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = pipelineLayout_;
        pipeline_info.renderPass = renderPass_;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;  // Optional
        pipeline_info.basePipelineIndex = -1;               // Optional

        if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1,
                                      &pipeline_info, nullptr,
                                      &graphicsPipeline_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        // Clean up
        vkDestroyShaderModule(device_, vert_shader_module, nullptr);
        vkDestroyShaderModule(device_, frag_shader_module, nullptr);
    };

    static std::vector<char> readFile(const std::string& file_name) {
        std::ifstream file{file_name, std::ios::ate | std::ios::binary};

        if (!file.is_open()) {
            throw std::runtime_error{"Failed to open file!"};
        }

        size_t file_size{static_cast<size_t>(file.tellg())};
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shader_module;
        if (vkCreateShaderModule(device_, &create_info, nullptr,
                                 &shader_module) != VK_SUCCESS) {
            throw std::runtime_error{"Failed to create shader module!"};
        }

        return shader_module;
    }

    void createRenderPass() {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = swapChainImageFormat_;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        if (vkCreateRenderPass(device_, &render_pass_info, nullptr,
                               &renderPass_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void createFramebuffers() {
        swapChainFramebuffers_.resize(swapChainImageViews_.size());

        for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
            VkImageView attachments[] = {swapChainImageViews_[i]};

            VkFramebufferCreateInfo framebuffer_info{};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = renderPass_;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = attachments;
            framebuffer_info.width = swapChainExtent_.width;
            framebuffer_info.height = swapChainExtent_.height;
            framebuffer_info.layers = 1;

            if (vkCreateFramebuffer(device_, &framebuffer_info, nullptr,
                                    &swapChainFramebuffers_[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queue_family_indices =
            findQueueFamilyIndices(physicalDevice_);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex =
            queue_family_indices.graphicsFamily.value();

        if (vkCreateCommandPool(device_, &pool_info, nullptr, &commandPool_) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void CreateCommandBuffer() {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = commandPool_;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device_, &alloc_info, &commandBuffer_) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer command_buffer,
                             uint32_t image_index) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;                   // Optional
        begin_info.pInheritanceInfo = nullptr;  // Optional

        if (vkBeginCommandBuffer(commandBuffer_, &begin_info) != VK_SUCCESS) {
            throw std::runtime_error(
                "Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = renderPass_;
        render_pass_info.framebuffer = swapChainFramebuffers_[image_index];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapChainExtent_;

        VkClearValue clear_color = {{{0.0F, 0.0F, 0.0F, 1.0F}}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(commandBuffer_, &render_pass_info,
                             VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphicsPipeline_);

        // Note: we did specify viewport and scissor state for this pipeline to
        // be dynamic. So we need to set them in the command buffer before
        // issuing our draw command.
        VkViewport viewport{};
        viewport.x = 0.0F;
        viewport.y = 0.0F;
        viewport.width = static_cast<float>(swapChainExtent_.width);
        viewport.height = static_cast<float>(swapChainExtent_.height);
        viewport.minDepth = 0.0F;
        viewport.maxDepth = 1.0F;
        vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent_;
        vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);

        // Draw 3 vertexes, defined in shaders
        vkCmdDraw(commandBuffer_, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer_);

        if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device_, &semaphore_info, nullptr,
                              &imageAvailableSemaphore_) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphore_info, nullptr,
                              &renderFinishedSemaphore_) != VK_SUCCESS ||
            vkCreateFence(device_, &fence_info, nullptr, &inFlightFence_) !=
                VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores!");
        }
    }

    void drawFrame() {
        // Wait for previous frame to finish
        vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, UINT64_MAX);
        vkResetFences(device_, 1, &inFlightFence_);

        uint32_t image_index;
        vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX,
                              imageAvailableSemaphore_, VK_NULL_HANDLE,
                              &image_index);

        vkResetCommandBuffer(commandBuffer_, 0);
        recordCommandBuffer(commandBuffer_, image_index);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {imageAvailableSemaphore_};
        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &commandBuffer_;

        VkSemaphore signal_semaphores[] = {renderFinishedSemaphore_};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        if (vkQueueSubmit(graphicsQueue_, 1, &submit_info, inFlightFence_) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {swapChain_};
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swap_chains;
        present_info.pImageIndices = &image_index;
        present_info.pResults = nullptr;  // Optional

        vkQueuePresentKHR(presentQueue_, &present_info);
    }

    const int32_t kWidth_{800};
    const int32_t kHeight_{600};
    GLFWwindow* window_{nullptr};

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkSurfaceKHR surface_;

    // Required device extensions
    const std::vector<const char*> deviceExtensions_{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkSwapchainKHR swapChain_;
    // SwapChain buffer
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;

    std::vector<VkImageView> swapChainImageViews_;

    VkRenderPass renderPass_;
    VkPipelineLayout pipelineLayout_{};
    VkPipeline graphicsPipeline_;

    std::vector<VkFramebuffer> swapChainFramebuffers_;

    VkCommandPool commandPool_;
    VkCommandBuffer commandBuffer_;

    VkSemaphore imageAvailableSemaphore_;
    VkSemaphore renderFinishedSemaphore_;
    VkFence inFlightFence_;
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
