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
    setMouseGrabEnabled(true);

    m_occupied.resize(m_rowCount * m_colCount, false);
}

VulkanWidget::~VulkanWidget()
{
    qDebug() << "destroy VulkanWidget";
    m_pVulkanRenderer = nullptr;
    qDebug() << "VulkanWidget destroyed";
}

void VulkanWidget::setRowCount(const uint32_t iRowCount)
{
    m_rowCount = iRowCount;
    sendDebugInfo("row count: " + QString::number(m_rowCount));
    m_occupied.resize(m_rowCount * m_colCount, false);
}

void VulkanWidget::setColumnCount(const uint32_t iColumnCount)
{
    m_colCount = iColumnCount;
    sendDebugInfo("column count: " + QString::number(m_colCount));
    m_occupied.resize(m_rowCount * m_colCount, false);
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
        draw();
        return true;
    }

    if (e->type() == QEvent::PlatformSurface) {
        auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(e);

        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            if (m_pVulkanRenderer) {
                emit sendDebugInfo("clean up");
                m_initisialized = false;
                m_pVulkanRenderer->cleanup();
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
    if (event->button() == Qt::LeftButton) {
        traceMousePosition(event->position());

        emit mousePressed(event->position());
    }
    QWindow::mousePressEvent(event);
}

void VulkanWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        traceMousePosition(event->position());

        emit mouseMoved(event->position());
    }
    QWindow::mouseMoveEvent(event);
}

void VulkanWidget::traceMousePosition(const QPointF& iPosition)
{
    const float rectangleWidth = (width() / static_cast<float>(m_rowCount));
    const float rectangleHeight = (height() / static_cast<float>(m_colCount));

    const size_t rectangleRowIndex = static_cast<size_t>(iPosition.x() / rectangleWidth);
    const size_t rectangleColIndex = static_cast<size_t>(iPosition.y() / rectangleHeight);

    if (m_occupied[(rectangleColIndex * m_rowCount) + rectangleRowIndex] == false) {
        m_occupied[(rectangleColIndex * m_rowCount) + rectangleRowIndex] = true;

        const float normalizedRectangleHalfWidth = rectangleWidth / static_cast<float>(width());
        const float normalizedRectangleHalfHeight = rectangleHeight / static_cast<float>(height());

        float halfWidth = 0.5f * width();
        float halfHeight = 0.5f * height();

        float normalizedXPos = (((rectangleRowIndex * rectangleWidth) - halfWidth) / static_cast<float>(halfWidth)) + normalizedRectangleHalfWidth;
        float normalizedYPos = -((((rectangleColIndex * rectangleHeight) - halfHeight) / static_cast<float>(halfHeight)) + normalizedRectangleHalfHeight);

        glm::vec2 recPos{normalizedXPos, normalizedYPos};
        m_pVulkanRenderer->addRectangle(recPos, normalizedRectangleHalfWidth, normalizedRectangleHalfHeight);
    }
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
    if (m_pVulkanRenderer && m_initisialized) {
        m_pVulkanRenderer->draw();

        auto currentTime = std::chrono::high_resolution_clock::now();

        if (m_hasLastFrameTime) {
            const float deltaTime = std::chrono::duration<float>(currentTime - m_lastFrameTime).count(); // in second
            updatePerformanceMetrics(deltaTime);
        }

        m_lastFrameTime = currentTime;
        m_hasLastFrameTime = true;
    }


    if (isExposed()) {
        requestUpdate();
    }

}

void VulkanWidget::updatePerformanceMetrics(const float iDeltaTime)
{
    m_cpuFrameSinceLastUpdate++;
    m_cpuFpsUpdateTimer += iDeltaTime;

    if (m_cpuFpsUpdateTimer >= FPS_UPDATE_INTERVAL_TIME) {
        float cpuFps = m_cpuFrameSinceLastUpdate / m_cpuFpsUpdateTimer;

        emit cpuFpsUpdated(cpuFps);

        // Reset counters
        m_cpuFrameSinceLastUpdate = 0;
        m_cpuFpsUpdateTimer = 0.f;
    }

    m_gpuFpsUpdateTimer += iDeltaTime;

    if (m_gpuFpsUpdateTimer >= (2.f * FPS_UPDATE_INTERVAL_TIME)) {
        float gpuTimeMs = m_pVulkanRenderer->readGPUFrameTime();

        emit gpuTimeUpdated(gpuTimeMs);

        // Reset timer
        m_gpuFpsUpdateTimer = 0.f;
    }
}

