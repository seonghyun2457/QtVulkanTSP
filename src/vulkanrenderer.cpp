#include "vulkanrenderer.h"

#include "vulkanwidget.h"

#include <QtAssert>


#include <exception>
#include <QDebug>

VulkanRenderer::VulkanRenderer(VulkanWidget* window)
    : m_window(window)
{

}

VulkanRenderer::~VulkanRenderer()
{
    qDebug() << "destroy VulkanRenderer";
}

void VulkanRenderer::initialize()
{
    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
}

void VulkanRenderer::cleanup()
{
    // Release user-created Vulkan resources here (swapchain, device, pipelines, etc.)
    // Do NOT destroy m_surface — it is owned by Qt's platform window.
    // Do NOT call m_vulkanInstance.destroy() — QVulkanInstance's destructor handles it;
    // calling it here causes a double-destroy when the member is later destructed.

    printDebugInfo("cleaned up");
}

void VulkanRenderer::createInstance()
{
    printDebugInfo("Create Instance");

    m_vulkanInstance.setApiVersion(m_vulkanInstance.supportedApiVersion());
    m_vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });

    if (!m_vulkanInstance.create()) {
        qCritical() << "Failed to create QVulkanInstance, error code:" << m_vulkanInstance.errorCode();
        return;
    }

    printVulkanInfo("Extensions:");
    for (const auto& extension : m_vulkanInstance.extensions()) {
        printVulkanInfo(extension);
    }

    printVulkanInfo("Vulkan api version: " + m_vulkanInstance.apiVersion().toString());

    m_window->setVulkanInstance(&m_vulkanInstance);

    m_pFunctions = m_vulkanInstance.functions();
    Q_ASSERT(m_pFunctions != nullptr);
}

void VulkanRenderer::createSurface()
{
    printDebugInfo("Create Surface");

    // Get VkSurfaceKHR info from QWindow
    m_surface = QVulkanInstance::surfaceForWindow(m_window);
}

void VulkanRenderer::selectPhysicalDevice()
{
    printDebugInfo("Select physical device");

    Q_ASSERT(m_pFunctions != nullptr);

    uint32_t deviceCount = 0;
    m_pFunctions->vkEnumeratePhysicalDevices(m_vulkanInstance.vkInstance(), &deviceCount, nullptr);

    // If no device available, then none support Vulkan!
    if (deviceCount == 0) throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");

    // Get list of physical devices
    std::vector<VkPhysicalDevice> gpuDeviceList(deviceCount);
    m_pFunctions->vkEnumeratePhysicalDevices(m_vulkanInstance.vkInstance(), &deviceCount, gpuDeviceList.data());

    // Pick suitable physical device
    // Always select first index device
    uint32_t selectedGPUIndex = 0;
    m_physicalDevice = gpuDeviceList[selectedGPUIndex];

    // Get properties of our new device
    VkPhysicalDeviceProperties deviceProperties;
    m_pFunctions->vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    printVulkanInfo("GPU name: " + QString(deviceProperties.deviceName));
}

void VulkanRenderer::createLogicalDevice()
{
    printDebugInfo("Select logical device");
}

void VulkanRenderer::printVulkanInfo(const QString &iString)
{
    emit m_window->sendVulkanInfo(iString);
}

void VulkanRenderer::printDebugInfo(const QString &iString)
{
    emit m_window->sendDebugInfo(iString);
}

