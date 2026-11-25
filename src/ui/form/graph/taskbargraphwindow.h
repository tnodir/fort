#ifndef TASKBARGRAPHWINDOW_H
#define TASKBARGRAPHWINDOW_H

#include <form/graph/graphbasewindow.h>

// The graph's host over the taskbar (the option "Show in taskbar")
class TaskbarGraphWindow : public GraphBaseWindow
{
    Q_OBJECT

public:
    explicit TaskbarGraphWindow(int graphId, QWidget *parent = nullptr);

    bool inTaskbar() const override { return true; }

private slots:
    void updatePlacement();

private:
    void setupUi();

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // TASKBARGRAPHWINDOW_H
