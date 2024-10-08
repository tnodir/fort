#ifndef MENUBUTTON_H
#define MENUBUTTON_H

#include <QPushButton>

class TrayIcon;

class MenuButton : public QPushButton
{
    Q_OBJECT

public:
    explicit MenuButton(QWidget *parent = nullptr);

    TrayIcon *trayIcon() const;

private:
    void setupUi();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
};

#endif // MENUBUTTON_H
