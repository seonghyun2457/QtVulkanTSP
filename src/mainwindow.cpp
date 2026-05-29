#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVulkanWindow>

#include <QPlainTextEdit>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
    , m_pVulkanWidget(new VulkanWidget())
{
    m_ui->setupUi(this);
    initializeVulkanWidget();
}

MainWindow::~MainWindow()
{

}


void MainWindow::initializeVulkanWidget()
{
    if (!m_pVulkanWidget) {
        qCritical() << "Failed to instantiate Vulkan window.";
        return;
    }

    QWidget* pWindowContainer = QWidget::createWindowContainer(m_pVulkanWidget, m_ui->vulkanWindow->parentWidget());
    pWindowContainer->setMouseTracking(true);
    pWindowContainer->setSizePolicy(m_ui->vulkanWindow->sizePolicy());
    pWindowContainer->setMinimumSize(m_ui->vulkanWindow->minimumSize());

    m_ui->horizontalLayout_2->replaceWidget(m_ui->vulkanWindow, pWindowContainer);
    delete m_ui->vulkanWindow;
    m_ui->vulkanWindow = pWindowContainer;
}
