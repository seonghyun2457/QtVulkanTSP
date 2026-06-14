#include "vulkanwidget.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QExposeEvent>
#include <QDebug>

#include "tspsolver.h"

VulkanWidget::VulkanWidget()
    : QWindow()
    , m_pVulkanRenderer(nullptr)
{
    setSurfaceType(QWindow::VulkanSurface);

    // Disable mouseMoveEvent
    setMouseGrabEnabled(true);
}

VulkanWidget::~VulkanWidget()
{
    qDebug() << "destroy VulkanWidget";
    m_pVulkanRenderer = nullptr;
    qDebug() << "VulkanWidget destroyed";
}

void VulkanWidget::wipeScreen()
{
    m_screenBlocked = false;
    m_startingNodeIndex = static_cast<uint32_t>(-1);
    m_endingNodeIndex = static_cast<uint32_t>(-1);

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        m_nodes[i].setNodeStatus(eNodeStatus::movableNode);
        m_nodes[i].setColor(m_colors[eNodeStatus::movableNode]);
        m_nodes[i].setVisited(false);
    }
}

void VulkanWidget::setSelectedNodeStatus(const eNodeStatus iNodeStatus)
{
    m_selectedNodeStatus = iNodeStatus;
}

void VulkanWidget::solve()
{
    bool solutionFound = TSPSolver::solve(m_solver, m_startingNodeIndex, m_endingNodeIndex, m_rowSize, m_colSize, m_nodes);

    m_screenBlocked = true;
}

void VulkanWidget::setRowSize(const uint32_t iRowSize)
{
    m_rowSize = iRowSize;
    resetNodes();
}

void VulkanWidget::setColumnSize(const uint32_t iColumnSize)
{
    m_colSize = iColumnSize;
    resetNodes();
}

void VulkanWidget::setColorSetting(const eNodeStatus iNodeStatus, glm::vec3 iColor)
{
    m_colors[iNodeStatus] = iColor;

    for (Node& node : m_nodes) {
        if (node.getNodeStatus() == iNodeStatus) {
            node.setColor(m_colors[iNodeStatus]);
        }
    }
}

void VulkanWidget::changeNodeStatus(const uint32_t iIndex)
{
    if (m_selectedNodeStatus == eNodeStatus::startingNode) {
        if (m_startingNodeIndex < m_nodes.size()) {
            m_nodes[m_startingNodeIndex].setNodeStatus(eNodeStatus::movableNode);
            m_nodes[m_startingNodeIndex].setColor(m_colors[eNodeStatus::movableNode]);
        }
        m_startingNodeIndex = iIndex;
    } else if (m_selectedNodeStatus == eNodeStatus::endingNode) {
        if (m_endingNodeIndex < m_nodes.size()) {
            m_nodes[m_endingNodeIndex].setNodeStatus(eNodeStatus::movableNode);
            m_nodes[m_endingNodeIndex].setColor(m_colors[eNodeStatus::movableNode]);
        }
        m_endingNodeIndex = iIndex;
    }

    m_nodes[iIndex].setNodeStatus(m_selectedNodeStatus);
    m_nodes[iIndex].setColor(m_colors[m_selectedNodeStatus]);
}

void VulkanWidget::setSolver(const eSolver iSolver)
{
    m_solver = iSolver;
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
                m_pVulkanRenderer->cleanup(m_nodes);
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
        const QPointF mousePosition = event->position();
        if ((0.f <= mousePosition.x() && mousePosition.x() <= static_cast<double>(width())) &&
            (0.f <= mousePosition.y() && mousePosition.y() <= static_cast<double>(height()))) {
            traceMousePosition(mousePosition);

            emit mouseMoved(mousePosition);
        }
    }
    QWindow::mouseMoveEvent(event);
}

void VulkanWidget::traceMousePosition(const QPointF& iPosition)
{
    const float rectangleWidth = width() / static_cast<float>(m_colSize);
    const float rectangleHeight = height() / static_cast<float>(m_rowSize);

    const size_t rectangleColIndex = static_cast<size_t>(iPosition.x() / rectangleWidth);
    const size_t rectangleRowIndex = static_cast<size_t>(iPosition.y() / rectangleHeight);

    const size_t index = (rectangleRowIndex * m_colSize) + rectangleColIndex;

    changeNodeStatus(index);

    emit sendDebugInfo("change Rectangle color");
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
        resetNodes();
        emit sendDebugInfo("Succeeded to initialize Vulkan renderer");
    };
}

void VulkanWidget::draw()
{
    if (m_pVulkanRenderer && m_initisialized) {
        m_pVulkanRenderer->draw(m_nodes);

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

void VulkanWidget::resetNodes()
{
    if (m_pVulkanRenderer != nullptr) {
        m_pVulkanRenderer->resetNodes(m_rowSize, m_colSize, m_colors[eNodeStatus::movableNode], m_nodes);
    }
}

