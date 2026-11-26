#include "graphoptionsarea.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <form/controls/labelcolor.h>
#include <form/controls/labelspin.h>
#include <form/controls/labelspincombo.h>
#include <form/graph/graph_types.h>
#include <form/graph/graphwindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/formatutil.h>
#include <util/iconcache.h>
#include <util/ini/inisetter.h>
#include <util/stringutil.h>

using namespace Fort;

namespace {

QLayout *createColumnLayout(const QList<QWidget *> &boxes)
{
    auto layout = ControlUtil::createVLayoutByWidgets(boxes);
    layout->setSpacing(10);
    layout->addStretch();

    return layout;
}

}

GraphOptionsArea::GraphOptionsArea(const IniGraph &iniGraph, QWidget *parent) :
    QWidget(parent), m_iniGraph(iniGraph)
{
    setupUi();
}

void GraphOptionsArea::resetToDefault()
{
    m_cbGraphHideOnClose->setChecked(IniGraph::hideOnCloseDefault());

    m_cbGraphAlwaysOnTop->setChecked(IniGraph::alwaysOnTopDefault());
    m_cbGraphFrameless->setChecked(IniGraph::framelessDefault());
    m_cbGraphClickThrough->setChecked(IniGraph::clickThroughDefault());
    m_cbGraphHideOnHover->setChecked(IniGraph::hideOnHoverDefault());
    m_graphOpacity->spinBox()->setValue(IniGraph::opacityDefault());
    m_graphHoverOpacity->spinBox()->setValue(IniGraph::hoverOpacityDefault());
    m_graphBorderWidth->spinBox()->setValue(IniGraph::borderWidthDefault());

    m_cbGraphShowInTaskbar->setChecked(IniGraph::showInTaskbarDefault());
    m_graphTaskbarWidth->spinBox()->setValue(IniGraph::taskbarWidthDefault());

    m_cbGraphShowGraph->setChecked(IniGraph::showGraphDefault());
    ControlUtil::activateComboBox(m_comboGraphType, IniGraph::graphTypeDefault());
    m_cbGraphShowTotal->setChecked(IniGraph::showTotalDefault());
    m_cbGraphShowIn->setChecked(IniGraph::showInDefault());
    m_cbGraphShowOut->setChecked(IniGraph::showOutDefault());
    m_graphBarWidth->spinBox()->setValue(IniGraph::barWidthDefault());
    m_graphMaxSeconds->spinBox()->setValue(IniGraph::maxSecondsDefault());
    m_graphFixedSpeed->spinBox()->setValue(IniGraph::fixedSpeedDefault());
    m_cbGraphAnimation->setChecked(IniGraph::animationDefault());
    m_cbGraphAxisTicks->setChecked(IniGraph::axisTicksDefault());
    m_cbGraphGridLines->setChecked(IniGraph::gridLinesDefault());
    m_cbGraphYAxisLabels->setChecked(IniGraph::yAxisLabelsDefault());
    m_graphTickLabelSize->spinBox()->setValue(IniGraph::tickLabelSizeDefault());

    m_cbGraphShowSpeed->setChecked(IniGraph::showSpeedDefault());
    m_graphSpeedTextSize->spinBox()->setValue(IniGraph::speedTextSizeDefault());
    ControlUtil::activateComboBox(m_comboSpeedPosition, IniGraph::speedPositionDefault());
    m_cbGraphSpeedInside->setChecked(IniGraph::speedInsideDefault());
    ControlUtil::activateComboBox(m_comboTrafUnit, IniGraph::trafUnitDefault());

    for (int i = 0; i < GraphColorCount; ++i) {
        const auto type = GraphColorType(i);

        m_graphColors[i]->setColor(IniGraph::colorDefault(type, /*dark=*/false));
        m_graphColors[i]->setDarkColor(IniGraph::colorDefault(type, /*dark=*/true));
    }
}

