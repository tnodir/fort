#ifndef LOGOWIDGET_H
#define LOGOWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QBoxLayout)

class LogoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogoWidget(bool compact = false, QWidget *parent = nullptr);

private:
    void setupUi(bool compact);
    static QBoxLayout *setupIconLayout(bool compact);
    static QBoxLayout *setupTextLayout(bool compact);
    void setupOpacity();
};

#endif // LOGOWIDGET_H
