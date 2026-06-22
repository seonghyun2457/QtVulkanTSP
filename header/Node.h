#ifndef NODE_H
#define NODE_H

#include "Rectangle.h"
#include "eNodeStatus.h"

class Node final : public Rectangle
{
public:
    Node(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeight, const glm::vec3 iColor);
    virtual ~Node();

    Node(const Node& iOther) = delete;
    Node& operator=(const Node& iOther) = delete;

    Node(Node&& iOther) noexcept;
    Node& operator=(Node&& iOther) noexcept;

    const eNodeStatus getNodeStatus() const;
    void setNodeStatus(const eNodeStatus iNodeStatus);

    void reset(const glm::vec2 iCenterPos, const float iHalfWidth, const float iHalfHeight, const eNodeStatus iNodeStatus, const glm::vec3 iColor);
private:
    eNodeStatus m_nodeStatus{eNodeStatus::MovableNode};
};

#endif // NODE_H
