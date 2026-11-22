#include "controlcommandui.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QDeadlineTimer>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

#include <algorithm>
#include <functional>
#include <limits>

#include <control/controlworker.h>
#include <util/variantutil.h>

#include "controlcommandmanager.h"
#include "controlcommandutil.h"
#include "uitestutil.h"
#include "windowshow.h"

namespace {

inline constexpr int defaultWaitTimeoutMsecs = 5 * 1000;
inline constexpr int maxWaitTimeoutMsecs = 10 * 60 * 1000;
inline constexpr int waitCheckMsecs = 50;

inline constexpr Qt::MouseButton mouseButtons[] = {
    Qt::LeftButton,
    Qt::RightButton,
    Qt::MiddleButton,
};

QStringList mouseButtonNames()
{
    // Sync with mouseButtons
    return { "left", "right", "middle" };
}

struct MouseClick
{
    Qt::MouseButton button = Qt::LeftButton;
    int count = 1;

    bool isSingleLeft() const { return button == Qt::LeftButton && count == 1; }
};

struct MouseStep
{
    QEvent::Type type;
    QPoint pos;
};

struct UiWait
{
    QString path;
    QString name;
    QString value;
};

struct IntField
{
    int value; // the default one
    int min = 0;
    int max = std::numeric_limits<int>::max();
    const char *key;
};

struct UiWaitState
{
    quint32 requestId = 0;
    ControlWorker *worker = nullptr;
    QDeadlineTimer deadline;
    UiWait wait;
};

QString argText(const QVariantList &args, int index)
{
    return args.value(index).toString();
}

QString propertyText(const QString &name, const QVariant &value)
{
    return ControlCommandUtil::escapedFieldsToText({ { name, value } });
}

bool findArgNode(const ProcessCommandArgs &p, UiTestNode &node, ProcessCommandResult &r)
{
    return UiTestUtil::findNode(argText(p.args, 1), node, r.errorMessage);
}

template<typename T>
T *findArgObject(const ProcessCommandArgs &p, ProcessCommandResult &r, const char *typeText)
{
    UiTestNode node;
    if (!findArgNode(p, node, r))
        return nullptr;

    T *o = qobject_cast<T *>(node.object);
    if (!o) {
        r.errorMessage = "Not " + QString(typeText) + ": " + node.path;
    }

    return o;
}

bool isAncestorWidget(const QWidget *ancestor, const QWidget *w)
{
    for (; w; w = w->parentWidget()) {
        if (w == ancestor)
            return true;
    }

    return false;
}

bool isModalWindow(const QWidget *w)
{
    return w->isWindow() && w->windowModality() != Qt::NonModal;
}

// The widget of a modal window or of its child window
bool isInModalWindow(const QWidget *w)
{
    for (; w; w = w->parentWidget()) {
        if (isModalWindow(w))
            return true;
    }

    return false;
}

// The modal windows' order isn't public: an application-modal one blocks the other modal windows
// only when it is the active one
bool isBlockingModal(const QWidget *modal, const QWidget *w)
{
    if (!modal->isVisible() || !isModalWindow(modal))
        return false;

    if (modal->windowModality() == Qt::WindowModal || modal == QApplication::activeModalWidget())
        return true;

    return !isInModalWindow(w);
}

bool isBlockedByModal(const QWidget *w, const QWidget *modal)
{
    if (!isBlockingModal(modal, w) || isAncestorWidget(modal, w))
        return false;

    // A window-modal one blocks only its parent windows
    return modal->windowModality() == Qt::ApplicationModal || isAncestorWidget(w->window(), modal);
}

const QWidget *blockingModal(const QWidget *w)
{
    const auto windows = QApplication::topLevelWidgets();
    const auto it = std::find_if(windows.begin(), windows.end(),
            [&](const QWidget *modal) { return isBlockedByModal(w, modal); });

    return (it != windows.end()) ? *it : nullptr;
}

// The events sent to the widget bypass the modal windows' blocking
bool checkNotBlocked(const QWidget *w, QString &errorMessage)
{
    const QWidget *modal = blockingModal(w);
    if (!modal)
        return true;

    errorMessage = "Blocked by the modal window: " + UiTestUtil::objectPath(modal);
    return false;
}

bool checkInputWidget(const QWidget *w, QString &errorMessage)
{
    if (!w->isVisible()) {
        errorMessage = "The widget is hidden";
        return false;
    }

    if (!w->isEnabled()) {
        errorMessage = "The widget is disabled";
        return false;
    }

    return checkNotBlocked(w, errorMessage);
}

bool checkInputAction(const QAction *action, QString &errorMessage)
{
    if (!action->isVisible()) {
        errorMessage = "The action is hidden";
        return false;
    }

    if (!action->isEnabled()) {
        errorMessage = "The action is disabled";
        return false;
    }

    const auto w = qobject_cast<QWidget *>(action->parent());

    return !w || checkNotBlocked(w, errorMessage);
}

QWidget *findArgInputWidget(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QWidget *w = findArgObject<QWidget>(p, r, "a widget");
    if (!w || !checkInputWidget(w, r.errorMessage))
        return nullptr;

    return w;
}

// The input is processed after the command's result is sent:
// it may show a modal window, which runs its own event loop
void postInput(QObject *context, const std::function<void()> &func)
{
    QTimer::singleShot(0, context, func);
}

// The mouse events of an item view are of its viewport
QWidget *mouseTarget(QWidget *w)
{
    auto scrollArea = qobject_cast<QAbstractScrollArea *>(w);

    return scrollArea ? scrollArea->viewport() : w;
}

const QWidget *finalFocusProxy(const QWidget *w)
{
    while (w->focusProxy()) {
        w = w->focusProxy();
    }

    return w;
}

bool hasClickFocusPolicy(const QWidget *w)
{
    return (w->focusPolicy() & Qt::ClickFocus) != 0;
}

bool acceptsClickFocus(const QWidget *w)
{
    if (!w->isEnabled() || !hasClickFocusPolicy(w))
        return false;

    return hasClickFocusPolicy(finalFocusProxy(w));
}

// Qt gives the focus by a spontaneous mouse press only
void giveClickFocus(QWidget *widget)
{
    for (QWidget *w = widget; w; w = w->parentWidget()) {
        if (acceptsClickFocus(w)) {
            w->setFocus(Qt::MouseFocusReason);
            return;
        }

        // A click on the focused widget's child keeps the focus
        if (w->isWindow() || finalFocusProxy(w)->hasFocus())
            return;
    }
}

void sendMouseEvent(QWidget *w, QEvent::Type type, const QPoint &pos, Qt::MouseButton button)
{
    // The button is held from its press till its release
    const Qt::MouseButtons buttons =
            (type == QEvent::MouseButtonRelease) ? Qt::NoButton : Qt::MouseButtons(button);
    const Qt::MouseButton eventButton = (type == QEvent::MouseMove) ? Qt::NoButton : button;

    QMouseEvent event(type, pos, w->mapToGlobal(pos), eventButton, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(w, &event);
}

void sendContextMenuEvent(QWidget *w, const QPoint &pos)
{
    QContextMenuEvent event(QContextMenuEvent::Mouse, pos, w->mapToGlobal(pos));
    QCoreApplication::sendEvent(w, &event);
}

bool sendMouseStep(const QPointer<QWidget> &w, const MouseStep &step, Qt::MouseButton button)
{
    if (w && step.type == QEvent::MouseButtonPress) {
        giveClickFocus(w);
    }

    if (!w)
        return false;

    sendMouseEvent(w, step.type, step.pos, button);

    return true;
}

// The steps are stopped by the widget's deletion: false then
bool sendMouseSteps(QWidget *widget, const QList<MouseStep> &steps, Qt::MouseButton button)
{
    const QPointer<QWidget> w = widget;

    for (const auto &step : steps) {
        if (!sendMouseStep(w, step, button))
            return false;
    }

    return !w.isNull();
}

void sendMouseClick(QWidget *w, const MouseClick &click)
{
    constexpr QEvent::Type types[] = {
        QEvent::MouseButtonPress,
        QEvent::MouseButtonRelease,
        QEvent::MouseButtonDblClick,
        QEvent::MouseButtonRelease,
    };

    const QPoint pos = w->rect().center();
    const int count = click.count * 2;

    QList<MouseStep> steps;
    for (int i = 0; i < count; ++i) {
        steps.append({ .type = types[i], .pos = pos });
    }

    if (sendMouseSteps(w, steps, click.button) && click.button == Qt::RightButton) {
        sendContextMenuEvent(w, pos);
    }
}

void clickButton(QAbstractButton *button)
{
    const QPointer<QAbstractButton> b = button;

    giveClickFocus(button);

    if (b) {
        b->click();
    }
}

void sendMouseHover(QWidget *widget)
{
    const QPointer<QWidget> w = widget;
    const QPoint pos = widget->rect().center();

    QEnterEvent event(pos, pos, widget->mapToGlobal(pos));
    QCoreApplication::sendEvent(widget, &event);

    if (w) {
        sendMouseEvent(w, QEvent::MouseMove, pos, Qt::NoButton);
    }
}

void sendMouseDrag(QWidget *w, const QPoint &offset)
{
    const QPoint from = w->rect().center();
    const QPoint to = from + offset;

    sendMouseSteps(w,
            {
                    { .type = QEvent::MouseButtonPress, .pos = from },
                    { .type = QEvent::MouseMove, .pos = from + offset / 2 },
                    { .type = QEvent::MouseMove, .pos = to },
                    { .type = QEvent::MouseButtonRelease, .pos = to },
            },
            Qt::LeftButton);
}

void sendKeyEvent(QWidget *w, QKeyCombination combination, const QString &text)
{
    QKeyEvent press(QEvent::KeyPress, combination.key(), combination.keyboardModifiers(), text);
    QCoreApplication::sendEvent(w, &press);

    QKeyEvent release(QEvent::KeyRelease, combination.key(), combination.keyboardModifiers(), text);
    QCoreApplication::sendEvent(w, &release);
}

Qt::Key charKey(QChar c)
{
    // The keys of the printable ASCII characters are their upper case codes
    const char16_t code = c.toUpper().unicode();

    return (code >= Qt::Key_Space && code <= Qt::Key_AsciiTilde) ? Qt::Key(code) : Qt::Key_unknown;
}

void sendKeyText(QWidget *widget, const QString &text)
{
    const QPointer<QWidget> w = widget;

    for (const QChar c : text) {
        if (!w)
            return;

        sendKeyEvent(w, charKey(c), QString(c));
    }
}

void sendKeySequence(QWidget *widget, const QKeySequence &keys)
{
    const QPointer<QWidget> w = widget;

    for (int i = 0; w && i < keys.count(); ++i) {
        sendKeyEvent(w, keys[uint(i)], QString());
    }
}

// An unknown key's name gives Qt::Key_unknown
bool isValidKeySequence(const QKeySequence &keys)
{
    if (keys.isEmpty())
        return false;

    for (int i = 0; i < keys.count(); ++i) {
        if (keys[uint(i)].key() == Qt::Key_unknown)
            return false;
    }

    return true;
}

// The action's rectangle in the menu or menu bar
QRect menuActionGeometry(QWidget *w, QAction *action)
{
    if (auto menu = qobject_cast<QMenu *>(w)) {
        return menu->actionGeometry(action);
    }

    if (auto menuBar = qobject_cast<QMenuBar *>(w)) {
        return menuBar->actionGeometry(action);
    }

    return {};
}

bool isVisibleMenuOfAction(QWidget *w, QAction *action)
{
    return w->isVisible() && menuActionGeometry(w, action).isValid();
}

// The visible menu or menu bar with the action
QWidget *visibleMenuOfAction(QAction *action)
{
    if (!action->isVisible())
        return nullptr;

    const auto objects = action->associatedObjects();
    for (QObject *o : objects) {
        auto w = qobject_cast<QWidget *>(o);
        if (w && isVisibleMenuOfAction(w, action))
            return w;
    }

    return nullptr;
}

// An action of an open menu or of a menu bar is clicked in it,
// so the menu is closed or opened as by the user
void triggerAction(QAction *action)
{
    QWidget *menu = visibleMenuOfAction(action);
    if (!menu) {
        action->trigger();
        return;
    }

    const QPoint pos = menuActionGeometry(menu, action).center();

    sendMouseSteps(menu,
            {
                    { .type = QEvent::MouseButtonPress, .pos = pos },
                    { .type = QEvent::MouseButtonRelease, .pos = pos },
            },
            Qt::LeftButton);
}

void selectIndex(QAbstractItemView *view, const QPersistentModelIndex &index)
{
    // The live model may have removed the row
    if (!index.isValid())
        return;

    view->setCurrentIndex(index);
    view->scrollTo(index);
}

bool argStartNodes(const ProcessCommandArgs &p, bool hasPath, QList<UiTestNode> &nodes,
        ProcessCommandResult &r)
{
    if (!hasPath) {
        nodes = UiTestUtil::rootNodes();
        return true;
    }

    UiTestNode node;
    if (!findArgNode(p, node, r))
        return false;

    nodes = { node };

    return true;
}

QString treeText(const QList<UiTestNode> &nodes)
{
    QStringList lines;

    for (const auto &node : nodes) {
        const QVariantMap fields = UiTestUtil::objectFields(node.object);

        lines.append(node.path + ' ' + ControlCommandUtil::escapedFieldsToText(fields));
    }

    return lines.join('\n');
}

bool processTree(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QVariantMap fields = { { "depth", -1 } };

    const bool hasPath =
            p.args.size() > 1 && !ControlCommandUtil::isFieldArg(argText(p.args, 1), fields);
    if (!hasPath) {
        fields["depth"] = 0; // the windows only
    }

    QList<UiTestNode> nodes;
    if (!ControlCommandUtil::parseFieldArgs(p.args, hasPath ? 2 : 1, fields, r.errorMessage)
            || !argStartNodes(p, hasPath, nodes, r))
        return false;

    nodes = UiTestUtil::treeNodes(nodes, fields["depth"].toInt());

    r.errorMessage = treeText(nodes);

    return true;
}

bool processGet(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const QString name = argText(p.args, 2);

    QVariant value;
    if (!UiTestUtil::pathPropertyValue(argText(p.args, 1), name, value, r.errorMessage))
        return false;

    r.errorMessage = propertyText(name, value);

    return true;
}

bool parseMouseClick(const QVariantMap &fields, MouseClick &click, QString &errorMessage)
{
    const int buttonIndex =
            ControlCommandUtil::nameIndex(fields, "button", mouseButtonNames(), errorMessage);
    if (buttonIndex < 0)
        return false;

    click.button = mouseButtons[buttonIndex];
    click.count = fields["count"].toInt();

    if (click.count < 1 || click.count > 2) {
        errorMessage = ControlCommandUtil::invalidValueText("count", fields["count"].toString());
        return false;
    }

    return true;
}

bool clickAction(QAction *action, const MouseClick &click, ProcessCommandResult &r)
{
    if (!click.isSingleLeft()) {
        r.errorMessage = "The action is clicked only once by the left button";
        return false;
    }

    if (!checkInputAction(action, r.errorMessage))
        return false;

    postInput(action, [action] { triggerAction(action); });

    return true;
}

bool clickWidget(QWidget *w, const MouseClick &click, ProcessCommandResult &r)
{
    if (!checkInputWidget(w, r.errorMessage))
        return false;

    auto button = qobject_cast<QAbstractButton *>(w);

    if (button && click.isSingleLeft()) {
        postInput(button, [button] { clickButton(button); });
    } else {
        QWidget *target = mouseTarget(w);
        postInput(target, [target, click] { sendMouseClick(target, click); });
    }

    return true;
}

bool clickObject(const UiTestNode &node, const MouseClick &click, ProcessCommandResult &r)
{
    if (auto action = qobject_cast<QAction *>(node.object)) {
        return clickAction(action, click, r);
    }

    if (auto w = qobject_cast<QWidget *>(node.object)) {
        return clickWidget(w, click, r);
    }

    r.errorMessage = "Not a widget or an action: " + node.path;
    return false;
}

bool processClick(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QVariantMap fields = { { "button", "left" }, { "count", 1 } };
    if (!ControlCommandUtil::parseFieldArgs(p.args, 2, fields, r.errorMessage))
        return false;

    MouseClick click;
    if (!parseMouseClick(fields, click, r.errorMessage))
        return false;

    UiTestNode node;
    if (!findArgNode(p, node, r))
        return false;

    return clickObject(node, click, r);
}

bool processHover(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QWidget *w = findArgInputWidget(p, r);
    if (!w)
        return false;

    QWidget *target = mouseTarget(w);
    postInput(target, [target] { sendMouseHover(target); });

    return true;
}

bool processDrag(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QVariantMap fields = { { "dx", 0 }, { "dy", 0 } };
    if (!ControlCommandUtil::parseFieldArgs(p.args, 2, fields, r.errorMessage))
        return false;

    QWidget *w = findArgInputWidget(p, r);
    if (!w)
        return false;

    const QPoint offset(fields["dx"].toInt(), fields["dy"].toInt());

    QWidget *target = mouseTarget(w);
    postInput(target, [target, offset] { sendMouseDrag(target, offset); });

    return true;
}

bool processType(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QWidget *w = findArgInputWidget(p, r);
    if (!w)
        return false;

    const QString text = argText(p.args, 2);

    postInput(w, [w, text] { sendKeyText(w, text); });

    return true;
}

bool processKey(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const QString keysText = argText(p.args, 2);

    const QKeySequence keys(keysText);
    if (!isValidKeySequence(keys)) {
        r.errorMessage = "Invalid <keys>: " + keysText;
        return false;
    }

    QWidget *w = findArgInputWidget(p, r);
    if (!w)
        return false;

    postInput(w, [w, keys] { sendKeySequence(w, keys); });

    return true;
}

bool parseRangeField(
        const QVariantMap &fields, const QString &key, UiTestRange &range, QString &errorMessage)
{
    const QString text = fields[key].toString();
    if (UiTestUtil::parseRange(text, range))
        return true;

    errorMessage = ControlCommandUtil::invalidValueText(key, text);
    return false;
}

bool parseCellRange(const QVariantMap &fields, UiTestCellRange &range, QString &errorMessage)
{
    if (!parseRangeField(fields, "rows", range.rows, errorMessage)
            || !parseRangeField(fields, "cols", range.columns, errorMessage))
        return false;

    range.hasColumns = !fields["cols"].toString().isEmpty();

    return true;
}

QAbstractItemView *findArgView(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto view = findArgObject<QAbstractItemView>(p, r, "an item view");
    if (!view)
        return nullptr;

    if (!view->model()) {
        r.errorMessage = "The view has no model";
        return nullptr;
    }

    return view;
}

bool processCells(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QVariantMap fields = { { "rows", QString() }, { "cols", QString() } };
    if (!ControlCommandUtil::parseFieldArgs(p.args, 2, fields, r.errorMessage))
        return false;

    UiTestCellRange range;
    if (!parseCellRange(fields, range, r.errorMessage))
        return false;

    QAbstractItemView *view = findArgView(p, r);
    if (!view)
        return false;

    r.errorMessage = UiTestUtil::cellLines(view, range).join('\n');

    return true;
}

bool processSelect(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    QVariantMap fields = { { "column", 0 } };
    if (!ControlCommandUtil::parseFieldArgs(p.args, 3, fields, r.errorMessage))
        return false;

    QAbstractItemView *view = findArgView(p, r);
    if (!view || !checkInputWidget(view, r.errorMessage))
        return false;

    const QString rowText = argText(p.args, 2);

    const QPersistentModelIndex index =
            UiTestUtil::rowIndex(view, rowText, fields["column"].toInt());
    if (!index.isValid()) {
        r.errorMessage = "Invalid <row> or column: " + rowText;
        return false;
    }

    postInput(view, [view, index] { selectIndex(view, index); });

    return true;
}

// The optional "key=N" argument after the command's path and its argument
bool parseIntField(const QVariantList &args, IntField &field, QString &errorMessage)
{
    QVariantMap fields = { { field.key, field.value } };

    if (!ControlCommandUtil::parseFieldArgs(args, 3, fields, errorMessage))
        return false;

    const QVariant &value = fields[field.key];

    field.value = value.toInt();

    if (field.value < field.min || field.value > field.max) {
        errorMessage = ControlCommandUtil::invalidValueText(field.key, value.toString());
        return false;
    }

    return true;
}

bool parseWaitTimeout(const QVariantList &args, int &timeoutMsecs, QString &errorMessage)
{
    IntField field = {
        .value = defaultWaitTimeoutMsecs,
        .max = maxWaitTimeoutMsecs,
        .key = "timeout",
    };

    if (!parseIntField(args, field, errorMessage))
        return false;

    timeoutMsecs = field.value;

    return true;
}

bool parseWait(const QVariantList &args, UiWait &wait, QString &errorMessage)
{
    const QString condition = argText(args, 2);

    const int sepIndex = condition.indexOf('=');
    if (sepIndex <= 0) {
        errorMessage = "Invalid <prop>=<value>: " + condition;
        return false;
    }

    wait = {
        .path = argText(args, 1),
        .name = condition.left(sepIndex),
        .value = condition.mid(sepIndex + 1),
    };

    return true;
}

// The text is of the property's value or of the error
bool checkWait(const UiWait &wait, QString &text)
{
    QVariant value;
    if (!UiTestUtil::pathPropertyValue(wait.path, wait.name, value, text))
        return false;

    text = propertyText(wait.name, value);

    return ControlCommandUtil::escapedText(VariantUtil::toText(value)) == wait.value;
}

void checkWaitState(QTimer *timer, const UiWaitState &s)
{
    ProcessCommandResult r;
    r.ok = checkWait(s.wait, r.errorMessage);

    if (!r.ok && !s.deadline.hasExpired())
        return;

    timer->stop();
    timer->deleteLater();

    if (!r.ok) {
        r.errorMessage = "Timeout: " + r.errorMessage;
    }

    ControlCommandManager::sendResult(s.worker, s.requestId, r);
}

void startWait(const ProcessCommandArgs &p, const UiWait &wait, int timeoutMsecs)
{
    const UiWaitState s = {
        .requestId = p.requestId,
        .worker = p.worker,
        .deadline = QDeadlineTimer(timeoutMsecs),
        .wait = wait,
    };

    // The timer is deleted with the worker of the disconnected client
    auto timer = new QTimer(p.worker);

    QObject::connect(timer, &QTimer::timeout, timer, [timer, s] { checkWaitState(timer, s); });

    timer->start(waitCheckMsecs);
}

bool processWait(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    int timeoutMsecs = 0;
    UiWait wait;
    if (!parseWaitTimeout(p.args, timeoutMsecs, r.errorMessage)
            || !parseWait(p.args, wait, r.errorMessage))
        return false;

    QString text;
    if (checkWait(wait, text)) {
        r.errorMessage = text;
        return true;
    }

    startWait(p, wait, timeoutMsecs);

    r.isResultDeferred = true;

    return true;
}

// The scale is in percents
QPixmap scaledPixmap(const QPixmap &pixmap, int scale)
{
    if (scale == 100)
        return pixmap;

    const QSize size = (pixmap.size() * scale / 100).expandedTo(QSize(1, 1));

    return pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool processGrab(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    IntField scale = { .value = 100, .min = 1, .max = 100, .key = "scale" };
    if (!parseIntField(p.args, scale, r.errorMessage))
        return false;

    QWidget *w = findArgObject<QWidget>(p, r, "a widget");
    if (!w)
        return false;

    const QPixmap pixmap = scaledPixmap(w->grab(), scale.value);

    QByteArray data;
    QBuffer buffer(&data);

    if (!buffer.open(QIODevice::WriteOnly) || !pixmap.save(&buffer, "PNG")) {
        r.errorMessage = "Cannot grab the widget";
        return false;
    }

    const QByteArray base64 = data.toBase64();

    // The result's data size is limited; the Base64 text compresses below its length
    if (base64.size() > qsizetype(ControlWorker::dataMaxSize)) {
        r.errorMessage = QString("The image is too large: %1 bytes in Base64, the maximum is %2")
                                 .arg(base64.size())
                                 .arg(ControlWorker::dataMaxSize);
        return false;
    }

    r.errorMessage = QString::fromLatin1(base64);

    return true;
}

struct UiCommand
{
    int argsCount; // the minimum, the command's name included
    const char *name;
    const char *usage;
    processCommand_func func;
};

inline constexpr UiCommand uiCommands[] = {
    { .argsCount = 1, .name = "tree", .usage = "tree [<path>] [depth=N]", .func = &processTree },
    { .argsCount = 3, .name = "get", .usage = "get <path> <prop>", .func = &processGet },
    { .argsCount = 2,
            .name = "click",
            .usage = "click <path> [button=left|right|middle] [count=1|2]",
            .func = &processClick },
    { .argsCount = 2, .name = "hover", .usage = "hover <path>", .func = &processHover },
    { .argsCount = 2, .name = "drag", .usage = "drag <path> [dx=N] [dy=N]", .func = &processDrag },
    { .argsCount = 3, .name = "type", .usage = "type <path> <text>", .func = &processType },
    { .argsCount = 3, .name = "key", .usage = "key <path> <keys>", .func = &processKey },
    { .argsCount = 2,
            .name = "cells",
            .usage = "cells <view> [rows=a-b] [cols=a-b]",
            .func = &processCells },
    { .argsCount = 3,
            .name = "select",
            .usage = "select <view> <row> [column=N]",
            .func = &processSelect },
    { .argsCount = 3,
            .name = "wait",
            .usage = "wait <path> <prop>=<value> [timeout=ms]",
            .func = &processWait },
    { .argsCount = 3,
            .name = "grab",
            .usage = "grab <path> <file> [scale=1-100]",
            .func = &processGrab },
};

const UiCommand *uiCommandByName(const QString &name)
{
    for (const auto &command : uiCommands) {
        if (name == QLatin1String(command.name))
            return &command;
    }

    return nullptr;
}

QString usageText()
{
    QStringList usages;

    for (const auto &command : uiCommands) {
        usages.append(command.usage);
    }

    return "Usage: ui " + usages.join(" | ")
            + "; <path>: <window>/<name>/..., a <name> is the object's name or Class[n]"
              " of the n-th unnamed child of the class, the intermediate ones may be skipped;"
              " <window>: "
            + WindowShow::namesText() + " or a top-level window's <name>";
}

}

bool ControlCommandUi::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const UiCommand *command = uiCommandByName(argText(p.args, 0));

    if (!command || p.args.size() < command->argsCount) {
        r.errorMessage = usageText();
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return command->func(p, r);
}

int ControlCommandUi::resultWaitMsecs(const QVariantList &args) const
{
    const int waitMsecs = ControlCommandBase::resultWaitMsecs(args);

    int timeoutMsecs = 0;
    QString errorMessage;
    if (argText(args, 0) != "wait" || !parseWaitTimeout(args, timeoutMsecs, errorMessage))
        return waitMsecs;

    return timeoutMsecs + waitMsecs;
}

ControlCommandBase::ClientFileArg ControlCommandUi::clientFileArg(const QVariantList &args) const
{
    return (argText(args, 0) == "grab") ? ClientFileWriteData : ClientFileNone;
}
