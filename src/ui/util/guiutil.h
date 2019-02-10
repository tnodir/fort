#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QColor>
#include <QObject>
#include <QVariant>

class GuiUtil : public QObject
{
    Q_OBJECT

public:
    explicit GuiUtil(QObject *parent = nullptr);

    Q_INVOKABLE static void setClipboardData(const QVariant &data);

    Q_INVOKABLE static QColor getColor(const QColor &initial = Qt::white);
    Q_INVOKABLE static bool isValidColor(const QColor &color);
};

#endif // GUIUTIL_H
