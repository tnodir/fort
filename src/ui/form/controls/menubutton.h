#ifndef MENUBUTTON_H
#define MENUBUTTON_H

#include "pushbutton.h"

class MenuButton : public PushButton
{
    Q_OBJECT

public:
    explicit MenuButton(QWidget *parent = nullptr);

private:
    void setupUi();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
};

#endif // MENUBUTTON_H
