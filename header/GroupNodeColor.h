#ifndef GROUPNODECOLOR_H
#define GROUPNODECOLOR_H

#include <QGroupBox>
#include <eNodeStatus.h>

#include <glm/glm.hpp>

QT_BEGIN_NAMESPACE
namespace Ui {
class GroupNodeColor;
}
QT_END_NAMESPACE

class GroupNodeColor : public QGroupBox
{
    Q_OBJECT

public:
    explicit GroupNodeColor(QWidget *parent = nullptr);
    virtual ~GroupNodeColor();

    void initialize();

signals:
    void nodeStatusSelected(const eNodeStatus iNodeStatus);
    void colorSelcted(const eNodeStatus iNodeStatus, const glm::vec3 iColor);

public slots:
    void on_rbStartingNode_clicked();
    void on_rbEndingNode_clicked();
    void on_rbBlockingNode_clicked();
    void on_rbMovableNode_clicked();

    void colorReceived(const eNodeStatus iNodeStatus, const glm::vec3 iColor);

private:
    Ui::GroupNodeColor* m_ui;
};

#endif // GROUPNODECOLOR_H
