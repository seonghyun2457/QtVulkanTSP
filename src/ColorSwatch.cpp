#include "Colorswatch.h"

#include <qdebug.h>
#include <QPalette>
#include <QColorDialog>
#include <QMouseEvent>

ColorSwatch::ColorSwatch(QWidget *parent)
    : QWidget(parent)
{
    // Set min size
    setMinimumSize(26, 4);

    // Enable hover event
    setAttribute(Qt::WA_Hover, true);

    // Enable stylesheet
    setAttribute(Qt::WA_StyledBackground, true);

    // apply style
    applyStyle();

    // Clickable event
    setCursor(Qt::PointingHandCursor);
}

ColorSwatch::~ColorSwatch()
{

}

void ColorSwatch::initialize(const eNodeStatus iNodeStatus, const QColor iColor)
{
    m_nodeStatus = iNodeStatus;
    setColor(iColor);
}

const eNodeStatus ColorSwatch::getNodeStatus() const
{
    return m_nodeStatus;
}

void ColorSwatch::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        changeColor();
    }
    QWidget::mousePressEvent(event);
}

void ColorSwatch::applyStyle()
{
    setStyleSheet(QString(
                      "ColorSwatch {"
                      "  background-color: %1;"            // Set background color
                      "  border: 2px solid transparent;"   // Set transparent
                      "  border-radius: 4px;"
                      "}"
                      "ColorSwatch:hover {"
                      "  border: 2px solid #ffaa00;"       // Oragne color outline appearing when hovering
                      "}"
                      ).arg(m_color.name()));
}

void ColorSwatch::changeColor()
{
    const QColor newColor = QColorDialog::getColor(m_color, this);
    if (newColor.isValid()) {
        setColor(newColor);
    }
}

void ColorSwatch::setColor(const QColor iColor)
{
    m_color = iColor;
    applyStyle();

    glm::vec3 color(m_color.redF(), m_color.greenF(), m_color.blueF());
    emit colorSelcted(m_nodeStatus, color);
}
