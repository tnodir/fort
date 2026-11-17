#ifndef HEADERBUTTONS_H
#define HEADERBUTTONS_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class HeaderButtons : public QWidget
{
    Q_OBJECT

public:
    explicit HeaderButtons(int optionsTabIndex = 0, QWidget *parent = nullptr);

    QToolButton *optionsButton() const { return m_btOptions; }
    QToolButton *statisticsButton() const { return m_btStatistics; }
    QPushButton *menuButton() const { return m_btMenu; }

private:
    void setupUi(int optionsTabIndex);

private:
    QToolButton *m_btOptions = nullptr;
    QToolButton *m_btStatistics = nullptr;
    QPushButton *m_btMenu = nullptr;
};

#endif // HEADERBUTTONS_H
