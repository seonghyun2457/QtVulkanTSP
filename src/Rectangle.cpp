#include "Rectangle.h"

#include <QtAssert>

#include "VulkanRenderer.h"

Rectangle::Rectangle(VulkanRenderer* renderer, const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor)
    : m_centerPos(iCenterPos)
    , m_halfWidth(iHalfWidth)
    , m_halfHeight(iHalfHeight)
    , m_color(iColor)
    , m_model(glm::mat4(1.f))
{

}

Rectangle::~Rectangle()
{

}

Rectangle::Rectangle(Rectangle&& iOther) noexcept
    : m_centerPos(iOther.m_centerPos)
    , m_halfWidth(iOther.m_halfWidth)
    , m_halfHeight(iOther.m_halfHeight)
    , m_color(iOther.m_color)
    , m_model(iOther.m_model)
{

}

const glm::vec2 Rectangle::getCenterPos() const
{
    return m_centerPos;
}

const float Rectangle::getHalfWidth() const
{
    return m_halfWidth;
}

const float Rectangle::getHalfHeight() const
{
    return m_halfHeight;
}

Rectangle& Rectangle::operator=(Rectangle&& iOther) noexcept
{
    if (this != &iOther) {
        m_centerPos = iOther.m_centerPos;
        m_halfWidth = iOther.m_halfWidth;
        m_halfHeight = iOther.m_halfHeight;
        m_color = iOther.m_color;
        m_model = iOther.m_model;
    }

    return *this;
}

const glm::mat4 Rectangle::getModel() const
{
    return m_model;
}

void Rectangle::setModel(const glm::mat4 &iModel)
{
    m_model = iModel;
}

const glm::vec3 Rectangle::getColor() const
{
    return m_color;
}

void Rectangle::setColor(const glm::vec3 &iColor)
{
    /*
    for (Vertex& vertex : m_vertices) {
        vertex.col = iColor;
    }
    */
    m_color = iColor;
}

void Rectangle::resetproperties(const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor)
{
    m_centerPos = iCenterPos;
    m_halfWidth = iHalfWidth;
    m_halfHeight = iHalfHeight;
    m_color = iColor;
}