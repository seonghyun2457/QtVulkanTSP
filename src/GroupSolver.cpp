#include "GroupSolver.h"

#include "ui_GroupSolver.h"

GroupSolver::GroupSolver(QWidget* parent)
    : QGroupBox(parent),
      ui(new Ui::GroupSolver) {
    ui->setupUi(this);

    // CONNECT
    connect(ui->rbBfs, &QRadioButton::clicked, this, &GroupSolver::BfsClicked);
    connect(ui->rbDfs, &QRadioButton::clicked, this, &GroupSolver::DfsClicked);
    connect(ui->rbDijkstra, &QRadioButton::clicked, this, &GroupSolver::DijkstraClicked);
    connect(ui->rbAStar, &QRadioButton::clicked, this, &GroupSolver::AstarClicked);
}

GroupSolver::~GroupSolver() {
    delete ui;
}

void GroupSolver::initialize()
{
    ui->rbDijkstra->click();
}

void GroupSolver::BfsClicked()
{
    emit solverChanged(eSolver::BFS);
}

void GroupSolver::DfsClicked()
{
    emit solverChanged(eSolver::DFS);
}

void GroupSolver::DijkstraClicked() {
    emit solverChanged(eSolver::Dijkstra);
}

void GroupSolver::AstarClicked() {
    emit solverChanged(eSolver::AStar);
}