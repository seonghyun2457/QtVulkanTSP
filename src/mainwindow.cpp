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

void MainWindow::displayVulkanInfo(const QString &iVulkanInfo)
{
    m_ui->vulkanInfo->appendPlainText(iVulkanInfo);
}

void MainWindow::displayDebugInfo(const QString &iDebugInfo)
{
    m_ui->debugLog->appendPlainText(iDebugInfo);
}


void MainWindow::initializeVulkanWidget()
{
    if (!m_pVulkanWidget) {
        displayDebugInfo("Failed to instantiate Vulkan window.");
        return;
    }

    // CONNECT
    connect(m_pVulkanWidget, &VulkanWidget::sendVulkanInfo, this, &MainWindow::displayVulkanInfo);
    connect(m_pVulkanWidget, &VulkanWidget::sendDebugInfo, this, &MainWindow::displayDebugInfo);

    // Window Container
    QWidget* pWindowContainer = QWidget::createWindowContainer(m_pVulkanWidget, m_ui->vulkanWindow->parentWidget());
    pWindowContainer->setMouseTracking(true);
    pWindowContainer->setSizePolicy(m_ui->vulkanWindow->sizePolicy());
    pWindowContainer->setMinimumSize(m_ui->vulkanWindow->minimumSize());

    // Replace Window widget
    m_ui->vulkanLayout->replaceWidget(m_ui->vulkanWindow, pWindowContainer);
    delete m_ui->vulkanWindow;
    m_ui->vulkanWindow = pWindowContainer;
}
