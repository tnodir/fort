#ifndef PROGRAMALERTWINDOW_H
#define PROGRAMALERTWINDOW_H

#include <form/form_types.h>

#include "programeditdialog.h"

class ConfAppManager;
class ConfManager;
class IniUser;

class ProgramAlertWindow : public ProgramEditDialog
{
    Q_OBJECT

public:
    explicit ProgramAlertWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowProgramAlert; }
    QString windowOverlayIconPath() const override { return ":/icons/error.png"; }

    ConfAppManager *confAppManager() const;
    ConfManager *confManager() const;
    IniUser *iniUser() const;

    bool isAutoActive() const;

    void initialize();

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    void closeOnSave() override;

protected slots:
    void retranslateWindowTitle() override;

private:
    void setupUi();
};

#endif // PROGRAMALERTWINDOW_H
