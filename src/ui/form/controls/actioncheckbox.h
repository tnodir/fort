#ifndef ACTIONCHECKBOX_H
#define ACTIONCHECKBOX_H

#include <QCheckBox>

QT_FORWARD_DECLARE_CLASS(QAction)

/* The check box of a checkable action, as QToolButton::setDefaultAction() for a button */
class ActionCheckBox : public QCheckBox
{
    Q_OBJECT

public:
    explicit ActionCheckBox(QAction *action, QWidget *parent = nullptr);

    QAction *action() const { return m_action; }

private:
    void updateState();

private:
    QAction *m_action = nullptr;
};

#endif // ACTIONCHECKBOX_H