void GraphOptionsArea::retranslateUi()
{
    m_gbWindow->setTitle(tr("Window"));
    m_gbGeometry->setTitle(tr("Window Geometry"));
    m_gbTaskbar->setTitle(tr("Taskbar"));
    m_gbGraph->setTitle(tr("Graph"));
    m_gbSpeed->setTitle(tr("Speed"));
    m_gbColors->setTitle(tr("Colors (Light | Dark)"));

    m_cbGraphHideOnClose->setText(tr("Hide on close"));

    m_cbGraphAlwaysOnTop->setText(tr("Always on top"));
    m_cbGraphFrameless->setText(tr("Frameless"));
    m_cbGraphClickThrough->setText(tr("Click through"));
    m_cbGraphHideOnHover->setText(tr("Hide on hover"));
    m_graphOpacity->label()->setText(tr("Opacity:"));
    m_graphHoverOpacity->label()->setText(tr("Hover opacity:"));
    m_graphBorderWidth->label()->setText(tr("Border width:"));

    m_cbGraphMaximized->setText(tr("Maximized"));
    m_graphX->label()->setText(tr("X:"));
    m_graphY->label()->setText(tr("Y:"));
    m_graphWidth->label()->setText(tr("Width:"));
    m_graphHeight->label()->setText(tr("Height:"));

    m_cbGraphShowInTaskbar->setText(tr("Show in taskbar"));
    m_graphTaskbarWidth->label()->setText(tr("Width:"));

    m_cbGraphShowGraph->setText(tr("Show graph"));
    m_labelGraphType->setText(tr("Graph type:"));
    retranslateGraphTypeCombo();
    m_labelGraphShow->setText(tr("Show:"));
    m_cbGraphShowTotal->setText(tr("Total"));
    m_cbGraphShowIn->setText(tr("Download"));
    m_cbGraphShowOut->setText(tr("Upload"));
    m_graphBarWidth->label()->setText(tr("Bar width:"));
    m_graphMaxSeconds->label()->setText(tr("History length:"));
    m_graphMaxSeconds->setToolTip(tr("How many seconds of the traffic the graph shows"));
    m_graphFixedSpeed->label()->setText(tr("Fixed speed:"));
    m_graphFixedSpeed->setToolTip(
            tr("The constant top of the speed scale instead of the automatic one"));
    retranslateFixedSpeedCombo();
    m_cbGraphAnimation->setText(tr("Animation"));
    m_cbGraphAxisTicks->setText(tr("Axis ticks"));
    m_cbGraphGridLines->setText(tr("Grid lines"));
    m_cbGraphYAxisLabels->setText(tr("Y axis scale labels"));
    m_graphTickLabelSize->label()->setText(tr("Tick label size:"));

    m_cbGraphShowSpeed->setText(tr("Show speed"));
    m_graphSpeedTextSize->label()->setText(tr("Speed text size:"));
    m_labelSpeedPosition->setText(tr("Position:"));
    retranslateSpeedPositionCombo();
    m_cbGraphSpeedInside->setText(tr("Inside the graph"));
    m_labelTrafUnits->setText(tr("Units:"));

    retranslateGraphColors();
}

void GraphOptionsArea::retranslateFixedSpeedCombo()
{
    auto names = m_graphFixedSpeed->names();

    names.replace(0, tr("Custom"));
    names.replace(1, tr("Auto-scale"));

    m_graphFixedSpeed->setNames(names);
}

void GraphOptionsArea::retranslateGraphColors()
{
    // Sync with GraphColorType
    const QStringList names = { tr("Background:"), tr("Border:"), tr("Download:"), tr("Upload:"),
        tr("Total:"), tr("Axis:"), tr("Tick label:"), tr("Speed label:"), tr("Grid:") };

    for (int i = 0; i < GraphColorCount; ++i) {
        m_graphColors[i]->label()->setText(names[i]);
    }
}

void GraphOptionsArea::retranslateGraphTypeCombo()
{
    ControlUtil::setComboBoxTexts(
            m_comboGraphType, GraphTypes::graphTypeNames(), iniGraph().graphType());
}

void GraphOptionsArea::retranslateSpeedPositionCombo()
{
    ControlUtil::setComboBoxTexts(
            m_comboSpeedPosition, GraphTypes::speedPositionNames(), iniGraph().speedPosition());
}

template<typename T>
std::function<void(T)> GraphOptionsArea::iniGraphSetter(
        T (IniGraph::*iniValue)() const, void (IniGraph::*setIniValue)(T))
{
    return iniSetter([this]() -> IniGraph & { return iniGraph(); }, iniValue, setIniValue,
            [this] { emit edited(); });
}

