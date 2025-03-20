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
#include <form/opt/optionscontroller.h>
#include <user/iniuser.h>
#include <util/formatutil.h>
#include <util/iconcache.h>

GraphPage::GraphPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void GraphPage::onResetToDefault()
{
    m_cbGraphHideOnClose->setChecked(iniUser()->graphWindowHideOnCloseDefault());

    m_cbGraphAlwaysOnTop->setChecked(iniUser()->graphWindowAlwaysOnTopDefault());
    m_cbGraphFrameless->setChecked(iniUser()->graphWindowFramelessDefault());
    m_cbGraphClickThrough->setChecked(iniUser()->graphWindowClickThroughDefault());
    m_cbGraphHideOnHover->setChecked(iniUser()->graphWindowHideOnHoverDefault());
    m_graphOpacity->spinBox()->setValue(iniUser()->graphWindowOpacityDefault());
    m_graphHoverOpacity->spinBox()->setValue(iniUser()->graphWindowHoverOpacityDefault());
    m_graphTickLabelSize->spinBox()->setValue(iniUser()->graphWindowTickLabelSizeDefault());
    m_graphMaxSeconds->spinBox()->setValue(iniUser()->graphWindowMaxSecondsDefault());
    m_graphFixedSpeed->spinBox()->setValue(iniUser()->graphWindowFixedSpeedDefault());
    m_comboTrafUnit->setCurrentIndex(iniUser()->graphWindowTrafUnitDefault());

    m_graphColor->setColor(iniUser()->graphWindowColorDefault());
    m_graphColorIn->setColor(iniUser()->graphWindowColorInDefault());
    m_graphColorOut->setColor(iniUser()->graphWindowColorOutDefault());
    m_graphAxisColor->setColor(iniUser()->graphWindowAxisColorDefault());
    m_graphTickLabelColor->setColor(iniUser()->graphWindowTickLabelColorDefault());
    m_graphLabelColor->setColor(iniUser()->graphWindowLabelColorDefault());
    m_graphGridColor->setColor(iniUser()->graphWindowGridColorDefault());

    m_graphColor->setDarkColor(iniUser()->graphWindowDarkColorDefault());
    m_graphColorIn->setDarkColor(iniUser()->graphWindowDarkColorInDefault());
    m_graphColorOut->setDarkColor(iniUser()->graphWindowDarkColorOutDefault());
    m_graphAxisColor->setDarkColor(iniUser()->graphWindowDarkAxisColorDefault());
    m_graphTickLabelColor->setDarkColor(iniUser()->graphWindowDarkTickLabelColorDefault());
    m_graphLabelColor->setDarkColor(iniUser()->graphWindowDarkLabelColorDefault());
    m_graphGridColor->setDarkColor(iniUser()->graphWindowDarkGridColorDefault());
}

void GraphPage::onRetranslateUi()
{
    m_gbGraph->setTitle(tr("Window"));
    m_gbColors->setTitle(tr("Colors (Light | Dark)"));

    m_cbGraphHideOnClose->setText(tr("Hide on close"));

    m_cbGraphAlwaysOnTop->setText(tr("Always on top"));
    m_cbGraphFrameless->setText(tr("Frameless"));
    m_cbGraphClickThrough->setText(tr("Click through"));
    m_cbGraphHideOnHover->setText(tr("Hide on hover"));
    m_graphOpacity->label()->setText(tr("Opacity:"));
    m_graphHoverOpacity->label()->setText(tr("Hover opacity:"));
    m_graphTickLabelSize->label()->setText(tr("Tick label size:"));
    m_graphMaxSeconds->label()->setText(tr("Max seconds:"));
    m_graphFixedSpeed->label()->setText(tr("Fixed speed:"));
    retranslateFixedSpeedCombo();
    m_traphUnits->setText(tr("Units:"));

    m_graphColor->label()->setText(tr("Background:"));
    m_graphColorIn->label()->setText(tr("Download:"));
    m_graphColorOut->label()->setText(tr("Upload:"));
    m_graphAxisColor->label()->setText(tr("Axis:"));
    m_graphTickLabelColor->label()->setText(tr("Tick label:"));
    m_graphLabelColor->label()->setText(tr("Label:"));
    m_graphGridColor->label()->setText(tr("Grid:"));
}

