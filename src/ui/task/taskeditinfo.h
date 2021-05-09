#ifndef TASKEDITINFO_H
#define TASKEDITINFO_H

#include <QObject>

class TaskEditInfo
{
public:
    explicit TaskEditInfo(bool enabled = 0, quint16 intervalHours = 0);
    explicit TaskEditInfo(quint32 v);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v) { m_enabled = v; }

    quint16 intervalHours() const { return m_intervalHours; }
    void setIntervalHours(quint16 v) { m_intervalHours = v; }

    quint32 value() const { return m_value; }

private:
    union {
        struct
        {
            quint16 m_enabled : 1;
            quint16 m_intervalHours;
        };
        quint32 m_value;
    };
};

#endif // TASKEDITINFO_H
