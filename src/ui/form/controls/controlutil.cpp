#include "controlutil.h"

#include <QAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QGraphicsOpacityEffect>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStyle>
#include <QToolButton>
#include <QWidgetAction>

#include <util/iconcache.h>
#include <util/osutil.h>

#include "combobox.h"
#include "focusablemenu.h"
#include "labelcolor.h"
#include "labeldoublespin.h"
#include "labelspin.h"
#include "labelspincombo.h"
#include "lineedit.h"
#include "menubutton.h"
#include "menuwidget.h"
#include "optionsbutton.h"
#include "pushbutton.h"
#include "sidebutton.h"
#include "spinbox.h"
#include "statisticsbutton.h"
#include "toolbutton.h"

namespace {

void setupSpinBox(QSpinBox *spinBox, const ControlUtil::SpinValue &v, const QString &suffix,
        const std::function<void(int)> &onValueChanged)
{
    spinBox->setRange(v.min, v.max);
    spinBox->setSuffix(suffix);
    spinBox->setValue(v.value);

    QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), onValueChanged);
}

inline constexpr int opacityAnimationDuration = 160; // as the line edits' Clear button

// The animation leads to the full transparency
bool isFadeOut(QPropertyAnimation *anim)
{
    return qFuzzyIsNull(anim->endValue().toReal());
}

// The widget's opacity effect and its animation are created once
QPropertyAnimation *opacityAnimation(QWidget *w)
{
    auto effect = qobject_cast<QGraphicsOpacityEffect *>(w->graphicsEffect());
    if (effect) {
        return effect->findChild<QPropertyAnimation *>();
    }

    effect = new QGraphicsOpacityEffect();
    w->setGraphicsEffect(effect);

    auto anim = new QPropertyAnimation(effect, "opacity", effect);
    anim->setDuration(opacityAnimationDuration);

    QObject::connect(anim, &QPropertyAnimation::finished, w, [=] {
        if (isFadeOut(anim)) {
            w->hide();
        }
    });

    return anim;
}

// The visibility, which the running animation leads to
bool isVisibleTarget(QWidget *w, QPropertyAnimation *anim)
{
    if (anim->state() == QAbstractAnimation::Running) {
        return !isFadeOut(anim);
    }

    return !w->isHidden();
}

}

QCheckBox *ControlUtil::createCheckBox(const QString &iconPath)
{
    auto c = new QCheckBox();

    if (!iconPath.isEmpty()) {
        c->setIcon(IconCache::icon(iconPath));
    }

    return c;
}

QCheckBox *ControlUtil::createCheckBox(
        bool checked, const std::function<void(bool checked)> &onToggled)
{
    auto c = new QCheckBox();
    c->setChecked(checked);

    c->connect(c, &QCheckBox::toggled, onToggled);

    return c;
}

