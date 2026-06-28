#include "MainWindow.h"

#include <QColorDialog>
#include <QDebug>
#include <QPlainTextEdit>
#include <QVulkanWindow>

#include "ui_MainWindow.h"

const QString MainWindow::s_performaceMessage = "Qt + Vulkan Pathfinder - [CPU FPS: %1 (%2ms/frame), GPU FPS equiv: %3 (%4ms/frame)]";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
    , m_pVulkanWidget(new VulkanWidget())
{
    m_ui->setupUi(this);

    // Initialize
    initializeVulkanWidget();
    initializeGuiWidgets();
    initializeGroupColor();
    initializeSolver();
}

MainWindow::~MainWindow()
{
    qDebug() << "destroying m_pVulkanWidget";
    // m_pVulkanWidget will be free in widget tree.
    m_pVulkanWidget = nullptr;
    qDebug() << "destroyed m_pVulkanWidget";
}

void MainWindow::displayVulkanInfo(const QString& iVulkanInfo)
{
    m_ui->vulkanInfo->appendPlainText(iVulkanInfo);
}

void MainWindow::displayDebugInfo(const QString& iDebugInfo)
{
    m_ui->debugLog->appendPlainText(iDebugInfo);
}

void MainWindow::onMousePressed(const QPointF& position)
{
    displayDebugInfo(QString("Mouse clicked at (%1, %2)").arg(position.x()).arg(position.y()));
}

void MainWindow::onMouseMoved(const QPointF& position) {
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

    setWindowTitle(s_performaceMessage.arg(m_cpuFps, 0, 'f', 3).arg(cpuMsPerFrame, 0, 'f', 3).arg(gpuFpsEquiv, 0, 'f', 3).arg(m_gpuTime, 0, 'f', 3));
}

void MainWindow::on_cbRow_currentIndexChanged(const int iIndex)
{
    bool ok = false;
    const uint32_t rowSize = m_ui->cbRow->itemText(iIndex).toUInt(&ok);
    if (ok) {
        emit transferRowSize(rowSize);
        on_btnClear_clicked();
    }
}

void MainWindow::on_cbCol_currentIndexChanged(const int iIndex)
{
    bool ok = false;
    const uint32_t columnSize = m_ui->cbCol->itemText(iIndex).toUInt(&ok);
    if (ok) {
        emit transferColumnSize(columnSize);
        on_btnClear_clicked();
    }
}

void MainWindow::nodeStatusSelected(const eNodeStatus iNodeStatus)
{
    m_pVulkanWidget->setSelectedNodeStatus(iNodeStatus);
}

void MainWindow::on_btnSolve_clicked()
{
    displayDebugInfo("Solve button clicked");
    // Reset solution then solve
    m_pVulkanWidget->resetSolution();
    m_pVulkanWidget->solve();
}

void MainWindow::on_btnReset_clicked()
{
    displayDebugInfo("Reset button clicked");
    m_pVulkanWidget->resetSolution();
}

void MainWindow::on_btnClear_clicked()
{
    displayDebugInfo("Clear button clicked");
    m_pVulkanWidget->clearWindow();
}

void MainWindow::initializeGuiWidgets()
{
    // CONNECT
    connect(this, &MainWindow::transferRowSize, m_pVulkanWidget, &VulkanWidget::setRowSize);
    connect(this, &MainWindow::transferColumnSize, m_pVulkanWidget, &VulkanWidget::setColumnSize);

    m_pVulkanWidget->setMaxProblemSize(MAX_ROW_SIZE, MAX_COLUMN_SIZE);

    // initialize cbRow
    for (size_t i = INITIAL_SIZE; i <= MAX_ROW_SIZE; ++i) {
        m_ui->cbRow->addItem(QString::number(i));
    }

    // initialize cbCol
    for (size_t i = INITIAL_SIZE; i <= MAX_COLUMN_SIZE; ++i) {
        m_ui->cbCol->addItem(QString::number(i));
    }

    // Set initial Row/Column
    {
        m_ui->cbRow->setCurrentIndex(m_ui->cbRow->count() - 1);
        m_ui->cbCol->setCurrentIndex(m_ui->cbCol->count() - 1);
    }
}

void MainWindow::initializeVulkanWidget()
{
    if (m_pVulkanWidget == nullptr) {
        qCritical() << "Failed to instantiate Vulkan widget.";
        exit(1);
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


    // Window Container
    QWidget* pWindowContainer = QWidget::createWindowContainer(m_pVulkanWidget, m_ui->vulkanWindow->parentWidget());
    pWindowContainer->setSizePolicy(m_ui->vulkanWindow->sizePolicy());
    pWindowContainer->setMinimumSize(m_ui->vulkanWindow->minimumSize());
    pWindowContainer->resize(VULKAN_WIDGET_WIDTH, VULKAN_WIDGET_HEIGHT);

    displayDebugInfo(QString("Initial vulkan window size: [width: %1, height: %2]").arg(pWindowContainer->width()).arg(pWindowContainer->height()));

    // Replace Window widget
    m_ui->vulkanLayout->replaceWidget(m_ui->vulkanWindow, pWindowContainer);
    delete m_ui->vulkanWindow;
    m_ui->vulkanWindow = pWindowContainer;
}

void MainWindow::initializeGroupColor()
{
    // CONNECT
    connect(m_ui->gbNodeColor, &GroupNodeColor::nodeStatusSelected, this, &MainWindow::nodeStatusSelected);
    connect(m_ui->gbNodeColor, &GroupNodeColor::colorSelcted, m_pVulkanWidget, &VulkanWidget::setColorSetting);

    m_ui->gbNodeColor->initialize();
}

void MainWindow::initializeSolver()
{
    connect(m_ui->gbSolver, &GroupSolver::solverChanged, m_pVulkanWidget, &VulkanWidget::setSolver);

    m_ui->gbSolver->initialize();
}
