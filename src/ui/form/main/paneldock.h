#ifndef PANELDOCK_H
#define PANELDOCK_H

#include <QDockWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ClipContainer;
class FormWindow;
class WindowMenuButton;

class PanelDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit PanelDock(FormWindow *formWindow, QWidget *parent = nullptr);

    FormWindow *formWindow() const { return m_formWindow; }

    void takeFormWindow();

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void onDroppedFloating();

    void retranslateUi();

    void setupUi();
    QWidget *setupTitleBar();

    void updateTitle();

private:
    FormWindow *m_formWindow = nullptr;

    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    ClipContainer *m_headerArea = nullptr;
    WindowMenuButton *m_btWindowMenu = nullptr;
    QToolButton *m_btClose = nullptr;
};

#endif // PANELDOCK_H
