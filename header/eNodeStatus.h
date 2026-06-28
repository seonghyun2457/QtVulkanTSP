#ifndef ENODESTATUS_H
#define ENODESTATUS_H

enum class eNodeStatus {
    StartingNode,
    EndingNode,
    BlockingNode,
    MovableNode,
    VisitedNode,
    SolutionNode
};

#endif // ENODESTATUS_H
