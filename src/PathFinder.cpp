#include "PathFinder.h"

#include <QDebug>
#include <QMessageBox>

#include <queue>
#include <stack>

#include "eNodeStatus.h"

bool PathFinder::solve(const eSolver iSolver, const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node>& iNodes, std::list<uint32_t>& oSolutionPaths)
{

    bool solutionFound = false;

    if (!(iStartingIndex < iNodes.size() && iEndingIndex < iNodes.size())) {
        QMessageBox::critical(nullptr, "", "Starting point and/or Ending point aren't set!");
        return solutionFound;
    }

    switch (iSolver) {
    case eSolver::BFS:
        solutionFound = bfs(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oSolutionPaths);
        break;
    case eSolver::DFS:
        solutionFound = dfs(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oSolutionPaths);
        break;
    case eSolver::Dijkstra:
        solutionFound = dijkstra(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oSolutionPaths);
        break;
    case eSolver::AStar:
        solutionFound = aStar(iStartingIndex, iEndingIndex, iRowSize, iColumnSize, iNodes, oSolutionPaths);
        break;
    default:
        qDebug() << "Undefined solver input.";
    }

    if (solutionFound) {
        QMessageBox::information(nullptr, "", "Solution found!");
    } else {
        QMessageBox::critical(nullptr, "", "Solution not found!");
    }

    return solutionFound;
}

bool PathFinder::bfs(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionPaths)
{
    bool found = false;
    std::set<uint32_t> discovered;

    // to - from
    std::map<uint32_t, uint32_t> prevs;

    std::queue<uint32_t> queue;
    queue.push(iStartingIndex);
    discovered.insert(iStartingIndex);

    while (!queue.empty()) {
        const uint32_t next = queue.front();
        queue.pop();

        if (next == iEndingIndex) {
            found = true;
            break;
        }

        std::vector<uint32_t> neighborIndices = getNeighborIndices(iRowSize, iColumnSize, next, iNodes);
        for (const uint32_t neighbor : neighborIndices) {
            if (discovered.find(neighbor) == discovered.end()) {
                queue.push(neighbor);
                discovered.insert(neighbor);

                prevs[neighbor] = next;
            }
        }
    }

    if (found) {
        storeSolution(prevs, iEndingIndex, oSolutionPaths);
    }

    return found;
}

bool PathFinder::dfs(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionPaths)
{
    bool found = false;

    std::set<uint32_t> discovered;
    // to - from
    std::map<uint32_t, uint32_t> prevs;
    std::stack<uint32_t> stack;

    stack.push(iStartingIndex);
    discovered.insert(iStartingIndex);

    while (!stack.empty()) {
        const uint32_t next = stack.top();
        stack.pop();

        if (next == iEndingIndex) {
            found = true;
            break;
        }

        std::vector<uint32_t> neighborIndices = getNeighborIndices(iRowSize, iColumnSize, next, iNodes);
        for (const uint32_t neighbor : neighborIndices) {
            if (discovered.find(neighbor) == discovered.end()) {
                stack.push(neighbor);
                discovered.insert(neighbor);

                prevs[neighbor] = next;
            }
        }
    }

    if (found) {
        storeSolution(prevs, iEndingIndex, oSolutionPaths);
    }

    return found;
}

