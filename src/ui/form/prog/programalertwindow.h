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
    QString windowOverlayIconPath() const override { return ":/icons/error.png"; }

    bool isAutoActive() const;

    void initialize();

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    void closeOnSave() override;

protected slots:
    void onAppDeleted(qint64 appId);

    void retranslateWindowTitle() override;

private:
    void setupUi();
};

#endif // PROGRAMALERTWINDOW_H
