#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <QVulkanFunctions>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos; // Vertex position (x, y, z)
    glm::vec2 uv;
};

class VulkanRenderer;

class Rectangle
{
public:
    Rectangle(VulkanRenderer* renderer, const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor);
    virtual ~Rectangle();

    Rectangle(const Rectangle& iOther) = delete;
    Rectangle& operator=(const Rectangle& iOther) = delete;

    Rectangle(Rectangle&& iOther) noexcept;
    Rectangle& operator=(Rectangle&& iOther) noexcept;

    const glm::vec2 getCenterPos() const;
    const float getHalfWidth() const;
    const float getHalfHeight() const;

    const glm::mat4 getModel() const;
    void setModel(const glm::mat4& iModel);

    const glm::vec3 getColor() const;
    void setColor(const glm::vec3& iColor);

protected:
    void resetproperties(const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor);


protected:
    glm::mat4 m_model;
    glm::vec2 m_centerPos;
    float m_halfWidth;
    float m_halfHeight;
    glm::vec3 m_color;
};

#endif // RECTANGLE_H
