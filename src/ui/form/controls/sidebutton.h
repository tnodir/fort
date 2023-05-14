#ifndef SIDEBUTTON_H
#define SIDEBUTTON_H

#include <QToolButton>

class SideButton : public QToolButton
{
    Q_OBJECT

public:
    explicit SideButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // SIDEBUTTON_H
