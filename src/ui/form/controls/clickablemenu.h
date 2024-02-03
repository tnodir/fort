#ifndef CLICKABLEMENU_H
#define CLICKABLEMENU_H

#include <QMenu>

class ClickableMenu : public QMenu
{
    Q_OBJECT

public:
    explicit ClickableMenu(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_pressed : 1 = false;
};

#endif // CLICKABLEMENU_H
