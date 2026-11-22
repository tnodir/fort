#include "dialogutil.h"

#include <QAbstractButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QMetaEnum>

#include <form/controls/controlutil.h>
#include <util/window/widgetwindow.h>

namespace {

// The buttons are named by their standard ones for the "ui" command: btYes, btNo, btOk...
void setButtonsObjectNames(QMessageBox *box)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QMessageBox::StandardButtons>();

    const auto buttons = box->buttons();
    for (QAbstractButton *button : buttons) {
        const char *key = metaEnum.valueToKey(int(box->standardButton(button)));

        button->setObjectName(QString("bt") + key);
    }
}

}

QString DialogUtil::getOpenFileName(const QString &title, const QString &dir, const QString &filter)
{
    return QFileDialog::getOpenFileName(
            nullptr, title, dir, filter, nullptr, QFileDialog::ReadOnly);
}

QStringList DialogUtil::getOpenFileNames(
        const QString &title, const QString &dir, const QString &filter)
{
    return QFileDialog::getOpenFileNames(
            nullptr, title, dir, filter, nullptr, QFileDialog::ReadOnly);
}

QString DialogUtil::getSaveFileName(const QString &title, const QString &dir, const QString &filter)
{
    return QFileDialog::getSaveFileName(
            nullptr, title, dir, filter, nullptr, QFileDialog::ReadOnly);
}

QString DialogUtil::getExistingDir(const QString &title, const QString &dir)
{
    return QFileDialog::getExistingDirectory(nullptr, title, dir);
}

QColor DialogUtil::getColor(const QColor &initial, const QString &title)
{
    return QColorDialog::getColor(initial, nullptr, title, QColorDialog::ShowAlphaChannel);
}

bool DialogUtil::getFont(QFont &font, const QString &title)
{
    bool ok = false;
    font = QFontDialog::getFont(&ok, font, nullptr, title);
    return ok;
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
    ControlUtil::deleteOnClose(box);

    setButtonsObjectNames(box);

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
