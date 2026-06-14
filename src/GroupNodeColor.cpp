#include "GroupNodeColor.h"
#include "ui_GroupNodeColor.h"
#include "colorswatch.h"

GroupNodeColor::GroupNodeColor(QWidget *parent)
    : QGroupBox(parent)
    , m_ui(new Ui::GroupNodeColor)
{
    m_ui->setupUi(this);

    // CONNECT
    connect(m_ui->csStartingNode, &ColorSwatch::colorSelcted, this, &GroupNodeColor::colorReceived);
    connect(m_ui->csEndingNode, &ColorSwatch::colorSelcted, this, &GroupNodeColor::colorReceived);
    connect(m_ui->csBlockingNode, &ColorSwatch::colorSelcted, this, &GroupNodeColor::colorReceived);
    connect(m_ui->csMovableNode, &ColorSwatch::colorSelcted, this, &GroupNodeColor::colorReceived);
}

GroupNodeColor::~GroupNodeColor()
{
    delete m_ui;
}

void GroupNodeColor::initialize()
{
    // Initialize
    m_ui->csStartingNode->initialize(eNodeStatus::startingNode, Qt::yellow);
    m_ui->csEndingNode->initialize(eNodeStatus::endingNode, Qt::green);
    m_ui->csBlockingNode->initialize(eNodeStatus::blockingNode, Qt::red);
    m_ui->csMovableNode->initialize(eNodeStatus::movableNode, Qt::black);

    // Click Blocking button by default
    m_ui->rbBlockingNode->click();
}

void GroupNodeColor::on_rbStartingNode_clicked()
{
    emit nodeStatusSelected(m_ui->csStartingNode->getNodeStatus());
}

void GroupNodeColor::on_rbEndingNode_clicked()
{
    emit nodeStatusSelected(m_ui->csEndingNode->getNodeStatus());
}

void GroupNodeColor::on_rbBlockingNode_clicked()
{
    emit nodeStatusSelected(m_ui->csBlockingNode->getNodeStatus());
}

void GroupNodeColor::on_rbMovableNode_clicked()
{
    emit nodeStatusSelected(m_ui->csMovableNode->getNodeStatus());
}

void GroupNodeColor::colorReceived(const eNodeStatus iNodeStatus, const glm::vec3 iColor)
{
    emit colorSelcted(iNodeStatus, iColor);
}
