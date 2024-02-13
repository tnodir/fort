#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

QT_FORWARD_DECLARE_CLASS(QLabel)

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen();

    static QLabel *createLogoIcon();
    static QLayout *createLogoTextLayout();

public slots:
    void showFading();
    void closeDelayed();
    void closeFading();

private:
    void setupUi();
    QLayout *setupMainLayout();
};

#endif // SPLASHSCREEN_H
