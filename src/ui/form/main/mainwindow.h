#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QMainWindow)

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

protected:
    bool closeOnEscape() const override { return false; }

private:
    void retranslateUi();

    void setupUi();
    void setupDockHost();

private:
    QLabel *m_labelHint = nullptr;
    QMainWindow *m_dockHost = nullptr;
};

#endif // MAINWINDOW_H
