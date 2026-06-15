#include "Rectangle.h"

#include <QtAssert>

#include "Vulkanrenderer.h"

Rectangle::Rectangle(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeight, const glm::vec3 iColor)
    : m_renderer(renderer)
    , m_color(iColor)
{
    m_vertices = {
                    Vertex(glm::vec3(iPos.x - halfWidth, iPos.y + halfHeight, 0.f), m_color, glm::vec2(0.f, 0.f)),  // 0
                    Vertex(glm::vec3(iPos.x - halfWidth, iPos.y - halfHeight, 0.f), m_color, glm::vec2(0.f, 1.f)),  // 1
                    Vertex(glm::vec3(iPos.x + halfWidth, iPos.y - halfHeight, 0.f), m_color, glm::vec2(1.f, 1.f)),  // 2
                    Vertex(glm::vec3(iPos.x + halfWidth, iPos.y + halfHeight, 0.f), m_color, glm::vec2(1.f, 0.f))   // 3
                 };


    m_indices = {0, 1, 2,
                 2, 3, 0};

    m_renderer->createVertexBuffer(m_vertices, m_vertexBuffer, m_vertexBufferMemory);
    m_renderer->createIndexBuffer(m_indices, m_indexBuffer, m_indexBufferMemory);

    m_model = glm::mat4(1.f);
}

Rectangle::~Rectangle()
{
    if (m_renderer) {
        destroyBuffers();
    }
    m_renderer = nullptr;
}

Rectangle::Rectangle(Rectangle&& iOther) noexcept
    : m_renderer(iOther.m_renderer)
    , m_model(iOther.m_model)
    , m_vertices(std::move(iOther.m_vertices))
    , m_indices(std::move(iOther.m_indices))
    , m_vertexBuffer(iOther.m_vertexBuffer)
    , m_vertexBufferMemory(iOther.m_vertexBufferMemory)
    , m_indexBuffer(iOther.m_indexBuffer)
    , m_indexBufferMemory(iOther.m_indexBufferMemory)
{
    iOther.m_renderer = nullptr;
    iOther.m_vertexBuffer = VK_NULL_HANDLE;
    iOther.m_vertexBufferMemory = VK_NULL_HANDLE;
    iOther.m_indexBuffer = VK_NULL_HANDLE;
    iOther.m_indexBufferMemory = VK_NULL_HANDLE;
}

Rectangle& Rectangle::operator=(Rectangle&& iOther) noexcept
{
    if (this != &iOther) {
        if (m_renderer) destroyBuffers();

        m_renderer = iOther.m_renderer;
        m_model = iOther.m_model;
        m_vertices = std::move(iOther.m_vertices);
        m_indices = std::move(iOther.m_indices);
        m_vertexBuffer = iOther.m_vertexBuffer;
        m_vertexBufferMemory = iOther.m_vertexBufferMemory;
        m_indexBuffer = iOther.m_indexBuffer;
        m_indexBufferMemory = iOther.m_indexBufferMemory;

        iOther.m_renderer = nullptr;
        iOther.m_vertexBuffer = VK_NULL_HANDLE;
        iOther.m_vertexBufferMemory = VK_NULL_HANDLE;
        iOther.m_indexBuffer = VK_NULL_HANDLE;
        iOther.m_indexBufferMemory = VK_NULL_HANDLE;
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

const uint32_t Rectangle::getIndexCount() const
{
    return m_indices.size();
}

const VkBuffer Rectangle::getVertexBuffer() const
{
    return m_vertexBuffer;
}

const VkBuffer Rectangle::getIndexBuffer() const
{
    return m_indexBuffer;
}

void Rectangle::destroyBuffers()
{
    Q_ASSERT(m_renderer != nullptr);

    // clean up vertex buffer parts
    m_renderer->destroyBuffer(m_vertexBuffer);
    m_renderer->destroyBufferMemory(m_vertexBufferMemory);

    // clean up index buffer parts
    m_renderer->destroyBuffer(m_indexBuffer);
    m_renderer->destroyBufferMemory(m_indexBufferMemory);
}

