#ifndef STATBASEPAGE_H
#define STATBASEPAGE_H

#include <QFrame>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QTabBar)

class IniUser;
class StatisticsController;

class StatBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit StatBasePage(StatisticsController *ctrl, QWidget *parent = nullptr);

protected:
    StatisticsController *ctrl() const { return m_ctrl; }

protected slots:
    virtual void onSaveWindowState(IniUser & /*ini*/) { }
    virtual void onRestoreWindowState(IniUser & /*ini*/) { }

    virtual void onRetranslateUi() { }

    void saveTabIndex(const QString &iniKey, int tabIndex);

private:
    void setupController();

private:
    StatisticsController *m_ctrl = nullptr;
};

#endif // STATBASEPAGE_H
