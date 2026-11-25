#include "graphoptionsarea.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/labelcolor.h>
#include <form/controls/labelspin.h>
#include <form/controls/labelspincombo.h>
#include <form/graph/graphwindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/formatutil.h>
#include <util/iconcache.h>
#include <util/ini/inisetter.h>
#include <util/stringutil.h>

using namespace Fort;

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
    m_cbGraphHideYAxisLabels->setChecked(IniGraph::hideYAxisLabelsDefault());
    m_cbGraphShowSpeed->setChecked(IniGraph::showSpeedDefault());
    m_cbGraphAnimation->setChecked(IniGraph::animationDefault());
    m_cbGraphAxisTicks->setChecked(IniGraph::axisTicksDefault());
    m_cbGraphShowTotal->setChecked(IniGraph::showTotalDefault());
    m_cbGraphShowIn->setChecked(IniGraph::showInDefault());
    m_cbGraphShowOut->setChecked(IniGraph::showOutDefault());
    m_graphOpacity->spinBox()->setValue(IniGraph::opacityDefault());
    m_graphHoverOpacity->spinBox()->setValue(IniGraph::hoverOpacityDefault());
    m_graphTickLabelSize->spinBox()->setValue(IniGraph::tickLabelSizeDefault());
    m_graphBarWidth->spinBox()->setValue(IniGraph::barWidthDefault());
    m_graphMaxSeconds->spinBox()->setValue(IniGraph::maxSecondsDefault());
    m_graphFixedSpeed->spinBox()->setValue(IniGraph::fixedSpeedDefault());
    changeGraphType(IniGraph::graphTypeDefault());
    changeTrafUnit(IniGraph::trafUnitDefault());

    for (int i = 0; i < GraphColorCount; ++i) {
        const auto type = GraphColorType(i);

        m_graphColors[i]->setColor(IniGraph::colorDefault(type, /*dark=*/false));
        m_graphColors[i]->setDarkColor(IniGraph::colorDefault(type, /*dark=*/true));
    }
}

void GraphOptionsArea::retranslateUi()
{
    m_gbGraph->setTitle(tr("Window"));
    m_gbColors->setTitle(tr("Colors (Light | Dark)"));
    m_gbGeometry->setTitle(tr("Window Geometry"));

    m_cbGraphHideOnClose->setText(tr("Hide on close"));

    m_cbGraphAlwaysOnTop->setText(tr("Always on top"));
    m_cbGraphFrameless->setText(tr("Frameless"));
    m_cbGraphClickThrough->setText(tr("Click through"));
    m_cbGraphHideOnHover->setText(tr("Hide on hover"));
    m_cbGraphHideYAxisLabels->setText(tr("Hide Y axis scale labels"));
    m_cbGraphShowSpeed->setText(tr("Show speed"));
    m_cbGraphAnimation->setText(tr("Animation"));
    m_cbGraphAxisTicks->setText(tr("Axis ticks"));
    m_labelGraphShow->setText(tr("Show:"));
    m_cbGraphShowTotal->setText(tr("Total"));
    m_cbGraphShowIn->setText(tr("Download"));
    m_cbGraphShowOut->setText(tr("Upload"));
    m_graphOpacity->label()->setText(tr("Opacity:"));
    m_graphHoverOpacity->label()->setText(tr("Hover opacity:"));
    m_graphTickLabelSize->label()->setText(tr("Tick label size:"));
    m_graphBarWidth->label()->setText(tr("Bar width:"));
    m_graphMaxSeconds->label()->setText(tr("History length:"));
    m_graphMaxSeconds->setToolTip(tr("How many seconds of the traffic the graph shows"));
    m_graphFixedSpeed->label()->setText(tr("Fixed speed:"));
    m_graphFixedSpeed->setToolTip(
            tr("The constant top of the speed scale instead of the automatic one"));
    retranslateFixedSpeedCombo();
    m_labelGraphType->setText(tr("Graph type:"));
    retranslateGraphTypeCombo();
    m_traphUnits->setText(tr("Units:"));

    retranslateGraphColors();

    m_cbGraphMaximized->setText(tr("Maximized"));
    m_graphX->label()->setText(tr("X:"));
    m_graphY->label()->setText(tr("Y:"));
    m_graphWidth->label()->setText(tr("Width:"));
    m_graphHeight->label()->setText(tr("Height:"));
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
        tr("Total:"), tr("Axis:"), tr("Tick label:"), tr("Label:"), tr("Grid:") };

    for (int i = 0; i < GraphColorCount; ++i) {
        m_graphColors[i]->label()->setText(names[i]);
    }
}

void GraphOptionsArea::retranslateGraphTypeCombo()
{
    ControlUtil::setComboBoxTexts(
            m_comboGraphType, GraphPlot::graphTypeNames(), iniGraph().graphType());
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

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();

    return layout;
}

QLayout *GraphOptionsArea::setupColumn1()
{
    // Graph Group Box
    setupGraphBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbGraph);
    layout->addStretch();

    return layout;
}