QCheckBox *GraphOptionsArea::createIniGraphCheckBox(
        bool (IniGraph::*iniFlag)() const, void (IniGraph::*setIniFlag)(bool))
{
    return ControlUtil::createCheckBox(
            (iniGraph().*iniFlag)(), iniGraphSetter(iniFlag, setIniFlag));
}

QComboBox *GraphOptionsArea::createIniGraphComboBox(const QStringList &texts,
        int (IniGraph::*iniValue)() const, void (IniGraph::*setIniValue)(int))
{
    auto c = ControlUtil::createComboBox(texts, iniGraphSetter(iniValue, setIniValue));
    c->setFixedWidth(110);
    c->setCurrentIndex((iniGraph().*iniValue)());

    return c;
}

void GraphOptionsArea::setupUi()
{
    auto layout = setupColumns();
    layout->setContentsMargins(0, 0, 0, 0);

    this->setLayout(layout);
}

QLayout *GraphOptionsArea::setupColumns()
{
    // Column #1
    auto colLayout1 = setupColumn1();

    // Column #2
    auto colLayout2 = setupColumn2();

    // Column #3
    auto colLayout3 = setupColumn3();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();
    layout->addLayout(colLayout3);
    layout->addStretch();

    return layout;
}

QLayout *GraphOptionsArea::setupColumn1()
{
    // Window Group Box
    setupWindowBox();

    // Window Geometry Group Box
    setupGeometryBox();

    // Taskbar Group Box
    setupTaskbarBox();

    return createColumnLayout({ m_gbWindow, m_gbGeometry, m_gbTaskbar });
}

QLayout *GraphOptionsArea::setupColumn2()
{
    // Graph Group Box
    setupGraphBox();

    // Speed Group Box
    setupSpeedBox();

    return createColumnLayout({ m_gbGraph, m_gbSpeed });
}

QLayout *GraphOptionsArea::setupColumn3()
{
    // Graph Colors Group Box
    setupColorsBox();

    return createColumnLayout({ m_gbColors });
}

void GraphOptionsArea::setupWindowBox()
{
    setupWindowCheckboxes();
    setupWindowOpacity();
    setupWindowBorder();

    auto layout = ControlUtil::createVLayoutByWidgets({
            m_cbGraphHideOnClose,
            ControlUtil::createSeparator(),
            m_cbGraphAlwaysOnTop,
            m_cbGraphFrameless,
            m_cbGraphClickThrough,
            m_cbGraphHideOnHover,
            ControlUtil::createSeparator(),
            m_graphOpacity,
            m_graphHoverOpacity,
            m_graphBorderWidth,
    });

    m_gbWindow = new QGroupBox();
    m_gbWindow->setLayout(layout);
}

void GraphOptionsArea::setupWindowCheckboxes()
{
    m_cbGraphHideOnClose =
            createIniGraphCheckBox(&IniGraph::hideOnClose, &IniGraph::setHideOnClose);
    m_cbGraphHideOnClose->setObjectName("cbGraphHideOnClose");
    m_cbGraphAlwaysOnTop =
            createIniGraphCheckBox(&IniGraph::alwaysOnTop, &IniGraph::setAlwaysOnTop);
    m_cbGraphAlwaysOnTop->setObjectName("cbGraphAlwaysOnTop");
    m_cbGraphFrameless = createIniGraphCheckBox(&IniGraph::frameless, &IniGraph::setFrameless);
    m_cbGraphFrameless->setObjectName("cbGraphFrameless");
    m_cbGraphClickThrough =
            createIniGraphCheckBox(&IniGraph::clickThrough, &IniGraph::setClickThrough);
    m_cbGraphClickThrough->setObjectName("cbGraphClickThrough");
    m_cbGraphHideOnHover =
            createIniGraphCheckBox(&IniGraph::hideOnHover, &IniGraph::setHideOnHover);
    m_cbGraphHideOnHover->setObjectName("cbGraphHideOnHover");
}

