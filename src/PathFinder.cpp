#include "PathFinder.h"
#include <queue>

#include <QDebug>

#include "eNodeStatus.h"

bool PathFinder::solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oPaths)
{

    bool solutionFound = false;

    if (!(iStartingIndex < iNodes.size() && iEndingIndex < iNodes.size())) {
        return solutionFound;
    }

    switch (iSolver) {
    case eSolver::BFS:
        break;
    case eSolver::DFS:
        break;
    case eSolver::Dijkstra:
        solutionFound = dijkstra(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oPaths);
        break;
    case eSolver::AStar:
        solutionFound = aStar(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oPaths);
        break;
    default:
        qDebug() << "Undefined solver input.";
    }

    return solutionFound;
}

bool PathFinder::bfs()
{

}

bool PathFinder::dfs()
{

}

bool PathFinder::dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oPaths)
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

        std::pair<int32_t, int32_t> from(indexX, indexY);

        const uint32_t minDist = minDists[candidateIndex];
        const uint32_t dist = candidate.second;

        if (minDist < dist) {
            continue;
        }

        std::vector<uint32_t> neighborIndices = getNeighborIndices(iRowSize, iColumnSize, indexX, indexY, iNodes);

        for (const uint32_t neighborIndex: neighborIndices) {

            const uint32_t neighborIndexX = neighborIndex % iColumnSize;
            const uint32_t neighborIndexY = neighborIndex / iColumnSize;

            std::pair<int32_t, int32_t> to(neighborIndexX, neighborIndexY);

            const uint32_t newDist = minDist + getDistance(from, to);
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

bool PathFinder::aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oPaths)
{
    return true;
}

std::vector<uint32_t> PathFinder::getNeighborIndices(const uint32_t iRowSize, const uint32_t iColumnSize, const uint32_t iIndexX, const uint32_t iIndexY, std::vector<Node>& iNodes)
{
    std::vector<uint32_t> neighborIndices;

    // Left neighbor
    std::pair<uint32_t, uint32_t> leftNeighbor(iIndexX - 1, iIndexY);
    if (leftNeighbor.first < iColumnSize && iNodes[leftNeighbor.second * iColumnSize  + leftNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(leftNeighbor.second * iColumnSize  + leftNeighbor.first);
    }

    // Right neighbor
    std::pair<uint32_t, uint32_t> rightNeighbor(iIndexX + 1, iIndexY);
    if (rightNeighbor.first < iColumnSize && iNodes[rightNeighbor.second * iColumnSize  + rightNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(rightNeighbor.second * iColumnSize  + rightNeighbor.first);
    }

    // Upper neighbor
    std::pair<uint32_t, uint32_t> upperNeighbor(iIndexX, iIndexY - 1);
    if (upperNeighbor.second < iRowSize && iNodes[upperNeighbor.second * iColumnSize  + upperNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(upperNeighbor.second * iColumnSize  + upperNeighbor.first);
    }

    // Bottom neighbor
    std::pair<uint32_t, uint32_t> bottomNeighbor(iIndexX, iIndexY + 1);
    if (bottomNeighbor.second < iRowSize && iNodes[bottomNeighbor.second * iColumnSize  + bottomNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(bottomNeighbor.second * iColumnSize  + bottomNeighbor.first);
    }

    return neighborIndices;
}

const uint32_t PathFinder::getDistance(const std::pair<int32_t, int32_t> iFrom, const std::pair<int32_t, int32_t> iTo)
{
    return std::abs(iFrom.first - iTo.first) + std::abs(iFrom.second - iTo.second);
}

