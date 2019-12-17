#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(QTabBar)

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(QWidget *parent = nullptr);

signals:

public slots:
    void retranslateUi();

private:
    void setupUi();

private:
    QTabBar *m_tabBar = nullptr;
};

#endif // OPTIONSWINDOW_H
