#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QObject>
#include <QVariant>

class GuiUtil : public QObject
{
    Q_OBJECT

public:
    explicit GuiUtil(QObject *parent = nullptr);

    Q_INVOKABLE static void setClipboardData(const QVariant &data);
};

#endif // GUIUTIL_H
