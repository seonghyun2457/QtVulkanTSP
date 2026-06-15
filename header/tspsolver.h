#ifndef TSPSOLVER_H
#define TSPSOLVER_H

#include <vector>
#include <list>

#include "node.h"
#include "eSolver.h"

class TSPSolver
{
public:
    TSPSolver() = delete;
    virtual ~TSPSolver() = delete;

    static bool solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);
    static bool dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);
    static bool aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths);

private:

};

#endif // TSPSOLVER_H
