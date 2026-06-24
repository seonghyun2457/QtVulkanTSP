#include "Vulkanwidget.h"

#include <QDebug>
#include <QExposeEvent>
#include <QMessageBox>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <list>
#include <thread>

#include "PathFinder.h"

VulkanWidget::VulkanWidget()
    : QWindow(), m_pVulkanRenderer(nullptr)
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

void VulkanWidget::setMaxProblemSize(const uint32_t iMaxRowSize, const uint32_t iMaxColumnSize)
{
    m_maxRowSize = iMaxRowSize;
    m_maxColumnSize = iMaxColumnSize;

    m_rowSize = m_maxRowSize;
    m_columnSize = m_maxColumnSize;
    m_problemSize = m_rowSize * m_columnSize;
}

void VulkanWidget::clearScreen()
{
    m_screenBlocked = false;
    m_startingNodeIndex = static_cast<uint32_t>(-1);
    m_endingNodeIndex = static_cast<uint32_t>(-1);

    if (m_pVulkanRenderer != nullptr) {
        for (size_t i = 0; i < m_problemSize; ++i) {
            m_nodes[i].setNodeStatus(eNodeStatus::MovableNode);
            m_nodes[i].setColor(m_colors[eNodeStatus::MovableNode]);
        }
    }

    sendDebugInfo(QString("Cleared screen. Screen is unblocked."));
}

void VulkanWidget::resetSolution()
{
    m_screenBlocked = false;

    for (size_t i = 0; i < m_problemSize; ++i) {
        if (m_nodes[i].getNodeStatus() == eNodeStatus::MovableNode || m_nodes[i].getNodeStatus() == eNodeStatus::VisitedNode) {
            m_nodes[i].setNodeStatus(eNodeStatus::MovableNode);
            m_nodes[i].setColor(m_colors[eNodeStatus::MovableNode]);
        }
    }

    sendDebugInfo(QString("Reset solution. Screen is unblocked."));
}

void VulkanWidget::setSelectedNodeStatus(const eNodeStatus iNodeStatus)
{
    m_selectedNodeStatus = iNodeStatus;

    sendDebugInfo(QString("Selected node status: %1").arg(static_cast<int>(m_selectedNodeStatus)));
}

void VulkanWidget::solve()
{
    bool solutionFound = false;

    sendDebugInfo(QString("Solve TSP problem. Solving algorithm: %1. Screen is blocked.").arg(static_cast<int>(m_solver)));

    if (!(m_startingNodeIndex < m_problemSize && m_endingNodeIndex < m_problemSize)) {
        QMessageBox::critical(nullptr, "", "Starting point and/or Ending point aren't set!");
        return;
    }

    std::list<uint32_t> pathIndices;
    std::list<uint32_t> visitedIndices;
    solutionFound = PathFinder::solve(m_solver, m_startingNodeIndex, m_endingNodeIndex, m_rowSize, m_columnSize, m_nodes, pathIndices, visitedIndices);

    // Set color for visited nodes
    {
        const long long NANO_SECONDS_THRESHOLD = 250;
        auto startTime = std::chrono::steady_clock ::now();
        long long nanoSeconds = 0;

        for (const uint32_t visitedIndex : visitedIndices) {
            while (nanoSeconds < NANO_SECONDS_THRESHOLD) {
                auto endTime = std::chrono::steady_clock ::now();
                auto diffMilliSeconds = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
                nanoSeconds += diffMilliSeconds;
                startTime = endTime;

                if (m_nodes[visitedIndex].getNodeStatus() != eNodeStatus::StartingNode && m_nodes[visitedIndex].getNodeStatus() != eNodeStatus::EndingNode) {
                    m_nodes[visitedIndex].setNodeStatus(eNodeStatus::VisitedNode);
                    m_nodes[visitedIndex].setColor(m_colors[eNodeStatus::VisitedNode]);
                    draw();
                }
            }

            nanoSeconds = 0;
        }

    }

    if (!solutionFound) {
        QMessageBox::critical(nullptr, "", "Solution not found!");
        return;
    }

    // Set color for solution paths
    {
        Q_ASSERT(pathIndices.size() >= 2);
        const float colorDiff = 1.f / static_cast<float>(pathIndices.size() - 1);
        float factor = 0.f;

        for (const uint32_t pathIndex : pathIndices) {
            m_nodes[pathIndex].setColor(((1.f - factor) * m_colors[eNodeStatus::EndingNode]) + (factor * m_colors[eNodeStatus::StartingNode]));

            factor += colorDiff;
        }
    }

    QMessageBox::information(nullptr, "", "Solution found!");

    m_screenBlocked = true;
}

void VulkanWidget::setRowSize(const uint32_t iRowSize)
{
    m_rowSize = iRowSize;
    m_problemSize = m_rowSize * m_columnSize;
    resetNodes();

    sendDebugInfo(QString("Row size changed to %1").arg(m_rowSize));
}

void VulkanWidget::setColumnSize(const uint32_t iColumnSize)
{
    m_columnSize = iColumnSize;
    m_problemSize = m_rowSize * m_columnSize;
    resetNodes();

    sendDebugInfo(QString("Column size changed to %1").arg(m_columnSize));
}

