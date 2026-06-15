#ifndef GROUPSOLVER_H
#define GROUPSOLVER_H

#include <QGroupBox>

#include "eSolver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class GroupSolver;
}
QT_END_NAMESPACE

class GroupSolver : public QGroupBox {
    Q_OBJECT

public:
    explicit GroupSolver(QWidget* parent = nullptr);
    virtual ~GroupSolver();

    void initialize();

private:
    void BfsClicked();
    void DfsClicked();
    void DijkstraClicked();
    void AstarClicked();

signals:
    void solverChanged(const eSolver iSolver);

private:
    Ui::GroupSolver* ui;
};

#endif  // GROUPSOLVER_H
