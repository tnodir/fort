#ifndef LABELCOLOR_H
#define LABELCOLOR_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class LabelColor : public QWidget
{
    Q_OBJECT

public:
    explicit LabelColor(QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &v);

    QLabel *label() const { return m_label; }
    QPushButton *button() const { return m_button; }

public slots:
    void selectColor();

signals:
    void colorChanged(const QColor &color);

private:
    void setupUi();
    void setupButton();

private:
    QColor m_color;

    QLabel *m_label = nullptr;
    QPushButton *m_button = nullptr;
};

#endif // LABELCOLOR_H
