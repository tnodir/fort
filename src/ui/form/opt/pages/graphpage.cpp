#include "graphpage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/labelcolor.h>
#include <form/controls/labelspin.h>
#include <form/controls/labelspincombo.h>
#include <form/graph/graphwindow.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/inigraph.h>
#include <user/iniuser.h>
#include <util/formatutil.h>
#include <util/iconcache.h>

using namespace Fort;

GraphPage::GraphPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void GraphPage::onResetToDefault()
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

    m_graphColor->setColor(IniGraph::colorDefault());
    m_graphBorderColor->setColor(IniGraph::borderColorDefault());
    m_graphColorIn->setColor(IniGraph::colorInDefault());
    m_graphColorOut->setColor(IniGraph::colorOutDefault());
    m_graphTotalColor->setColor(IniGraph::totalColorDefault());
    m_graphAxisColor->setColor(IniGraph::axisColorDefault());
    m_graphTickLabelColor->setColor(IniGraph::tickLabelColorDefault());
    m_graphLabelColor->setColor(IniGraph::labelColorDefault());
    m_graphGridColor->setColor(IniGraph::gridColorDefault());

    m_graphColor->setDarkColor(IniGraph::darkColorDefault());
    m_graphBorderColor->setDarkColor(IniGraph::darkBorderColorDefault());
    m_graphColorIn->setDarkColor(IniGraph::darkColorInDefault());
    m_graphColorOut->setDarkColor(IniGraph::darkColorOutDefault());
    m_graphTotalColor->setDarkColor(IniGraph::darkTotalColorDefault());
    m_graphAxisColor->setDarkColor(IniGraph::darkAxisColorDefault());
    m_graphTickLabelColor->setDarkColor(IniGraph::darkTickLabelColorDefault());
    m_graphLabelColor->setDarkColor(IniGraph::darkLabelColorDefault());
    m_graphGridColor->setDarkColor(IniGraph::darkGridColorDefault());
}

void GraphPage::onRetranslateUi()
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

    m_graphColor->label()->setText(tr("Background:"));
    m_graphBorderColor->label()->setText(tr("Border:"));
    m_graphColorIn->label()->setText(tr("Download:"));
    m_graphColorOut->label()->setText(tr("Upload:"));
    m_graphTotalColor->label()->setText(tr("Total:"));
    m_graphAxisColor->label()->setText(tr("Axis:"));
    m_graphTickLabelColor->label()->setText(tr("Tick label:"));
    m_graphLabelColor->label()->setText(tr("Label:"));
    m_graphGridColor->label()->setText(tr("Grid:"));

    m_cbGraphMaximized->setText(tr("Maximized"));
    m_graphX->label()->setText(tr("X:"));
    m_graphY->label()->setText(tr("Y:"));
    m_graphWidth->label()->setText(tr("Width:"));
    m_graphHeight->label()->setText(tr("Height:"));
}

void GraphPage::retranslateFixedSpeedCombo()
{
    auto names = m_graphFixedSpeed->names();

    names.replace(0, tr("Custom"));
    names.replace(1, tr("Auto-scale"));

    m_graphFixedSpeed->setNames(names);
}

void GraphPage::retranslateGraphTypeCombo()
{
    // Sync with GraphPlot::GraphType
    const QStringList list = { tr("Column"), tr("Line") };

    ControlUtil::setComboBoxTexts(m_comboGraphType, list, iniGraph().graphType());
}

IniGraph GraphPage::iniGraph() const
{
    return IniGraph(iniUser());
}

QCheckBox *GraphPage::createIniGraphCheckBox(
        bool (IniGraph::*iniFlag)() const, void (IniGraph::*setIniFlag)(bool))
{
    return ControlUtil::createCheckBox(
            (iniGraph().*iniFlag)(), iniGraphSetter(iniFlag, setIniFlag));
}

std::function<void(int)> GraphPage::iniGraphValueSetter(
        int (IniGraph::*iniValue)() const, void (IniGraph::*setIniValue)(int))
{
    return iniGraphSetter(iniValue, setIniValue);
}

template<typename T>
std::function<void(T)> GraphPage::iniGraphSetter(
        T (IniGraph::*iniValue)() const, void (IniGraph::*setIniValue)(T))
{
    return [=, this](T v) {
        IniGraph ini = iniGraph();
        if ((ini.*iniValue)() == v)
            return;

        (ini.*setIniValue)(v);
        ctrl()->setIniUserEdited();
    };
}

void GraphPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Columns
    auto colLayout = setupColumns();
    layout->addLayout(colLayout, 1);

    this->setLayout(layout);
}

QLayout *GraphPage::setupColumns()
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

