#ifndef VULKANWIDGET_H
#define VULKANWIDGET_H

#include <QWindow>
#include <QMouseEvent>

#include "vulkanrenderer.h"
#include "rectangle.h"

class VulkanWidget : public QWindow
{
    Q_OBJECT
public:
    VulkanWidget();
    virtual ~VulkanWidget();

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
    void setRowCount(const uint32_t iRowCount);
    void setColumnCount(const uint32_t iColumnCount);

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

private:
    std::unique_ptr<VulkanRenderer> m_pVulkanRenderer;
    bool m_initisialized{false};

    uint32_t m_rowCount{2};
    uint32_t m_colCount{2};

    std::vector<bool> m_occupied;

    // Performace metrics
    static constexpr float FPS_UPDATE_INTERVAL_TIME{0.1f};
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    bool m_hasLastFrameTime{false};
    uint32_t m_cpuFrameSinceLastUpdate{0};
    float m_cpuFpsUpdateTimer{0.f};
    float m_gpuFpsUpdateTimer{0.f};
};

#endif // VULKANWIDGET_H
