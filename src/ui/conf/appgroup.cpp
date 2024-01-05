#include "appgroup.h"

#include <util/dateutil.h>
#include <util/net/netutil.h>

AppGroup::AppGroup(QObject *parent) : QObject(parent) { }

void AppGroup::setEnabled(bool enabled)
{
    if (bool(m_enabled) != enabled) {
        m_enabled = enabled;
        setEdited(true);
    }
}

void AppGroup::setApplyChild(bool on)
{
    if (bool(m_applyChild) != on) {
        m_applyChild = on;
        setEdited(true);
    }
}

void AppGroup::setLanOnly(bool on)
{
    if (bool(m_lanOnly) != on) {
        m_lanOnly = on;
        setEdited(true);
    }
}

void AppGroup::setLogBlocked(bool on)
{
    if (bool(m_logBlocked) != on) {
        m_logBlocked = on;
        setEdited(true);
    }
}

void AppGroup::setLogConn(bool on)
{
    if (bool(m_logConn) != on) {
        m_logConn = on;
        setEdited(true);
    }
}

void AppGroup::setPeriodEnabled(bool enabled)
{
    if (bool(m_periodEnabled) != enabled) {
        m_periodEnabled = enabled;
        setEdited(true);
    }
}

void AppGroup::setLimitInEnabled(bool enabled)
{
    if (bool(m_limitInEnabled) != enabled) {
        m_limitInEnabled = enabled;
        setEdited(true);
    }
}

void AppGroup::setLimitOutEnabled(bool enabled)
{
    if (bool(m_limitOutEnabled) != enabled) {
        m_limitOutEnabled = enabled;
        setEdited(true);
    }
}

void AppGroup::setLimitPacketLoss(quint16 v)
{
    if (m_limitPacketLoss != v) {
        m_limitPacketLoss = v;
        setEdited(true);
    }
}

void AppGroup::setSpeedLimitIn(quint32 limit)
{
    if (m_speedLimitIn != limit) {
        m_speedLimitIn = limit;
        setEdited(true);
    }
}

void AppGroup::setSpeedLimitOut(quint32 limit)
{
    if (m_speedLimitOut != limit) {
        m_speedLimitOut = limit;
        setEdited(true);
    }
}

void AppGroup::setLimitLatency(quint32 v)
{
    if (m_limitLatency != v) {
        m_limitLatency = v;
        setEdited(true);
    }
}

void AppGroup::setLimitBufferSizeIn(quint32 v)
{
    if (m_limitBufferSizeIn != v) {
        m_limitBufferSizeIn = v;
        setEdited(true);
    }
}

void AppGroup::setLimitBufferSizeOut(quint32 v)
{
    if (m_limitBufferSizeOut != v) {
        m_limitBufferSizeOut = v;
        setEdited(true);
    }
}

void AppGroup::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        setEdited(true);
    }
}

void AppGroup::setKillText(const QString &killText)
{
    if (m_killText != killText) {
        m_killText = killText;
        setEdited(true);
    }
}

void AppGroup::setBlockText(const QString &blockText)
{
    if (m_blockText != blockText) {
        m_blockText = blockText;
        setEdited(true);
    }
}

void AppGroup::setAllowText(const QString &allowText)
{
    if (m_allowText != allowText) {
        m_allowText = allowText;
        setEdited(true);
    }
}

void AppGroup::setPeriodFrom(const QString &periodFrom)
{
    if (m_periodFrom != periodFrom) {
        m_periodFrom = periodFrom;
        setEdited(true);
    }
}

void AppGroup::setPeriodTo(const QString &periodTo)
{
    if (m_periodTo != periodTo) {
        m_periodTo = periodTo;
        setEdited(true);
    }
}

