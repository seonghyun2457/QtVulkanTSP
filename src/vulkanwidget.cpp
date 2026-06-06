#include "vulkanwidget.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QExposeEvent>
#include <QDebug>

VulkanWidget::VulkanWidget()
    : QWindow()
    , m_pVulkanRenderer(nullptr)
{
    setSurfaceType(QWindow::VulkanSurface);

    // Disable mouseMoveEvent
    setMouseGrabEnabled(false);

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

bool VulkanWidget::event(QEvent* e)
{
    if (e->type() == QEvent::UpdateRequest) {
        for (size_t i = 0; i < m_objects.size(); ++i) {
            glm::mat4 modelMat = glm::rotate(glm::mat4(1.f), glm::radians(10.f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMat = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f));
            m_objects[i].setModel(modelMat);
        }
        draw();
        return true;
    }

    if (e->type() == QEvent::PlatformSurface) {
        auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(e);

        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            if (m_pVulkanRenderer) {
                emit sendDebugInfo("clean up");
                m_pVulkanRenderer->cleanup(m_objects);
                m_initisialized = false;
            }
        }
    }
    return QWindow::event(e);
}

void VulkanWidget::resizeEvent(QResizeEvent* event)
{
    if (m_initisialized && m_pVulkanRenderer != nullptr) {
        m_pVulkanRenderer->recreateSwapChain();
    }

    QWindow::resizeEvent(event);
}

void VulkanWidget::mousePressEvent(QMouseEvent* event)
{
    emit mousePressed(event->button(), event->position());
    QWindow::mousePressEvent(event);
}

void VulkanWidget::mouseMoveEvent(QMouseEvent* event)
{
    emit mouseMoved(event->position());
    QWindow::mouseMoveEvent(event);
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

        m_objects.emplace_back(m_pVulkanRenderer.get());
    };
}

void VulkanWidget::draw()
{
    emit sendDebugInfo("draw");

    if (m_pVulkanRenderer && m_initisialized) {
        m_pVulkanRenderer->draw(m_objects);
    }


    if (isExposed()) {
        requestUpdate();
    }

}

