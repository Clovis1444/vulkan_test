#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>

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
    void initVulkan() { createInstance(); }
    void mainLoop() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }
    void cleanup() {
        vkDestroyInstance(instance_, nullptr);

        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void createInstance() {
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

        uint32_t glfw_extension_count{};
        const char** glfw_extensions{
            glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;
        create_info.enabledLayerCount = 0;

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

    const int32_t kWidth_{800};
    const int32_t kHeight_{600};
    GLFWwindow* window_{nullptr};

    VkInstance instance_;
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
