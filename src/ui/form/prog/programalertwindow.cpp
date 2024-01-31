#include "programalertwindow.h"

#include "programscontroller.h"

ProgramAlertWindow::ProgramAlertWindow(QWidget *parent) :
    ProgramEditDialog(new ProgramsController(this), parent)
{
}
