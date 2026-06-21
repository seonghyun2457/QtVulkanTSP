#include "GroupNodeColor.h"
#include "ui_GroupNodeColor.h"
#include "Colorswatch.h"

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
    connect(m_ui->csVisitedNode, &ColorSwatch::colorSelcted, this, &GroupNodeColor::colorReceived);
}

GroupNodeColor::~GroupNodeColor()
{
    delete m_ui;
}

void GroupNodeColor::initialize()
{
    // Initialize
    m_ui->csStartingNode->initialize(eNodeStatus::StartingNode, Qt::green);
    m_ui->csEndingNode->initialize(eNodeStatus::EndingNode, Qt::red);
    m_ui->csBlockingNode->initialize(eNodeStatus::BlockingNode, Qt::white);
    m_ui->csMovableNode->initialize(eNodeStatus::MovableNode, Qt::black);

    // Set color of visited Node
    {
        QColor skyBlue;
        skyBlue.setRgbF(0.f / 255.f, 181.f / 255.f, 226.f / 255.f);
        m_ui->csVisitedNode->initialize(eNodeStatus::VisitedNode, skyBlue);
        m_ui->rbVisitedNode->click();
        m_ui->rbVisitedNode->setCheckable(false);
    }

    // Click Starting button by default
    m_ui->rbStartingNode->click();
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
