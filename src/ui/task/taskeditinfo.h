#ifndef TASKEDITINFO_H
#define TASKEDITINFO_H

#include <QObject>

class TaskEditInfo
{
public:
    explicit TaskEditInfo(
            bool enabled = false, bool runOnStartup = false, quint16 intervalHours = 0);
    explicit TaskEditInfo(quint32 v);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v) { m_enabled = v; }

    bool runOnStartup() const { return m_runOnStartup; }
    void setRunOnStartup(bool v) { m_runOnStartup = v; }

    quint16 intervalHours() const { return m_intervalHours; }
    void setIntervalHours(quint16 v) { m_intervalHours = v; }

    quint32 value() const { return m_value; }

private:
    union {
        struct
        {
            quint16 m_enabled : 1;
            quint16 m_runOnStartup : 1;
            quint16 m_intervalHours;
        };
        quint32 m_value = 0;
    };
};

#endif // TASKEDITINFO_H
