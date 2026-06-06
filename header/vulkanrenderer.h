#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanInstance>
#include <QVulkanFunctions>

#include <glm/glm.hpp>


#include "Rectangle.h"

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
    void cleanup(std::vector<Rectangle>& iObjects);

    void recreateSwapChain();

    void draw(const std::vector<Rectangle>& iObjects);

    void createVertexBuffer(const std::vector<Vertex>& iVertices, VkBuffer& oBuffer, VkDeviceMemory& oBufferMemory);
    void createIndexBuffer(const std::vector<uint32_t>& iIndices, VkBuffer& oBuffer, VkDeviceMemory& oBufferMemory);
    void destroyBuffer(VkBuffer& ioBuffer);
    void destroyBufferMemory(VkDeviceMemory& ioBufferMemory);

private:
    void createInstance();
    void createSurface();

    // Before graphics pipeline
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createQueues();
    void createSwapChain();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& iShaderCode);

    // After graphics pipeline
    void createCommandPools();
    VkCommandPool createCommandPool(const uint32_t iQueueFamilyIndex);
    void createCommandBuffers();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    void recordCommands(const uint32_t iImageIndex, const std::vector<Rectangle>& iObjects);
    void updateUniformBuffers(const uint32_t iImageIndex);

    // Synchronization
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

    // Buffer methods
    void copyBuffer(const VkDeviceSize iBufferSize, VkBuffer& iSrcBuffer, VkBuffer& iDstBuffer);
    void createBuffer(const VkPhysicalDevice iPhysicalDevice,
                      const VkDevice iDevice,
                      const VkDeviceSize iBufferSize,
                      const VkBufferUsageFlags iBufferUsageFlags,
                      const VkMemoryPropertyFlags iBufferProperties,
                      VkBuffer& oBuffer,
                      VkDeviceMemory& oBufferMemory);
    const uint32_t findMemoryTypeIndex(const VkPhysicalDevice iPhysicalDevice, const uint32_t allowdedTypes, const VkMemoryPropertyFlags properties);

    // Extension functions
    bool extensionSupported(const char* iExtension) const;
private:
    // Window
    VulkanWidget* m_window{nullptr};

    struct UboModelViewProjection {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    } m_uboModelViewProjection;

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

    // Graphics Pipeline
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};

    // - Command Pool
    VkCommandPool m_graphicsCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_computeCommandPool{VK_NULL_HANDLE};
    VkCommandPool m_transferCommandPool{VK_NULL_HANDLE};

    // - Command buffer
    std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

    // - Uniform buffer
    std::vector<VkBuffer> m_uboModelViewProjectionBuffers;
    std::vector<VkDeviceMemory> m_uboModelViewProjectionBuffersMemory;

    // - Descriptor pool
    VkDescriptorPool m_descriptorPool{VK_NULL_HANDLE};

    // - Descriptor Set Layout
    VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};

    // - Descriptor sets
    std::vector<VkDescriptorSet> m_descriptorSets;


    // - Synchronizaiton resources
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{2}; // Normally 2 or 3
    std::vector<VkSemaphore> m_imagesAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_fences;
    size_t m_currentFrame{0};

    // SUPPORT
    // - Pointer to functions
    QVulkanFunctions* m_pFunctions{VK_NULL_HANDLE};
    QVulkanDeviceFunctions* m_pDeviceFunctions{VK_NULL_HANDLE};

    // - Read File
    std::vector<char> readFile(const std::string& iFilePath);
};

#endif // VULKANRENDERER_H
