#include "Rectangle.h"

#include <QtAssert>

#include "Vulkanrenderer.h"

Rectangle::Rectangle(VulkanRenderer* renderer, const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor)
    : m_renderer(renderer)
    , m_centerPos(iCenterPos)
    , m_halfWidth(iHalfWidth)
    , m_halfHeight(iHalfHeight)
    , m_color(iColor)
{
    buildUnitQuad();

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
    , m_centerPos(iOther.m_centerPos)
    , m_halfWidth(iOther.m_halfWidth)
    , m_halfHeight(iOther.m_halfHeight)
    , m_color(iOther.m_color)
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
        if (m_renderer) destroyBuffers();

        m_renderer = iOther.m_renderer;
        m_centerPos = iOther.m_centerPos;
        m_halfWidth = iOther.m_halfWidth;
        m_halfHeight = iOther.m_halfHeight;
        m_color = iOther.m_color;
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

void Rectangle::resetVertices(const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const glm::vec3 iColor)
{
    m_centerPos = iCenterPos;
    m_halfWidth = iHalfWidth;
    m_halfHeight = iHalfHeight;
    m_color = iColor;
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

void Rectangle::buildUnitQuad()
{
    m_vertices = {
        Vertex(glm::vec3(-1.f,  1.f, 0.f), m_color, glm::vec2(0.f, 0.f)),  // 0 top-left
        Vertex(glm::vec3(-1.f, -1.f, 0.f), m_color, glm::vec2(0.f, 1.f)),  // 1 bottom-left
        Vertex(glm::vec3( 1.f, -1.f, 0.f), m_color, glm::vec2(1.f, 1.f)),  // 2 bottom-right
        Vertex(glm::vec3( 1.f,  1.f, 0.f), m_color, glm::vec2(1.f, 0.f))   // 3 top-right
    };
}

