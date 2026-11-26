#ifndef TASKBARGRAPHWINDOW_H
#define TASKBARGRAPHWINDOW_H

#include <form/graph/graphbasewindow.h>

// The graph's host over the taskbar (the option "Show in taskbar")
class TaskbarGraphWindow : public GraphBaseWindow
{
    Q_OBJECT

public:
    explicit TaskbarGraphWindow(int graphId, QWidget *parent = nullptr);
    ~TaskbarGraphWindow() override;

    bool inTaskbar() const override { return true; }

    bool isHostChanged() const override;

private slots:
    void updatePlacement();

private:
    void setupUi();
    void setupTaskbarHost();

    void recreate();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
};

#endif // TASKBARGRAPHWINDOW_H
