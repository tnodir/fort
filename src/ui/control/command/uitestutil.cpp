#include "uitestutil.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QHash>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>

#include <algorithm>
#include <iterator>

#include <form/controls/formwindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

#include "controlcommandutil.h"
#include "windowshow.h"

using namespace Fort;

namespace {

// The properties shown by the tree: of the widgets' and actions' states and texts
inline constexpr const char *treePropertyNames[] = {
    "enabled",
    "visible",
    "text",
    "currentText",
    "value",
    "windowTitle",
};

QString className(const QObject *o)
{
    return QLatin1String(o->metaObject()->className());
}

// The rows' or columns' count of the item view's root
QVariant viewCount(QObject *o, Qt::Orientation orientation)
{
    const auto view = qobject_cast<QAbstractItemView *>(o);
    if (!view || !view->model())
        return {};

    const QAbstractItemModel *model = view->model();
    const QModelIndex rootIndex = view->rootIndex();

    return (orientation == Qt::Vertical) ? model->rowCount(rootIndex)
                                         : model->columnCount(rootIndex);
}

struct PseudoProperty
{
    const char *name;
    QVariant (*value)(QObject *o);
};

inline constexpr PseudoProperty pseudoProperties[] = {
    { .name = "class", .value = [](QObject *o) -> QVariant { return className(o); } },
    { .name = "rowCount", .value = [](QObject *o) { return viewCount(o, Qt::Vertical); } },
    { .name = "columnCount", .value = [](QObject *o) { return viewCount(o, Qt::Horizontal); } },
};

// The object's property or the pseudo one; invalid if absent
QVariant propertyValue(QObject *o, const QString &name)
{
    for (const auto &pseudoProperty : pseudoProperties) {
        if (name == QLatin1String(pseudoProperty.name)) {
            return pseudoProperty.value(o);
        }
    }

    return o->property(name.toLatin1());
}

QObjectList childObjects(QObject *o)
{
    QObjectList list;

    const QObjectList &children = o->children();
    std::copy_if(children.begin(), children.end(), std::back_inserter(list),
            [](QObject *child) { return child->isWidgetType(); });

    // The actions of the menus and the windows' shortcuts
    if (auto w = qobject_cast<QWidget *>(o)) {
        const auto actions = w->actions();
        std::copy(actions.begin(), actions.end(), std::back_inserter(list));
    }

    return list;
}

QList<UiTestNode> objectNodes(const QObjectList &objects, const QString &pathPrefix)
{
    QList<UiTestNode> nodes;
    QHash<QString, int> classCounts;

    for (QObject *o : objects) {
        QString segment = o->objectName();

        if (segment.isEmpty()) {
            const QString name = className(o);
            segment = name + '[' + QString::number(classCounts[name]++) + ']';
        }

        nodes.append({ .object = o, .segment = segment, .path = pathPrefix + segment });
    }

    return nodes;
}

QList<UiTestNode> childNodes(const UiTestNode &node)
{
    return objectNodes(childObjects(node.object), node.path + '/');
}

qsizetype objectNodeIndex(const QList<UiTestNode> &nodes, const QObject *o)
{
    const auto it = std::find_if(
            nodes.begin(), nodes.end(), [&](const UiTestNode &n) { return n.object == o; });

    return (it == nodes.end()) ? -1 : std::distance(nodes.begin(), it);
}

QList<UiTestNode> formNodes()
{
    QList<UiTestNode> nodes;

    for (const auto &windowShow : windowShows) {
        if (windowShow.alias)
            continue;

        FormWindow *window = windowManager()->windowByCode(windowShow.code);
        if (!window)
            continue;

        const QString name = windowShow.name;
        nodes.append({ .object = window, .segment = name, .path = name });
    }

    return nodes;
}

QList<UiTestNode> topLevelNodes(const QList<UiTestNode> &forms)
{
    QObjectList windows;

    const auto widgets = QApplication::topLevelWidgets();
    std::copy_if(widgets.begin(), widgets.end(), std::back_inserter(windows),
            [&](QWidget *w) { return !w->parentWidget() && objectNodeIndex(forms, w) < 0; });

    return objectNodes(windows, {});
}

bool objectNodeOf(const QList<UiTestNode> &nodes, const QObject *o, UiTestNode &node)
{
    const qsizetype index = objectNodeIndex(nodes, o);
    if (index < 0)
        return false;

    node = nodes[index];

    return true;
}

// The node of the root or of its descendant by the objects' parents
bool objectNodeOfRoots(const QList<UiTestNode> &roots, const QObject *o, UiTestNode &node)
{
    if (objectNodeOf(roots, o, node))
        return true;

    const QObject *parent = o->parent();

    UiTestNode parentNode;
    if (!parent || !objectNodeOfRoots(roots, parent, parentNode))
        return false;

    return objectNodeOf(childNodes(parentNode), o, node);
}

void appendUniqueNodes(QList<UiTestNode> &nodes, const QList<UiTestNode> &newNodes)
{
    for (const auto &node : newNodes) {
        if (objectNodeIndex(nodes, node.object) < 0) {
            nodes.append(node);
        }
    }
}

QList<UiTestNode> segmentNodes(const QList<UiTestNode> &nodes, const QString &segment)
{
    QList<UiTestNode> found;

    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(found),
            [&](const UiTestNode &node) { return node.segment == segment; });

