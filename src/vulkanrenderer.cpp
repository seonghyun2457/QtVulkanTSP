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
}

void VulkanRenderer::cleanup()
{
    // Release user-created Vulkan resources here (swapchain, device, pipelines, etc.)
    // Do NOT destroy m_surface — it is owned by Qt's platform window.
    // Do NOT call m_vulkanInstance.destroy() — QVulkanInstance's destructor handles it;
    // calling it here causes a double-destroy when the member is later destructed.

    qDebug() << "cleaned up";
}

void VulkanRenderer::createInstance()
{
    qDebug() << "Create Instance";

    m_vulkanInstance.setApiVersion(m_vulkanInstance.supportedApiVersion());
    m_vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });

    if (!m_vulkanInstance.create()) {
        qCritical() << "Failed to create QVulkanInstance, error code:" << m_vulkanInstance.errorCode();
        return;
    }

    qDebug() << "Extensions";
    for (const auto& extension : m_vulkanInstance.extensions()) {
        qDebug() << extension;
    }
    qDebug() << "api version: " << m_vulkanInstance.apiVersion();

    m_window->setVulkanInstance(&m_vulkanInstance);

    m_pFunctions = m_vulkanInstance.functions();
    Q_ASSERT(m_pFunctions != nullptr);
}

void VulkanRenderer::createSurface()
{
    qDebug() << "Create Surface";

    // Get VkSurfaceKHR info from QWindow
    m_surface = QVulkanInstance::surfaceForWindow(m_window);
}

