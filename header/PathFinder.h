#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <list>

#include "Node.h"
#include "eSolver.h"

class PathFinder
{
public:
    PathFinder() = delete;
    virtual ~PathFinder() = delete;

    static bool solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);
    static bool dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);
    static bool aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);

private:

};

#endif // PATHFINDER_H
