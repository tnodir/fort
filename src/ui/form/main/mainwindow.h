#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QMainWindow)

class PanelDock;

class MainWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowMain; }
    QString helpSection() const override;
    QString iniGroup() const override;

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

    void restoreDocks();

    void retranslateUi();

    void setupUi();
    QWidget *setupHeader();
    void setupDockHost();

    void updateHint();

private:
    QLabel *m_labelHint = nullptr;
    QMainWindow *m_dockHost = nullptr;
};

#endif // MAINWINDOW_H
