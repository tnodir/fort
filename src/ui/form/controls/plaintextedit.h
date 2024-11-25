#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

    bool isEmpty() const;

    void addContextAction(QAction *a) { return m_contextActions.append(a); }

public slots:
    void setText(const QString &text);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QList<QAction *> m_contextActions;
};

#endif // PLAINTEXTEDIT_H
