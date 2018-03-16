#include "graphwindow.h"

#include <QGraphicsLayout>
#include <QVBoxLayout>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>

QT_CHARTS_USE_NAMESPACE

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
    QChart *chart = new QChart();
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(chartView);
    setLayout(mainLayout);
}
