#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include <QToolBar>

class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(QWidget *parent = nullptr);

private:
    void setupUi();

    void addCheckBox(QAction *action);
    void addMenu(QMenu *menu);
};

#endif // MAINTOOLBAR_H
