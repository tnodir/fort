#ifndef APPSCOLUMN_H
#define APPSCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

class AppsColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AppsColumn(QWidget *parent = nullptr);

    QLabel *labelTitle() const { return m_labelTitle; }
    QPlainTextEdit *editText() const { return m_editText; }

private:
    void setupUi();

private:
    QLabel *m_labelTitle = nullptr;
    QPlainTextEdit *m_editText = nullptr;
};

#endif // APPSCOLUMN_H
