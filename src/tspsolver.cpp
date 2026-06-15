#include "tspsolver.h"
#include <queue>

#include <QDebug>

#include "eNodeStatus.h"

bool TSPSolver::solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths)
{

    bool solutionFound = false;

    if (!(iStartingIndex < iNodes.size() && iEndingIndex < iNodes.size())) {
        return solutionFound;
    }

    switch (iSolver) {
    case eSolver::Dijkstra:
        solutionFound = dijkstra(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oPaths);
    case eSolver::AStar:
        solutionFound = aStar(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oPaths);
    default:
        qDebug() << "Undefined solver input.";
    }

    return solutionFound;
}

bool TSPSolver::dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oPaths)
{
    std::vector<uint32_t> minDists(iNodes.size(), std::numeric_limits<uint32_t>::max());
    minDists[iStartingIndex] = 0;

    // to - from
    std::map<uint32_t, uint32_t> prevs;
    prevs[iEndingIndex] = std::numeric_limits<uint32_t>::max();

    struct compare {
        bool operator() (const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b){ return a.second < b.second; };
    } ;

    std::priority_queue<std::pair<uint32_t,uint32_t>, std::vector<std::pair<uint32_t,uint32_t>>,compare> open;

    open.push(std::pair<uint32_t, uint32_t>{iStartingIndex, 0});

    while (!open.empty()) {
        const std::pair<uint32_t, uint32_t> candidate = open.top();
        open.pop();

        const uint32_t candidateIndex = candidate.first;

        const uint32_t indexX = candidateIndex % iColumnSize;
        const uint32_t indexY = candidateIndex / iColumnSize;

        const uint32_t minDist = minDists[candidateIndex];
        const uint32_t dist = candidate.second;

        if (minDist < dist) {
            continue;
        }

        std::vector<uint32_t> neighborIndices;

        // Find neighbors
        {
            // Left neighbor
            if (indexX - 1 < iColumnSize && iNodes[indexY * iColumnSize  + (indexX - 1)].getNodeStatus() != eNodeStatus::blockingNode ) {
                neighborIndices.push_back(indexY * iColumnSize  + (indexX - 1));
            }

            // Right neighbor
            if (indexX + 1 < iColumnSize && iNodes[indexY * iColumnSize  + (indexX + 1)].getNodeStatus() != eNodeStatus::blockingNode ) {
                neighborIndices.push_back(indexY * iColumnSize  + (indexX + 1));
            }

            // Upper neighbor
            if (indexY - 1 < iRowSize && iNodes[(indexY - 1) * iColumnSize  + indexX].getNodeStatus() != eNodeStatus::blockingNode ) {
                neighborIndices.push_back((indexY - 1) * iColumnSize  + indexX);
            }

            // Bottom neighbor
            if (indexY + 1 < iRowSize && iNodes[(indexY + 1) * iColumnSize  + indexX].getNodeStatus() != eNodeStatus::blockingNode ) {
                neighborIndices.push_back((indexY + 1) * iColumnSize  + indexX);
            }
        }

        for (const uint32_t neighborIndex: neighborIndices) {
            const uint32_t newDist = minDist + 1;
            const uint32_t nextMinDist = minDists[neighborIndex];

            if (newDist >= nextMinDist) {
                continue;
            }

            minDists[neighborIndex] = newDist;
            prevs[neighborIndex] = candidateIndex;

            open.push(std::pair<uint32_t, uint32_t>{neighborIndex, newDist});
        }
    }

    bool found = false;
    uint32_t prevIndex = iEndingIndex;
    oPaths.push_back(prevIndex);

    while (prevs.find(prevIndex) != prevs.end()) {
        uint32_t tmp = prevs[prevIndex];
        prevIndex = tmp;

        oPaths.push_back(prevIndex);
    }

    if (prevIndex == iStartingIndex) {
        found = true;
    }

    if (!found) {
        oPaths.clear();
    }

    return found;
}

bool TSPSolver::aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oPaths)
{
    return true;
}

