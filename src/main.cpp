#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
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
    }
    void mainLoop() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }
    void cleanup() {
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
