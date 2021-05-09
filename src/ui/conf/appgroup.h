#ifndef APPGROUP_H
#define APPGROUP_H

#include <QObject>
#include <QVariant>

#define MAX_APP_GROUP_COUNT 16

class AppGroup : public QObject
{
    Q_OBJECT

public:
    explicit AppGroup(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool edited) { m_edited = edited; }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool logConn() const { return m_logConn; }
    void setLogConn(bool on);

    bool fragmentPacket() const { return m_fragmentPacket; }
    void setFragmentPacket(bool on);

    bool periodEnabled() const { return m_periodEnabled; }
    void setPeriodEnabled(bool enabled);

    bool limitInEnabled() const { return m_limitInEnabled; }
    void setLimitInEnabled(bool enabled);

    bool limitOutEnabled() const { return m_limitOutEnabled; }
    void setLimitOutEnabled(bool enabled);

    quint32 speedLimitIn() const { return m_speedLimitIn; }
    void setSpeedLimitIn(quint32 limit);

    quint32 speedLimitOut() const { return m_speedLimitOut; }
    void setSpeedLimitOut(quint32 limit);

    quint32 enabledSpeedLimitIn() const { return limitInEnabled() ? speedLimitIn() : 0; }
    quint32 enabledSpeedLimitOut() const { return limitOutEnabled() ? speedLimitOut() : 0; }

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name);

    QString blockText() const { return m_blockText; }
    void setBlockText(const QString &blockText);

    QString allowText() const { return m_allowText; }
    void setAllowText(const QString &allowText);

    QString periodFrom() const { return m_periodFrom; }
    void setPeriodFrom(const QString &periodFrom);

    QString periodTo() const { return m_periodTo; }
    void setPeriodTo(const QString &periodTo);

    QString menuLabel() const;

    void copy(const AppGroup &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

private:
    bool m_edited : 1;
    bool m_enabled : 1;

    bool m_logConn : 1;
    bool m_fragmentPacket : 1;

    bool m_periodEnabled : 1;

    bool m_limitInEnabled : 1;
    bool m_limitOutEnabled : 1;

    // KiBytes per sec.
    quint32 m_speedLimitIn = 0;
    quint32 m_speedLimitOut = 0;

    qint64 m_id = 0;

    QString m_name;

    QString m_blockText;
    QString m_allowText;

    // In format "hh:mm"
    QString m_periodFrom;
    QString m_periodTo;
};

#endif // APPGROUP_H
