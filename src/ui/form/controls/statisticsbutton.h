#ifndef STATISTICSBUTTON_H
#define STATISTICSBUTTON_H

#include "toolbutton.h"

class StatisticsButton : public ToolButton
{
    Q_OBJECT

public:
    explicit StatisticsButton(QWidget *parent = nullptr);

public slots:
    void showTrafficWindow();

private:
    void setupUi();
};

#endif // STATISTICSBUTTON_H
