#ifndef APPGROUP_H
#define APPGROUP_H

#include <QObject>
#include <QVariant>

#define MAX_APP_GROUP_COUNT       16
#define DEFAULT_LIMIT_BUFFER_SIZE 150000

class AppGroup : public QObject
{
    Q_OBJECT

public:
    explicit AppGroup(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool edited) { m_edited = edited; }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool applyChild() const { return m_applyChild; }
    void setApplyChild(bool on);

    bool lanOnly() const { return m_lanOnly; }
    void setLanOnly(bool on);

    bool logBlocked() const { return m_logBlocked; }
    void setLogBlocked(bool on);

    bool logConn() const { return m_logConn; }
    void setLogConn(bool on);

    bool periodEnabled() const { return m_periodEnabled; }
    void setPeriodEnabled(bool enabled);

    bool limitInEnabled() const { return m_limitInEnabled; }
    void setLimitInEnabled(bool enabled);

    bool limitOutEnabled() const { return m_limitOutEnabled; }
    void setLimitOutEnabled(bool enabled);

    quint16 limitPacketLoss() const { return m_limitPacketLoss; }
    void setLimitPacketLoss(quint16 v);

    quint32 limitLatency() const { return m_limitLatency; }
    void setLimitLatency(quint32 v);

    quint32 speedLimitIn() const { return m_speedLimitIn; }
    void setSpeedLimitIn(quint32 limit);

    quint32 speedLimitOut() const { return m_speedLimitOut; }
    void setSpeedLimitOut(quint32 limit);

    quint32 limitBufferSizeIn() const { return m_limitBufferSizeIn; }
    void setLimitBufferSizeIn(quint32 v);

    quint32 limitBufferSizeOut() const { return m_limitBufferSizeOut; }
    void setLimitBufferSizeOut(quint32 v);

    quint32 enabledSpeedLimitIn() const { return limitInEnabled() ? speedLimitIn() : 0; }
    quint32 enabledSpeedLimitOut() const { return limitOutEnabled() ? speedLimitOut() : 0; }

    bool isNull() const { return m_id == 0; }

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name);

    QString killText() const { return m_killText; }
    void setKillText(const QString &killText);

    QString blockText() const { return m_blockText; }
    void setBlockText(const QString &blockText);

    QString allowText() const { return m_allowText; }
    void setAllowText(const QString &allowText);

    bool hasAnyText() const
    {
        return !killText().isEmpty() || !blockText().isEmpty() || !allowText().isEmpty();
    }

    QString periodFrom() const { return m_periodFrom; }
    void setPeriodFrom(const QString &periodFrom);

    QString periodTo() const { return m_periodTo; }
    void setPeriodTo(const QString &periodTo);

    QString menuLabel() const;

    void copy(const AppGroup &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

private:
    bool m_edited : 1 = false;
    bool m_enabled : 1 = true;

    bool m_applyChild : 1 = false;
    bool m_lanOnly : 1 = false;
    bool m_logBlocked : 1 = true;
    bool m_logConn : 1 = true;

    bool m_periodEnabled : 1 = false;

    bool m_limitInEnabled : 1 = false;
    bool m_limitOutEnabled : 1 = false;

    quint16 m_limitPacketLoss = 0; // Percent
    quint32 m_limitLatency = 0; // Milliseconds

    // kbits per sec.
    quint32 m_speedLimitIn = 0;
    quint32 m_speedLimitOut = 0;

    quint32 m_limitBufferSizeIn = DEFAULT_LIMIT_BUFFER_SIZE;
    quint32 m_limitBufferSizeOut = DEFAULT_LIMIT_BUFFER_SIZE;

    qint64 m_id = 0;

    QString m_name;

    QString m_killText;
    QString m_blockText;
    QString m_allowText;

    // In format "hh:mm"
    QString m_periodFrom;
    QString m_periodTo;
};

#endif // APPGROUP_H