    return found;
}

QList<UiTestNode> nextLevelNodes(const QList<UiTestNode> &nodes)
{
    QList<UiTestNode> level;

    for (const auto &node : nodes) {
        level.append(childNodes(node));
    }

    return level;
}

// The descendants of the segment at the least depth
QList<UiTestNode> descendantNodes(const UiTestNode &node, const QString &segment)
{
    QList<UiTestNode> level = childNodes(node);

    while (!level.isEmpty()) {
        const QList<UiTestNode> found = segmentNodes(level, segment);
        if (!found.isEmpty())
            return found;

        level = nextLevelNodes(level);
    }

    return {};
}

QList<UiTestNode> descendantNodesOfAll(const QList<UiTestNode> &nodes, const QString &segment)
{
    QList<UiTestNode> found;

    for (const auto &node : nodes) {
        appendUniqueNodes(found, descendantNodes(node, segment));
    }

    return found;
}

QStringList nodePaths(const QList<UiTestNode> &nodes)
{
    QStringList paths;

    for (const auto &node : nodes) {
        paths.append(node.path);
    }

    return paths;
}

bool isEmptyText(const QVariant &value)
{
    return value.metaType().id() == QMetaType::QString && value.toString().isEmpty();
}

void appendTreeNodes(QList<UiTestNode> &list, const UiTestNode &node, int depth)
{
    list.append(node);

    if (depth == 0)
        return;

    for (const auto &child : childNodes(node)) {
        appendTreeNodes(list, child, depth - 1);
    }
}

// The text after the range's separator: empty till the last one
bool parseRangeLast(const QString &text, UiTestRange &range)
{
    if (text.isEmpty()) {
        range.last = -1;
        return true;
    }

    bool ok = false;
    range.last = text.toInt(&ok);

    return ok && range.last >= range.first;
}

QHeaderView *viewHeader(QAbstractItemView *view)
{
    if (auto tableView = qobject_cast<QTableView *>(view)) {
        return tableView->horizontalHeader();
    }

    if (auto treeView = qobject_cast<QTreeView *>(view)) {
        return treeView->header();
    }

    return nullptr;
}

int rangeLast(const UiTestRange &range, int count)
{
    return (range.last < 0) ? count - 1 : qMin(range.last, count - 1);
}

QList<int> cellColumns(QAbstractItemView *view, const UiTestCellRange &range)
{
    const int count = view->model()->columnCount(view->rootIndex());
    const int last = rangeLast(range.columns, count);

    const QHeaderView *header = range.hasColumns ? nullptr : viewHeader(view);

    QList<int> columns;

    for (int column = range.columns.first; column <= last; ++column) {
        if (header && header->isSectionHidden(column))
            continue;

        columns.append(column);
    }

    return columns;
}

// The text of an icon's cell is in its tooltip
QString cellText(const QModelIndex &index)
{
    QString text = index.data().toString();
    if (text.isEmpty()) {
        text = index.data(Qt::ToolTipRole).toString();
    }

    return ControlCommandUtil::escapedText(text);
}

struct CellLines
{
    QTreeView *treeView = nullptr;
    QAbstractItemModel *model = nullptr;
    QList<int> columns;
    QStringList lines;
};

QString headerLine(const CellLines &c)
{
    QString line = "row";

    for (const int column : c.columns) {
        line += '\t'
                + ControlCommandUtil::escapedText(
                        c.model->headerData(column, Qt::Horizontal).toString());
    }

    return line;
}

void appendRowLine(CellLines &c, const QModelIndex &index, const QString &rowText);

void appendChildLines(CellLines &c, const QModelIndex &parent, const QString &parentText)
{
    const int count = c.model->rowCount(parent);

    for (int row = 0; row < count; ++row) {
        appendRowLine(c, c.model->index(row, 0, parent), parentText + '.' + QString::number(row));
    }
}

