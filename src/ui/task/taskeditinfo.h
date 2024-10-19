#ifndef TASKEDITINFO_H
#define TASKEDITINFO_H

#include <QObject>

class TaskEditInfo
{
public:
    explicit TaskEditInfo(quint64 v = 0);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v) { m_enabled = v; }

    bool runOnStartup() const { return m_runOnStartup; }
    void setRunOnStartup(bool v) { m_runOnStartup = v; }

    bool delayStartup() const { return m_delayStartup; }
    void setDelayStartup(bool v) { m_delayStartup = v; }

    int maxRetries() const { return m_maxRetries; }
    void setMaxRetries(int v) { m_maxRetries = quint8(v); }

    int retrySeconds() const { return m_retrySeconds; }
    void setRetrySeconds(int v) { m_retrySeconds = quint16(v); }

    int intervalHours() const { return m_intervalHours; }
    void setIntervalHours(int v) { m_intervalHours = quint16(v); }

    quint64 value() const { return m_value; }

    void resetToDefault();

private:
    union {
        struct
        {
            quint8 m_enabled : 1;
            quint8 m_runOnStartup : 1;
            quint8 m_delayStartup : 1;
            quint8 m_maxRetries;
            quint16 m_retrySeconds;
            quint16 m_intervalHours;
            quint16 m_reserved; // not used
        };

        quint64 m_value = 0;
    };
};

#endif // TASKEDITINFO_H
