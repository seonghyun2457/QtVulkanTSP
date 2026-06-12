#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVulkanWindow>

#include <QColorDialog>
#include <QPlainTextEdit>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
    , m_pVulkanWidget(new VulkanWidget())
    , m_performaceMessage("Qt + Vulkan TSP - [CPU FPS: %1 (%2ms/frame), GPU FPS equiv: %3 (%4ms/frame)]")
{
    m_ui->setupUi(this);

    m_performaceMessage.reserve(256);

    initializeGuiWidgets();
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

void MainWindow::onMousePressed(const QPointF &position)
{
    displayDebugInfo(QString("Mouse clicked at (%1, %2)").arg(position.x()).arg(position.y()));
}

void MainWindow::onMouseMoved(const QPointF &position)
{
    displayDebugInfo(QString("Mouse moved to (%1, %2)").arg(position.x()).arg(position.y()));
}

void MainWindow::updateGpuTime(const float iGpuTime)
{
    m_gpuTime = iGpuTime;
    displayPerformace();
}

void MainWindow::updateCpuFps(const float iFps)
{
    m_cpuFps = iFps;
    displayPerformace();
}

void MainWindow::displayPerformace()
{
    const float cpuMsPerFrame = (m_cpuFps > 0.f) ? (1000.f / m_cpuFps) : 0.f;
    const float gpuFpsEquiv = (m_gpuTime > 0.f) ? (1000.f / m_gpuTime) : 0.f;

    setWindowTitle(m_performaceMessage.arg(m_cpuFps, 0, 'f', 3).arg(cpuMsPerFrame, 0, 'f', 3).arg(gpuFpsEquiv, 0, 'f', 3).arg(m_gpuTime, 0, 'f', 3));
}

void MainWindow::on_cbRow_activated(const int iIndex)
{
    bool ok = false;
    const uint32_t rowCount = m_ui->cbRow->itemText(iIndex).toUInt(&ok);
    if (ok) emit transferRowCount(rowCount);
}

void MainWindow::on_cbCol_activated(const int iIndex)
{
    bool ok = false;
    const uint32_t columnCount = m_ui->cbCol->itemText(iIndex).toUInt(&ok);
    if (ok) emit transferColumnCount(columnCount);
}

void MainWindow::initializeGuiWidgets()
{
    // CONNECT
    connect(this, &MainWindow::transferRowCount, m_pVulkanWidget, &VulkanWidget::setRowCount);
    connect(this, &MainWindow::transferColumnCount, m_pVulkanWidget, &VulkanWidget::setColumnCount);

    // Set initial Row/Column count
    emit transferRowCount(INITIAL_COUNT);
    emit transferColumnCount(INITIAL_COUNT);

    // initialize cbRow
    for (size_t i = INITIAL_COUNT; i <= MAX_ROW_COUNT; ++i) {
        m_ui->cbRow->addItem(QString::number(i));
    }

    // initialize cbCol
    for (size_t i = INITIAL_COUNT; i <= MAX_COLUMN_COUNT; ++i) {
        m_ui->cbCol->addItem(QString::number(i));
    }
}


void MainWindow::initializeVulkanWidget()
{
    if (!m_pVulkanWidget) {
        displayDebugInfo("Failed to instantiate Vulkan window.");
        return;
    }

    // CONNECT
    // - Logging
    connect(m_pVulkanWidget, &VulkanWidget::sendVulkanInfo, this, &MainWindow::displayVulkanInfo);
    connect(m_pVulkanWidget, &VulkanWidget::sendDebugInfo, this, &MainWindow::displayDebugInfo);
    connect(m_pVulkanWidget, &VulkanWidget::gpuTimeUpdated, this, &MainWindow::updateGpuTime);
    connect(m_pVulkanWidget, &VulkanWidget::cpuFpsUpdated, this, &MainWindow::updateCpuFps);

    // - Mouse events
    connect(m_pVulkanWidget, &VulkanWidget::mousePressed, this, &MainWindow::onMousePressed);
    connect(m_pVulkanWidget, &VulkanWidget::mouseMoved, this, &MainWindow::onMouseMoved);


    qDebug() << "m_pVulkanWidget->width(): " << m_pVulkanWidget->width();
    qDebug() << "m_pVulkanWidget->height(): " << m_pVulkanWidget->height();

    m_pVulkanWidget->setWidth(900);
    m_pVulkanWidget->setHeight(500);

    qDebug() << "m_pVulkanWidget->width(): " << m_pVulkanWidget->width();
    qDebug() << "m_pVulkanWidget->height(): " << m_pVulkanWidget->height();

    // Window Container
    QWidget* pWindowContainer = QWidget::createWindowContainer(m_pVulkanWidget, m_ui->vulkanWindow->parentWidget());
    //pWindowContainer->setMouseTracking(true);
    pWindowContainer->setSizePolicy(m_ui->vulkanWindow->sizePolicy());
    pWindowContainer->setMinimumSize(m_ui->vulkanWindow->minimumSize());

    qDebug() << "pWindowContainer->sizePolicy(): " << pWindowContainer->sizePolicy();
    qDebug() << "pWindowContainer->minimumSize(): " << pWindowContainer->minimumSize();
    qDebug() << "pWindowContainer->width(): " << pWindowContainer->width();
    qDebug() << "pWindowContainer->height(): " << pWindowContainer->height();
    pWindowContainer->resize(900, 500);
    qDebug() << "pWindowContainer->width(): " << pWindowContainer->width();
    qDebug() << "pWindowContainer->height(): " << pWindowContainer->height();

    // Replace Window widget
    m_ui->vulkanLayout->replaceWidget(m_ui->vulkanWindow, pWindowContainer);
    delete m_ui->vulkanWindow;
    m_ui->vulkanWindow = pWindowContainer;
}
