#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

class MenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MenuWidget(QMenu *menu, QAction *action, QWidget *parent = nullptr);

signals:
    void layoutChanged();

protected:
    bool event(QEvent *event) override;

private:
    void relayoutMenu();

private:
    QMenu *m_menu = nullptr;
    QAction *m_action = nullptr;
};

#endif // MENUWIDGET_H
