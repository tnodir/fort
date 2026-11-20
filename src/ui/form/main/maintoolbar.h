#ifndef MAINTOOLBAR_H
#define MAINTOOLBAR_H

#include <QToolBar>

class HeaderButtons;

class MainToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit MainToolBar(QWidget *parent = nullptr);

    HeaderButtons *headerButtons() const { return m_headerButtons; }

private:
    void setupUi();

    void addCheckBox(QAction *action);
    void addMenu(QMenu *menu);

private:
    HeaderButtons *m_headerButtons = nullptr;
};

#endif // MAINTOOLBAR_H
