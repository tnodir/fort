#include "splashscreen.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

#include <form/controls/controlutil.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

namespace {

void startOpacityAnimation(
        QWidget *w, const std::function<void()> &onFinished, bool backward = false)
{
    auto anim = new QPropertyAnimation(w, "windowOpacity", w);
    anim->setDirection(backward ? QPropertyAnimation::Backward : QPropertyAnimation::Forward);
    anim->setEasingCurve(QEasingCurve::InCubic);
    anim->setDuration(900);
    anim->setStartValue(0.0f);
    anim->setEndValue(1.0f);

    QObject::connect(anim, &QPropertyAnimation::finished, onFinished);

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

}

SplashScreen::SplashScreen() : QSplashScreen()
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setAttribute(Qt::WA_ShowWithoutActivating);

    setupUi();
}

void SplashScreen::showFading()
{
    setWindowOpacity(0.0f);
    show();

    startOpacityAnimation(this, [&] { closeDelayed(); });
}

void SplashScreen::closeDelayed()
{
    QTimer::singleShot(1600, this, &SplashScreen::closeFading);
}

void SplashScreen::closeFading()
{
    startOpacityAnimation(this, [&] { close(); }, /*backward=*/true);
}

void SplashScreen::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Background Color
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x26, 0x26, 0x26));

    this->setAutoFillBackground(true);
    this->setPalette(palette);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(IconCache::icon(":/icons/fort.png"));

    // Size
    this->resize(250, 80);

    // Position
    this->move(x() - width() / 2, y() - height() / 2);
}

QLayout *SplashScreen::setupMainLayout()
{
    // Logo image
    auto logoIcon = createLogoIcon();

    // Logo text
    auto logoText = createLogoTextLayout();

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(10);

    layout->addStretch();
    layout->addWidget(logoIcon);
    layout->addLayout(logoText);
    layout->addStretch();

    return layout;
}

QLabel *SplashScreen::createLogoIcon()
{
    auto iconLogo = ControlUtil::createLabel();

    iconLogo->setScaledContents(true);
    const QSize logoSize(48, 48);
    iconLogo->setMinimumSize(logoSize);
    iconLogo->setMaximumSize(logoSize);

    iconLogo->setPixmap(IconCache::file(":/icons/fort-96.png"));

    return iconLogo;
}

QLayout *SplashScreen::createLogoTextLayout()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);

    QFont font("Franklin Gothic", 22, QFont::Bold);

    // Label
    auto label = ControlUtil::createLabel(QGuiApplication::applicationName());
    label->setPalette(palette);
    label->setFont(font);

    // Sub-label
    font.setPointSize(10);
    font.setWeight(QFont::DemiBold);

    auto subLabel = ControlUtil::createLabel("- keeping you secure -");
    subLabel->setPalette(palette);
    subLabel->setFont(font);

    layout->addWidget(label, 2, Qt::AlignHCenter | Qt::AlignBottom);
    layout->addWidget(subLabel, 1, Qt::AlignHCenter | Qt::AlignTop);

    return layout;
}
