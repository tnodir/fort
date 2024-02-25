#ifndef FOCUSABLEMENU_H
#define FOCUSABLEMENU_H

#include <QMenu>

class FocusableMenu : public QMenu
{
    Q_OBJECT

public:
    explicit FocusableMenu(QWidget *parent = nullptr);

protected:
    bool event(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    bool focusNextPrevChild(bool next) override;
};

#endif // FOCUSABLEMENU_H
