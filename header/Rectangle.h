#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <QVulkanFunctions>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos; // Vertex position (x, y, z)
    glm::vec3 col; // Vertex color (r, g, b)
    glm::vec2 uv;
};

class VulkanRenderer;

class Rectangle
{
public:
    Rectangle(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeigh, const glm::vec3 iColor);
    virtual ~Rectangle();

    Rectangle(const Rectangle& iOther) = delete;
    Rectangle& operator=(const Rectangle& iOther) = delete;

    Rectangle(Rectangle&& iOther) noexcept;
    Rectangle& operator=(Rectangle&& iOther) noexcept;

    const glm::mat4 getModel() const;
    void setModel(const glm::mat4& iModel);

    const glm::vec3 getColor() const;
    void setColor(const glm::vec3& iColor);

    const uint32_t getIndexCount() const;

    const VkBuffer getVertexBuffer() const;
    const VkBuffer getIndexBuffer() const;

protected:
    void destroyBuffers();

protected:
    VulkanRenderer* m_renderer{nullptr};
    glm::mat4 m_model;
    glm::vec3 m_color;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer m_vertexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_vertexBufferMemory{VK_NULL_HANDLE};

    VkBuffer m_indexBuffer{VK_NULL_HANDLE};
    VkDeviceMemory m_indexBufferMemory{VK_NULL_HANDLE};
};

#endif // RECTANGLE_H
