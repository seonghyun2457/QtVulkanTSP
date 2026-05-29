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

private slots:

private:
    void initializeVulkanWidget();

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    VulkanWidget* m_pVulkanWidget;
};
#endif // MAINWINDOW_H
