#include "graphpopup.h"

#include <QGridLayout>
#include <QLabel>
#include <QScreen>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QToolTip>

#include <activity/activitymanager.h>
#include <conf/confappmanager.h>
#include <form/controls/controlutil.h>
#include <form/graph/graphbasewindow.h>
#include <fortglobal.h>
#include <model/applistmodeldata.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>
#include <util/formatutil.h>

using namespace Fort;

namespace {

inline constexpr int topAppsCount = 7;

inline constexpr int appIconSize = 16;
inline constexpr int maxNameChars = 32;

enum PopupColumn : qint8 {
    ColumnIcon = 0,
    ColumnName,
    ColumnIn,
    ColumnOut,
    ColumnCount,
};

// Next to the anchor on the side, centered along it
QRect sideRect(const QRect &anchor, const QSize &size, GraphSide side)
{
    const QPoint center = anchor.center();
    const int x = center.x() - size.width() / 2;
    const int y = center.y() - size.height() / 2;

    switch (side) {
    case GraphSideTop:
        return QRect(QPoint(x, anchor.top() - size.height()), size);
    case GraphSideBottom:
        return QRect(QPoint(x, anchor.bottom() + 1), size);
    case GraphSideLeft:
        return QRect(QPoint(anchor.left() - size.width(), y), size);
    case GraphSideRight:
        return QRect(QPoint(anchor.right() + 1, y), size);
    default:
        Q_UNREACHABLE();
        return {};
    }
}

// The start of [start, start + length) moved inside [boundsStart, boundsEnd), else to boundsStart
int boundedStart(int start, int length, int boundsStart, int boundsEnd)
{
    return qMax(boundsStart, qMin(start, boundsEnd - length));
}

QRect boundedRect(const QRect &rect, const QRect &bounds)
{
    const int x = boundedStart(rect.x(), rect.width(), bounds.x(), bounds.x() + bounds.width());
    const int y = boundedStart(rect.y(), rect.height(), bounds.y(), bounds.y() + bounds.height());

    return QRect(QPoint(x, y), rect.size());
}

// Inside the bounds, next to the anchor on the first side where it doesn't cover the anchor, else
// on the first side
QRect popupRect(
        const QRect &anchor, const QSize &size, const QList<GraphSide> &sides, const QRect &bounds)
{
    for (const GraphSide side : sides) {
        const QRect rect = boundedRect(sideRect(anchor, size, side), bounds);
        if (!rect.intersects(anchor))
            return rect;
    }

    return boundedRect(sideRect(anchor, size, sides.first()), bounds);
}

QLabel *createValueLabel()
{
    auto c = ControlUtil::createLabel();
    c->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return c;
}

qint64 otherBits(quint64 totalBits, qint64 topBits)
{
    return qMax(qint64(totalBits) - topBits, qint64(0));
}

}

GraphPopup::GraphPopup(GraphBaseWindow *graphWindow) : m_graphWindow(graphWindow)
{
    setupUi();
    setupRefresh();
}

void GraphPopup::showPopup()
{
    updateTraffic();
    updateTopApps();

    adjustSize();
    updatePlacement();

    show();
}

void GraphPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
}

void GraphPopup::setupUi()
{
    // The tooltip's look
    QPalette palette = QToolTip::palette();
    palette.setColor(QPalette::WindowText, palette.color(QPalette::ToolTipText));
    setPalette(palette);
    setFont(QToolTip::font());

    auto layout = new QGridLayout();
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setHorizontalSpacing(10);
    layout->setColumnStretch(ColumnName, 1);

    setupHeader(layout);
    setupRows(layout);

    setLayout(layout);

    setAttribute(Qt::WA_ShowWithoutActivating);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
            | Qt::WindowTransparentForInput
            | (m_graphWindow->popupOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget));

    // Created with the graph's scale
    setScreen(m_graphWindow->screen());
}

void GraphPopup::setupHeader(QGridLayout *layout)
{
    const QSize iconSize(appIconSize, appIconSize);

    layout->addWidget(ControlUtil::createIconLabel(":/icons/bullet_green_down.png", iconSize), 0,
            ColumnIn, Qt::AlignRight);
    layout->addWidget(ControlUtil::createIconLabel(":/icons/bullet_blue_up.png", iconSize), 0,
            ColumnOut, Qt::AlignRight);
}

void GraphPopup::setupRows(QGridLayout *layout)
{
    int rowIndex = 1;

    m_todayRow = createRow(layout, rowIndex++, /*withIcon=*/false);
    m_todayRow.nameLabel->setText(tr("Today"));

    m_monthRow = createRow(layout, rowIndex++, /*withIcon=*/false);
    m_monthRow.nameLabel->setText(tr("Month"));

    layout->addWidget(ControlUtil::createHSeparator(), rowIndex++, 0, 1, ColumnCount);

    for (int i = 0; i < topAppsCount; ++i) {
        m_appRows.append(createRow(layout, rowIndex++, /*withIcon=*/true));
    }

    m_otherRow = createRow(layout, rowIndex++, /*withIcon=*/false);
    m_otherRow.nameLabel->setText(tr("Other"));
}

