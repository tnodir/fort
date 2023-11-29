#ifndef APPSCOLUMN_H
#define APPSCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

class PlainTextEdit;

class AppsColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AppsColumn(const QString &iconPath, QWidget *parent = nullptr);

    QHBoxLayout *headerLayout() const { return m_headerLayout; }
    QLabel *icon() const { return m_icon; }
    QLabel *labelTitle() const { return m_labelTitle; }
    PlainTextEdit *editText() const { return m_editText; }

signals:
    void textEdited(const QString &text);

private:
    void setupUi();
    void setupTextEdit();
    void setupIcon(const QString &iconPath);

    void onTextChanged();

private:
    QHBoxLayout *m_headerLayout = nullptr;
    QLabel *m_icon = nullptr;
    QLabel *m_labelTitle = nullptr;
    PlainTextEdit *m_editText = nullptr;
};

#endif // APPSCOLUMN_H
