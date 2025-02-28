#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include <QPushButton>

class PushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit PushButton(QWidget *parent = nullptr);
    explicit PushButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

protected:
    bool mousePressed() const { return m_mousePressed; }
    void setMousePressed(bool v) { m_mousePressed = v; }

    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void onRightClicked();

private:
    bool m_mousePressed : 1 = false;
};

#endif // PUSHBUTTON_H
