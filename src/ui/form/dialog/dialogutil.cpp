#include "dialogutil.h"

#include <QColorDialog>
#include <QFileDialog>

#include <util/window/widgetwindow.h>

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

QString DialogUtil::getExistingDir(const QString &title)
{
    return QFileDialog::getExistingDirectory(nullptr, title);
}

QColor DialogUtil::getColor(const QColor &initial, const QString &title)
{
    return QColorDialog::getColor(initial, nullptr, title);
}

void DialogUtil::setupModalDialog(QWidget *box)
{
    const bool isAppModal = !box->parent();

    box->setWindowModality(isAppModal ? Qt::ApplicationModal : Qt::WindowModal);

    if (isAppModal) {
        box->setWindowFlag(Qt::WindowStaysOnTopHint);
    }
}

QMessageBox *DialogUtil::createMessageBox(const MessageBoxArg &ba, QWidget *parent)
{
    auto box = new QMessageBox(ba.icon, ba.title, ba.text, ba.buttons, parent);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setInformativeText(ba.info);
    setupModalDialog(box);
    return box;
}

void DialogUtil::showDialog(QWidget *box)
{
    static bool g_isDialogShown = false;

    // Workaround to show initial dialog on WinPE
    if (!g_isDialogShown) {
        g_isDialogShown = true;
        box->show();
        box->hide();
    }

    WidgetWindow::showWidget(box);
}
