#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include "../util/window/widgetwindow.h"

class GraphWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);

signals:

public slots:

private:
    void setupUi();
};

#endif // GRAPHWINDOW_H
