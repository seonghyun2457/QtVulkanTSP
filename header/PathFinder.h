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

private:
    static bool bfs();
    static bool dfs();
    static bool dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);
    static bool aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);

    static std::vector<uint32_t> getNeighborIndices(const uint32_t iRowSize, const uint32_t iColumnSize, const uint32_t iIndexX, const uint32_t iIndexY, std::vector<Node>& iNodes);
    static const uint32_t getDistance(const std::pair<int32_t, int32_t> iFrom,  const std::pair<int32_t, int32_t> iTo);
};

#endif // PATHFINDER_H
