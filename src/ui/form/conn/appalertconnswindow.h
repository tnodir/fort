#ifndef APPALERTCONNSWINDOW_H
#define APPALERTCONNSWINDOW_H

#include "appconnswindow.h"

class AppAlertConnsWindow : public AppConnsWindow
{
    Q_OBJECT

public:
    explicit AppAlertConnsWindow(const App &app, QWidget *parent = nullptr);

    void restoreWindowState() override;

protected:
    QString iniGroup() const override;
};

#endif // APPALERTCONNSWINDOW_H
