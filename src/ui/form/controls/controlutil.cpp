#include "controlutil.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

QCheckBox *ControlUtil::createCheckBox(bool checked,
                                       const std::function<void (bool)> &onToggled)
{
    auto c = new QCheckBox();
    c->setChecked(checked);

    c->connect(c, &QCheckBox::toggled, onToggled);

    return c;
}

QComboBox *ControlUtil::createComboBox(const QStringList &texts,
                                       const std::function<void (int)> &onActivated)
{
    auto c = new QComboBox();
    c->addItems(texts);

    c->connect(c, QOverload<int>::of(&QComboBox::activated), onActivated);

    return c;
}

QPushButton *ControlUtil::createButton(const QString &iconPath,
                                       const std::function<void ()> &onClicked)
{
    auto c = new QPushButton(QIcon(iconPath), QString());

    c->connect(c, &QPushButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createToolButton(const QString &iconPath,
                                           const std::function<void ()> &onClicked)
{
    auto c = new QToolButton();
    c->setIcon(QIcon(iconPath));

    c->connect(c, &QToolButton::clicked, onClicked);

    return c;
}

QPushButton *ControlUtil::createLinkButton(const QString &iconPath,
                                           const QString &linkPath,
                                           const QString &toolTip)
{
    auto c = new QPushButton(QIcon(iconPath), QString());
    c->setWindowFilePath(linkPath);
    c->setToolTip(!toolTip.isEmpty() ? toolTip : linkPath);
    return c;
}

QMenu *ControlUtil::createMenuByWidgets(const QList<QWidget *> &widgets,
                                        QWidget *parent)
{
    auto menu = new QMenu(parent);

    auto layout = new QVBoxLayout();
    for (auto w : widgets) {
        layout->addWidget(w);
    }

    auto menuWidget = new QWidget();
    menuWidget->setLayout(layout);

    auto wa = new QWidgetAction(menu);
    wa->setDefaultWidget(menuWidget);
    menu->addAction(wa);

    return menu;
}

QFrame *ControlUtil::createHSeparator()
{
    auto c = new QFrame();
    c->setFrameShape(QFrame::HLine);
    return c;
}

QStringList ControlUtil::getOpenFileNames(const QString &title,
                                          const QString &filter)
{
    return QFileDialog::getOpenFileNames(
                nullptr, title, QString(), filter,
                nullptr, QFileDialog::ReadOnly);
}
