#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "vulkanwidget.h"

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
    void transferRowCount(const uint32_t iRowCount);
    void transferColumnCount(const uint32_t iColumnCount);

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
    void setRowCount(const int iIndex);
    void setColumnCount(const int iIndex);
private:
    void initializeGuiWidgets();
    void initializeVulkanWidget();

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    VulkanWidget* m_pVulkanWidget;

    static constexpr size_t INITIAL_COUNT{2};
    static constexpr size_t MAX_ROW_COUNT{40};
    static constexpr size_t MAX_COLUMN_COUNT{30};

    // Performance Metrics
    QString m_performaceMessage{""};
    float m_gpuTime{0.f};
    float m_cpuFps{0.f};
};
#endif // MAINWINDOW_H
