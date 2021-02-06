#include "appgroup.h"

#include "../util/dateutil.h"
#include "../util/net/netutil.h"

AppGroup::AppGroup(QObject *parent) :
    QObject(parent),
    m_edited(false),
    m_enabled(true),
    m_logConn(true),
    m_fragmentPacket(false),
    m_periodEnabled(false),
    m_limitInEnabled(false),
    m_limitOutEnabled(false)
{
}

void AppGroup::setEnabled(bool enabled)
{
    if (bool(m_enabled) != enabled) {
        m_enabled = enabled;
        emit enabledChanged();

        setEdited(true);
    }
}

void AppGroup::setLogConn(bool on)
{
    if (bool(m_logConn) != on) {
        m_logConn = on;
        emit logConnChanged();

        setEdited(true);
    }
}

void AppGroup::setFragmentPacket(bool on)
{
    if (bool(m_fragmentPacket) != on) {
        m_fragmentPacket = on;
        emit fragmentPacketChanged();

        setEdited(true);
    }
}

void AppGroup::setPeriodEnabled(bool enabled)
{
    if (bool(m_periodEnabled) != enabled) {
        m_periodEnabled = enabled;
        emit periodEnabledChanged();

        setEdited(true);
    }
}

void AppGroup::setLimitInEnabled(bool enabled)
{
    if (bool(m_limitInEnabled) != enabled) {
        m_limitInEnabled = enabled;
        emit limitInEnabledChanged();

        setEdited(true);
    }
}

void AppGroup::setLimitOutEnabled(bool enabled)
{
    if (bool(m_limitOutEnabled) != enabled) {
        m_limitOutEnabled = enabled;
        emit limitOutEnabledChanged();

        setEdited(true);
    }
}

void AppGroup::setSpeedLimitIn(quint32 limit)
{
    if (m_speedLimitIn != limit) {
        m_speedLimitIn = limit;
        emit speedLimitInChanged();

        setEdited(true);
    }
}

void AppGroup::setSpeedLimitOut(quint32 limit)
{
    if (m_speedLimitOut != limit) {
        m_speedLimitOut = limit;
        emit speedLimitOutChanged();

        setEdited(true);
    }
}

void AppGroup::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();

        setEdited(true);
    }
}

void AppGroup::setBlockText(const QString &blockText)
{
    if (m_blockText != blockText) {
        m_blockText = blockText;
        emit blockTextChanged();

        setEdited(true);
    }
}

void AppGroup::setAllowText(const QString &allowText)
{
    if (m_allowText != allowText) {
        m_allowText = allowText;
        emit allowTextChanged();

        setEdited(true);
    }
}

void AppGroup::setPeriodFrom(const QString &periodFrom)
{
    if (m_periodFrom != periodFrom) {
        m_periodFrom = periodFrom;
        emit periodFromChanged();

        setEdited(true);
    }
}

void AppGroup::setPeriodTo(const QString &periodTo)
{
    if (m_periodTo != periodTo) {
        m_periodTo = periodTo;
        emit periodToChanged();

        setEdited(true);
    }
}

QString AppGroup::menuLabel() const
{
    QString text = name();

    if (fragmentPacket()) {
        text += QLatin1Char(' ') + QChar(0x00F7); // ÷
    }

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

void AppGroup::clear()
{
    m_edited = true;
    m_enabled = true;

    m_logConn = true;
    m_fragmentPacket = false;
    m_periodEnabled = false;

    m_limitInEnabled = false;
    m_limitOutEnabled = false;

    m_speedLimitIn = 0;
    m_speedLimitOut = 0;

    // m_id should pe preserved

    m_name = QString();

    m_blockText = QString();
    m_allowText = QString();

    m_periodFrom = QString();
    m_periodTo = QString();
}

void AppGroup::copy(const AppGroup &o)
{
    m_edited = o.edited();

    m_enabled = o.enabled();
    m_logConn = o.logConn();
    m_fragmentPacket = o.fragmentPacket();

    m_periodEnabled = o.periodEnabled();
    m_periodFrom = o.periodFrom();
    m_periodTo = o.periodTo();

    m_limitInEnabled = o.limitInEnabled();
    m_limitOutEnabled = o.limitOutEnabled();
    m_speedLimitIn = o.speedLimitIn();
    m_speedLimitOut = o.speedLimitOut();

    m_id = o.id();

    m_name = o.name();
    m_blockText = o.blockText();
    m_allowText = o.allowText();
}