void GraphPage::retranslateFixedSpeedCombo()
{
    auto names = m_graphFixedSpeed->names();

    names.replace(0, tr("Custom"));
    names.replace(1, tr("Auto-scale"));

    m_graphFixedSpeed->setNames(names);
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

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);
    layout->addWidget(m_gbColors);
    layout->addStretch();

    return layout;
}

void GraphPage::setupGraphBox()
{
    setupGraphCheckboxes();
    setupGraphOptions();

    // Traffic Units
    auto trafUnitsLayout = setupTrafUnitsLayout();

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
            m_graphTickLabelSize,
            m_graphMaxSeconds,
            m_graphFixedSpeed,
            ControlUtil::createSeparator(),
    });
    layout->addLayout(trafUnitsLayout);

    m_gbGraph = new QGroupBox();
    m_gbGraph->setLayout(layout);
}

void GraphPage::setupGraphCheckboxes()
{
    m_cbGraphHideOnClose =
            ControlUtil::createCheckBox(iniUser()->graphWindowHideOnClose(), [&](bool checked) {
                if (iniUser()->graphWindowHideOnClose() != checked) {
                    iniUser()->setGraphWindowHideOnClose(checked);
                    ctrl()->setIniUserEdited();
                }
            });
    m_cbGraphAlwaysOnTop =
            ControlUtil::createCheckBox(iniUser()->graphWindowAlwaysOnTop(), [&](bool checked) {
                if (iniUser()->graphWindowAlwaysOnTop() != checked) {
                    iniUser()->setGraphWindowAlwaysOnTop(checked);
                    ctrl()->setIniUserEdited();
                }
            });
    m_cbGraphFrameless =
            ControlUtil::createCheckBox(iniUser()->graphWindowFrameless(), [&](bool checked) {
                if (iniUser()->graphWindowFrameless() != checked) {
                    iniUser()->setGraphWindowFrameless(checked);
                    ctrl()->setIniUserEdited();
                }
            });
    m_cbGraphClickThrough =
            ControlUtil::createCheckBox(iniUser()->graphWindowClickThrough(), [&](bool checked) {
                if (iniUser()->graphWindowClickThrough() != checked) {
                    iniUser()->setGraphWindowClickThrough(checked);
                    ctrl()->setIniUserEdited();
                }
            });
    m_cbGraphHideOnHover =
            ControlUtil::createCheckBox(iniUser()->graphWindowHideOnHover(), [&](bool checked) {
                if (iniUser()->graphWindowHideOnHover() != checked) {
                    iniUser()->setGraphWindowHideOnHover(checked);
                    ctrl()->setIniUserEdited();
                }
            });
}

