#ifndef VULKANWIDGET_H
#define VULKANWIDGET_H

#include <QWindow>
#include <QMouseEvent>

#include "vulkanrenderer.h"
#include "eSolver.h"

class VulkanWidget : public QWindow
{
    Q_OBJECT
public:
    VulkanWidget();
    virtual ~VulkanWidget();

    void wipeScreen();

    void setSelectedNodeStatus(const eNodeStatus iNodeStatus);

    void solve();

signals:
    // Logging
    void sendVulkanInfo(const QString& iVulkanInfoString);
    void sendDebugInfo(const QString& iDebugString);

    // Mouse Event signals
    void mousePressed(const QPointF& iPosition);
    void mouseMoved(const QPointF& iPosition);

    // Performance metrics
    void gpuTimeUpdated(const float iGpuTimeMs);
    void cpuFpsUpdated(const float iCpuFps);

public slots:
    void setRowSize(const uint32_t iRowSize);
    void setColumnSize(const uint32_t iColumnSize);

    void setColorSetting(const eNodeStatus iNodeStatus, glm::vec3 iColor);
    void changeNodeStatus(const uint32_t iIndex);

    void setSolver(const eSolver iSolver);

protected:
    virtual void exposeEvent(QExposeEvent* event) override;
    virtual bool event(QEvent* e) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    // Mouse Events
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    void traceMousePosition(const QPointF& iPosition);
    void initializeRenderer();
    void draw();
    void updatePerformanceMetrics(const float iDeltaTime);
    void resetNodes();

private:
    std::unique_ptr<VulkanRenderer> m_pVulkanRenderer;
    bool m_initisialized{false};

    // Row, Column size
    uint32_t m_rowSize{2};
    uint32_t m_colSize{2};

    // Performace metrics
    static constexpr float FPS_UPDATE_INTERVAL_TIME{0.1f};
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    bool m_hasLastFrameTime{false};
    uint32_t m_cpuFrameSinceLastUpdate{0};
    float m_cpuFpsUpdateTimer{0.f};
    float m_gpuFpsUpdateTimer{0.f};

    // Node
    eNodeStatus m_selectedNodeStatus{eNodeStatus::movableNode};
    uint32_t m_startingNodeIndex{static_cast<uint32_t>(-1)};
    uint32_t m_endingNodeIndex{static_cast<uint32_t>(-1)};
    std::map<eNodeStatus, glm::vec3> m_colors;
    std::vector<Node> m_nodes;

    bool m_screenBlocked{false};
    eSolver m_solver{eSolver::Dijkstra};
};

#endif // VULKANWIDGET_H