void GraphOptionsArea::setupWindowOpacity()
{
    m_graphOpacity = ControlUtil::createSpin({ iniGraph().opacity(), 0, 100 }, " %",
            iniGraphSetter(&IniGraph::opacity, &IniGraph::setOpacity));
    m_graphOpacity->setObjectName("graphOpacity");

    m_graphHoverOpacity = ControlUtil::createSpin({ iniGraph().hoverOpacity(), 0, 100 }, " %",
            iniGraphSetter(&IniGraph::hoverOpacity, &IniGraph::setHoverOpacity));
    m_graphHoverOpacity->setObjectName("graphHoverOpacity");
}

void GraphOptionsArea::setupWindowBorder()
{
    m_graphBorderWidth = ControlUtil::createSpin({ iniGraph().borderWidth(), 0, 99 }, " px",
            iniGraphSetter(&IniGraph::borderWidth, &IniGraph::setBorderWidth));
    m_graphBorderWidth->setObjectName("graphBorderWidth");
}

void GraphOptionsArea::setupGeometryBox()
{
    saveGraphWindowGeometry();

    setupGeometryControls();

    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbGraphMaximized, m_graphX, m_graphY, m_graphWidth, m_graphHeight });

    m_gbGeometry = new QGroupBox();
    m_gbGeometry->setLayout(layout);
}

void GraphOptionsArea::saveGraphWindowGeometry()
{
    // The window saves its geometry on close only
    const auto w = windowManager()->graphWindow(iniGraph().graphId());
    if (w) {
        w->saveWindowGeometry();
    }
}

void GraphOptionsArea::setupGeometryControls()
{
    m_cbGraphMaximized = createIniGraphCheckBox(&IniGraph::maximized, &IniGraph::setMaximized);
    m_cbGraphMaximized->setObjectName("cbGraphMaximized");

    const QRect rect = iniGraph().geometry();

    m_graphX = createGeometrySpin(rect.x(), -99999);
    m_graphX->setObjectName("graphX");
    m_graphY = createGeometrySpin(rect.y(), -99999);
    m_graphY->setObjectName("graphY");
    m_graphWidth = createGeometrySpin(rect.width(), 0);
    m_graphWidth->setObjectName("graphWidth");
    m_graphHeight = createGeometrySpin(rect.height(), 0);
    m_graphHeight->setObjectName("graphHeight");
}

LabelSpin *GraphOptionsArea::createGeometrySpin(int value, int min)
{
    return ControlUtil::createSpin(
            { value, min, 99999 }, " px", [&](int /*v*/) { onGeometryEdited(); });
}

void GraphOptionsArea::onGeometryEdited()
{
    const QRect rect(m_graphX->spinBox()->value(), m_graphY->spinBox()->value(),
            m_graphWidth->spinBox()->value(), m_graphHeight->spinBox()->value());

    if (iniGraph().geometry() != rect) {
        iniGraph().setGeometry(rect);
        emit edited();
    }
}

void GraphOptionsArea::setupTaskbarBox()
{
    m_cbGraphShowInTaskbar =
            createIniGraphCheckBox(&IniGraph::showInTaskbar, &IniGraph::setShowInTaskbar);
    m_cbGraphShowInTaskbar->setObjectName("cbGraphShowInTaskbar");

    m_graphTaskbarWidth = ControlUtil::createSpin({ iniGraph().taskbarWidth(), 1, 9999 }, " px",
            iniGraphSetter(&IniGraph::taskbarWidth, &IniGraph::setTaskbarWidth));
    m_graphTaskbarWidth->setObjectName("graphTaskbarWidth");

    auto layout =
            ControlUtil::createVLayoutByWidgets({ m_cbGraphShowInTaskbar, m_graphTaskbarWidth });

    m_gbTaskbar = new QGroupBox();
    m_gbTaskbar->setLayout(layout);
}

void GraphOptionsArea::setupGraphBox()
{
    m_cbGraphShowGraph = createIniGraphCheckBox(&IniGraph::showGraph, &IniGraph::setShowGraph);
    m_cbGraphShowGraph->setObjectName("cbGraphShowGraph");

    // Graph Type
    auto graphTypeLayout = setupGraphTypeLayout();

    // Graphs to Show
    auto graphShowLayout = setupGraphShowLayout();

    setupGraphOptions();
    setupGraphViewOptions();

    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbGraphShowGraph });
    layout->addLayout(graphTypeLayout);
    layout->addLayout(graphShowLayout);

    ControlUtil::fillLayoutByWidgets(layout,
            { ControlUtil::createSeparator(), m_graphBarWidth, m_graphMaxSeconds, m_graphFixedSpeed,
                    ControlUtil::createSeparator(), m_cbGraphAnimation, m_cbGraphAxisTicks,
                    m_cbGraphGridLines, m_cbGraphYAxisLabels, m_graphTickLabelSize });

    m_gbGraph = new QGroupBox();
    m_gbGraph->setLayout(layout);
}

