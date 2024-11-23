#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget *parent = nullptr);

    bool isEmpty() const;

    void addMenuAction(QAction *a) { return m_menuActions.append(a); }

public slots:
    void setText(const QString &text);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QList<QAction *> m_menuActions;
};

#endif // PLAINTEXTEDIT_H
