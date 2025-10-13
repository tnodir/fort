#ifndef HOMEBASEPAGE_H
#define HOMEBASEPAGE_H

#include <QFrame>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class HomeController;
class IniUser;

class HomeBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit HomeBasePage(HomeController *ctrl, QWidget *parent = nullptr);

protected:
    HomeController *ctrl() const { return m_ctrl; }

protected slots:
    virtual void onSaveWindowState(IniUser & /*ini*/) { }
    virtual void onRestoreWindowState(IniUser & /*ini*/) { }

    virtual void onRetranslateUi() { }

private:
    void setupController();

    void onPasswordLockedChanged();

private:
    HomeController *m_ctrl = nullptr;
};

#endif // HOMEBASEPAGE_H
