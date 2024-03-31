#include "controlutil.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QWidgetAction>

#include <util/iconcache.h>

#include "combobox.h"
#include "focusablemenu.h"
#include "labelcolor.h"
#include "labeldoublespin.h"
#include "labelspin.h"
#include "labelspincombo.h"
#include "lineedit.h"
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
    c->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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

void ControlUtil::setComboBoxTexts(QComboBox *c, const QStringList &texts, int currentIndex)
{
    if (currentIndex < -1) {
        currentIndex = c->currentIndex();
    }

    currentIndex = qBound(-1, currentIndex, texts.size() - 1);

    c->clear();
    c->addItems(texts);

    c->setCurrentIndex(currentIndex);
}

void ControlUtil::setComboBoxIcons(QComboBox *c, const QStringList &iconPaths)
{
    int index = 0;
    for (const QString &iconPath : iconPaths) {
        c->setItemIcon(index, IconCache::icon(iconPath));

        ++index;
    }
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

QToolButton *ControlUtil::createToolButton(const QString &iconPath)
{
    auto c = new QToolButton();
    c->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    c->setIcon(IconCache::icon(iconPath));
    return c;
}

QToolButton *ControlUtil::createToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createToolButton(iconPath);

    c->connect(c, &QToolButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createFlatToolButton(const QString &iconPath)
{
    auto c = createToolButton(iconPath);
    c->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    c->setCursor(Qt::PointingHandCursor);
    c->setAutoRaise(true);
    c->setFocusPolicy(Qt::TabFocus);
    return c;
}

QToolButton *ControlUtil::createFlatToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createFlatToolButton(iconPath);

    c->connect(c, &QToolButton::clicked, onClicked);

    return c;
}

QToolButton *ControlUtil::createIconToolButton(const QString &iconPath)
{
    auto c = createFlatToolButton(iconPath);
    c->setToolButtonStyle(Qt::ToolButtonIconOnly);
    return c;
}

QToolButton *ControlUtil::createIconToolButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createFlatToolButton(iconPath, onClicked);
    c->setToolButtonStyle(Qt::ToolButtonIconOnly);
    return c;
}

QToolButton *ControlUtil::createSplitterButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = createFlatToolButton(iconPath, onClicked);
    c->setToolButtonStyle(Qt::ToolButtonIconOnly);
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
    auto c = new LineEdit();
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
    auto menu = new FocusableMenu(parent);
    auto wa = new QWidgetAction(menu);

    auto menuWidget = new MenuWidget(menu, wa);
    menuWidget->setLayout(layout);

    wa->setDefaultWidget(menuWidget);
    menu->addAction(wa);

    return menu;
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

void ControlUtil::clearLayout(QLayout *layout)
{
    if (!layout)
        return;

    int i = layout->count();
    while (--i >= 0) {
        auto item = layout->takeAt(i);

        auto w = item->widget();
        if (w) {
            w->deleteLater();
        } else {
            clearLayout(item->layout());
        }

        delete item;
    }
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

LabelSpinCombo *ControlUtil::createSpinCombo(int v, int min, int max, const QVector<int> &values,
        const QString &suffix, const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpinCombo();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    c->spinBox()->setValue(v);
    c->setValues(values);

    c->connect(c->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelSpin *ControlUtil::createSpin(int v, int min, int max, const QString &suffix,
        const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpin();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    c->spinBox()->setValue(v);

    c->connect(c->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelDoubleSpin *ControlUtil::createDoubleSpin(double v, double min, double max,
        const QString &suffix, const std::function<void(double)> &onValueChanged)
{
    auto c = new LabelDoubleSpin();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    c->spinBox()->setValue(v);

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

QLabel *ControlUtil::formRowLabel(QFormLayout *formLayout, QWidget *field)
{
    auto label = qobject_cast<QLabel *>(formLayout->labelForField(field));
    Q_ASSERT(label);

    label->setMinimumWidth(100);

    return label;
}

QLabel *ControlUtil::formRowLabel(QFormLayout *formLayout, QLayout *field)
{
    auto label = qobject_cast<QLabel *>(formLayout->labelForField(field));
    Q_ASSERT(label);

    label->setMinimumWidth(100);

    return label;
}
