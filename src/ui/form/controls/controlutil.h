#ifndef CONTROLUTIL_H
#define CONTROLUTIL_H

#include <QObject>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ControlUtil
{
public:
    static QCheckBox *createCheckBox(bool checked,
                                     const std::function<void (bool checked)> &onToggled);
    static QComboBox *createComboBox(const QStringList &texts,
                                     const std::function<void (int index)> &onActivated);
    static QPushButton *createButton(const QString &iconPath,
                                     const std::function<void ()> &onClicked);
    static QToolButton *createToolButton(const QString &iconPath,
                                         const std::function<void ()> &onClicked);
    static QPushButton *createLinkButton(const QString &iconPath,
                                         const QString &linkPath = QString(),
                                         const QString &toolTip = QString());
    static QMenu *createMenuByWidgets(const QList<QWidget *> &widgets,
                                      QWidget *parent);
    static QFrame *createHSeparator();
};

#endif // CONTROLUTIL_H
