#include "vulkanwidget.h"

#include <QExposeEvent>
#include <QDebug>

VulkanWidget::VulkanWidget()
    : QWindow()
    , m_pVulkanRenderer(nullptr)
{
    setSurfaceType(QWindow::VulkanSurface);
}

VulkanWidget::~VulkanWidget()
{
    qDebug() << "destroy VulkanWidget";
    m_pVulkanRenderer = nullptr;
    qDebug() << "VulkanWidget destroyed";
}

void VulkanWidget::exposeEvent(QExposeEvent* event)
{
    if (isExposed() && !m_initisialized) {
        m_initisialized = true;
        initializeRenderer();
    }

    if (isExposed()) {
        draw();
    }

    QWindow::exposeEvent(event);
}

bool VulkanWidget::event(QEvent *e)
{
    if (e->type() == QEvent::PlatformSurface) {
        auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(e);

        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            if (m_pVulkanRenderer) {
                emit sendDebugInfo("clean up");
                m_pVulkanRenderer->cleanup();
            }
        }
    }
    return QWindow::event(e);
}

void VulkanWidget::initializeRenderer()
{
    m_pVulkanRenderer = std::make_unique<VulkanRenderer>(this);
    if (m_pVulkanRenderer == nullptr) {
        emit sendDebugInfo("Failed to crate Vulkan renderer");
        return;
    }

    sendDebugInfo("Initialize Vulkan renderer");
    m_initisialized = m_pVulkanRenderer->initialize();
    if (!m_initisialized) {
        emit sendDebugInfo("Failed to initialize Vulkan renderer");
    } else {
        emit sendDebugInfo("Succeeded to initialize Vulkan renderer");
    };
}

void VulkanWidget::draw()
{
    emit sendDebugInfo("draw");

}

