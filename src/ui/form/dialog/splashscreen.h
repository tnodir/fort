#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen();

public slots:
    void showFading();
    void closeDelayed();
    void closeFading();

private:
    void setupUi();
    QLayout *setupMainLayout();
};

#endif // SPLASHSCREEN_H
