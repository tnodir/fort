#ifndef DIALOGUTIL_H
#define DIALOGUTIL_H

#include <QColor>
#include <QMessageBox>
#include <QObject>

struct MessageBoxArg
{
    QMessageBox::Icon icon;
    QMessageBox::StandardButtons buttons;
    const QString text;
    const QString title;
    const QString info;
};

class DialogUtil
{
public:
    static QString getOpenFileName(
            const QString &title = QString(), const QString &filter = QString());
    static QStringList getOpenFileNames(
            const QString &title = QString(), const QString &filter = QString());
    static QString getSaveFileName(
            const QString &title = QString(), const QString &filter = QString());
    static QString getExistingDir(const QString &title = QString());

    static QColor getColor(const QColor &initial = Qt::white, const QString &title = QString());

    static void setupModalDialog(QWidget *box);

    static QMessageBox *createMessageBox(const MessageBoxArg &ba, QWidget *parent = nullptr);

    static void showDialog(QWidget *box);
};

#endif // DIALOGUTIL_H
