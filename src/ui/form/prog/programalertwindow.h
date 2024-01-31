#ifndef PROGRAMALERTWINDOW_H
#define PROGRAMALERTWINDOW_H

#include <form/windowtypes.h>

#include "programeditdialog.h"

class WidgetWindowStateWatcher;

class ProgramAlertWindow : public ProgramEditDialog
{
    Q_OBJECT

public:
    explicit ProgramAlertWindow(QWidget *parent = nullptr);

    quint32 windowCode() const override { return WindowProgramAlert; }

    void initialize();

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    void closeOnSave() override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateWindowTitle() override;

    void setupUi();

private:
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
};

#endif // PROGRAMALERTWINDOW_H
