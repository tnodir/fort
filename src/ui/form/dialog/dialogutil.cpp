#include "dialogutil.h"

#include <QColorDialog>
#include <QFileDialog>

QString DialogUtil::getOpenFileName(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileName(
            nullptr, title, QString(), filter, nullptr, QFileDialog::ReadOnly);
}

QStringList DialogUtil::getOpenFileNames(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileNames(
            nullptr, title, QString(), filter, nullptr, QFileDialog::ReadOnly);
}

QString DialogUtil::getSaveFileName(const QString &title, const QString &filter)
{
    return QFileDialog::getSaveFileName(
            nullptr, title, QString(), filter, nullptr, QFileDialog::ReadOnly);
}

QColor DialogUtil::getColor(const QColor &initial, const QString &title)
{
    return QColorDialog::getColor(initial, nullptr, title);
}
