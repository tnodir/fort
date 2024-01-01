#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

    bool isEmpty() const;

public slots:
    void setText(const QString &text);
};

#endif // PLAINTEXTEDIT_H
