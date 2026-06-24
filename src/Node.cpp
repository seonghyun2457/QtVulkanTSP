#include "Node.h"

Node::Node(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeight, const glm::vec3 iColor)
    : Rectangle(renderer, iPos, halfWidth, halfHeight, iColor)
{

}

Node::~Node()
{

}

Node::Node(Node&& iOther) noexcept
    : Rectangle(std::move(iOther))
    , m_nodeStatus(iOther.m_nodeStatus)
{

}

Node& Node::operator=(Node&& iOther) noexcept
{
    if (this != &iOther) {
        Rectangle::operator=(std::move(iOther));
        m_nodeStatus = iOther.m_nodeStatus;
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

void Node::reset(const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const eNodeStatus iNodeStatus, const glm::vec3 iColor)
{
    m_nodeStatus = iNodeStatus;
    resetproperties(iCenterPos, iHalfWidth, iHalfHeight, iColor);
}