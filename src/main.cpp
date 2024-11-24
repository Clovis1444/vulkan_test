#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
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
    }
    void mainLoop() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }
    void cleanup() {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        }
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

    const int32_t kWidth_{800};
    const int32_t kHeight_{600};
    GLFWwindow* window_{nullptr};

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
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