QLayout *GraphPage::setupColumn1()
{
    // Graph Group Box
    setupGraphBox();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbGraph);
    layout->addStretch();

    return layout;
}

QLayout *GraphPage::setupColumn2()
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

void GraphPage::setupGraphBox()
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

void GraphPage::setupGraphCheckboxes()
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

void GraphPage::setupGraphViewCheckboxes()
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

QLayout *GraphPage::setupGraphShowLayout()
{
    m_labelGraphShow = ControlUtil::createLabel();

    setupGraphShowCheckboxes();

    return ControlUtil::createHLayoutByWidgets(
            { m_labelGraphShow, /*stretch*/ nullptr, m_cbGraphShowTotal, m_cbGraphShowIn,
                    m_cbGraphShowOut },
            /*margin=*/0);
}

void GraphPage::setupGraphShowCheckboxes()
{
    m_cbGraphShowTotal = createIniGraphCheckBox(&IniGraph::showTotal, &IniGraph::setShowTotal);
    m_cbGraphShowTotal->setObjectName("cbGraphShowTotal");
    m_cbGraphShowIn = createIniGraphCheckBox(&IniGraph::showIn, &IniGraph::setShowIn);
    m_cbGraphShowIn->setObjectName("cbGraphShowIn");
    m_cbGraphShowOut = createIniGraphCheckBox(&IniGraph::showOut, &IniGraph::setShowOut);
    m_cbGraphShowOut->setObjectName("cbGraphShowOut");
}

void GraphPage::setupGraphOptions()
{
    m_graphOpacity = ControlUtil::createSpin({ iniGraph().opacity(), 0, 100 }, " %",
            iniGraphValueSetter(&IniGraph::opacity, &IniGraph::setOpacity));
    m_graphOpacity->setObjectName("graphOpacity");

    m_graphHoverOpacity = ControlUtil::createSpin({ iniGraph().hoverOpacity(), 0, 100 }, " %",
            iniGraphValueSetter(&IniGraph::hoverOpacity, &IniGraph::setHoverOpacity));
    m_graphHoverOpacity->setObjectName("graphHoverOpacity");

    m_graphTickLabelSize = ControlUtil::createSpin({ iniGraph().tickLabelSize(), 1, 99 }, {},
            iniGraphValueSetter(&IniGraph::tickLabelSize, &IniGraph::setTickLabelSize));
    m_graphTickLabelSize->setObjectName("graphTickLabelSize");

    m_graphBarWidth = ControlUtil::createSpin({ iniGraph().barWidth(), 1, 99 }, " px",
            iniGraphValueSetter(&IniGraph::barWidth, &IniGraph::setBarWidth));
    m_graphBarWidth->setObjectName("graphBarWidth");

    m_graphMaxSeconds = ControlUtil::createSpin({ iniGraph().maxSeconds(), 0, 9999 }, {},
            iniGraphValueSetter(&IniGraph::maxSeconds, &IniGraph::setMaxSeconds));
    m_graphMaxSeconds->setObjectName("graphMaxSeconds");

    setupGraphFixedSpeed();
}

void GraphPage::setupGraphFixedSpeed()
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
                    " Kb/s", iniGraphValueSetter(&IniGraph::fixedSpeed, &IniGraph::setFixedSpeed));
    m_graphFixedSpeed->setObjectName("graphFixedSpeed");
    m_graphFixedSpeed->setNames(speedNames);
}

QLayout *GraphPage::setupGraphTypeLayout()
{
    m_labelGraphType = ControlUtil::createLabel();

    m_comboGraphType = ControlUtil::createComboBox({}, [&](int index) { changeGraphType(index); });
    m_comboGraphType->setObjectName("comboGraphType");
    m_comboGraphType->setFixedWidth(110);

    return ControlUtil::createRowLayout(m_labelGraphType, m_comboGraphType);
}

void GraphPage::changeGraphType(int index)
{
    if (iniGraph().graphType() == index)
        return;

    iniGraph().setGraphType(index);
    ctrl()->setIniUserEdited();

    m_comboGraphType->setCurrentIndex(index);
}

QLayout *GraphPage::setupTrafUnitsLayout()
{
    const QStringList list = { "b/s", "B/s", "ib/s", "iB/s" };

    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(list, [&](int index) { changeTrafUnit(index); });
    m_comboTrafUnit->setObjectName("comboTrafUnit");
    m_comboTrafUnit->setFixedWidth(110);

    m_comboTrafUnit->setCurrentIndex(iniGraph().trafUnit());

    return ControlUtil::createRowLayout(m_traphUnits, m_comboTrafUnit);
}

