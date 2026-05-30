#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanInstance>
#include <QVulkanFunctions>

class VulkanWidget;

class VulkanRenderer
{

public:
    VulkanRenderer(VulkanWidget* vulkanWindow);
    virtual ~VulkanRenderer();

    bool initialize();
    void cleanup();
private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createQueues();
    void createCommandPools();
    VkCommandPool createCommandPool(const uint32_t iQueueFamilyIndex);
    void createSwapChain();

    // Print information
    void printVulkanInfo(const QString& iString) const;
    void printDebugInfo(const QString& iString) const;

    // Queue Family functions
    const uint32_t getQueueFamilyIndex(const VkQueueFlags iQueueFlags) const;

    // Extension functions
    bool extensionSupported(const char* iExtension) const;
private:
    // Window
    VulkanWidget* m_window{nullptr};

    // Vulkan components
    // - QVulkanInstance
    QVulkanInstance m_vulkanInstance;

    // - Surface
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};

    // - Devices
    // -- Physical Deivce
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};
    VkPhysicalDeviceProperties m_physicalDeviceProperties{};
    VkPhysicalDeviceFeatures m_physicalDeviceFeatures{};
    VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties{};

    // -- Logical Deivce
    VkDevice m_logicalDevice{VK_NULL_HANDLE};

    // - Extensions
    std::vector<std::string> m_supportedExtensions;

    // - Queue Familiy
    std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
    struct {
        uint32_t graphics = uint32_t(-1);
        uint32_t compute = uint32_t(-1);
        uint32_t transfer = uint32_t(-1);
    } m_queueFaimilyIndices{};

    // - Queue
    VkQueue m_graphicsQueue{VK_NULL_HANDLE};
    VkQueue m_computeQueue{VK_NULL_HANDLE};
    VkQueue m_transferQueue{VK_NULL_HANDLE};

    // Command Pool
    VkCommandPool m_graphicsCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_computeCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_transferCommandPool{VK_NULL_HANDLE};

    // Support
    // - Pointer to functions
    QVulkanFunctions* m_pFunctions{VK_NULL_HANDLE};
    QVulkanDeviceFunctions* m_pDeviceFunctions{VK_NULL_HANDLE};

};

#endif // VULKANRENDERER_H