QCheckBox *ControlUtil::createCheckStateBox(
        const QString &iconPath, Qt::CheckState state, const std::function<void()> &onClicked)
{
    auto c = new QCheckBox();
    c->setIcon(IconCache::icon(iconPath));

    c->setTristate(true);
    c->setCheckState(state);

    c->connect(c, &QCheckBox::clicked, c, onClicked);

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

    for (int i = 0; i < texts.size(); ++i) {
        c->setItemData(i, texts[i], Qt::ToolTipRole);
    }

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

void ControlUtil::setComboBoxItemsEnabled(QComboBox *c, bool enabled, int fromIndex, int toIndex)
{
    auto comboModel = qobject_cast<QStandardItemModel *>(c->model());
    Q_ASSERT(comboModel);

    if (toIndex == -1) {
        toIndex = fromIndex;
    }

    for (int i = fromIndex; i <= toIndex; ++i) {
        auto item = comboModel->item(i);
        Q_ASSERT(item);

        item->setEnabled(enabled);
    }
}

QPushButton *ControlUtil::createButton(const QString &iconPath, const QString &text)
{
    auto c = new PushButton(IconCache::icon(iconPath), text);

    return c;
}

QPushButton *ControlUtil::createButton(
        const QString &iconPath, const std::function<void()> &onClicked)
{
    auto c = new PushButton(IconCache::icon(iconPath), QString());

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
    auto c = new ToolButton();
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

void ControlUtil::setToolButtonFlat(QToolButton *c)
{
    c->setCursor(Qt::PointingHandCursor);
    c->setAutoRaise(true);
    c->setFocusPolicy(Qt::TabFocus);
}

QToolButton *ControlUtil::createFlatToolButton(const QString &iconPath)
{
    auto c = createToolButton(iconPath);
    c->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setToolButtonFlat(c);
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

QToolButton *ControlUtil::createCheckableIconToolButton(
        const QString &iconPath, const QString &checkedIconPath)
{
    auto c = createIconToolButton(QString());
    c->setCheckable(true);
    c->setIcon(checkableIcon(iconPath, checkedIconPath));

    return c;
}

QIcon ControlUtil::checkableIcon(const QString &iconPath, const QString &checkedIconPath)
{
    // A button or menu draws the icon's On state, when it's checked
    QIcon icon = IconCache::icon(iconPath);
    icon.addFile(checkedIconPath, {}, QIcon::Normal, QIcon::On);
    return icon;
}

QToolButton *ControlUtil::createClearButton(const std::function<void()> &onClicked)
{
    auto c = createIconToolButton(QString(), onClicked);
    c->setIcon(clearIcon(c));
    return c;
}

QIcon ControlUtil::clearIcon(const QWidget *w)
{
    return w->style()->standardIcon(QStyle::SP_LineEditClearButton);
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

QLabel *ControlUtil::createIconLabel(const QString &iconPath, const QSize &size)
{
    auto c = createLabel();
    c->setScaledContents(true);
    c->setFixedSize(size);
    c->setPixmap(IconCache::pixmap(iconPath, size));
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

QLineEdit *ControlUtil::createSearchEdit(const std::function<void(const QString &text)> &onEdited)
{
    auto c = createLineEdit(QString(), onEdited);
    c->setObjectName("editSearch");
    c->setClearButtonEnabled(true);
    c->setMaxLength(200);
    c->setMinimumWidth(100);
    c->setMaximumWidth(200);

    return c;
}

QMenu *ControlUtil::createMenu(QWidget *parent)
{
    auto menu = new QMenu(parent);

    menu->setAttribute(Qt::WA_WindowPropagation); // to inherit default font

    return menu;
}

namespace {

void addHeaderColumnActions(
        QMenu *menu, QHeaderView *header, const std::function<QString(int column)> &columnName)
{
    const int count = header->count();
    const bool canHide = (header->hiddenSectionCount() < count - 1);

    for (int i = 0; i < count; ++i) {
        const bool isHidden = header->isSectionHidden(i);

        auto a = menu->addAction(columnName(i));
        a->setCheckable(true);
        a->setChecked(!isHidden);
        a->setEnabled(isHidden || canHide);

        QObject::connect(a, &QAction::triggered, header,
                [=](bool checked) { header->setSectionHidden(i, !checked); });
    }
}

void addHeaderStretchAction(QMenu *menu, QHeaderView *header, const QString &text)
{
    auto a = menu->addAction(text);
    a->setCheckable(true);
    a->setChecked(header->stretchLastSection());

    QObject::connect(a, &QAction::triggered, header, &QHeaderView::setStretchLastSection);
}

}

void ControlUtil::showHeaderMenu(QHeaderView *header, const QPoint &pos,
        const std::function<QString(int column)> &columnName, const QString &stretchLastText)
{
    auto menu = createMenu();
    deleteOnClose(menu);

    addHeaderColumnActions(menu, header, columnName);

    menu->addSeparator();

    addHeaderStretchAction(menu, header, stretchLastText);

    menu->popup(header->mapToGlobal(pos));
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

void ControlUtil::setActionTexts(const QList<QAction *> &actions, const QStringList &texts)
{
    int index = 0;
    for (QAction *a : actions) {
        a->setText(texts.value(index));

        ++index;
    }
}

QBoxLayout *ControlUtil::createLayout(QBoxLayout::Direction direction, int margin)
{
    auto layout = new QBoxLayout(direction);

    if (margin >= 0) {
        layout->setContentsMargins(margin, margin, margin, margin);
    }

    return layout;
}

QLayout *ControlUtil::createRowLayout(QWidget *w1, QWidget *w2, int stretch1)
{
    auto layout = createHLayout();
    layout->addWidget(w1, stretch1);
    layout->addWidget(w2);
    return layout;
}

void ControlUtil::fillLayoutByWidgets(QBoxLayout *layout, const QList<QWidget *> &widgets)
{
    for (auto w : widgets) {
        if (!w) {
            layout->addStretch();
        } else {
            layout->addWidget(w);
        }
    }
}

QBoxLayout *ControlUtil::createLayoutByWidgets(
        const QList<QWidget *> &widgets, QBoxLayout::Direction direction, int margin)
{
    auto layout = createLayout(direction, margin);

    fillLayoutByWidgets(layout, widgets);

    return layout;
}

QFrame *ControlUtil::createSeparator(Qt::Orientation o)
{
    auto c = new QFrame();
    c->setFrameShape(o == Qt::Horizontal ? QFrame::HLine : QFrame::VLine);
    c->setFrameShadow(QFrame::Sunken);
    return c;
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

QWidget *ControlUtil::wrapToWidget(QLayout *layout)
{
    auto widget = new QWidget();
    widget->setLayout(layout);

    return widget;
}

QLayout *ControlUtil::createScrollLayout(QLayout *content, bool isBgTransparent)
{
    auto contentWidget = wrapToWidget(content);

    auto scrollArea = wrapToScrollArea(contentWidget, isBgTransparent);

    auto layout = createHLayout();
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

LabelSpinCombo *ControlUtil::createSpinCombo(const SpinValue &v, const QVector<int> &values,
        const QString &suffix, const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpinCombo();
    c->setValues(values);

    setupSpinBox(c->spinBox(), v, suffix, onValueChanged);

    return c;
}

LabelSpin *ControlUtil::createSpin(
        const SpinValue &v, const QString &suffix, const std::function<void(int)> &onValueChanged)
{
    auto c = new LabelSpin();

    setupSpinBox(c->spinBox(), v, suffix, onValueChanged);

    return c;
}

LabelDoubleSpin *ControlUtil::createDoubleSpin(double v, double min, double max,
        const QString &suffix, const std::function<void(double)> &onValueChanged)
{
    auto c = new LabelDoubleSpin();

    auto spinBox = c->spinBox();
    spinBox->setRange(min, max);
    spinBox->setSuffix(suffix);
    spinBox->setValue(v);

    c->connect(c->spinBox(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), onValueChanged);

    return c;
}

LabelColor *ControlUtil::createLabelColor(const QColor &color, const QColor &darkColor,
        const std::function<void(const QColor &color)> &onColorChanged,
        const std::function<void(const QColor &color)> &onDarkColorChanged)
{
    auto c = new LabelColor();
    c->setColor(color);
    c->setDarkColor(darkColor);

    c->connect(c, &LabelColor::colorChanged, onColorChanged);
    c->connect(c, &LabelColor::darkColorChanged, onDarkColorChanged);

    return c;
}

static QLabel *formRowLabelField(QWidget *field)
{
    auto label = qobject_cast<QLabel *>(field);
    Q_ASSERT(label);

    label->setMinimumWidth(70);

    return label;
}

QLabel *ControlUtil::formRowLabel(QFormLayout *formLayout, QWidget *field)
{
    return formRowLabelField(formLayout->labelForField(field));
}

QLabel *ControlUtil::formRowLabel(QFormLayout *formLayout, QLayout *field)
{
    return formRowLabelField(formLayout->labelForField(field));
}

void ControlUtil::setFormRowVisible(QLabel *label, bool visible)
{
    label->setVisible(visible);
    label->buddy()->setVisible(visible);
}

QPushButton *ControlUtil::createMenuButton()
{
    return new MenuButton();
}

QToolButton *ControlUtil::createOptionsButton(int tabIndex)
{
    return new OptionsButton(tabIndex);
}

QToolButton *ControlUtil::createStatisticsButton()
{
    return new StatisticsButton();
}

void ControlUtil::deleteOnClose(QWidget *w)
{
    w->setAttribute(Qt::WA_DeleteOnClose);
}

QAction *ControlUtil::setupHelpShortcut(QWidget *w, const QString &helpSection)
{
    return setupHelpShortcut(w, [=] { return helpSection; });
}

QAction *ControlUtil::setupHelpShortcut(QWidget *w, const std::function<QString()> &getHelpSection)
{
    auto a = new QAction(w);
    a->setShortcut(QKeySequence::HelpContents);

    QObject::connect(a, &QAction::triggered, w, [=] { OsUtil::openUserGuide(getHelpSection()); });

    w->addAction(a);

    return a;
}

void ControlUtil::setVisibleAnimated(QWidget *w, bool visible)
{
    auto anim = opacityAnimation(w);
    if (isVisibleTarget(w, anim) == visible)
        return;

    if (w->isHidden()) {
        static_cast<QGraphicsOpacityEffect *>(anim->targetObject())->setOpacity(0.0);
        w->show();
    }

    anim->stop();
    anim->setEndValue(visible ? 1.0 : 0.0);
    anim->start();

    // At once while the window is hidden
    if (!w->window()->isVisible()) {
        anim->setCurrentTime(anim->duration());
    }
}
