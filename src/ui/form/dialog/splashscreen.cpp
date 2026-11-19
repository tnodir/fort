#include "splashscreen.h"

#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QTimer>

#include <form/controls/controlutil.h>
#include <form/controls/logowidget.h>
#include <manager/windowmanager.h>

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
    palette.setColor(QPalette::WindowText, Qt::white);

    this->setAutoFillBackground(true);
    this->setPalette(palette);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->resize(250, 80);

    // Position
    this->move(x() - width() / 2, y() - height() / 2);
}

QLayout *SplashScreen::setupMainLayout()
{
    auto logo = new LogoWidget();

    auto layout = ControlUtil::createHLayout(/*margin=*/10);

    layout->addStretch();
    layout->addWidget(logo);
    layout->addStretch();

    return layout;
}
