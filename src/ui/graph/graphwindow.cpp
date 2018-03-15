#include "graphwindow.h"

#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>

QT_CHARTS_USE_NAMESPACE

GraphWindow::GraphWindow(QWidget *parent) :
    WidgetWindow(parent)
{
    setupUi();

    setWindowFlags(Qt::SplashScreen);
    setMinimumSize(QSize(400, 300));
}

void GraphWindow::setupUi()
{
    QChartView *chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->addWidget(chartView);
    setLayout(mainLayout);
}
