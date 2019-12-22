#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);

    int tabMinimumWidth() const { return m_tabMinimumWidth; }
    void setTabMinimumWidth(int v) { m_tabMinimumWidth = v; }

protected:
    QSize tabSizeHint(int index) const override;

private:
    int m_tabMinimumWidth = 0;
};

#endif // TABBAR_H