void GraphPage::changeTrafUnit(int index)
{
    if (iniGraph().trafUnit() == index)
        return;

    iniGraph().setTrafUnit(index);
    ctrl()->setIniUserEdited();

    m_comboTrafUnit->setCurrentIndex(index);
}

void GraphPage::setupColorsBox()
{
    setupGraphColors();

    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_graphColor, m_graphBorderColor, m_graphColorIn, m_graphColorOut, m_graphTotalColor,
                    m_graphAxisColor, m_graphTickLabelColor, m_graphLabelColor, m_graphGridColor });

    m_gbColors = new QGroupBox();
    m_gbColors->setLayout(layout);
}

void GraphPage::setupGraphColors()
{
    setupGraphColors1();
    setupGraphColors2();
}

void GraphPage::setupGraphColors1()
{
    m_graphColor = ControlUtil::createLabelColor(
            iniGraph().color(), iniGraph().darkColor(),
            [&](const QColor &v) {
                iniGraph().setColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphColor->setObjectName("graphColor");

    m_graphBorderColor = ControlUtil::createLabelColor(
            iniGraph().borderColor(), iniGraph().darkBorderColor(),
            [&](const QColor &v) {
                iniGraph().setBorderColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkBorderColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphBorderColor->setObjectName("graphBorderColor");

    m_graphColorIn = ControlUtil::createLabelColor(
            iniGraph().colorIn(), iniGraph().darkColorIn(),
            [&](const QColor &v) {
                iniGraph().setColorIn(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkColorIn(v);
                ctrl()->setIniUserEdited();
            });
    m_graphColorIn->setObjectName("graphColorIn");

    m_graphColorOut = ControlUtil::createLabelColor(
            iniGraph().colorOut(), iniGraph().darkColorOut(),
            [&](const QColor &v) {
                iniGraph().setColorOut(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkColorOut(v);
                ctrl()->setIniUserEdited();
            });
    m_graphColorOut->setObjectName("graphColorOut");

    m_graphTotalColor = ControlUtil::createLabelColor(
            iniGraph().totalColor(), iniGraph().darkTotalColor(),
            [&](const QColor &v) {
                iniGraph().setTotalColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkTotalColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphTotalColor->setObjectName("graphTotalColor");
}

void GraphPage::setupGraphColors2()
{
    m_graphAxisColor = ControlUtil::createLabelColor(
            iniGraph().axisColor(), iniGraph().darkAxisColor(),
            [&](const QColor &v) {
                iniGraph().setAxisColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkAxisColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphAxisColor->setObjectName("graphAxisColor");

    m_graphTickLabelColor = ControlUtil::createLabelColor(
            iniGraph().tickLabelColor(), iniGraph().darkTickLabelColor(),
            [&](const QColor &v) {
                iniGraph().setTickLabelColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkTickLabelColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphTickLabelColor->setObjectName("graphTickLabelColor");

    m_graphLabelColor = ControlUtil::createLabelColor(
            iniGraph().labelColor(), iniGraph().darkLabelColor(),
            [&](const QColor &v) {
                iniGraph().setLabelColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkLabelColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphLabelColor->setObjectName("graphLabelColor");

    m_graphGridColor = ControlUtil::createLabelColor(
            iniGraph().gridColor(), iniGraph().darkGridColor(),
            [&](const QColor &v) {
                iniGraph().setGridColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniGraph().setDarkGridColor(v);
                ctrl()->setIniUserEdited();
            });
    m_graphGridColor->setObjectName("graphGridColor");
}

void GraphPage::setupGeometryBox()
{
    saveGraphWindowGeometry();

    setupGeometryControls();

    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbGraphMaximized, m_graphX, m_graphY, m_graphWidth, m_graphHeight });

    m_gbGeometry = new QGroupBox();
    m_gbGeometry->setLayout(layout);
}

void GraphPage::saveGraphWindowGeometry()
{
    // The window saves its geometry on close only
    const auto w = windowManager()->graphWindow();
    if (w) {
        w->saveWindowGeometry();
    }
}

void GraphPage::setupGeometryControls()
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

LabelSpin *GraphPage::createGeometrySpin(int value, int min)
{
    return ControlUtil::createSpin(
            { value, min, 99999 }, " px", [&](int /*v*/) { onGeometryEdited(); });
}

void GraphPage::onGeometryEdited()
{
    const QRect rect(m_graphX->spinBox()->value(), m_graphY->spinBox()->value(),
            m_graphWidth->spinBox()->value(), m_graphHeight->spinBox()->value());

    if (iniGraph().geometry() != rect) {
        iniGraph().setGeometry(rect);
        ctrl()->setIniUserEdited();
    }
}
