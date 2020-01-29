#ifndef APPSCOLUMN_H
#define APPSCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)

QT_FORWARD_DECLARE_CLASS(PlainTextEdit)

class AppsColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AppsColumn(QWidget *parent = nullptr);

    QLabel *icon() const { return m_icon; }
    QLabel *labelTitle() const { return m_labelTitle; }
    PlainTextEdit *editText() const { return m_editText; }

private:
    void setupUi();

private:
    QLabel *m_icon = nullptr;
    QLabel *m_labelTitle = nullptr;
    PlainTextEdit *m_editText = nullptr;
};

#endif // APPSCOLUMN_H
