#include "graphpage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/labelcolor.h>
#include <form/controls/labelspin.h>
#include <form/opt/optionscontroller.h>
#include <util/iconcache.h>

GraphPage::GraphPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void GraphPage::onRetranslateUi()
{
    m_gbGraph->setTitle(tr("Graph"));

    m_cbGraphAlwaysOnTop->setText(tr("Always on top"));
    m_cbGraphFrameless->setText(tr("Frameless"));
    m_cbGraphClickThrough->setText(tr("Click through"));
    m_cbGraphHideOnHover->setText(tr("Hide on hover"));
    m_graphOpacity->label()->setText(tr("Opacity:"));
    m_graphHoverOpacity->label()->setText(tr("Hover opacity:"));
    m_graphMaxSeconds->label()->setText(tr("Max seconds:"));
    m_graphColor->label()->setText(tr("Background:"));
    m_graphColorIn->label()->setText(tr("Download:"));
    m_graphColorOut->label()->setText(tr("Upload:"));
    m_graphAxisColor->label()->setText(tr("Axis:"));
    m_graphTickLabelColor->label()->setText(tr("Tick label:"));
    m_graphLabelColor->label()->setText(tr("Label:"));
    m_graphGridColor->label()->setText(tr("Grid:"));
}

void GraphPage::setupUi()
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

    this->setLayout(layout);
}

QLayout *GraphPage::setupColumn1()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    // Graph Group Box
    setupGraphBox();
    layout->addWidget(m_gbGraph);

    layout->addStretch();

    return layout;
}

QLayout *GraphPage::setupColumn2()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    layout->addStretch();

    return layout;
}

void GraphPage::setupGraphBox()
{
    setupGraphCheckboxes();
    setupGraphOptions();
    setupGraphColors();

    // Layout
    auto colLayout1 = ControlUtil::createLayoutByWidgets({ m_cbGraphAlwaysOnTop, m_cbGraphFrameless,
            m_cbGraphClickThrough, m_cbGraphHideOnHover, ControlUtil::createSeparator(),
            m_graphOpacity, m_graphHoverOpacity, m_graphMaxSeconds, nullptr });

    auto colLayout2 =
            ControlUtil::createLayoutByWidgets({ m_graphColor, m_graphColorIn, m_graphColorOut,
                    m_graphAxisColor, m_graphTickLabelColor, m_graphLabelColor, m_graphGridColor });

    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addLayout(colLayout2);

    m_gbGraph = new QGroupBox();
    m_gbGraph->setLayout(layout);
}

void GraphPage::setupGraphCheckboxes()
{
    m_cbGraphAlwaysOnTop =
            ControlUtil::createCheckBox(ini()->graphWindowAlwaysOnTop(), [&](bool checked) {
                if (ini()->graphWindowAlwaysOnTop() != checked) {
                    ini()->setGraphWindowAlwaysOnTop(checked);
                    ctrl()->setIniEdited();
                }
            });
    m_cbGraphFrameless =
            ControlUtil::createCheckBox(ini()->graphWindowFrameless(), [&](bool checked) {
                if (ini()->graphWindowFrameless() != checked) {
                    ini()->setGraphWindowFrameless(checked);
                    ctrl()->setIniEdited();
                }
            });
    m_cbGraphClickThrough =
            ControlUtil::createCheckBox(ini()->graphWindowClickThrough(), [&](bool checked) {
                if (ini()->graphWindowClickThrough() != checked) {
                    ini()->setGraphWindowClickThrough(checked);
                    ctrl()->setIniEdited();
                }
            });
    m_cbGraphHideOnHover =
            ControlUtil::createCheckBox(ini()->graphWindowHideOnHover(), [&](bool checked) {
                if (ini()->graphWindowHideOnHover() != checked) {
                    ini()->setGraphWindowHideOnHover(checked);
                    ctrl()->setIniEdited();
                }
            });
}

void GraphPage::setupGraphOptions()
{
    m_graphOpacity = ControlUtil::createSpin(ini()->graphWindowOpacity(), 0, 100, " %", [&](int v) {
        if (ini()->graphWindowOpacity() != v) {
            ini()->setGraphWindowOpacity(v);
            ctrl()->setIniEdited();
        }
    });

    m_graphHoverOpacity =
            ControlUtil::createSpin(ini()->graphWindowHoverOpacity(), 0, 100, " %", [&](int v) {
                if (ini()->graphWindowHoverOpacity() != v) {
                    ini()->setGraphWindowHoverOpacity(v);
                    ctrl()->setIniEdited();
                }
            });

    m_graphMaxSeconds =
            ControlUtil::createSpin(ini()->graphWindowMaxSeconds(), 0, 9999, {}, [&](int v) {
                if (ini()->graphWindowMaxSeconds() != v) {
                    ini()->setGraphWindowMaxSeconds(v);
                    ctrl()->setIniEdited();
                }
            });
}

void GraphPage::setupGraphColors()
{
    m_graphColor = ControlUtil::createLabelColor(ini()->graphWindowColor(), [&](const QColor &v) {
        if (ini()->graphWindowColor() != v) {
            ini()->setGraphWindowColor(v);
            ctrl()->setIniEdited();
        }
    });
    m_graphColorIn =
            ControlUtil::createLabelColor(ini()->graphWindowColorIn(), [&](const QColor &v) {
                if (ini()->graphWindowColorIn() != v) {
                    ini()->setGraphWindowColorIn(v);
                    ctrl()->setIniEdited();
                }
            });
    m_graphColorOut =
            ControlUtil::createLabelColor(ini()->graphWindowColorOut(), [&](const QColor &v) {
                if (ini()->graphWindowColorOut() != v) {
                    ini()->setGraphWindowColorOut(v);
                    ctrl()->setIniEdited();
                }
            });
    m_graphAxisColor =
            ControlUtil::createLabelColor(ini()->graphWindowAxisColor(), [&](const QColor &v) {
                if (ini()->graphWindowAxisColor() != v) {
                    ini()->setGraphWindowAxisColor(v);
                    ctrl()->setIniEdited();
                }
            });
    m_graphTickLabelColor =
            ControlUtil::createLabelColor(ini()->graphWindowTickLabelColor(), [&](const QColor &v) {
                if (ini()->graphWindowTickLabelColor() != v) {
                    ini()->setGraphWindowTickLabelColor(v);
                    ctrl()->setIniEdited();
                }
            });
    m_graphLabelColor =
            ControlUtil::createLabelColor(ini()->graphWindowLabelColor(), [&](const QColor &v) {
                if (ini()->graphWindowLabelColor() != v) {
                    ini()->setGraphWindowLabelColor(v);
                    ctrl()->setIniEdited();
                }
            });
    m_graphGridColor =
            ControlUtil::createLabelColor(ini()->graphWindowGridColor(), [&](const QColor &v) {
                if (ini()->graphWindowGridColor() != v) {
                    ini()->setGraphWindowGridColor(v);
                    ctrl()->setIniEdited();
                }
            });
}
