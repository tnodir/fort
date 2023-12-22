#include "controlutil.h"

#include <QActionEvent>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QWidgetAction>

#include <util/iconcache.h>

#include "combobox.h"
#include "labelcolor.h"
#include "labeldoublespin.h"
#include "labelspin.h"
#include "labelspincombo.h"
#include "menuwidget.h"
#include "sidebutton.h"
#include "spinbox.h"

QCheckBox *ControlUtil::createCheckBox(
        bool checked, const std::function<void(bool checked)> &onToggled)
{
    auto c = new QCheckBox();
    c->setChecked(checked);

    c->connect(c, &QCheckBox::toggled, onToggled);

    return c;
}

QSpinBox *ControlUtil::createSpinBox()
{
    auto c = new SpinBox();
    return c;
}

QComboBox *ControlUtil::createComboBox(const QStringList &texts)
{
    auto c = new ComboBox();
    c->addItems(texts);
    return c;
}

QComboBox *ControlUtil::createComboBox(
        const QStringList &texts, const std::function<void(int index)> &onActivated)
{
    auto c = createComboBox(texts);

    c->connect(c, QOverload<int>::of(&QComboBox::activated), onActivated);

    return c;
}

QPushButton *ControlUtil::createButton(const QString &iconPath, const QString &text)
{
    auto c = new QPushButton(IconCache::icon(iconPath), text);

    return c;
}

QPushButton *ControlUtil::createButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = new QPushButton(IconCache::icon(iconPath), QString());

    c->connect(c, &QPushButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createSideButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = new SideButton();
    c->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    c->setAutoRaise(true);
    c->setAutoExclusive(true);
    c->setCheckable(true);

    c->setIcon(IconCache::icon(iconPath));

    c->setIconSize(QSize(24, 24));
    c->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    c->connect(c, &QToolButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = new QToolButton();
    c->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    c->setIcon(IconCache::icon(iconPath));

    c->connect(c, &QToolButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createFlatToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createToolButton(iconPath, onClicked);
    c->setToolButtonStyle(Qt::ToolButtonIconOnly);
    c->setAutoRaise(true);
    c->setFocusPolicy(Qt::NoFocus);
    return c;
}

QPushButton *ControlUtil::createLinkButton(
        const QString &iconPath, const QString &linkPath, const QString &toolTip)
{
    auto c = new QPushButton(IconCache::icon(iconPath), QString());
    c->setFlat(true);
    c->setCursor(Qt::PointingHandCursor);
    c->setWindowFilePath(linkPath);
    c->setToolTip(!toolTip.isEmpty() ? toolTip : linkPath);
    return c;
}

QToolButton *ControlUtil::createSplitterButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createFlatToolButton(iconPath, onClicked);
    c->setCursor(Qt::ArrowCursor);
    c->setFixedSize(32, 32);
    return c;
}

QLabel *ControlUtil::createLabel(const QString &text)
{
    auto c = new QLabel(text);
    return c;
}

QLineEdit *ControlUtil::createLineLabel()
{
    auto c = new QLineEdit();
    c->setReadOnly(true);
    c->setFrame(false);

    QPalette pal;
    pal.setColor(QPalette::Base, Qt::transparent);
    c->setPalette(pal);

    return c;
}

QLineEdit *ControlUtil::createLineEdit(
        const QString &text, const std::function<void(const QString &text)> &onEdited)
{
    auto c = new QLineEdit(text);

    c->connect(c, &QLineEdit::textChanged, onEdited);

    return c;
}

QMenu *ControlUtil::createMenu(QWidget *parent)
{
    auto menu = new QMenu(parent);

    menu->setAttribute(Qt::WA_WindowPropagation); // to inherit default font

    return menu;
}

QMenu *ControlUtil::createMenuByLayout(QBoxLayout *layout, QWidget *parent)
{
    auto menu = createMenu(parent);

    auto menuWidget = new MenuWidget();
    menuWidget->setLayout(layout);

    auto wa = new QWidgetAction(menu);
    wa->setDefaultWidget(menuWidget);
    menu->addAction(wa);

    menu->connect(menuWidget, &MenuWidget::layoutRequested, [=] { relayoutMenu(menu, wa); });

    return menu;
}

void ControlUtil::relayoutMenu(QMenu *menu, QAction *action)
{
    if (!action) {
        action = menu->actions().first();
    }

    QActionEvent e(QEvent::ActionChanged, action);
    QCoreApplication::sendEvent(menu, &e);
}

QBoxLayout *ControlUtil::createLayoutByWidgets(const QList<QWidget *> &widgets, Qt::Orientation o)
{
    auto layout =
            new QBoxLayout(o == Qt::Vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);

    for (auto w : widgets) {
        if (!w) {
            layout->addStretch();
        } else {
            layout->addWidget(w);
        }
    }

    return layout;
}

QFrame *ControlUtil::createSeparator(Qt::Orientation o)
{
    auto c = new QFrame();
    c->setFrameShape(o == Qt::Horizontal ? QFrame::HLine : QFrame::VLine);
    c->setFrameShadow(QFrame::Sunken);
    return c;
}

QLayout *ControlUtil::createRowLayout(QWidget *w1, QWidget *w2, int stretch1)
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(w1, stretch1);
    layout->addWidget(w2);
    return layout;
}

QLayout *ControlUtil::createScrollLayout(QLayout *content, bool isBgTransparent)
{
    auto scrollAreaContent = new QWidget();
    scrollAreaContent->setLayout(content);

    auto scrollArea = wrapToScrollArea(scrollAreaContent, isBgTransparent);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);

    return layout;
}

QWidget *ControlUtil::wrapToScrollArea(QWidget *content, bool isBgTransparent)
{
    auto c = new QScrollArea();
    c->setContentsMargins(0, 0, 0, 0);
    c->setFrameShape(QFrame::NoFrame);
    c->setWidgetResizable(true);
    c->setWidget(content);

    if (isBgTransparent) {
        c->viewport()->setAutoFillBackground(false);
        content->setAutoFillBackground(false);
    }

    return c;
}

QFont ControlUtil::fontDemiBold()
{
    QFont font;
    font.setWeight(QFont::DemiBold);
    return font;
}

LabelSpinCombo *ControlUtil::createSpinCombo(int v, int min, int max, const QVector<int> &values,
        const QString &suffix, const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpinCombo();
    c->spinBox()->setValue(v);
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    c->setValues(values);

    c->connect(c->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelSpin *ControlUtil::createSpin(int v, int min, int max, const QString &suffix,
        const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpin();
    c->spinBox()->setValue(v);
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);

    c->connect(c->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelDoubleSpin *ControlUtil::createDoubleSpin(double v, double min, double max,
        const QString &suffix, const std::function<void(double)> &onValueChanged)
{
    auto c = new LabelDoubleSpin();
    c->spinBox()->setValue(v);
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);

    c->connect(c->spinBox(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelColor *ControlUtil::createLabelColor(
        const QColor &v, const std::function<void(const QColor &)> &onColorChanged)
{
    auto c = new LabelColor();
    c->setColor(v);

    c->connect(c, &LabelColor::colorChanged, onColorChanged);

    return c;
}