void GraphPopup::setupRefresh()
{
    connect(statManager(), &StatManager::trafficAdded, this, &GraphPopup::updateTraffic);
    connect(activityManager(), &ActivityManager::activityChanged, this, &GraphPopup::updateTopApps);

    connect(m_graphWindow, &GraphBaseWindow::positionChanged, this, &GraphPopup::updatePlacement);
    connect(m_graphWindow, &GraphBaseWindow::sizeChanged, this, &GraphPopup::updatePlacement);
}

void GraphPopup::updateTraffic()
{
    auto statManager = Fort::statManager();

    setRowBytes(m_todayRow, statManager->getTrafficDay());
    setRowBytes(m_monthRow, statManager->getTrafficMonth());

    updateOtherSpeed();
    updateSize();
}

void GraphPopup::updateTopApps()
{
    const QList<ActivityAppSpeed> appSpeeds = activityManager()->topApps(topAppsCount);

    m_topInBits = 0;
    m_topOutBits = 0;

    for (int i = 0; i < topAppsCount; ++i) {
        const Row &row = m_appRows[i];
        const bool visible = (i < appSpeeds.size());

        if (visible) {
            fillAppRow(row, appSpeeds[i]);
        }

        setRowVisible(row, visible);
    }

    updateOtherSpeed();
    updateSize();
}

void GraphPopup::fillAppRow(const Row &row, const ActivityAppSpeed &appSpeed)
{
    const qint64 appId = appSpeed.appKey.appId;
    const App app = (appId != 0) ? confAppManager()->appById(appId) : App();

    const QString name = AppListModelData::appNameOf(app, appSpeed.procPath);
    const QIcon icon = AppListModelData::appIconOf(app, appSpeed.procPath);

    const int maxNameWidth = row.nameLabel->fontMetrics().averageCharWidth() * maxNameChars;

    row.iconLabel->setPixmap(icon.pixmap(appIconSize));
    row.nameLabel->setText(
            row.nameLabel->fontMetrics().elidedText(name, Qt::ElideRight, maxNameWidth));

    const qint64 inBits = appSpeed.inSpeed * 8;
    const qint64 outBits = appSpeed.outSpeed * 8;

    setRowSpeeds(row, inBits, outBits);

    m_topInBits += inBits;
    m_topOutBits += outBits;
}

void GraphPopup::setRowSpeeds(const Row &row, qint64 inBits, qint64 outBits)
{
    const auto unitFormat = m_graphWindow->plot()->unitFormat();

    row.inLabel->setText(FormatUtil::formatSpeed(inBits, unitFormat));
    row.outLabel->setText(FormatUtil::formatSpeed(outBits, unitFormat));
}

void GraphPopup::updateOtherSpeed()
{
    // The graph's speed of the last complete second
    const GraphPoint point = m_graphWindow->plot()->pointAt(DateUtil::getUnixTime() - 1);

    setRowSpeeds(m_otherRow, otherBits(point.inBits, m_topInBits),
            otherBits(point.outBits, m_topOutBits));
}

void GraphPopup::updateSize()
{
    if (!isVisible())
        return;

    adjustSize();
    updatePlacement();
}

void GraphPopup::updatePlacement()
{
    // The graph's frame in the screen's coordinates: a taskbar's graph is a native child window
    const QRect frame = m_graphWindow->frameGeometry();
    const QPoint frameOffset = frame.topLeft() - m_graphWindow->geometry().topLeft();
    const QRect anchor(m_graphWindow->mapToGlobal(frameOffset), frame.size());

    const QRect workRect = m_graphWindow->screen()->availableGeometry();

    move(popupRect(anchor, size(), m_graphWindow->popupSides(), workRect).topLeft());
}

GraphPopup::Row GraphPopup::createRow(QGridLayout *layout, int rowIndex, bool withIcon)
{
    Row row;

    if (withIcon) {
        row.iconLabel = ControlUtil::createLabel();
        row.iconLabel->setFixedSize(appIconSize, appIconSize);
        row.iconLabel->setScaledContents(true);

        layout->addWidget(row.iconLabel, rowIndex, ColumnIcon);
    }

    row.nameLabel = ControlUtil::createLabel();
    row.inLabel = createValueLabel();
    row.outLabel = createValueLabel();

    // A row without the icon has the name in its place
    const int nameColumn = withIcon ? ColumnName : ColumnIcon;
    const int nameColumnSpan = ColumnIn - nameColumn;

    layout->addWidget(row.nameLabel, rowIndex, nameColumn, 1, nameColumnSpan);
    layout->addWidget(row.inLabel, rowIndex, ColumnIn);
    layout->addWidget(row.outLabel, rowIndex, ColumnOut);

    return row;
}

void GraphPopup::setRowBytes(const Row &row, const TrafBytes &bytes)
{
    row.inLabel->setText(FormatUtil::formatDataSize(bytes.inBytes));
    row.outLabel->setText(FormatUtil::formatDataSize(bytes.outBytes));
}

void GraphPopup::setRowVisible(const Row &row, bool visible)
{
    row.iconLabel->setVisible(visible);
    row.nameLabel->setVisible(visible);
    row.inLabel->setVisible(visible);
    row.outLabel->setVisible(visible);
}