void VulkanWidget::setColorSetting(const eNodeStatus iNodeStatus, const glm::vec3 iColor)
{
    m_colors[iNodeStatus] = iColor;

    for (Node& node : m_nodes) {
        if (node.getNodeStatus() == iNodeStatus) {
            node.setColor(m_colors[iNodeStatus]);
        }
    }
    sendDebugInfo(QString("Node status %1's color changed to RGB(%2, %3, %4)").arg(static_cast<int>(iNodeStatus)).arg(iColor.r).arg(iColor.g).arg(iColor.b));
}

void VulkanWidget::changeNodeStatus(const uint32_t iIndex)
{
    if (m_selectedNodeStatus == eNodeStatus::StartingNode) {
        if (m_startingNodeIndex < m_problemSize) {
            m_nodes[m_startingNodeIndex].setNodeStatus(eNodeStatus::MovableNode);
            m_nodes[m_startingNodeIndex].setColor(m_colors[eNodeStatus::MovableNode]);
        }

        m_startingNodeIndex = iIndex;

    } else if (m_selectedNodeStatus == eNodeStatus::EndingNode) {
        if (m_endingNodeIndex < m_problemSize) {
            m_nodes[m_endingNodeIndex].setNodeStatus(eNodeStatus::MovableNode);
            m_nodes[m_endingNodeIndex].setColor(m_colors[eNodeStatus::MovableNode]);
        }

        m_endingNodeIndex = iIndex;
    } else {
        if (m_startingNodeIndex == iIndex && iIndex < m_problemSize) {
            m_startingNodeIndex = static_cast<uint32_t>(-1);
        } else if(m_endingNodeIndex == iIndex && iIndex < m_problemSize) {
            m_endingNodeIndex = static_cast<uint32_t>(-1);
        }
    }


    m_nodes[iIndex].setNodeStatus(m_selectedNodeStatus);
    m_nodes[iIndex].setColor(m_colors[m_selectedNodeStatus]);
}

void VulkanWidget::setSolver(const eSolver iSolver)
{
    m_solver = iSolver;
    sendDebugInfo(QString("Solver changed to %1").arg(static_cast<int>(m_solver)));
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
    if (!m_screenBlocked && (event->button() == Qt::LeftButton)) {
        traceMousePosition(event->position());

        emit mousePressed(event->position());
    }
    QWindow::mousePressEvent(event);
}

void VulkanWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_screenBlocked && (event->buttons() & Qt::LeftButton)) {
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
    const float rectangleWidth = width() / static_cast<float>(m_columnSize);
    const float rectangleHeight = height() / static_cast<float>(m_rowSize);

    const size_t rectangleColIndex = static_cast<size_t>(iPosition.x() / rectangleWidth);
    const size_t rectangleRowIndex = static_cast<size_t>(iPosition.y() / rectangleHeight);

    const size_t index = (rectangleRowIndex * m_columnSize) + rectangleColIndex;

    changeNodeStatus(index);
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
        createNodes();
        emit sendDebugInfo("Succeeded to initialize Vulkan renderer");
    };
}

void VulkanWidget::draw()
{
    if (m_pVulkanRenderer && m_initisialized) {
        m_pVulkanRenderer->draw(m_nodes, m_problemSize);

        auto currentTime = std::chrono::high_resolution_clock::now();

        if (m_hasLastFrameTime) {
            const float deltaTime = std::chrono::duration<float>(currentTime - m_lastFrameTime).count();  // in second
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
        const float rectangleWidth = width() / static_cast<float>(m_columnSize);
        const float rectangleHeight = height() / static_cast<float>(m_rowSize);


        const float normalizedRectangleHalfWidth = 1.f / static_cast<float>(m_columnSize);
        const float normalizedRectangleHalfHeight = 1.f / static_cast<float>(m_rowSize);

        float halfWidth = 0.5f * width();
        float halfHeight = 0.5f * height();

        for (size_t i = 0; i < m_rowSize; ++i) {
            for (size_t j = 0; j < m_columnSize; ++j) {
                const size_t index = (i * m_columnSize) + j;

                float normalizedXPos = (((j * rectangleWidth) - halfWidth) / static_cast<float>(halfWidth)) + normalizedRectangleHalfWidth;
                float normalizedYPos = -((((i * rectangleHeight) - halfHeight) / static_cast<float>(halfHeight)) + normalizedRectangleHalfHeight);
                glm::vec2 centerPos{normalizedXPos, normalizedYPos};

                m_nodes[index].reset(centerPos, normalizedRectangleHalfWidth, normalizedRectangleHalfHeight, eNodeStatus::MovableNode, m_colors[eNodeStatus::MovableNode]);
            }
        }
    }
}

void VulkanWidget::createNodes()
{
    if (m_pVulkanRenderer != nullptr) {
        m_pVulkanRenderer->createNodes(m_maxRowSize, m_maxColumnSize, m_colors[eNodeStatus::MovableNode], m_nodes);
        resetNodes();
    }
}
