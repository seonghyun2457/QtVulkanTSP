#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "vulkanwidget.h"
#include "tspsolver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

signals:
    void transferRowSize(const uint32_t iRowSize);
    void transferColumnSize(const uint32_t iColumnSize);

private slots:
    // Logging slot
    void displayVulkanInfo(const QString& iVulkanInfo);
    void displayDebugInfo(const QString& iDebugInfo);

    // Mouse event slot
    void onMousePressed(const QPointF& position);
    void onMouseMoved(const QPointF& position);

    // Disply performance metrics
    void updateGpuTime(const float iGpuTime);
    void updateCpuFps(const float iFps);
    void displayPerformace();

    // Row & Column slot
    void on_cbRow_activated(const int iIndex);
    void on_cbCol_activated(const int iIndex);

    // Node Status slot
    void on_rbStartingNode_clicked();
    void on_rbEndingNode_clicked();
    void on_rbBlockingNode_clicked();
    void on_rbMovableNode_clicked();

    // Test
    //void on_btnColorDialog_clicked();
private:
    void initializeGuiWidgets();
    void initializeVulkanWidget();
    void initializeColorSwatch();
    void initializeSolver();

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    std::unique_ptr<VulkanWidget> m_pVulkanWidget{nullptr};
    std::unique_ptr<TSPSolver> m_solver{nullptr};

    static constexpr uint32_t INITIAL_SIZE{2};
    static constexpr uint32_t MAX_ROW_SIZE{30};
    static constexpr uint32_t MAX_COLUMN_SIZE{40};

    // Performance Metrics
    QString m_performaceMessage{""};
    float m_gpuTime{0.f};
    float m_cpuFps{0.f};
};
#endif // MAINWINDOW_H
