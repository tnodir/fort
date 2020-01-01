#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include <QObject>

class ProgramsWindow : public QObject
{
    Q_OBJECT

public:
    explicit ProgramsWindow(QObject *parent = nullptr);

};

#endif // PROGRAMSWINDOW_H
