#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QPointer>

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QMainWindow)

class MainMenuBar;
class PanelDock;

class MainWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowMain; }
    QString helpSection() const override;
    QString iniGroup() const override;

    QAction *lockAction() const { return m_actLock; }
    QAction *unlockAction() const { return m_actUnlock; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    void onWindowClosed() override;

    QList<PanelDock *> docks() const;

    // The dock with the focus
    PanelDock *currentDock() const;

    PanelDock *addDock(FormWindow *w);
    void removeDock(PanelDock *dock);

    // The docks as tabs of one group, as they are added
    void resetDocksLayout();

protected:
    bool closeOnEscape() const override { return false; }

protected slots:
    bool checkAboutToClose() override;

private:
    QStringList dockGroups() const;

    PanelDock *dockOf(const QWidget *w) const;
    bool isShownWidget(const QWidget *w) const;

    void restoreDocks();
    void restoreDockedWindows(QList<QPointer<QWidget>> &focuses);
    void setFocusToShown(const QList<QPointer<QWidget>> &focuses);
    void updateLocked();
    void updateLockToolTip();

    void retranslateUi();

    void setupUi();
    void setupLockActions();
    QWidget *setupHeader();
    void setupDockHost();

    void updateHint();

private:
    QAction *m_actLock = nullptr;
    QAction *m_actUnlock = nullptr;
    MainMenuBar *m_menuBar = nullptr;
    QLabel *m_labelHint = nullptr;
    QMainWindow *m_dockHost = nullptr;
};

#endif // MAINWINDOW_H
