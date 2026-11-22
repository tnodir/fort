#ifndef UITESTUTIL_H
#define UITESTUTIL_H

#include <QModelIndex>
#include <QObject>
#include <QStringList>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(QAbstractItemView)

struct UiTestNode
{
    QObject *object = nullptr;
    QString segment; // the object name, or Class[n] of the n-th unnamed sibling of the class
    QString path; // the segments from the top-level window separated by '/'
};

struct UiTestRange
{
    int first = 0;
    int last = -1; // till the last one
};

struct UiTestCellRange
{
    bool hasColumns = false; // else the hidden columns are skipped

    UiTestRange rows;
    UiTestRange columns;
};

class UiTestUtil
{
public:
    // The top-level windows: the forms by their "window show" names, then the other parentless
    static QList<UiTestNode> rootNodes();

    // The node by the path: the top-level window, then the descendants' segments, each one found
    // at the least depth below the previous one, so the intermediate ones may be skipped
    static bool findNode(const QString &path, UiTestNode &node, QString &errorMessage);

    // The object's full path, empty if it isn't in the tree
    static QString objectPath(const QObject *o);

    // The nodes and their descendants up to the depth (-1: unlimited)
    static QList<UiTestNode> treeNodes(const QList<UiTestNode> &nodes, int depth);

    // The fields of the object's class, state and text: of the tree's nodes
    static QVariantMap objectFields(QObject *o);

    // The property's value of the node by the path, or the pseudo one: class, rowCount,
    // columnCount, or the path's one: exists
    static bool pathPropertyValue(
            const QString &path, const QString &name, QVariant &value, QString &errorMessage);

    // The range "first-last", "first-" or "index"; an empty text keeps the range
    static bool parseRange(const QString &text, UiTestRange &range);

    // The header's line, then the rows' lines: the row's text and the cells' texts separated by
    // tabs, an empty text replaced by the tooltip, escaped by ControlCommandUtil::escapedText();
    // the expanded rows of a tree view are followed by their children's
    static QStringList cellLines(QAbstractItemView *view, const UiTestCellRange &range);

    // The index of the row's text: its number, of a tree's child prefixed by its parent's text
    // and a dot; invalid if absent
    static QModelIndex rowIndex(QAbstractItemView *view, const QString &rowText, int column);
};

#endif // UITESTUTIL_H
