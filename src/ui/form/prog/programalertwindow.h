#ifndef PROGRAMALERTWINDOW_H
#define PROGRAMALERTWINDOW_H

#include "programeditdialog.h"

class ProgramAlertWindow : public ProgramEditDialog
{
    Q_OBJECT

public:
    explicit ProgramAlertWindow(QWidget *parent = nullptr);

};

#endif // PROGRAMALERTWINDOW_H
