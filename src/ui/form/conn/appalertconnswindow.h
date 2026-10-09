#ifndef APPALERTCONNSWINDOW_H
#define APPALERTCONNSWINDOW_H

#include "appconnswindow.h"

class AppAlertConnsWindow : public AppConnsWindow
{
    Q_OBJECT

public:
    explicit AppAlertConnsWindow(const App &app, QWidget *parent = nullptr);

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;
};

#endif // APPALERTCONNSWINDOW_H
