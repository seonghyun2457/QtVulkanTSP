#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanInstance>
#include <QVulkanFunctions>

class VulkanWidget;

typedef struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;     // Surface properties, e.g. image size/extent
    std::vector<VkSurfaceFormatKHR> formats;          // Surface image formats, e.g. RGBA and size of each color
    std::vector<VkPresentModeKHR> presentationModes;   // How images should be presented to screen
} SwapChainDetails_t;


typedef struct SwapchainImage {
    VkImage image{VK_NULL_HANDLE};
    VkImageView imageView{VK_NULL_HANDLE};
} SwapchainImage_t;


class VulkanRenderer
{

public:
    VulkanRenderer(VulkanWidget* vulkanWindow);
    virtual ~VulkanRenderer();

    bool initialize();
    void cleanup();

    void recreateSwapChain();
private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createQueues();
    void createCommandPools();
    VkCommandPool createCommandPool(const uint32_t iQueueFamilyIndex);
    void createSwapChain();
    void createDescriptorSetLayout();
    void createSynchronization();

    // Print information
    void printVulkanInfo(const QString& iString) const;
    void printDebugInfo(const QString& iString) const;

    // Queue Family methods
    const uint32_t getQueueFamilyIndex(const VkQueueFlags iQueueFlags) const;

    // SwapChain methods
    const SwapChainDetails_t getSwapChainDetails(const VkPhysicalDevice& iDevice);
    const VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& iFormats);
    const VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& iPresentationModes);
    const VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& iSurfaceCapabilities);
    const VkImageView createImageView(const VkImage iImage, const VkFormat iFormat, const VkImageAspectFlags iAspectFlags);
    void destroySwapChainResources();

    // Extension functions
    bool extensionSupported(const char* iExtension) const;
private:
    // Window
    VulkanWidget* m_window{nullptr};

    // VULKAN COMPONENTS
    // - QVulkanInstance
    QVulkanInstance m_vulkanInstance;

    // - Surface
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    VkSurfaceFormatKHR m_surfaceFormat{};

    // - SwapChain
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    VkSwapchainKHR m_oldSwapchain{VK_NULL_HANDLE};
    SwapChainDetails_t m_swapchainDetails;
    std::vector<SwapchainImage_t> m_swapchainImages;
    VkExtent2D m_extent{};

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

    // - Command Pool
    VkCommandPool m_graphicsCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_computeCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_transferCommandPool{VK_NULL_HANDLE};

    // - Descriptor Set Layout
    VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};

    // - Synchronizaiton resources
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{2};
    std::vector<VkSemaphore> m_imagesAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_fences;


    // SUPPORT
    // - Pointer to functions
    QVulkanFunctions* m_pFunctions{VK_NULL_HANDLE};
    QVulkanDeviceFunctions* m_pDeviceFunctions{VK_NULL_HANDLE};

};

#endif // VULKANRENDERER_H
