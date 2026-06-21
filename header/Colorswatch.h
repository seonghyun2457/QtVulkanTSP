#ifndef COLORSWATCH_H
#define COLORSWATCH_H

#include <QWidget>
#include <QColor>

// GLM
#include <glm/glm.hpp>

#include "eNodeStatus.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ColorSwatch;
}
QT_END_NAMESPACE

class ColorSwatch : public QWidget
{
    Q_OBJECT

public:
    explicit ColorSwatch(QWidget *parent = nullptr);
    ~ColorSwatch();
    void initialize(const eNodeStatus iNodeStatus, const QColor iColor);
    const eNodeStatus getNodeStatus() const;
signals:
    void colorSelcted(const eNodeStatus iNodeStatus, const glm::vec3 iColor);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void applyStyle();
    void changeColor();
    void setColor(const QColor iColor);

private:
    eNodeStatus m_nodeStatus{eNodeStatus::MovableNode};
    QColor m_color{Qt::black};

};

#endif // COLORSWATCH_H