void appendRowLine(CellLines &c, const QModelIndex &index, const QString &rowText)
{
    QString line = rowText;

    for (const int column : c.columns) {
        line += '\t' + cellText(index.siblingAtColumn(column));
    }

    c.lines.append(line);

    if (c.treeView && c.treeView->isExpanded(index)) {
        appendChildLines(c, index, rowText);
    }
}

bool parseRow(
        const QAbstractItemModel *model, const QModelIndex &parent, const QString &text, int &row)
{
    bool ok = false;
    row = text.toInt(&ok);
    if (!ok)
        return false;

    return row >= 0 && row < model->rowCount(parent);
}

QList<UiTestNode> findNodes(const QString &path)
{
    const QStringList segments = path.split('/', Qt::SkipEmptyParts);
    if (segments.isEmpty())
        return {};

    QList<UiTestNode> nodes = segmentNodes(UiTestUtil::rootNodes(), segments.first());

    for (int i = 1; !nodes.isEmpty() && i < segments.size(); ++i) {
        nodes = descendantNodesOfAll(nodes, segments[i]);
    }

    return nodes;
}

}

QList<UiTestNode> UiTestUtil::rootNodes()
{
    QList<UiTestNode> nodes = formNodes();
    nodes.append(topLevelNodes(nodes));

    return nodes;
}

bool UiTestUtil::findNode(const QString &path, UiTestNode &node, QString &errorMessage)
{
    const QList<UiTestNode> nodes = findNodes(path);

    if (nodes.size() != 1) {
        errorMessage = nodes.isEmpty() ? "Not found: " + path
                                       : "Ambiguous path: " + nodePaths(nodes).join(' ');
        return false;
    }

    node = nodes.first();

    return true;
}

QString UiTestUtil::objectPath(const QObject *o)
{
    UiTestNode node;
    objectNodeOfRoots(rootNodes(), o, node);

    return node.path;
}

QList<UiTestNode> UiTestUtil::treeNodes(const QList<UiTestNode> &nodes, int depth)
{
    QList<UiTestNode> list;

    for (const auto &node : nodes) {
        appendTreeNodes(list, node, depth);
    }

    return list;
}

QVariantMap UiTestUtil::objectFields(QObject *o)
{
    QVariantMap fields = { { "class", className(o) } };

    for (const char *name : treePropertyNames) {
        const QVariant value = o->property(name);
        if (!value.isValid() || isEmptyText(value))
            continue;

        fields.insert(name, value);
    }

    if (o->property("checkable").toBool()) {
        fields.insert("checked", o->property("checked"));
    }

    return fields;
}

bool UiTestUtil::pathPropertyValue(
        const QString &path, const QString &name, QVariant &value, QString &errorMessage)
{
    if (name == "exists") {
        value = !findNodes(path).isEmpty();
        return true;
    }

    UiTestNode node;
    if (!findNode(path, node, errorMessage))
        return false;

    value = propertyValue(node.object, name);
    if (!value.isValid()) {
        errorMessage = "Unknown property: " + name;
        return false;
    }

    return true;
}

bool UiTestUtil::parseRange(const QString &text, UiTestRange &range)
{
    if (text.isEmpty())
        return true;

    const int sepIndex = text.indexOf('-');
    const QString firstText = (sepIndex < 0) ? text : text.left(sepIndex);

    bool ok = false;
    range.first = firstText.toInt(&ok);
    if (!ok || range.first < 0)
        return false;

    if (sepIndex < 0) {
        range.last = range.first;
        return true;
    }

    return parseRangeLast(text.mid(sepIndex + 1), range);
}

QStringList UiTestUtil::cellLines(QAbstractItemView *view, const UiTestCellRange &range)
{
    CellLines c = {
        .treeView = qobject_cast<QTreeView *>(view),
        .model = view->model(),
        .columns = cellColumns(view, range),
    };

    c.lines.append(headerLine(c));

    const QModelIndex rootIndex = view->rootIndex();
    const int last = rangeLast(range.rows, c.model->rowCount(rootIndex));

    for (int row = range.rows.first; row <= last; ++row) {
        appendRowLine(c, c.model->index(row, 0, rootIndex), QString::number(row));
    }

    return c.lines;
}

QModelIndex UiTestUtil::rowIndex(QAbstractItemView *view, const QString &rowText, int column)
{
    const QAbstractItemModel *model = view->model();

    QModelIndex index = view->rootIndex();

    for (const QString &text : rowText.split('.')) {
        int row;
        if (!parseRow(model, index, text, row))
            return {};

        index = model->index(row, 0, index);
    }

    if (column < 0 || column >= model->columnCount(index.parent()))
        return {};

    return index.siblingAtColumn(column);
}