QLayout *GraphOptionsArea::setupColumn2()
{
    // Graph Colors Group Box
    setupColorsBox();

    // Window Geometry Group Box
    setupGeometryBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbColors);
    layout->addWidget(m_gbGeometry);
    layout->addStretch();

    return layout;
}

void GraphOptionsArea::setupGraphBox()
{
    setupGraphCheckboxes();
    setupGraphViewCheckboxes();

    // Graphs to Show
    auto graphShowLayout = setupGraphShowLayout();

    setupGraphOptions();

    // Graph Type
    auto graphTypeLayout = setupGraphTypeLayout();

    // Traffic Units
    auto trafUnitsLayout = setupTrafUnitsLayout();

    auto layout = ControlUtil::createVLayoutByWidgets({
            m_cbGraphHideOnClose,
            ControlUtil::createSeparator(),
            m_cbGraphAlwaysOnTop,
            m_cbGraphFrameless,
            m_cbGraphClickThrough,
            m_cbGraphHideOnHover,
            m_cbGraphHideYAxisLabels,
            m_cbGraphShowSpeed,
            m_cbGraphAnimation,
            m_cbGraphAxisTicks,
            ControlUtil::createSeparator(),
    });
    layout->addLayout(graphShowLayout);

    ControlUtil::fillLayoutByWidgets(layout,
            { ControlUtil::createSeparator(), m_graphOpacity, m_graphHoverOpacity,
                    m_graphTickLabelSize, m_graphBarWidth, m_graphMaxSeconds, m_graphFixedSpeed,
                    ControlUtil::createSeparator() });
    layout->addLayout(graphTypeLayout);
    layout->addLayout(trafUnitsLayout);

    m_gbGraph = new QGroupBox();
    m_gbGraph->setLayout(layout);
}

void GraphOptionsArea::setupGraphCheckboxes()
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

void GraphOptionsArea::setupGraphViewCheckboxes()
{
    m_cbGraphHideYAxisLabels =
            createIniGraphCheckBox(&IniGraph::hideYAxisLabels, &IniGraph::setHideYAxisLabels);
    m_cbGraphHideYAxisLabels->setObjectName("cbGraphHideYAxisLabels");
    m_cbGraphShowSpeed = createIniGraphCheckBox(&IniGraph::showSpeed, &IniGraph::setShowSpeed);
    m_cbGraphShowSpeed->setObjectName("cbGraphShowSpeed");
    m_cbGraphAnimation = createIniGraphCheckBox(&IniGraph::animation, &IniGraph::setAnimation);
    m_cbGraphAnimation->setObjectName("cbGraphAnimation");
    m_cbGraphAxisTicks = createIniGraphCheckBox(&IniGraph::axisTicks, &IniGraph::setAxisTicks);
    m_cbGraphAxisTicks->setObjectName("cbGraphAxisTicks");
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
    m_graphOpacity = ControlUtil::createSpin({ iniGraph().opacity(), 0, 100 }, " %",
            iniGraphSetter(&IniGraph::opacity, &IniGraph::setOpacity));
    m_graphOpacity->setObjectName("graphOpacity");

    m_graphHoverOpacity = ControlUtil::createSpin({ iniGraph().hoverOpacity(), 0, 100 }, " %",
            iniGraphSetter(&IniGraph::hoverOpacity, &IniGraph::setHoverOpacity));
    m_graphHoverOpacity->setObjectName("graphHoverOpacity");

    m_graphTickLabelSize = ControlUtil::createSpin({ iniGraph().tickLabelSize(), 1, 99 }, {},
            iniGraphSetter(&IniGraph::tickLabelSize, &IniGraph::setTickLabelSize));
    m_graphTickLabelSize->setObjectName("graphTickLabelSize");

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

QLayout *GraphOptionsArea::setupGraphTypeLayout()
{
    m_labelGraphType = ControlUtil::createLabel();

    m_comboGraphType = ControlUtil::createComboBox({}, [&](int index) { changeGraphType(index); });
    m_comboGraphType->setObjectName("comboGraphType");
    m_comboGraphType->setFixedWidth(110);

    return ControlUtil::createRowLayout(m_labelGraphType, m_comboGraphType);
}

void GraphOptionsArea::changeGraphType(int index)
{
    if (iniGraph().graphType() == index)
        return;

    iniGraph().setGraphType(index);
    emit edited();

    m_comboGraphType->setCurrentIndex(index);
}

QLayout *GraphOptionsArea::setupTrafUnitsLayout()
{
    const QStringList list = { "b/s", "B/s", "ib/s", "iB/s" };

    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(list, [&](int index) { changeTrafUnit(index); });
    m_comboTrafUnit->setObjectName("comboTrafUnit");
    m_comboTrafUnit->setFixedWidth(110);

    m_comboTrafUnit->setCurrentIndex(iniGraph().trafUnit());

    return ControlUtil::createRowLayout(m_traphUnits, m_comboTrafUnit);
}

void GraphOptionsArea::changeTrafUnit(int index)
{
    if (iniGraph().trafUnit() == index)
        return;

    iniGraph().setTrafUnit(index);
    emit edited();

    m_comboTrafUnit->setCurrentIndex(index);
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
    const auto w = windowManager()->graphWindow();
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
