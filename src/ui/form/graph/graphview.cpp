#include "graphview.h"

#include <QApplication>
#include <QDateTime>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QStyleHints>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <stat/statmanager.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/guiutil.h>

using namespace Fort;

GraphView::GraphView(int graphId, QWidget *parent) :
    QWidget(parent), m_iniGraph(Fort::iniUser(), graphId)
{
    setupUi();
    setupOptions();
    setupTimer();

    connect(statManager(), &StatManager::trafficAdded, this, &GraphView::addTraffic);
}

void GraphView::setupUi()
{
    m_plot = new GraphPlot();

    connect(m_plot, &GraphPlot::resized, this, &GraphView::updateGraph);

    connect(m_plot, &GraphPlot::mouseRightClick, this, [&](QMouseEvent *event) {
        windowManager()->trayIcon()->showTrayMenu(GuiUtil::globalPos(event));
    });

    auto layout = ControlUtil::createVLayoutByWidgets({ m_plot }, /*margin=*/0);
    setLayout(layout);
}

void GraphView::setupOptions()
{
    forceUpdateOptions();

    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphView::updateOptions);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &GraphView::forceUpdateOptions);
#endif
}

void GraphView::forceUpdateOptions()
{
    updateOptions();
}

void GraphView::updateOptions(bool onlyFlags)
{
    if (onlyFlags)
        return;

    const IniGraph &ini = iniGraph();

    updateColors(ini);
    updateFonts(ini);
    updateFormat(ini);

    updateGraph();

    emit optionsUpdated();
}

void GraphView::updateColors(const IniGraph &ini)
{
    m_plot->setColors(getColors(ini));
}

void GraphView::updateFonts(const IniGraph &ini)
{
    m_plot->setTickLabelSize(ini.tickLabelSize());
    m_plot->setSpeedTextSize(ini.speedTextSize());
    m_plot->setSpeedVisible(ini.showSpeed());
}

void GraphView::updateFormat(const IniGraph &ini)
{
    m_plot->setGraphVisible(ini.showGraph());
    m_plot->setMaxSeconds(ini.maxSeconds());
    m_plot->setGraphType(GraphPlot::graphTypeByIndex(ini.graphType()));
    m_plot->setTotalVisible(ini.showTotal());
    m_plot->setInVisible(ini.showIn());
    m_plot->setOutVisible(ini.showOut());
    m_plot->setBarWidth(ini.barWidth());
    m_plot->setAxisTicksVisible(ini.axisTicks());
    m_plot->setYAxisLabelsVisible(!ini.hideYAxisLabels());
    m_plot->setUnitFormat(FormatUtil::graphUnitFormat(ini.trafUnit()));
    m_plot->setFixedValueMax(ini.fixedSpeed() * 1024LL);
    m_plot->setAnimated(ini.animation());
}

void GraphView::setupTimer()
{
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphView::updateGraph);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphView::startUpdateTimer);

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setTimerType(Qt::PreciseTimer);

    startUpdateTimer();
}

void GraphView::startUpdateTimer()
{
    // Update right after the next second's start
    const int msecs = int(QDateTime::currentMSecsSinceEpoch() % 1000);

    m_updateTimer.start(1000 - msecs + 10);
}

void GraphView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    updateGraph(); // the hidden view isn't updated
}

void GraphView::addTraffic(qint64 unixTime, quint64 inBytes, quint64 outBytes)
{
    m_plot->addPoint({ unixTime, inBytes * 8, outBytes * 8 });
}

void GraphView::updateGraph()
{
    const qint64 unixTime = DateUtil::getUnixTime();

    // Move the graph to the current time
    addTraffic(unixTime, 0, 0);

    if (!isVisible())
        return;

    updateSpeed(unixTime - 1); // the last complete second

    m_plot->replot();
}

void GraphView::updateSpeed(qint64 unixTime)
{
    const GraphPoint point = m_plot->pointAt(unixTime);
    const auto unitFormat = m_plot->unitFormat();

    const QString inText = FormatUtil::formatSpeed(qint64(point.inBits), unitFormat);
    const QString outText = FormatUtil::formatSpeed(qint64(point.outBits), unitFormat);

    if (m_plot->speedVisible()) {
        m_plot->setSpeedText(inText, outText);
    }

    emit speedUpdated(inText, outText);
}

GraphPlot::ColorArray GraphView::getColors(const IniGraph &ini)
{
    GraphPlot::ColorArray colors;

    const bool isLightTheme =
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark;
#else
            true;
#endif

    for (int i = 0; i < GraphColorCount; ++i) {
        colors << ini.color(GraphColorType(i), /*dark=*/!isLightTheme);
    }

    return colors;
}
