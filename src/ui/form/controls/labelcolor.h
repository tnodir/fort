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

    QColor darkColor() const { return m_darkColor; }
    void setDarkColor(const QColor &v);

    QLabel *label() const { return m_label; }
    QPushButton *button() const { return m_button; }
    QPushButton *darkButton() const { return m_darkButton; }

public slots:
    void selectColor();
    void selectDarkColor();

signals:
    void colorChanged(const QColor &color);
    void darkColorChanged(const QColor &color);

private:
    void setupUi();
    void setupButton();
    void setupDarkButton();

private:
    QColor m_color;
    QColor m_darkColor;

    QLabel *m_label = nullptr;
    QPushButton *m_button = nullptr;
    QPushButton *m_darkButton = nullptr;
};

#endif // LABELCOLOR_H