bool PathFinder::dijkstra(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionPaths)
{
    bool found = false;
    std::vector<uint32_t> minDists(iNodes.size(), std::numeric_limits<uint32_t>::max());
    minDists[iStartingIndex] = 0;

    // to - from
    std::map<uint32_t, uint32_t> prevs;
    prevs[iEndingIndex] = std::numeric_limits<uint32_t>::max();

    struct compare {
        bool operator() (const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b){ return a.second > b.second; };
    } ;

    std::priority_queue<std::pair<uint32_t,uint32_t>, std::vector<std::pair<uint32_t,uint32_t>>,compare> open;

    open.push(std::pair<uint32_t, uint32_t>(iStartingIndex, 0));

    while (!open.empty()) {
        const std::pair<uint32_t, uint32_t> candidate = open.top();
        open.pop();

        const uint32_t candidateIndex = candidate.first;

        if (candidateIndex == iEndingIndex) {
            found = true;
            break;
        }

        const std::pair<int32_t, int32_t> from = getIndex2D(iColumnSize, candidateIndex);

        const uint32_t minDist = minDists[candidateIndex];
        const uint32_t dist = candidate.second;

        if (minDist < dist) {
            continue;
        }

        std::vector<uint32_t> neighborIndices = getNeighborIndices(iRowSize, iColumnSize, candidateIndex, iNodes);

        for (const uint32_t neighborIndex: neighborIndices) {
            const std::pair<int32_t, int32_t> to = getIndex2D(iColumnSize, neighborIndex);

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

    if (found) {
        storeSolution(prevs, iEndingIndex, oSolutionPaths);
    }

    return found;
}

bool PathFinder::aStar(const uint32_t iStartingIndex, const uint32_t iEndingIndex, const uint32_t iRowSize, const uint32_t iColumnSize, std::vector<Node> &iNodes, std::list<uint32_t>& oSolutionPaths)
{
    bool found = false;

    // gn: Moved distance from Origin to index
    std::vector<uint32_t> gns(iNodes.size(), std::numeric_limits<uint32_t>::max());
    // hn: Estimated distrance from index to Destination
    std::vector<uint32_t> hns(iNodes.size(), std::numeric_limits<uint32_t>::max());
    // fn: gn + hn
    std::vector<uint32_t> fns(iNodes.size(), std::numeric_limits<uint32_t>::max());

    // Initialize
    {
        // Initialize gns
        gns[iStartingIndex] = 0;

        // Initialize hns
        const std::pair<uint32_t, uint32_t> endIndex2D = getIndex2D(iColumnSize, iEndingIndex);

        for (size_t i = 0; i < iNodes.size(); ++i) {
            const std::pair<uint32_t, uint32_t> fromIndex2D = getIndex2D(iColumnSize, i);
            hns[i] = getDistance(fromIndex2D, endIndex2D);
        }

        // Initialize fns
        fns[iStartingIndex] = gns[iStartingIndex] + hns[iStartingIndex];
    }

    // to - from
    std::map<uint32_t, uint32_t> prevs;
    prevs[iEndingIndex] = std::numeric_limits<uint32_t>::max();

    struct compare {
        bool operator() (const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b){ return a.second > b.second; };
    } ;

    // first: index, second: fn(=gn + hn)
    std::priority_queue<std::pair<uint32_t, uint32_t>, std::vector<std::pair<uint32_t,uint32_t>>,compare> open;

    open.push(std::pair<uint32_t, uint32_t>(iStartingIndex, fns[iStartingIndex]));

    while (!open.empty()) {
        const std::pair<uint32_t, uint32_t> candidate = open.top();
        open.pop();
        const uint32_t candidateIndex = candidate.first;

        if (candidateIndex == iEndingIndex) {
            found = true;
            break;
        }

        const std::pair<int32_t, int32_t> from = getIndex2D(iColumnSize, candidateIndex);

        std::vector<uint32_t> neighborIndices = getNeighborIndices(iRowSize, iColumnSize, candidateIndex, iNodes);
        for (const uint32_t neighborIndex: neighborIndices) {
            const std::pair<int32_t, int32_t> to = getIndex2D(iColumnSize, neighborIndex);

            const uint32_t newGn = gns[candidateIndex] + getDistance(from, to);
            if (newGn >= gns[neighborIndex]) {
                continue;
            }
            gns[neighborIndex] = newGn;
            fns[neighborIndex] = gns[neighborIndex] + hns[neighborIndex];
            open.push(std::pair<uint32_t, uint32_t>(neighborIndex, fns[neighborIndex]));

            prevs[neighborIndex] = candidateIndex;
        }
    }

    if (found) {
        storeSolution(prevs, iEndingIndex, oSolutionPaths);
    }

    return found;
}

std::vector<uint32_t> PathFinder::getNeighborIndices(const uint32_t iRowSize, const uint32_t iColumnSize, const uint32_t iNodeIndex, std::vector<Node>& iNodes)
{
    std::vector<uint32_t> neighborIndices;

    const std::pair<uint32_t, uint32_t> index2D = getIndex2D(iColumnSize, iNodeIndex);

    // Left neighbor
    std::pair<uint32_t, uint32_t> leftNeighbor(index2D.first - 1, index2D.second);
    if (leftNeighbor.first < iColumnSize && iNodes[leftNeighbor.second * iColumnSize  + leftNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(leftNeighbor.second * iColumnSize  + leftNeighbor.first);
    }

    // Right neighbor
    std::pair<uint32_t, uint32_t> rightNeighbor(index2D.first + 1, index2D.second);
    if (rightNeighbor.first < iColumnSize && iNodes[rightNeighbor.second * iColumnSize  + rightNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(rightNeighbor.second * iColumnSize  + rightNeighbor.first);
    }

    // Upper neighbor
    std::pair<uint32_t, uint32_t> upperNeighbor(index2D.first, index2D.second - 1);
    if (upperNeighbor.second < iRowSize && iNodes[upperNeighbor.second * iColumnSize  + upperNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(upperNeighbor.second * iColumnSize  + upperNeighbor.first);
    }

    // Bottom neighbor
    std::pair<uint32_t, uint32_t> bottomNeighbor(index2D.first, index2D.second + 1);
    if (bottomNeighbor.second < iRowSize && iNodes[bottomNeighbor.second * iColumnSize  + bottomNeighbor.first].getNodeStatus() != eNodeStatus::blockingNode) {
        neighborIndices.push_back(bottomNeighbor.second * iColumnSize  + bottomNeighbor.first);
    }

    return neighborIndices;
}

const uint32_t PathFinder::getDistance(const std::pair<int32_t, int32_t> iSrcIndex2D, const std::pair<int32_t, int32_t> iDstIndex2D)
{
    return std::abs(iSrcIndex2D.first - iDstIndex2D.first) + std::abs(iSrcIndex2D.second - iDstIndex2D.second);
}

const std::pair<uint32_t, uint32_t> PathFinder::getIndex2D(const uint32_t iColumnSize, const uint32_t iIndex)
{
    return std::pair<uint32_t, uint32_t>(iIndex % iColumnSize, iIndex / iColumnSize);
}

void PathFinder::storeSolution(const std::map<uint32_t, uint32_t> &iPrevs, const uint32_t iEndingIndex, std::list<uint32_t>& oSolutionPaths)
{

    uint32_t prevIndex = iEndingIndex;
    oSolutionPaths.push_back(prevIndex);

    auto it = iPrevs.find(prevIndex);
    while (it != iPrevs.end()) {
        const uint32_t tmp = it->second;
        prevIndex = tmp;

        oSolutionPaths.push_back(prevIndex);

        it = iPrevs.find(prevIndex);
    }
}

