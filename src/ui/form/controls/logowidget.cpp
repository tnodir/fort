#include "logowidget.h"

#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QVBoxLayout>

#include "controlutil.h"

namespace {

inline constexpr float COMPACT_OPACITY = 0.6f;

}

LogoWidget::LogoWidget(bool compact, QWidget *parent) : QWidget(parent)
{
    setupUi(compact);
}

void LogoWidget::setupUi(bool compact)
{
    auto iconLayout = setupIconLayout(compact);

    auto textLayout = setupTextLayout(compact);

    auto layout = ControlUtil::createHLayout(/*margin=*/compact ? 0 : 6);
    layout->setSpacing(compact ? 8 : 10);

    layout->addLayout(iconLayout);
    layout->addLayout(textLayout);

    this->setLayout(layout);

    if (compact) {
        setupOpacity();
    }
}

QBoxLayout *LogoWidget::setupIconLayout(bool compact)
{
    const int iconSize = compact ? 36 : 48;

    auto iconLogo = ControlUtil::createIconLabel(":/icons/fort-96.png", QSize(iconSize, iconSize));

    // Layout
    auto layout = ControlUtil::createVLayout();
    layout->setSpacing(0);

    layout->addStretch();
    layout->addSpacing(1);
    layout->addWidget(iconLogo);
    layout->addStretch();

    return layout;
}

QBoxLayout *LogoWidget::setupTextLayout(bool compact)
{
    QFont font("Franklin Gothic", compact ? 18 : 22, QFont::Bold);

    // Label
    auto label = ControlUtil::createLabel(QGuiApplication::applicationName());
    label->setFont(font);

    // Layout
    auto layout = ControlUtil::createVLayout();
    layout->setSpacing(0);

    layout->addStretch();
    layout->addWidget(label, 0, Qt::AlignHCenter);

    // Sub-label
    font.setPointSize(compact ? 8 : 10);
    font.setWeight(QFont::DemiBold);

    auto subLabel = ControlUtil::createLabel("- keeping you secure -");
    subLabel->setFont(font);

    layout->addWidget(subLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(3);

    layout->addStretch();

    return layout;
}

void LogoWidget::setupOpacity()
{
    // Dimmed, but brighter than the placeholder text
    auto effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(COMPACT_OPACITY);

    setGraphicsEffect(effect);
}
