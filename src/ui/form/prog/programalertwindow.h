#ifndef PROGRAMALERTWINDOW_H
#define PROGRAMALERTWINDOW_H

#include <form/form_types.h>

#include "programeditdialog.h"

class ProgramAlertWindow : public ProgramEditDialog
{
    Q_OBJECT

public:
    explicit ProgramAlertWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowProgramAlert; }

    void initialize();

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    void closeOnSave() override;

private:
    void setupController();

    void retranslateWindowTitle() override;

    void setupUi();
};

#endif // PROGRAMALERTWINDOW_H