QString AppGroup::menuLabel() const
{
    QString text = name();

    if (enabledSpeedLimitIn() != 0) {
        text += QLatin1Char(' ') + QChar(0x2193) // ↓
                + NetUtil::formatSpeed(speedLimitIn() * 1024);
    }

    if (enabledSpeedLimitOut() != 0) {
        text += QLatin1Char(' ') + QChar(0x2191) // ↑
                + NetUtil::formatSpeed(speedLimitOut() * 1024);
    }

    if (periodEnabled()) {
        text += QLatin1Char(' ') + DateUtil::formatPeriod(periodFrom(), periodTo());
    }

    return text;
}

void AppGroup::copy(const AppGroup &o)
{
    m_edited = o.edited();

    m_enabled = o.enabled();
    m_applyChild = o.applyChild();
    m_lanOnly = o.lanOnly();
    m_logBlocked = o.logBlocked();
    m_logConn = o.logConn();

    m_periodEnabled = o.periodEnabled();
    m_periodFrom = o.periodFrom();
    m_periodTo = o.periodTo();

    m_limitInEnabled = o.limitInEnabled();
    m_limitOutEnabled = o.limitOutEnabled();
    m_speedLimitIn = o.speedLimitIn();
    m_speedLimitOut = o.speedLimitOut();

    m_limitPacketLoss = o.limitPacketLoss();
    m_limitLatency = o.limitLatency();
    m_limitBufferSizeIn = o.limitBufferSizeIn();
    m_limitBufferSizeOut = o.limitBufferSizeOut();

    m_id = o.id();
    m_name = o.name();

    m_killText = o.killText();
    m_blockText = o.blockText();
    m_allowText = o.allowText();
}

QVariant AppGroup::toVariant() const
{
    QVariantMap map;

    map["edited"] = edited();
    map["enabled"] = enabled();

    map["applyChild"] = applyChild();
    map["lanOnly"] = lanOnly();
    map["logBlocked"] = logBlocked();
    map["logConn"] = logConn();

    map["periodEnabled"] = periodEnabled();
    map["periodFrom"] = periodFrom();
    map["periodTo"] = periodTo();

    map["limitInEnabled"] = limitInEnabled();
    map["limitOutEnabled"] = limitOutEnabled();
    map["speedLimitIn"] = speedLimitIn();
    map["speedLimitOut"] = speedLimitOut();

    map["limitPacketLoss"] = limitPacketLoss();
    map["limitLatency"] = limitLatency();
    map["limitBufferSizeIn"] = limitBufferSizeIn();
    map["limitBufferSizeOut"] = limitBufferSizeOut();

    map["id"] = id();
    map["name"] = name();

    map["killText"] = killText();
    map["blockText"] = blockText();
    map["allowText"] = allowText();

    return map;
}

void AppGroup::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_edited = map["edited"].toBool();
    m_enabled = map["enabled"].toBool();

    m_applyChild = map["applyChild"].toBool();
    m_lanOnly = map["lanOnly"].toBool();
    m_logBlocked = map["logBlocked"].toBool();
    m_logConn = map["logConn"].toBool();

    m_periodEnabled = map["periodEnabled"].toBool();
    m_periodFrom = DateUtil::reformatTime(map["periodFrom"].toString());
    m_periodTo = DateUtil::reformatTime(map["periodTo"].toString());

    m_limitInEnabled = map["limitInEnabled"].toBool();
    m_limitOutEnabled = map["limitOutEnabled"].toBool();
    m_speedLimitIn = map["speedLimitIn"].toUInt();
    m_speedLimitOut = map["speedLimitOut"].toUInt();

    m_limitPacketLoss = map["limitPacketLoss"].toUInt();
    m_limitLatency = map["limitLatency"].toUInt();
    m_limitBufferSizeIn = map["limitBufferSizeIn"].toUInt();
    m_limitBufferSizeOut = map["limitBufferSizeOut"].toUInt();

    m_id = map["id"].toLongLong();
    m_name = map["name"].toString();

    m_killText = map["killText"].toString();
    m_blockText = map["blockText"].toString();
    m_allowText = map["allowText"].toString();
}
