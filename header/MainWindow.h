#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <GroupSolver.h>

#include <QMainWindow>

#include "PathFinder.h"
#include "VulkanWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

signals:
    void transferRowSize(const uint32_t iRowSize);
    void transferColumnSize(const uint32_t iColumnSize);

private slots:
    // Logging slots
    void displayVulkanInfo(const QString& iVulkanInfo);
    void displayDebugInfo(const QString& iDebugInfo);

    // Mouse event slots
    void onMousePressed(const QPointF& position);
    void onMouseMoved(const QPointF& position);

    // Disply performance metrics
    void updateGpuTime(const float iGpuTime);
    void updateCpuFps(const float iFps);
    void displayPerformace();

    // Row & Column slots
    void on_cbRow_currentIndexChanged(const int iIndex);
    void on_cbCol_currentIndexChanged(const int iIndex);


    // Node Status slot
    void nodeStatusSelected(const eNodeStatus iNodeStatus);

    // TSP slots
    void on_btnSolve_clicked();
    void on_btnReset_clicked();
    void on_btnClear_clicked();

private:
    void initializeGuiWidgets();
    void initializeVulkanWidget();
    void initializeGroupColor();
    void initializeSolver();

private:
    std::unique_ptr<Ui::MainWindow> m_ui;

    // Vulkan Widget
    VulkanWidget* m_pVulkanWidget{nullptr};
    static constexpr uint32_t VULKAN_WIDGET_WIDTH{900};
    static constexpr uint32_t VULKAN_WIDGET_HEIGHT{500};


    // Problem size definitions
    static constexpr uint32_t INITIAL_SIZE{2};
    static constexpr uint32_t MAX_ROW_SIZE{40};
    static constexpr uint32_t MAX_COLUMN_SIZE{50};

    // Performance Metrics
    static const QString s_performaceMessage;
    float m_gpuTime{0.f};
    float m_cpuFps{0.f};
};
#endif  // MAINWINDOW_H
