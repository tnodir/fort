#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget *parent = nullptr);

    void setStartText(const QString &v);

    bool event(QEvent *event) override;
};

#endif // LINEEDIT_H
