#include "graphwindow.h"

#include <QVBoxLayout>

#include "qcustomplot.h"

GraphWindow::GraphWindow(QWidget *parent) :
    WidgetWindow(parent)
{
    setupUi();

    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint
                   | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
                   //| Qt::FramelessWindowHint | Qt::WindowTransparentForInput
                   );

    setWindowTitle(tr("Graph"));
    setMinimumSize(QSize(200, 50));
}

void GraphWindow::setupUi()
{
    QCustomPlot *plot = new QCustomPlot(this);
    plot->setContentsMargins(0, 0, 0, 0);

    QCPGraph *graph = plot->addGraph();
    graph->setPen(QColor(250, 120, 0));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->addWidget(plot);
    setLayout(mainLayout);
}