QLayout *GraphOptionsArea::setupGraphTypeLayout()
{
    m_labelGraphType = ControlUtil::createLabel();

    m_comboGraphType = createIniGraphComboBox({}, &IniGraph::graphType, &IniGraph::setGraphType);
    m_comboGraphType->setObjectName("comboGraphType");

    return ControlUtil::createRowLayout(m_labelGraphType, m_comboGraphType);
}

QLayout *GraphOptionsArea::setupGraphShowLayout()
{
    m_labelGraphShow = ControlUtil::createLabel();

    setupGraphShowCheckboxes();

    return ControlUtil::createHLayoutByWidgets(
            { m_labelGraphShow, /*stretch*/ nullptr, m_cbGraphShowTotal, m_cbGraphShowIn,
                    m_cbGraphShowOut },
            /*margin=*/0);
}

void GraphOptionsArea::setupGraphShowCheckboxes()
{
    m_cbGraphShowTotal = createIniGraphCheckBox(&IniGraph::showTotal, &IniGraph::setShowTotal);
    m_cbGraphShowTotal->setObjectName("cbGraphShowTotal");
    m_cbGraphShowIn = createIniGraphCheckBox(&IniGraph::showIn, &IniGraph::setShowIn);
    m_cbGraphShowIn->setObjectName("cbGraphShowIn");
    m_cbGraphShowOut = createIniGraphCheckBox(&IniGraph::showOut, &IniGraph::setShowOut);
    m_cbGraphShowOut->setObjectName("cbGraphShowOut");
}

void GraphOptionsArea::setupGraphOptions()
{
    m_graphBarWidth = ControlUtil::createSpin({ iniGraph().barWidth(), 1, 99 }, " px",
            iniGraphSetter(&IniGraph::barWidth, &IniGraph::setBarWidth));
    m_graphBarWidth->setObjectName("graphBarWidth");

    m_graphMaxSeconds = ControlUtil::createSpin({ iniGraph().maxSeconds(), 0, 9999 }, {},
            iniGraphSetter(&IniGraph::maxSeconds, &IniGraph::setMaxSeconds));
    m_graphMaxSeconds->setObjectName("graphMaxSeconds");

    setupGraphFixedSpeed();
}

void GraphOptionsArea::setupGraphFixedSpeed()
{
    const std::array speedValues = { 100, 0, 500, 1024, 3 * 1024, 5 * 1024, 10 * 1024, 20 * 1024,
        50 * 1024 };

    QStringList speedNames;
    for (const int kbits : speedValues) {
        const auto name = FormatUtil::formatSpeed(kbits * 1024LL);
        speedNames.append(name);
    }

    const auto speedValuesList = SpinCombo::makeValuesList(speedValues);
    m_graphFixedSpeed =
            ControlUtil::createSpinCombo({ iniGraph().fixedSpeed(), 0, 9999999 }, speedValuesList,
                    " Kb/s", iniGraphSetter(&IniGraph::fixedSpeed, &IniGraph::setFixedSpeed));
    m_graphFixedSpeed->setObjectName("graphFixedSpeed");
    m_graphFixedSpeed->setNames(speedNames);
}

void GraphOptionsArea::setupGraphViewOptions()
{
    m_cbGraphAnimation = createIniGraphCheckBox(&IniGraph::animation, &IniGraph::setAnimation);
    m_cbGraphAnimation->setObjectName("cbGraphAnimation");
    m_cbGraphAxisTicks = createIniGraphCheckBox(&IniGraph::axisTicks, &IniGraph::setAxisTicks);
    m_cbGraphAxisTicks->setObjectName("cbGraphAxisTicks");
    m_cbGraphGridLines = createIniGraphCheckBox(&IniGraph::gridLines, &IniGraph::setGridLines);
    m_cbGraphGridLines->setObjectName("cbGraphGridLines");
    m_cbGraphYAxisLabels =
            createIniGraphCheckBox(&IniGraph::yAxisLabels, &IniGraph::setYAxisLabels);
    m_cbGraphYAxisLabels->setObjectName("cbGraphYAxisLabels");

    m_graphTickLabelSize = ControlUtil::createSpin({ iniGraph().tickLabelSize(), 1, 99 }, {},
            iniGraphSetter(&IniGraph::tickLabelSize, &IniGraph::setTickLabelSize));
    m_graphTickLabelSize->setObjectName("graphTickLabelSize");
}

