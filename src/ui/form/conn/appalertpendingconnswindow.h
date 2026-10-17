#ifndef APPALERTPENDINGCONNSWINDOW_H
#define APPALERTPENDINGCONNSWINDOW_H

#include "apppendingconnswindow.h"

class AppAlertPendingConnsWindow : public AppPendingConnsWindow
{
    Q_OBJECT

public:
    explicit AppAlertPendingConnsWindow(const App &app, QWidget *parent = nullptr);

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;
};

#endif // APPALERTPENDINGCONNSWINDOW_H
