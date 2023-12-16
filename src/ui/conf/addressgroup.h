#ifndef ADDRESSGROUP_H
#define ADDRESSGROUP_H

#include <QObject>
#include <QVariant>
#include <QVector>

class AddressGroup : public QObject
{
    Q_OBJECT

public:
    explicit AddressGroup(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool edited) { m_edited = edited; }

    bool includeAll() const { return m_includeAll; }
    void setIncludeAll(bool includeAll);

    bool excludeAll() const { return m_excludeAll; }
    void setExcludeAll(bool excludeAll);

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    quint32 includeZones() const { return m_includeZones; }
    void setIncludeZones(quint32 v);

    quint32 excludeZones() const { return m_excludeZones; }
    void setExcludeZones(quint32 v);

    QString includeText() const { return m_includeText; }
    void setIncludeText(const QString &includeText);

    QString excludeText() const { return m_excludeText; }
    void setExcludeText(const QString &excludeText);

    void copy(const AddressGroup &o);

    QVariant toVariant() const;
    void fromVariant(const QVariant &v);

private:
    bool m_edited : 1 = false;

    bool m_includeAll : 1 = true;
    bool m_excludeAll : 1 = false;

    qint64 m_id = 0;

    quint32 m_includeZones = 0;
    quint32 m_excludeZones = 0;

    QString m_includeText;
    QString m_excludeText;
};

#endif // ADDRESSGROUP_H
