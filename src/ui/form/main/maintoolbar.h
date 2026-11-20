#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include <QToolBar>

class MainWindow;

class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(MainWindow *mainWindow);

private:
    void setupUi(MainWindow *mainWindow);

    void addCheckBox(QAction *action);
    void addMenu(QMenu *menu);
};

#endif // MAINTOOLBAR_H