void GraphOptionsArea::setupSpeedBox()
{
    m_cbGraphShowSpeed = createIniGraphCheckBox(&IniGraph::showSpeed, &IniGraph::setShowSpeed);
    m_cbGraphShowSpeed->setObjectName("cbGraphShowSpeed");

    m_graphSpeedTextSize = ControlUtil::createSpin({ iniGraph().speedTextSize(), 1, 99 }, {},
            iniGraphSetter(&IniGraph::speedTextSize, &IniGraph::setSpeedTextSize));
    m_graphSpeedTextSize->setObjectName("graphSpeedTextSize");

    // Position
    auto speedPositionLayout = setupSpeedPositionLayout();

    m_cbGraphSpeedInside =
            createIniGraphCheckBox(&IniGraph::speedInside, &IniGraph::setSpeedInside);
    m_cbGraphSpeedInside->setObjectName("cbGraphSpeedInside");

    // Traffic Units
    auto trafUnitsLayout = setupTrafUnitsLayout();

    auto layout = ControlUtil::createVLayoutByWidgets({ m_cbGraphShowSpeed, m_graphSpeedTextSize });
    layout->addLayout(speedPositionLayout);
    layout->addWidget(m_cbGraphSpeedInside);
    layout->addLayout(trafUnitsLayout);

    m_gbSpeed = new QGroupBox();
    m_gbSpeed->setLayout(layout);
}

QLayout *GraphOptionsArea::setupSpeedPositionLayout()
{
    m_labelSpeedPosition = ControlUtil::createLabel();

    m_comboSpeedPosition =
            createIniGraphComboBox({}, &IniGraph::speedPosition, &IniGraph::setSpeedPosition);
    m_comboSpeedPosition->setObjectName("comboSpeedPosition");

    return ControlUtil::createRowLayout(m_labelSpeedPosition, m_comboSpeedPosition);
}

QLayout *GraphOptionsArea::setupTrafUnitsLayout()
{
    const QStringList list = { "b/s", "B/s", "ib/s", "iB/s" };

    m_labelTrafUnits = ControlUtil::createLabel();

    m_comboTrafUnit = createIniGraphComboBox(list, &IniGraph::trafUnit, &IniGraph::setTrafUnit);
    m_comboTrafUnit->setObjectName("comboTrafUnit");

    return ControlUtil::createRowLayout(m_labelTrafUnits, m_comboTrafUnit);
}

void GraphOptionsArea::setupColorsBox()
{
    setupGraphColors();

    auto layout = ControlUtil::createVLayoutByWidgets(
            QList<QWidget *>(m_graphColors.begin(), m_graphColors.end()));

    m_gbColors = new QGroupBox();
    m_gbColors->setLayout(layout);
}

void GraphOptionsArea::setupGraphColors()
{
    for (int i = 0; i < GraphColorCount; ++i) {
        m_graphColors[i] = createGraphColor(GraphColorType(i));
    }
}

LabelColor *GraphOptionsArea::createGraphColor(GraphColorType type)
{
    auto c = ControlUtil::createLabelColor(
            iniGraph().color(type, /*dark=*/false), iniGraph().color(type, /*dark=*/true),
            [=, this](const QColor &v) { setGraphColor(type, /*dark=*/false, v); },
            [=, this](const QColor &v) { setGraphColor(type, /*dark=*/true, v); });

    const QString key = IniGraph::colorKey(type, /*dark=*/false);
    c->setObjectName("graph" + StringUtil::capitalize(key));

    return c;
}

void GraphOptionsArea::setGraphColor(GraphColorType type, bool dark, const QColor &v)
{
    if (iniGraph().color(type, dark) == v)
        return;

    iniGraph().setColor(type, dark, v);
    emit edited();
}
