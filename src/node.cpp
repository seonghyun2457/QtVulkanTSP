#include "node.h"

Node::Node(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeigh, const glm::vec3 iColor)
    : Rectangle(renderer, iPos, halfWidth, halfHeigh, iColor)
{

}

Node::~Node()
{

}

Node::Node(Node&& iOther) noexcept
    : Rectangle(std::move(iOther))
    , m_nodeStatus(iOther.m_nodeStatus)
    , m_visited(iOther.m_visited)
{

}

Node& Node::operator=(Node&& iOther) noexcept
{
    if (this != &iOther) {
        Rectangle::operator=(std::move(iOther));
        m_nodeStatus = iOther.m_nodeStatus;
        m_visited = iOther.m_visited;
    }

    return *this;
}

const eNodeStatus Node::getNodeStatus() const
{
    return m_nodeStatus;
}

void Node::setNodeStatus(const eNodeStatus iNodeStatus)
{
    m_nodeStatus = iNodeStatus;
}

const bool Node::getVisited() const
{
    return m_visited;
}

void Node::setVisited(const bool iVisited)
{
    m_visited = iVisited;
}