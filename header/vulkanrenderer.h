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

    void initialize();
    void cleanup();
private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();

    // Print information
    void printVulkanInfo(const QString& iString);
    void printDebugInfo(const QString& iString);

private:
    // Window
    VulkanWidget* m_window;

    // QVulkanInstance
    QVulkanInstance m_vulkanInstance;

    // Vulkan components
    VkSurfaceKHR m_surface{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicalDevice;


    // Pointer to function
    QVulkanFunctions* m_pFunctions{nullptr};
    QVulkanDeviceFunctions* m_pDeviceFunctions{nullptr};

};

#endif // VULKANRENDERER_H
