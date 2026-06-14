#ifndef NODE_H
#define NODE_H

#include "rectangle.h"
#include "eNodeStatus.h"

class Node final : public Rectangle
{
public:
    Node(VulkanRenderer* renderer, const glm::vec2& iPos, const float halfWidth, const float halfHeigh, const glm::vec3 iColor);
    virtual ~Node();

    Node(const Node& iOther) = delete;
    Node& operator=(const Node& iOther) = delete;

    Node(Node&& iOther) noexcept;
    Node& operator=(Node&& iOther) noexcept;

    const eNodeStatus getNodeStatus() const;
    void setNodeStatus(const eNodeStatus iNodeStatus);

    const bool getVisited() const;
    void setVisited(const bool iVisited);
private:
    eNodeStatus m_nodeStatus{eNodeStatus::movableNode};
    bool m_visited{false};
};

#endif // NODE_H
