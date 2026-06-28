#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <list>
#include <map>

#include "Node.h"
#include "eSolver.h"

class PathFinder
{
public:
    PathFinder() = delete;
    virtual ~PathFinder() = delete;

    static bool solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex,
                      const uint32_t iRowSize, const uint32_t iColumnSize,
                      std::vector<Node>& iNodes, std::list<uint32_t>& oSolutionIndices, std::list<uint32_t>& oVisitedIndices);

private:
    static bool bfs(const uint32_t iStartingIndex, const uint32_t iEndingIndex,
                    const uint32_t iRowSize, const uint32_t iColumnSize,
                    std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionIndices, std::list<uint32_t>& oVisitedIndices);

    static bool dfs(const uint32_t iStartingIndex, const uint32_t iEndingIndex,
                    const uint32_t iRowSize, const uint32_t iColumnSize,
                    std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionIndices, std::list<uint32_t>& oVisitedIndices);

    static bool dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex,
                         const uint32_t iRowSize, const uint32_t iColumnSize,
                         std::vector<Node>& iNodes, std::list<uint32_t>& oSolutionIndices, std::list<uint32_t>& oVisitedIndices);

    static bool aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex,
                      const uint32_t iRowSize, const uint32_t iColumnSize,
                      std::vector<Node>& iNodes, std::list<uint32_t>& oSolutionIndices, std::list<uint32_t>& oVisitedIndices);

    static std::vector<uint32_t> getNeighborIndices(const uint32_t iRowSize, const uint32_t iColumnSize, const uint32_t iNodeIndex, std::vector<Node>& iNodes);
    static const uint32_t getDistance(const std::pair<int32_t, int32_t> iSrcIndex2D, const std::pair<int32_t, int32_t> iDstIndex2D);

    static const std::pair<uint32_t, uint32_t> getIndex2D(const uint32_t iColumnSize, const uint32_t iIndex);

    static void storeSolution(const std::map<uint32_t, uint32_t>& iPrevs, const uint32_t iEndingIndex, std::list<uint32_t>& oSolutionIndices);
};

#endif // PATHFINDER_H