void GraphPage::setupGraphOptions()
{
    m_graphOpacity =
            ControlUtil::createSpin(iniUser()->graphWindowOpacity(), 0, 100, " %", [&](int v) {
                if (iniUser()->graphWindowOpacity() != v) {
                    iniUser()->setGraphWindowOpacity(v);
                    ctrl()->setIniUserEdited();
                }
            });

    m_graphHoverOpacity =
            ControlUtil::createSpin(iniUser()->graphWindowHoverOpacity(), 0, 100, " %", [&](int v) {
                if (iniUser()->graphWindowHoverOpacity() != v) {
                    iniUser()->setGraphWindowHoverOpacity(v);
                    ctrl()->setIniUserEdited();
                }
            });

    m_graphTickLabelSize =
            ControlUtil::createSpin(iniUser()->graphWindowTickLabelSize(), 1, 99, {}, [&](int v) {
                if (iniUser()->graphWindowTickLabelSize() != v) {
                    iniUser()->setGraphWindowTickLabelSize(v);
                    ctrl()->setIniUserEdited();
                }
            });

    m_graphMaxSeconds =
            ControlUtil::createSpin(iniUser()->graphWindowMaxSeconds(), 0, 9999, {}, [&](int v) {
                if (iniUser()->graphWindowMaxSeconds() != v) {
                    iniUser()->setGraphWindowMaxSeconds(v);
                    ctrl()->setIniUserEdited();
                }
            });

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
    m_graphFixedSpeed = ControlUtil::createSpinCombo(iniUser()->graphWindowFixedSpeed(), 0, 9999999,
            speedValuesList, " Kb/s", [&](int value) {
                if (iniUser()->graphWindowFixedSpeed() != value) {
                    iniUser()->setGraphWindowFixedSpeed(value);
                    ctrl()->setIniUserEdited();
                }
            });
    m_graphFixedSpeed->setNames(speedNames);
}

QLayout *GraphPage::setupTrafUnitsLayout()
{
    const QStringList list = { "b/s", "B/s", "ib/s", "iB/s" };

    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(list, [&](int index) {
        if (iniUser()->graphWindowTrafUnit() != index) {
            iniUser()->setGraphWindowTrafUnit(index);
            ctrl()->setIniUserEdited();
        }
    });
    m_comboTrafUnit->setFixedWidth(110);

    m_comboTrafUnit->setCurrentIndex(iniUser()->graphWindowTrafUnit());

    return ControlUtil::createRowLayout(m_traphUnits, m_comboTrafUnit);
}

void GraphPage::setupColorsBox()
{
    setupGraphColors();

    auto layout =
            ControlUtil::createVLayoutByWidgets({ m_graphColor, m_graphColorIn, m_graphColorOut,
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
            iniUser()->graphWindowColor(), iniUser()->graphWindowDarkColor(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkColor(v);
                ctrl()->setIniUserEdited();
            });

    m_graphColorIn = ControlUtil::createLabelColor(
            iniUser()->graphWindowColorIn(), iniUser()->graphWindowDarkColorIn(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowColorIn(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkColorIn(v);
                ctrl()->setIniUserEdited();
            });

    m_graphColorOut = ControlUtil::createLabelColor(
            iniUser()->graphWindowColorOut(), iniUser()->graphWindowDarkColorOut(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowColorOut(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkColorOut(v);
                ctrl()->setIniUserEdited();
            });
}

void GraphPage::setupGraphColors2()
{
    m_graphAxisColor = ControlUtil::createLabelColor(
            iniUser()->graphWindowAxisColor(), iniUser()->graphWindowDarkAxisColor(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowAxisColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkAxisColor(v);
                ctrl()->setIniUserEdited();
            });

    m_graphTickLabelColor = ControlUtil::createLabelColor(
            iniUser()->graphWindowTickLabelColor(), iniUser()->graphWindowDarkTickLabelColor(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowTickLabelColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkTickLabelColor(v);
                ctrl()->setIniUserEdited();
            });

    m_graphLabelColor = ControlUtil::createLabelColor(
            iniUser()->graphWindowLabelColor(), iniUser()->graphWindowDarkLabelColor(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowLabelColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkLabelColor(v);
                ctrl()->setIniUserEdited();
            });

    m_graphGridColor = ControlUtil::createLabelColor(
            iniUser()->graphWindowGridColor(), iniUser()->graphWindowDarkGridColor(),
            [&](const QColor &v) {
                iniUser()->setGraphWindowGridColor(v);
                ctrl()->setIniUserEdited();
            },
            [&](const QColor &v) {
                iniUser()->setGraphWindowDarkGridColor(v);
                ctrl()->setIniUserEdited();
            });
}
