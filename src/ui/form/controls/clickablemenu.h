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
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // CLICKABLEMENU_H
