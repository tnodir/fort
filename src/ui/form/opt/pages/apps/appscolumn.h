#ifndef APPSCOLUMN_H
#define APPSCOLUMN_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class PlainTextEdit;

class AppsColumn : public QWidget
{
    Q_OBJECT

public:
    explicit AppsColumn(const QString &iconPath, QWidget *parent = nullptr);

    QLayout *headerLayout() const { return m_headerLayout; }
    QLabel *icon() const { return m_icon; }
    QToolButton *btClear() const { return m_btClear; }
    QLabel *labelTitle() const { return m_labelTitle; }
    PlainTextEdit *editText() const { return m_editText; }

    void setText(const QString &text);

signals:
    void textEdited(const QString &text);

private:
    void setupUi();
    QLayout *setupHeaderLayout();
    void setupTextEdit();
    void setupIcon(const QString &iconPath);

    void updateBtClear();

    void onTextChanged();

private:
    QLayout *m_headerLayout = nullptr;
    QLabel *m_icon = nullptr;
    QToolButton *m_btClear = nullptr;
    QLabel *m_labelTitle = nullptr;
    PlainTextEdit *m_editText = nullptr;
};

#endif // APPSCOLUMN_H
